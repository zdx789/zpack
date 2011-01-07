#include "fileEnum.h"
#include <string>
#include <iostream>
#include <cassert>
#include "zpack.h"
#include "zpExplorer.h"
#include "windows.h"

bool enumFile(const std::string& searchPath, EnumCallback callback, void* param)
{
	WIN32_FIND_DATAA fd;
	HANDLE findFile = ::FindFirstFileA((searchPath + "*").c_str(), &fd);

	if (findFile == INVALID_HANDLE_VALUE)
	{
		::FindClose(findFile);
		return true;
	}

	while (true)
	{
		if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			//file
			if (!callback(searchPath + fd.cFileName, param))
			{
				return false;
			}
		}
		else if (strcmp(fd.cFileName, ".") != 0 && strcmp(fd.cFileName, "..")  != 0)
		{
			//folder
			if (!enumFile(searchPath + fd.cFileName + DIR_STR, callback, param))
			{
				return false;
			}
		}
		if (!FindNextFileA(findFile, &fd))
		{
			::FindClose(findFile);
			return true;
		}
	}
	return true;
}

bool addPackFile(const std::string& filename, void* param)
{
	ZpExplorer* explorer = reinterpret_cast<ZpExplorer*>(param);
	std::string relativePath = filename.substr(explorer->m_basePath.length(), filename.length() - explorer->m_basePath.length());
	return explorer->addFile(filename, relativePath);
}

bool countFile(const std::string& filename, void* param)
{
	ZpExplorer* explorer = reinterpret_cast<ZpExplorer*>(param);
	++explorer->m_fileCount;
	return true;
}