#ifndef __ZP_EXPLORER_H__
#define __ZP_EXPLORER_H__

#include <list>
#include <string>

namespace zp
{
class IPackage;
}

//use '/' and "/" on linux or mac
#define DIR_CHAR '\\'
#define DIR_STR "\\"

typedef bool (*FileCallback)(const std::string& path, size_t fileIndex, size_t totalFileCount);

struct ZpNode
{
	std::string			name;
	std::list<ZpNode>	children;
	ZpNode*				parent;
	bool				isDirectory;
};

class ZpExplorer
{
	friend bool addPackFile(const std::string& filename, void* param);
	friend bool countFile(const std::string& filename, void* param);

public:
	ZpExplorer();
	~ZpExplorer();

	void setCallback(FileCallback callback);

	bool open(const std::string& path);
	bool create(const std::string& path, const std::string& inputPath);
	void close();
	bool isOpen();

	zp::IPackage* getPack();
	const std::string& currentPath();

	bool enter(const std::string& path);

	//srcPath can't be empty
	//if dstPath is empty, file/dir will be add to current path of package
	bool add(const std::string& srcPath, const std::string& dstPath);

	bool remove(const std::string& path);
	
	//if srcPath is empty, current path of package will be extracted
	//if dstPath is empty, file/dir will be extracted to current path of system
	bool extract(const std::string& srcPath, const std::string& dstPath);

	const ZpNode* getNode();

private:
	void clear();

	bool addFile(const std::string& externalPath, const std::string& internalPath);
	bool extractFile(const std::string& externalPath, const std::string& internalPath);

	void countChildRecursively(ZpNode* node);

	bool removeChild(ZpNode* node, ZpNode* child);
	bool removeChildRecursively(ZpNode* node, std::string path);

	bool extractRecursively(ZpNode* node, std::string externalPath, std::string internalPath);

	void insertFileToTree(const std::string& filename);

	enum FindType
	{
		FIND_ANY = 0,
		FIND_FILE = 1,
		FIND_DIR = 2
	};
	ZpNode* findChild(ZpNode* node, const std::string& name, FindType type);
	ZpNode* findChildRecursively(ZpNode* node, const std::string& name, FindType type);

	void getPath(const ZpNode* node, std::string& path);

	std::string getArchivedName(const std::string& filename);

private:
	zp::IPackage*	m_pack;
	ZpNode			m_root;
	ZpNode*			m_currentNode;
	std::string		m_currentPath;
	std::string		m_workingPath;	//user can operate on directory other than current one
	std::string		m_basePath;		//base path of external path (of file system)
	FileCallback	m_callback;
	size_t			m_fileCount;
	size_t			m_fileIndex;
};

#endif
