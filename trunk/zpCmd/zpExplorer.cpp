#include "Stdafx.h"
#include "zpExplorer.h"
#include "zpack.h"
#include <cassert>
#include <fstream>
#include <algorithm>
#include "fileEnum.h"
#include "windows.h"

using namespace std;

///////////////////////////////////////////////////////////////////////////////////////////////////
ZpExplorer::ZpExplorer()
	: m_pack(NULL)
	, m_callback(NULL)
	, m_fileCount(0)
	, m_callbackParam(NULL)
{
	m_root.isDirectory = true;
	m_root.parent = NULL;
	m_currentNode = &m_root;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
ZpExplorer::~ZpExplorer()
{
	if (m_pack != NULL)
	{
		zp::close(m_pack);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void ZpExplorer::setCallback(FileCallback callback, void* param)
{
	m_callback = callback;
	m_callbackParam = param;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool ZpExplorer::open(const string& path)
{
	clear();
	m_pack = zp::open(path.c_str(), 0);
	if (m_pack == NULL)
	{
		return false;
	}
	m_packageFilename = path;
	unsigned long count = m_pack->getFileCount();
	for (unsigned long i = 0; i < count; ++i)
	{
		char buffer[256];
		zp::u32 fileSize;
		m_pack->getFileInfoByIndex(i, buffer, sizeof(buffer), &fileSize);
		string filename = buffer;
		insertFileToTree(filename, fileSize, false);
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool ZpExplorer::create(const string& path, const string& inputPath)
{
	clear();
	if (path.empty())
	{
		return false;
	}
	m_packageFilename = path;
	m_pack = zp::create(path.c_str());
	if (m_pack == NULL)
	{
		return false;
	}
	if (inputPath.empty())
	{
		return true;
	}
	m_basePath = inputPath;
	if (m_basePath.c_str()[m_basePath.length() - 1] != DIR_CHAR)
	{
		m_basePath += DIR_STR;
	}
	enumFile(m_basePath, addPackFile, this);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void ZpExplorer::close()
{
	clear();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool ZpExplorer::isOpen() const
{
	return (m_pack != NULL);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
zp::IPackage* ZpExplorer::getPack()
{
	return m_pack;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
const std::string& ZpExplorer::packageFilename() const
{
	return m_packageFilename;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
const string& ZpExplorer::currentPath() const
{
	return m_currentPath;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool ZpExplorer::enterDir(const string& path)
{
	assert(m_currentNode != NULL);
#if !(ZP_CASE_SENSITIVE)
	string lowerPath = path;
	transform(path.begin(), path.end(), lowerPath.begin(), ::tolower);
	ZpNode* child = findChildRecursively(m_currentNode, lowerPath, FIND_DIR);
#else
	ZpNode* child = findChildRecursively(m_currentNode, path, FIND_DIR);
#endif
	if (child == NULL)
	{
		return false;
	}
	m_currentNode = child;
	getNodePath(m_currentNode, m_currentPath);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool ZpExplorer::add(const string& srcPath, const string& dstPath)
{
	if (m_pack == NULL || srcPath.empty())
	{
		return false;
	}
	if (dstPath.empty())
	{
		m_workingPath = m_currentPath;
	}
	else if (dstPath[0] == DIR_CHAR)
	{
		m_workingPath = dstPath.substr(1, dstPath.length() - 1);
	}
	else
	{
		m_workingPath = m_currentPath + dstPath;
	}
	if (!m_workingPath.empty() && m_workingPath[m_workingPath.length() - 1] != DIR_CHAR)
	{
		m_workingPath += DIR_STR;
	}
	m_basePath.clear();
	size_t pos = srcPath.rfind(DIR_CHAR);

	WIN32_FIND_DATAA fd;
	HANDLE findFile = ::FindFirstFileA(srcPath.c_str(), &fd);
	if (findFile != INVALID_HANDLE_VALUE && (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
	{
		//it's a file
		string nakedFilename = srcPath.substr(pos + 1, srcPath.length() - pos - 1);
		bool ret = addFile(srcPath, nakedFilename);
		m_pack->flush();
		return ret;
	}
	//it's a directory
	if (pos != string::npos)
	{
		//dir
		m_basePath = srcPath.substr(0, pos + 1);
	}
	string searchDirectory = srcPath;
	if (srcPath.c_str()[srcPath.length() - 1] != DIR_CHAR)
	{
		searchDirectory += DIR_STR;
	}
	enumFile(searchDirectory, addPackFile, this);
	m_pack->flush();
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool ZpExplorer::remove(const string& path)
{
	if (path.empty())
	{
		return false;
	}
	list<ZpNode>::iterator found;
#if !(ZP_CASE_SENSITIVE)
	string lowerPath = path;
	transform(path.begin(), path.end(), lowerPath.begin(), ::tolower);
	ZpNode* child = findChildRecursively(m_currentNode, lowerPath, FIND_ANY);
#else
	ZpNode* child = findChildRecursively(m_currentNode, path, FIND_ANY);
#endif
	if (child == NULL)
	{
		return false;
	}
	string internalPath;
	getNodePath(child, internalPath);
	//remove '\'
	if (!internalPath.empty())
	{
		internalPath.resize(internalPath.size() - 1);
	}
	bool ret = false;
	if (removeChildRecursively(child, internalPath))
	{
		ret = true;
		if (child->parent != NULL)
		{
			if (child == m_currentNode)
			{
				m_currentNode = child->parent;
			}
			removeChild(child->parent, child);
		}
	}
	getNodePath(m_currentNode, m_currentPath);
	m_pack->flush();
	return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool ZpExplorer::extract(const string& srcPath, const string& dstPath)
{
	string externalPath = dstPath;
	if (externalPath.empty())
	{
		externalPath = "."DIR_STR;
	}
	else if (externalPath.c_str()[externalPath.length() - 1] != DIR_CHAR)
	{
		externalPath += DIR_STR;
	}
#if !(ZP_CASE_SENSITIVE)
	string lowerPath = srcPath;
	transform(srcPath.begin(), srcPath.end(), lowerPath.begin(), ::tolower);
	ZpNode* child = findChildRecursively(m_currentNode, lowerPath, FIND_ANY);
#else
	ZpNode* child = findChildRecursively(m_currentNode, srcPath, FIND_ANY);
#endif
	if (child == NULL)
	{
		return false;
	}
	string internalPath;
	getNodePath(child, internalPath);
	//remove '\'
	if (!internalPath.empty())
	{
		internalPath.resize(internalPath.size() - 1);
	}
	return extractRecursively(child, externalPath, internalPath);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void ZpExplorer::setCurrentNode(const ZpNode* node)
{
	assert(node != NULL);
	m_currentNode = const_cast<ZpNode*>(node);
	getNodePath(m_currentNode, m_currentPath);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
const ZpNode* ZpExplorer::currentNode() const
{
	return m_currentNode;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
const ZpNode* ZpExplorer::rootNode() const
{
	return &m_root;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void ZpExplorer::clear()
{
	m_root.children.clear();
	m_root.userData = NULL;
	m_currentNode = &m_root;
	getNodePath(m_currentNode, m_currentPath);
	m_workingPath = m_currentPath;
	if (m_pack != NULL)
	{
		zp::close(m_pack);
		m_pack = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool ZpExplorer::addFile(const string& filename, const string& relativePath)
{
	if (m_callback != NULL && !m_callback(relativePath, m_callbackParam))
	{
		return false;
	}
	string internalName = m_workingPath + relativePath;
	if (!m_pack->addFile(filename.c_str(), internalName.c_str()))
	{
		return false;
	}

	zp::u32 fileSize = 0;
	fstream stream;
	stream.open(filename.c_str(), ios_base::in | ios_base::binary);
	if (stream.is_open())
	{
		stream.seekg(0, ios::end);
		fileSize = static_cast<zp::u32>(stream.tellg());
	}
	insertFileToTree(internalName, fileSize, true);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool ZpExplorer::extractFile(const string& externalPath, const string& internalPath)
{
	assert(m_pack != NULL);
	zp::IFile* file = m_pack->openFile(internalPath.c_str());
	if (file == NULL)
	{
		return false;
	}
	fstream stream;
	stream.open(externalPath.c_str(), ios_base::out | ios_base::trunc | ios_base::binary);
	if (!stream.is_open())
	{
		return false;
	}
	char* buffer = new char[file->size()];
	file->read(buffer, file->size());
	stream.write(buffer, file->size());
	stream.close();
	m_pack->closeFile(file);
	delete [] buffer;
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void ZpExplorer::countChildRecursively(const ZpNode* node)
{
	if (!node->isDirectory)
	{
		++m_fileCount;
	}
	for (list<ZpNode>::const_iterator iter = node->children.begin();
		iter != node->children.end();
		++iter)
	{
		countChildRecursively(&(*iter));
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool ZpExplorer::removeChild(ZpNode* node, ZpNode* child)
{
	assert(node != NULL && child != NULL);
	for (list<ZpNode>::iterator iter = node->children.begin();
		iter != node->children.end();
		++iter)
	{
		if (child == &(*iter))
		{
			node->children.erase(iter);
			return true;
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool ZpExplorer::removeChildRecursively(ZpNode* node, string path)
{
	assert(node != NULL && m_pack != NULL);
	if (!node->isDirectory)
	{
		if (m_callback != NULL && !m_callback(node->name, m_callbackParam))
		{
			return false;
		}
		if (!m_pack->removeFile(path.c_str()))
		{
			return false;
		}
		return true;
	}
	if (!path.empty())
	{
		path += DIR_STR;
	}
	//recurse
	for (list<ZpNode>::iterator iter = node->children.begin();
		iter != node->children.end();)
	{
		ZpNode* child = &(*iter);
		if (!removeChildRecursively(child, path + child->name))
		{
			return false;
		}
		if (child == m_currentNode)
		{
			m_currentNode = m_currentNode->parent;
			assert(m_currentNode != NULL);
		}
		iter = node->children.erase(iter);
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool ZpExplorer::extractRecursively(ZpNode* node, string externalPath, string internalPath)
{
	assert(node != NULL && m_pack != NULL);
	externalPath += node->name;
	if (!node->isDirectory)
	{
		if (m_callback != NULL && !m_callback(internalPath, m_callbackParam))
		{
			return false;
		}
		if (!extractFile(externalPath, internalPath))
		{
			return false;
		}
		return true;
	}
	if (!internalPath.empty())	//in case extract the root directory
	{
		externalPath += DIR_STR;
		internalPath += DIR_STR;
		//create directory if necessary
		WIN32_FIND_DATAA fd;
		HANDLE findFile = ::FindFirstFileA(externalPath.c_str(), &fd);
		if (findFile == INVALID_HANDLE_VALUE)
		{
			if (!::CreateDirectoryA(externalPath.c_str(), NULL))
			{
				return false;
			}
		}
		else if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			return false;
		}
	}
	//recurse
	for (list<ZpNode>::iterator iter = node->children.begin();
		iter != node->children.end();
		++iter)
	{
		if (!extractRecursively(&(*iter), externalPath, internalPath + iter->name))
		{
			return false;
		}
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void ZpExplorer::insertFileToTree(const string& filename, unsigned long fileSize, bool checkFileExist)
{
	ZpNode* node = &m_root;
	string filenameLeft = filename;
	while (filenameLeft.length() > 0)
	{
		size_t pos = filenameLeft.find_first_of(DIR_STR);
		if (pos == string::npos)
		{
			//it's a file
		#if !(ZP_CASE_SENSITIVE)
			string lowerName = filenameLeft;
			transform(filenameLeft.begin(), filenameLeft.end(), lowerName.begin(), ::tolower);
			ZpNode* child = checkFileExist ? findChild(node, lowerName, FIND_FILE) : NULL;
		#else
			ZpNode* child = checkFileExist ? findChild(node, filenameLeft, FIND_FILE) : NULL;
		#endif
			if (child == NULL)
			{
				ZpNode newNode;
				newNode.parent = node;
				newNode.isDirectory = false;
				newNode.name = filenameLeft;
			#if !(ZP_CASE_SENSITIVE)
				newNode.lowerName = lowerName;
			#endif
				newNode.fileSize = fileSize;
				node->children.push_back(newNode);
			}
			else
			{
				child->fileSize = fileSize;
			}
			return;
		}
		string dirName = filenameLeft.substr(0, pos);
		filenameLeft = filenameLeft.substr(pos + 1, filenameLeft.length() - pos - 1);
	#if !(ZP_CASE_SENSITIVE)
		string lowerName = dirName;
		transform(dirName.begin(), dirName.end(), lowerName.begin(), ::tolower);
		ZpNode* child = findChild(node, lowerName, FIND_DIR);
	#else
		ZpNode* child = findChild(node, dirName, FIND_DIR);
	#endif
		if (child != NULL)
		{
			node = child;
		}
		else
		{
			ZpNode newNode;
			newNode.isDirectory = true;
			newNode.parent = node;
			newNode.name = dirName;
		#if !(ZP_CASE_SENSITIVE)
			newNode.lowerName = lowerName;
		#endif
			newNode.fileSize = 0;
			//insert after all directories
			list<ZpNode>::iterator insertPoint;
			for (insertPoint = node->children.begin(); insertPoint != node->children.end();	++insertPoint)
			{
				if (!insertPoint->isDirectory)
				{
					break;
				}
			}
			list<ZpNode>::iterator inserted;
			inserted = node->children.insert(insertPoint, newNode);
			node = &(*inserted);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
ZpNode* ZpExplorer::findChild(ZpNode* node, const string& name, FindType type)
{
	if (name.empty())
	{
		return  type == FIND_FILE ? NULL : &m_root;
	}
	assert(node != NULL);
	if (name == ".")
	{
		return type == FIND_FILE ? NULL : node;
	}
	else if (name == "..")
	{
		return type == FIND_FILE ? NULL : node->parent;
	}
	for (list<ZpNode>::iterator iter = node->children.begin();
		iter != node->children.end();
		++iter)
	{
		if ((type == FIND_DIR && !iter->isDirectory) || (type == FIND_FILE && iter->isDirectory))
		{
			continue;
		}
	#if !(ZP_CASE_SENSITIVE)
		if (name == iter->lowerName)
		{
			return &(*iter);
		}
	#else
		if (name == iter->name)
		{
			return &(*iter);
		}
	#endif
	}
	return NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
ZpNode* ZpExplorer::findChildRecursively(ZpNode* node, const string& name, FindType type)
{
	size_t pos = name.find_first_of(DIR_STR);
	if (pos == string::npos)
	{
		//doesn't have any directory name
		return name.empty()? node : findChild(node, name, type);
	}
	ZpNode* nextNode = NULL;
	string dirName = name.substr(0, pos);
	nextNode = findChild(node, dirName, FIND_DIR);
	if (nextNode == NULL)
	{
		return NULL;
	}
	string nameLeft = name.substr(pos + 1, name.length() - pos - 1);
	return findChildRecursively(nextNode, nameLeft, type);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void ZpExplorer::getNodePath(const ZpNode* node, std::string& path) const
{
	path.clear();
	while (node != NULL && node != &m_root)
	{
		path = node->name + DIR_STR + path;
		node = node->parent;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
unsigned long ZpExplorer::countDiskFile(const std::string& path)
{
	m_basePath = path;
	if (m_basePath.c_str()[m_basePath.length() - 1] != DIR_CHAR)
	{
		m_basePath += DIR_STR;
	}
	m_fileCount = 0;
	enumFile(m_basePath, countFile, this);
	if (m_fileCount == 0)
	{
		return 1;	//single file
	}
	return m_fileCount;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
unsigned long ZpExplorer::countNodeFile(const ZpNode* node)
{
	m_fileCount = 0;
	countChildRecursively(node);
	return m_fileCount;
}
