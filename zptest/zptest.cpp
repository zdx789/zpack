// ptest.cpp : Defines the entry point for the console application.
//

//#include <SDKDDKVer.h>
#include <stdio.h>
#include <tchar.h>
#include "zpack.h"
#include <cassert>
#include <string>
#include <sstream>
#include <iostream>
#include <map>
#include "zpexplorer.h"
//#include "fileenum.h"

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

bool zpcallback(const std::string& path, size_t fileIndex, size_t totalFileCount)
{
	std::cout << path << std::endl;
	return true;
}

typedef bool (*CommandProc)(const std::string& param0, const std::string& param1);

std::map<std::string, CommandProc> g_commandHandlers;

#define CMD_PROC(cmd) bool cmd##_proc(const std::string& param0, const std::string& param1)
#define REGISTER_CMD(cmd) g_commandHandlers[#cmd] = &cmd##_proc;

std::string g_packName;
ZpExplorer g_explorer;

CMD_PROC(exit)
{
	exit(0);
	return true;
}

CMD_PROC(open)
{
	if (g_explorer.open(param0))
	{
		g_packName = param0;
		return true;
	}
	return false;
}

CMD_PROC(create)
{
	if (g_explorer.create(param0, param1))
	{
		g_packName = param0;
		return true;
	}
	return false;
}

CMD_PROC(close)
{
	g_packName.clear();
	g_explorer.close();
	return true;
}

CMD_PROC(list)
{
	zp::IPackage* pack = g_explorer.getPack();
	if (pack == NULL)
	{
		return false;
	}
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
	return true;
}

CMD_PROC(dir)
{
	const ZpNode* node = g_explorer.getNode();
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
	if (param0 == "..")
	{
		g_explorer.exit();
	}
	else if (param0 == "/")
	{
		g_explorer.enterRoot();
	}
	else
	{
		g_explorer.enter(param0);
	}
	return true;
}

CMD_PROC(add)
{
	return g_explorer.add(param0);
}

CMD_PROC(del)
{
	return g_explorer.remove(param0);
}

CMD_PROC(extract)
{
	return g_explorer.extract(param0, param1);
}

CMD_PROC(fragment)
{
	zp::IPackage* pack = g_explorer.getPack();
	if (pack == NULL)
	{
		return false;
	}
	unsigned __int64 toMove;
	unsigned __int64 fragSize = pack->countFragmentSize(toMove);
	std::cout << "fragment:" << fragSize << " bytes, " << toMove << " bytes need moving" << std::endl;
	return true;
}

CMD_PROC(defrag)
{
	zp::IPackage* pack = g_explorer.getPack();
	if (pack == NULL)
	{
		return false;
	}
	return (pack != NULL && pack->defrag());
}

CMD_PROC(help)
{
	std::cout << "  create packagePath srcPath" << std::endl;
	std::cout << "  open packagePath" << std::endl;
	std::cout << "  close" << std::endl;
	std::cout << "  list" << std::endl;
	std::cout << "  cd path" << std::endl;
	std::cout << "  dir" << std::endl;
	std::cout << "  add srcPath/srcDir" << std::endl;
	std::cout << "  del file/dir" << std::endl;
	std::cout << "  extract file/dir dstPath" << std::endl;
	std::cout << "  fragment" << std::endl;
	std::cout << "  defrag" << std::endl;
	std::cout << "  exit" << std::endl;
	return true;
}

int _tmain(int argc, _TCHAR* argv[])
{
	g_explorer.setCallback(zpcallback);

	REGISTER_CMD(exit);
	REGISTER_CMD(create);
	REGISTER_CMD(open);
	REGISTER_CMD(add);
	REGISTER_CMD(del);
	REGISTER_CMD(extract);
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
			if (g_explorer.isOpen())
			{
				std::cout << "/" << g_explorer.getPath();
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

		std::istringstream iss(input, std::istringstream::in);
		iss >> command;
		iss >> param0;
		iss >> param1;
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
	return 0;
}
