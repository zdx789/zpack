#ifndef __ZP_EXPLORER_H__
#define __ZP_EXPLORER_H__

#include <list>
#include <string>
#include "zpack.h"

namespace zp
{
class IPackage;
}

//use '/' and "/" on linux or mac
#define DIR_CHAR _T('\\')
#define DIR_STR _T("\\")

struct ZpNode
{
	ZpNode() : userData(NULL){}

	zp::String			name;
#if !(ZP_CASE_SENSITIVE)
	zp::String			lowerName;
#endif
	std::list<ZpNode>	children;
	ZpNode*				parent;
	unsigned long		fileSize;
	mutable void*		userData;
	bool				isDirectory;
};

class ZpExplorer
{
	friend bool addPackFile(const zp::String& filename, void* param);
	friend bool countFile(const zp::String& filename, void* param);

public:
	ZpExplorer();
	~ZpExplorer();

	void setCallback(zp::Callback callback, void* param);

	bool open(const zp::String& path, bool readonly = false);
	bool create(const zp::String& path, const zp::String& inputPath);
	bool attach(zp::IPackage* package);
	void close();
	bool isOpen() const;

	bool defrag();

	zp::IPackage* getPack() const;

	const zp::Char* packageFilename() const;

	bool enterDir(const zp::String& path);

	//srcPath can't be empty
	//if dstPath is empty, file/dir will be add to current path of package
	bool add(const zp::String& srcPath, const zp::String& dstPath);

	bool remove(const zp::String& path);
	
	//if srcPath is empty, current path of package will be extracted
	//if dstPath is empty, file/dir will be extracted to current path of system
	bool extract(const zp::String& srcPath, const zp::String& dstPath);

	void setCurrentNode(const ZpNode* node);
	const ZpNode* currentNode() const;
	const ZpNode* rootNode() const;

	const zp::String& currentPath() const;
	void getNodePath(const ZpNode* node, zp::String& path) const;

	unsigned long countDiskFile(const zp::String& path);
	unsigned long countNodeFile(const ZpNode* node);

private:
	void clear();

	void build();

	bool addFile(const zp::String& externalPath, const zp::String& internalPath);
	bool extractFile(const zp::String& externalPath, const zp::String& internalPath);

	void countChildRecursively(const ZpNode* node);

	bool removeChild(ZpNode* node, ZpNode* child);
	bool removeChildRecursively(ZpNode* node, zp::String path);

	bool extractRecursively(ZpNode* node, zp::String externalPath, zp::String internalPath);

	void insertFileToTree(const zp::String& filename, unsigned long fileSize, bool checkFileExist);

	enum FindType
	{
		FIND_ANY = 0,
		FIND_FILE = 1,
		FIND_DIR = 2
	};
	ZpNode* findChild(ZpNode* node, const zp::String& name, FindType type);
	ZpNode* findChildRecursively(ZpNode* node, const zp::String& name, FindType type);

	zp::String getArchivedName(const zp::String& filename);

private:
	zp::IPackage*	m_pack;
	ZpNode			m_root;
	ZpNode*			m_currentNode;
	zp::String		m_currentPath;
	zp::String		m_workingPath;	//user can operate on directory other than current one
	zp::String		m_basePath;		//base path of external path (of file system)
	zp::Callback	m_callback;
	void*			m_callbackParam;
	unsigned long	m_fileCount;
};

#endif
