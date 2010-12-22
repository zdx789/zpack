#ifndef __ZP_EXPLORER_H__
#define __ZP_EXPLORER_H__

#include <list>
#include <string>

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
public:
	ZpExplorer(zp::IPackage* pack);

	const std::string& getPath();

	void enterRoot();
	void enter(const std::string& dir);
	void exit();

	const ZpNode* getNode();

private:
	void insertFile(std::string& filename);

	ZpNode* findSubDir(ZpNode* node, const std::string& name);

	void updatePath();

private:
	zp::IPackage*	m_pack;
	ZpNode			m_root;
	ZpNode*			m_currentNode;
	std::string		m_currentPath;
};

#endif
