// ptest.cpp : Defines the entry point for the console application.
//

//#include <SDKDDKVer.h>
#include <stdio.h>
#include <tchar.h>
#include "windows.h"
#include "zpack.h"
#include <string>
#include <iostream>
#include <map>
#include "zpexplorer.h"

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

typedef bool (WINAPI *CommandProc)(const std::string& param0, const std::string& param1);

std::map<std::string, CommandProc> g_commandHandlers;

#define CMD_PROC(cmd) bool WINAPI cmd##_proc(const std::string& param0, const std::string& param1)
#define REGISTER_CMD(cmd) g_commandHandlers[#cmd] = &cmd##_proc;

zp::IPackage* g_pack = NULL;
ZpExplorer* g_explorer = NULL;
std::string g_basePath;
std::string g_packName;

void WINAPI addPackFile(const std::string& filename, void* param)
{
	zp::IPackage* g_pack = reinterpret_cast<zp::IPackage*>(param);
	std::string nameInPack = filename.substr(g_basePath.length(), filename.length() - g_basePath.length());
	g_pack->addFile(filename.c_str(), nameInPack.c_str());
	std::cout << nameInPack << std::endl;
}

CMD_PROC(exit)
{
	exit(0);
	return true;
}

CMD_PROC(open)
{
	if (param0.empty())
	{
		return false;
	}
	if (g_pack != NULL)
	{
		zp::close(g_pack);
		g_packName.clear();
	}
	if (g_explorer != NULL)
	{
		delete g_explorer;
		g_explorer = NULL;
	}
	g_pack = zp::open(param0.c_str(), 0);
	if (g_pack == NULL)
	{
		return false;
	}
	g_packName = param0;
	g_explorer = new ZpExplorer(g_pack);
	return true;
}

CMD_PROC(create)
{
	if (param0.empty())
	{
		return false;
	}
	if (g_pack != NULL)
	{
		zp::close(g_pack);
		g_packName.clear();
	}
	g_pack = zp::create(param0.c_str());
	if (g_pack != NULL)
	{
		g_packName = param0;
	}
	return (g_pack != NULL);
}

CMD_PROC(close)
{
	g_packName.clear();
	if (g_pack != NULL)
	{
		zp::close(g_pack);
		g_pack = NULL;
	}
	return true;
}

CMD_PROC(list)
{
	if (g_pack == NULL)
	{
		return false;
	}
	size_t count = g_pack->getFileCount();
	for (unsigned long i = 0; i < count; ++i)
	{
		char filename[256];
		g_pack->getFilenameByIndex(filename, sizeof(filename), i);
		zp::IFile* file = g_pack->openFile(filename);
		if (file != NULL)
		{
			std::cout << filename << "[" << file->getSize() << "]" << std::endl;
			g_pack->closeFile(file);
		}
		else
		{
			std::cout << filename << std::endl;
		}
	}
	return true;
}

CMD_PROC(dir)
{
	if (g_explorer == NULL)
	{
		return false;
	}
	const ZpNode* node = g_explorer->getNode();
	for (std::list<ZpNode>::const_iterator iter = node->children.begin();
		iter != node->children.end();
		++iter)
	{
		if (iter->isDirectory)
		{
			std::cout << "  <" << iter->name << ">" << std::endl; 
		}
		else
		{
			std::cout << "  " << iter->name << std::endl; 
		}
	}
	return true;
}

CMD_PROC(cd)
{
	if (g_explorer == NULL && !param0.empty())
	{
		return false;
	}
	if (param0 == "..")
	{
		g_explorer->exit();
	}
	else if (param0 == "/")
	{
		g_explorer->enterRoot();
	}
	else
	{
		g_explorer->enter(param0);
	}
	return true;
}

CMD_PROC(add)
{
	if (g_pack == NULL || param0.empty() || param1.empty())
	{
		return false;
	}
	return g_pack->addFile(param0.c_str(), param1.c_str());
}

CMD_PROC(adddir)
{
	if (g_pack == NULL || param0.empty())
	{
		return false;
	}
	g_basePath = param0;
	enumFile(param0, addPackFile, g_pack);
	return true;
}

CMD_PROC(remove)
{
	if (g_pack == NULL || param0.empty())
	{
		return false;
	}
	return g_pack->removeFile(param0.c_str());
}

CMD_PROC(flush)
{
	if (g_pack == NULL)
	{
		return false;
	}
	g_pack->flush();
	return true;
}

CMD_PROC(fragment)
{
	if (g_pack == NULL)
	{
		return false;
	}
	unsigned __int64 toMove;
	unsigned __int64 fragSize = g_pack->countFragmentSize(toMove);
	std::cout << "fragment:" << fragSize << " bytes, " << toMove << " bytes need moving" << std::endl;
	return true;
}

CMD_PROC(defrag)
{
	return (g_pack != NULL && g_pack->defrag());
}

CMD_PROC(help)
{
	std::cout << "  create path" << std::endl;
	std::cout << "  open path" << std::endl;
	std::cout << "  close" << std::endl;
	std::cout << "  list" << std::endl;
	std::cout << "  cd path" << std::endl;
	std::cout << "  dir" << std::endl;
	std::cout << "  add externalPath filename" << std::endl;
	std::cout << "  adddir externalPath" << std::endl;
	std::cout << "  remove filename" << std::endl;
	std::cout << "  flush" << std::endl;
	std::cout << "  fragment" << std::endl;
	std::cout << "  defrag" << std::endl;
	std::cout << "  exit" << std::endl;
	return true;
}

int _tmain(int argc, _TCHAR* argv[])
{	
	REGISTER_CMD(exit);
	REGISTER_CMD(create);
	REGISTER_CMD(open);
	REGISTER_CMD(add);
	REGISTER_CMD(remove);
	REGISTER_CMD(adddir);
	REGISTER_CMD(flush);
	REGISTER_CMD(close);
	REGISTER_CMD(list);
	REGISTER_CMD(dir);
	REGISTER_CMD(cd);
	REGISTER_CMD(fragment);
	REGISTER_CMD(defrag);
	REGISTER_CMD(help);

	std::cout << "please type <help> to checkout commands available." << std::endl;
	while (true)
	{
		if (!g_packName.empty())
		{
			std::cout << g_packName;
			if (g_pack->dirty())
			{
				std::cout << "*";
			}
			if (g_explorer != NULL)
			{
				std::cout << g_explorer->getPath();
			}
			std::cout << ">";
		}
		else
		{
			std::cout << "zp>";
		}
		std::string input, command, param0, param1;
		std::getline(std::cin, input);

		size_t pos = 0;
		command = getWord(input, pos);
		param0 = getWord(input, pos);
		param1 = getWord(input, pos);

		std::map<std::string, CommandProc>::iterator found = g_commandHandlers.find(command);
		if (found == g_commandHandlers.end())
		{
			std::cout << "<" << command << "> command not found." << std::endl;
		}
		else if (!found->second(param0, param1))
		{
			std::cout << "<" << command << "> failed." << std::endl;
		}
	}
	if (g_pack != NULL)
	{
		zp::close(g_pack);
	}
	return 0;
}
