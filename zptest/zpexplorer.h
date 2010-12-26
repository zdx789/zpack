#ifndef __ZP_EXPLORER_H__
#define __ZP_EXPLORER_H__

#include <list>
#include <string>
#include "fileenum.h"

namespace zp
{
class IPackage;
}

struct ZpNode
{
	std::string			name;
	std::list<ZpNode>	children;
	ZpNode*				parent;
	bool				isDirectory;
};

class ZpExplorer
{
	friend void WINAPI addPackFile(const std::string& filename, void* param);

public:
	ZpExplorer();
	~ZpExplorer();

	bool open(const std::string& path);
	bool create(const std::string& path, const std::string& inputPath);
	void close();
	bool isOpen();

	zp::IPackage* getPack();
	const std::string& getPath();

	void enterRoot();
	void enter(const std::string& dir);
	void exit();

	bool add(const std::string& path);
	bool remove(const std::string& path);
	bool extract(const std::string& name, const std::string& path);

	const ZpNode* getNode();

private:
	void clear();

	bool addFile(const std::string& filename, const std::string& archivedName);

	void removeChildRecursively(ZpNode* node, const std::string& path);

	void insertFileToTree(const std::string& filename);

	enum FindType
	{
		FIND_ANY = 0,
		FIND_FILE = 1,
		FIND_DIR = 2
	};
	ZpNode* findChild(ZpNode* node, const std::string& name, FindType type, std::list<ZpNode>::iterator* returnIter = NULL);

	void updatePath();

	std::string getArchivedName(const std::string& filename);

private:
	zp::IPackage*	m_pack;
	ZpNode			m_root;
	ZpNode*			m_currentNode;
	std::string		m_currentPath;
};

#endif
