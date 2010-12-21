// ptest.cpp : Defines the entry point for the console application.
//

//#include <SDKDDKVer.h>
#include <stdio.h>
#include <tchar.h>
#include "windows.h"
#include "zpack.h"
#include <string>
#include <hash_map>
#include <iostream>

std::string getWord(const std::string& input, size_t& pos)
{
	size_t start = input.find_first_not_of(' ', pos);
	if (start == std::string::npos)
	{
		start = pos;
	}
	pos = input.find_first_of(' ', start);
	if (pos == std::string::npos)
	{
		pos = input.length();
	}
	return input.substr(start, pos - start);
}

typedef void (WINAPI *EnumCallback)(const std::string& path, void* param);
void enumFile(const std::string& searchPath, EnumCallback callback, void* param)
{
	WIN32_FIND_DATAA fd;
	HANDLE findFile = ::FindFirstFileA((searchPath + "*").c_str(), &fd);

	if (findFile == INVALID_HANDLE_VALUE)
	{
		::FindClose(findFile);
		return;
	}

	while (true)
	{
		if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			//file
			callback(searchPath + fd.cFileName, param);
		}
		else if (strcmp(fd.cFileName, ".") != 0 && strcmp(fd.cFileName, "..")  != 0)
		{
			//folder
			enumFile(searchPath + fd.cFileName + "/", callback, param);
		}
		if (!FindNextFileA(findFile, &fd))
		{
			::FindClose(findFile);
			return;
		}
	}
}

std::string g_basePath;
void WINAPI addPackFile(const std::string& filename, void* param)
{
	zp::IPackage* pack = reinterpret_cast<zp::IPackage*>(param);
	std::string nameInPack = filename.substr(g_basePath.length(), filename.length() - g_basePath.length());
	pack->addFile(filename.c_str(), nameInPack.c_str());
	std::cout << nameInPack << std::endl;
}

int _tmain(int argc, _TCHAR* argv[])
{
	zp::IPackage* pack = NULL;
	std::string currentPackageName;

	std::cout << "please type <help> to checkout commands available." << std::endl;

	while (true)
	{
		if (pack != NULL && pack->dirty())
		{
			std::cout << "*:->";
		}
		else
		{
			std::cout << ":->";
		}
		std::string input, command, param0, param1;
		std::getline(std::cin, input);

		size_t pos = 0;
		command = getWord(input, pos);
		param0 = getWord(input, pos);
		param1 = getWord(input, pos);
		if (command == "exit")
		{
			break;
		}
		else if (command == "pack")
		{
			if (currentPackageName.empty())
			{
				std::cout << "no package opened." << std::endl;
			}
			else
			{
				std::cout << currentPackageName << std::endl;
			}
		}
		else if (command == "open")
		{
			if (pack != NULL)
			{
				zp::close(pack);
				currentPackageName = "";
			}
			pack = zp::open(param0.c_str(), 0);
			if (pack != NULL)
			{
				currentPackageName = param0;
			}
			else
			{
				std::cout << "failed." << std::endl;
			}
		}
		else if (command == "create")
		{
			if (pack != NULL)
			{
				zp::close(pack);
				pack = NULL;
				currentPackageName = "";
			}
			pack = zp::create(param0.c_str());
			if (pack != NULL)
			{
				currentPackageName = param0;
			}
			else
			{
				std::cout << "failed." << std::endl;
			}
		}
		else if (command == "close")
		{
			currentPackageName = "";
			if (pack != NULL)
			{
				zp::close(pack);
				pack = NULL;
			}
		}
		else if (command == "list")
		{
			if (pack != NULL)
			{
				size_t count = pack->getFileCount();
				for (unsigned long i = 0; i < count; ++i)
				{
					char filename[256];
					pack->getFilenameByIndex(filename, sizeof(filename), i);
					zp::IFile* file = pack->openFile(filename);
					if (file != NULL)
					{
						std::cout << filename << "[" << file->getSize() << "]" << std::endl;
						pack->closeFile(file);
					}
					else
					{
						std::cout << filename << std::endl;
					}
				}
			}
		}
		else if (command == "add")
		{
			if (pack != NULL && !param0.empty() && !param1.empty())
			{
				if (!pack->addFile(param0.c_str(), param1.c_str()))
				{
					std::cout << "failed." << std::endl;
				}
			}
			else
			{
				std::cout << "failed." << std::endl;
			}
		}
		else if (command == "adddir")
		{
			if (pack != NULL && !param0.empty())
			{
				g_basePath = param0;
				enumFile(param0, addPackFile, pack);
			}
			else
			{
				std::cout << "failed." << std::endl;
			}
		}
		else if (command == "remove")
		{
			if (pack != NULL)
			{
				if (!pack->removeFile(param0.c_str()))
				{
					std::cout << "failed." << std::endl;
				}
			}
			else
			{
				std::cout << "failed." << std::endl;
			}
		}
		else if (command == "flush")
		{
			if (pack != NULL)
			{
				pack->flush();
			}
			else
			{
				std::cout << "failed." << std::endl;
			}
		}
		else if (command == "fragment")
		{
			if (pack != NULL)
			{
				unsigned __int64 toMove;
				unsigned __int64 fragSize = pack->countFragmentSize(toMove);
				std::cout << "fragment:" << fragSize << " bytes, " << toMove << " bytes need moving" << std::endl;
			}
			else
			{
				std::cout << "failed." << std::endl;
			}
		}
		else if (command == "defrag")
		{
			if (pack != NULL)
			{
				pack->defrag();
			}
			else
			{
				std::cout << "failed." << std::endl;
			}
		}
		else if (command == "help")
		{
			std::cout << "create path" << std::endl;
			std::cout << "open path" << std::endl;
			std::cout << "close" << std::endl;
			std::cout << "list" << std::endl;
			std::cout << "add externalPath filename" << std::endl;
			std::cout << "adddir externalPath" << std::endl;
			std::cout << "remove filename" << std::endl;
			std::cout << "flush" << std::endl;
			std::cout << "fragment" << std::endl;
			std::cout << "defrag" << std::endl;
			std::cout << "exit" << std::endl;
		}
		else
		{
			std::cout << command << " command not found." << std::endl;
		}
	}
	if (pack != NULL)
	{
		zp::close(pack);
	}
	return 0;
}
