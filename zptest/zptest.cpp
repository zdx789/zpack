// ptest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
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
				for (size_t i = 0; i < count; ++i)
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
		else if (command == "help")
		{
			std::cout << "create path" << std::endl;
			std::cout << "open path" << std::endl;
			std::cout << "close" << std::endl;
			std::cout << "list" << std::endl;
			std::cout << "add externalPath filename" << std::endl;
			std::cout << "remove filename" << std::endl;
			std::cout << "flush" << std::endl;
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
