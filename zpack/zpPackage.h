#ifndef __ZP_PACKAGE_H__
#define __ZP_PACKAGE_H__

#include "zpack.h"
#include <string>
#include <vector>
#include <fstream>

namespace zp
{

const u32 PACKAGE_FILE_SIGN = 'KAPZ';
const u32 CURRENT_VERSION = '0010';

const u32 FILE_FLAG_DELETED = 1;

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
	Package(const char* filename, bool readonly);
	~Package();

	bool valid() const;

	virtual bool hasFile(const char* filename);
	virtual IFile* openFile(const char* filename);
	virtual void closeFile(IFile* file);

	virtual u32 getFileCount();
	virtual bool getFileInfoByIndex(u32 index, char* filenameBuffer, u32 filenameBufferSize, u32* fileSize = NULL);

	virtual bool addFile(const char* externalFilename, const char* filename, u32 flag = FLAG_REPLACE);
	virtual bool removeFile(const char* filename);
	virtual bool dirty();
	virtual void flush();

	virtual u64 countFragmentSize(u64& bytesToMove);
	virtual bool defrag();

private:
	bool readHeader();
	bool readFileEntries();
	bool readFilenames();

	bool buildHashTable();
	int getFileIndex(const char* filename);
	void insertFile(FileEntry& entry, const char* filename);
	
	u32 stringHash(const char* str, u32 seed);

	void fixHashTable(u32 index);

private:
	std::string					m_packageName;
	std::fstream				m_stream;
	PackageHeader				m_header;
	std::vector<int>			m_hashTable;
	std::vector<FileEntry>		m_fileEntries;
	std::vector<std::string>	m_filenames;
	bool						m_readonly;
	bool						m_dirty;
};

}

#endif
