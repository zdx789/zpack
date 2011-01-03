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
	, m_fileIndex(0)
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
void ZpExplorer::setCallback(FileCallback callback)
{
	m_callback = callback;
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
		m_pack->getFilenameByIndex(buffer, sizeof(buffer), i);
		string filename = buffer;
		insertFileToTree(filename);
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
	m_fileCount = 0;
	m_fileIndex = 0;
	enumFile(m_basePath, countFile, this);
	if (m_fileCount > 0)
	{
		enumFile(m_basePath, addPackFile, this);
	}
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
	ZpNode* child = findChildRecursively(m_currentNode, path, FIND_DIR);
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
		return addFile(srcPath, nakedFilename);
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
	m_fileCount = 0;
	m_fileIndex = 0;
	enumFile(searchDirectory, countFile, this);
	if (m_fileCount == 0)
	{
		return false;
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
	ZpNode* child = findChildRecursively(m_currentNode, path, FIND_ANY);
	if (child == NULL)
	{
		return false;
	}
	m_fileCount = 0;
	m_fileIndex = 0;
	countChildRecursively(child);
	if (m_fileCount == 0)
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
	ZpNode* child = findChildRecursively(m_currentNode, srcPath, FIND_ANY);
	if (child == NULL)
	{
		return false;
	}
	m_fileCount = 0;
	m_fileIndex = 0;
	countChildRecursively(child);
	if (m_fileCount == 0)
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
	string internalName = m_workingPath + relativePath;
	if (!m_pack->addFile(filename.c_str(), internalName.c_str()))
	{
		return false;
	}
	insertFileToTree(internalName);
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
void ZpExplorer::countChildRecursively(ZpNode* node)
{
	if (!node->isDirectory)
	{
		++m_fileCount;
	}
	for (list<ZpNode>::iterator iter = node->children.begin();
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
		if (!m_pack->removeFile(path.c_str()))
		{
			return false;
		}
		++m_fileIndex;
		if (m_callback != NULL && !m_callback(node->name, m_fileIndex, m_fileCount))
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
		if (!extractFile(externalPath, internalPath))
		{
			return false;
		}
		++m_fileIndex;
		if (m_callback != NULL && !m_callback(externalPath, m_fileIndex, m_fileCount))
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
void ZpExplorer::insertFileToTree(const string& filename)
{
	ZpNode* node = &m_root;
	string filenameLeft = filename;
	while (filenameLeft.length() > 0)
	{
		size_t pos = filenameLeft.find_first_of(DIR_STR);
		if (pos == string::npos)
		{
			//it's a file
			ZpNode* child = findChild(node, filenameLeft, FIND_FILE);
			if (child == NULL)
			{
				ZpNode newNode;
				newNode.parent = node;
				newNode.isDirectory = false;
				newNode.name = filenameLeft;
				node->children.push_back(newNode);
			}
			return;
		}
		string dirName = filenameLeft.substr(0, pos);
		filenameLeft = filenameLeft.substr(pos + 1, filenameLeft.length() - pos - 1);
		ZpNode* child = findChild(node, dirName, FIND_DIR);
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
			node->children.push_back(newNode);
			node = &node->children.back();
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
	string lowerName = name;
	if (!zp::CASE_SENSITIVE)
	{
		transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
	}
	for (list<ZpNode>::iterator iter = node->children.begin();
		iter != node->children.end();
		++iter)
	{
		if ((type == FIND_DIR && !iter->isDirectory) || (type == FIND_FILE && iter->isDirectory))
		{
			continue;
		}
		string dirName = iter->name;
		if (!zp::CASE_SENSITIVE)
		{
			transform(dirName.begin(), dirName.end(), dirName.begin(), ::tolower);
		}
		if (dirName == lowerName)
		{
			return &(*iter);
		}
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
