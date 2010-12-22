#include "zpexplorer.h"
#include "zpack.h"
#include <cassert>
#include <algorithm>

///////////////////////////////////////////////////////////////////////////////////////////////////
ZpExplorer::ZpExplorer(zp::IPackage* pack)
	: m_pack(pack)
{
	m_root.isDirectory = true;
	m_root.parent = NULL;
	m_currentNode = &m_root;

	unsigned long count = pack->getFileCount();
	for (unsigned long i = 0; i < count; ++i)
	{
		char buffer[256];
		pack->getFilenameByIndex(buffer, sizeof(buffer), i);
		std::string filename = buffer;
		insertFile(filename);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
const std::string& ZpExplorer::getPath()
{
	return m_currentPath;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void ZpExplorer::enterRoot()
{
	m_currentNode = &m_root;
	updatePath();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void ZpExplorer::enter(const std::string& dir)
{
	assert(m_currentNode != NULL);
	ZpNode* child = findSubDir(m_currentNode, dir);
	if (child != NULL)
	{
		m_currentNode = child;
		updatePath();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void ZpExplorer::exit()
{
	if (m_currentNode->parent != NULL)
	{
		m_currentNode = m_currentNode->parent;
		updatePath();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
const ZpNode* ZpExplorer::getNode()
{
	return m_currentNode;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void ZpExplorer::insertFile(std::string& filename)
{
	assert(m_currentNode == &m_root);
	ZpNode* node = m_currentNode;
	while (filename.length() > 0)
	{
		size_t pos = filename.find_first_of("/");
		if (pos == std::string::npos)
		{
			ZpNode newNode;
			newNode.parent = node;
			newNode.isDirectory = false;
			newNode.name = filename;
			node->children.push_back(newNode);
			return;
		}
		std::string dirName = filename.substr(0, pos);
		filename = filename.substr(pos + 1, filename.length() - pos - 1);
		ZpNode* child = findSubDir(node, dirName);
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
ZpNode* ZpExplorer::findSubDir(ZpNode* node, const std::string& name)
{
	assert(node != NULL);
	std::string lowerName = name;
	std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
	for (std::list<ZpNode>::iterator iter = node->children.begin();
		iter != node->children.end();
		++iter)
	{
		if (!iter->isDirectory)
		{
			continue;
		}
		std::string dirName = iter->name;
		std::transform(dirName.begin(), dirName.end(), dirName.begin(), ::tolower);
		if (dirName == lowerName)
		{
			return &(*iter);
		}
	}
	return NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void ZpExplorer::updatePath()
{
	m_currentPath.clear();
	ZpNode* node = m_currentNode;
	while (node != NULL)
	{
		m_currentPath = node->name + "/" + m_currentPath;
		node = node->parent;
	}
}
