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

typedef bool (*FileCallback)(const std::string& path, void* param);

struct ZpNode
{
	ZpNode() : userData(NULL){}

	std::string			name;
#if !(ZP_CASE_SENSITIVE)
	std::string			lowerName;
#endif
	std::list<ZpNode>	children;
	ZpNode*				parent;
	unsigned long		fileSize;
	mutable void*		userData;
	bool				isDirectory;
};

class ZpExplorer
{
	friend bool addPackFile(const std::string& filename, void* param);
	friend bool countFile(const std::string& filename, void* param);

public:
	ZpExplorer();
	~ZpExplorer();

	void setCallback(FileCallback callback, void* param);

	bool open(const std::string& path);
	bool create(const std::string& path, const std::string& inputPath);
	void close();
	bool isOpen() const;

	zp::IPackage* getPack();
	
	const std::string& packageFilename() const;

	bool enterDir(const std::string& path);

	//srcPath can't be empty
	//if dstPath is empty, file/dir will be add to current path of package
	bool add(const std::string& srcPath, const std::string& dstPath);

	bool remove(const std::string& path);
	
	//if srcPath is empty, current path of package will be extracted
	//if dstPath is empty, file/dir will be extracted to current path of system
	bool extract(const std::string& srcPath, const std::string& dstPath);

	void setCurrentNode(const ZpNode* node);
	const ZpNode* currentNode() const;
	const ZpNode* rootNode() const;

	const std::string& currentPath() const;
	void getNodePath(const ZpNode* node, std::string& path) const;

	unsigned long countDiskFile(const std::string& path);
	unsigned long countNodeFile(const ZpNode* node);

private:
	void clear();

	bool addFile(const std::string& externalPath, const std::string& internalPath);
	bool extractFile(const std::string& externalPath, const std::string& internalPath);

	void countChildRecursively(const ZpNode* node);

	bool removeChild(ZpNode* node, ZpNode* child);
	bool removeChildRecursively(ZpNode* node, std::string path);

	bool extractRecursively(ZpNode* node, std::string externalPath, std::string internalPath);

	void insertFileToTree(const std::string& filename, unsigned long fileSize, bool checkFileExist);

	enum FindType
	{
		FIND_ANY = 0,
		FIND_FILE = 1,
		FIND_DIR = 2
	};
	ZpNode* findChild(ZpNode* node, const std::string& name, FindType type);
	ZpNode* findChildRecursively(ZpNode* node, const std::string& name, FindType type);

	std::string getArchivedName(const std::string& filename);

private:
	zp::IPackage*	m_pack;
	ZpNode			m_root;
	ZpNode*			m_currentNode;
	std::string		m_packageFilename;
	std::string		m_currentPath;
	std::string		m_workingPath;	//user can operate on directory other than current one
	std::string		m_basePath;		//base path of external path (of file system)
	FileCallback	m_callback;
	void*			m_callbackParam;
	size_t			m_fileCount;
};

#endif
