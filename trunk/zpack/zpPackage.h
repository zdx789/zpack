#ifndef __ZP_PACKAGE_H__
#define __ZP_PACKAGE_H__

#include "zpack.h"
#include <string>
#include <vector>
//#include <fstream>
#include "stdio.h"

namespace zp
{

#if defined (ZP_USE_WCHAR)
	typedef std::wistringstream IStringStream;
	#define Remove _wremove
	#define Rename _wrename
#else
	typedef std::istringstream IStringStream;
	#define Remove remove
	#define Rename rename
#endif

const u32 PACKAGE_FILE_SIGN = 'KAPZ';
const u32 CURRENT_VERSION = '0010';

const u32 FILE_FLAG_DELETED = 1;

#define ZP_HASH_NUM		3	//can be 1, 2 or 3. the less the faster	

///////////////////////////////////////////////////////////////////////////////////////////////////
struct PackageHeader
{
	u32	sign;
	u32	version;
	u32	headerSize;
	u32	fileCount;
	u64	fileEntryOffset;
	u64 filenameOffset;
	u32	fileEntrySize;	//size of single entry
	u32 filenameSize;	//size of all filenames
	u32	flag;
	u32 reserved;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
struct FileEntry
{
	u64	byteOffset;
	u32	fileSize;
	u32	hash0;
	u32	hash1;
	u32	hash2;
	u32 flag;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
class Package : public IPackage
{
public:
	Package(const Char* filename, bool readonly, bool readFilename);
	~Package();

	bool valid() const;

	virtual bool readonly() const;

	virtual const Char* packageFilename() const;

	virtual bool hasFile(const Char* filename) const;
	virtual IFile* openFile(const Char* filename);
	virtual void closeFile(IFile* file);

	virtual u32 getFileCount() const;
	virtual bool getFileInfo(u32 index, Char* filenameBuffer, u32 filenameBufferSize, u32* fileSize = NULL) const;

	virtual bool addFile(const Char* filename, void* buffer, u32 size, u32 flag = FLAG_REPLACE);
	virtual bool removeFile(const Char* filename);
	virtual bool dirty() const;
	virtual void flush();

	virtual u64 countFragmentSize() const;
	virtual bool defrag(Callback callback, void* callbackParam);

private:
	bool readHeader();
	bool readFileEntries();
	bool readFilenames();

	bool buildHashTable();
	int getFileIndex(const Char* filename) const;
	void insertFile(FileEntry& entry, const Char* filename);

	u32 stringHash(const Char* str, u32 seed) const;

	void fixHashTable(u32 index);

	u32 countFilenameSize() const;

private:
	String					m_packageFilename;
	//mutable std::fstream	m_stream;
	mutable FILE*			m_stream;
	PackageHeader			m_header;
	std::vector<int>		m_hashTable;
	std::vector<FileEntry>	m_fileEntries;
	std::vector<String>		m_filenames;
	u64						m_packageEnd;
	u32						m_hashMask;
	bool					m_readonly;
	bool					m_readFilename;
	bool					m_dirty;
};

}

#endif
