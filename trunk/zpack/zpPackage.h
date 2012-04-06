#ifndef __ZP_PACKAGE_H__
#define __ZP_PACKAGE_H__

#include "zpack.h"
#include <string>
#include <vector>
#include "stdio.h"

namespace zp
{

#if defined (ZP_USE_WCHAR)
	#define Remove _wremove
	#define Rename _wrename
	typedef std::wistringstream IStringStream;
#else
	#define Remove remove
	#define Rename rename
	typedef std::istringstream IStringStream;
#endif

const u32 PACKAGE_FILE_SIGN = 'KAPZ';
const u32 CURRENT_VERSION = '0030';

///////////////////////////////////////////////////////////////////////////////////////////////////
struct PackageHeader
{
	u32	sign;
	u32	version;
	u32	headerSize;
	u32	fileCount;
	u64	fileEntryOffset;
	u64 filenameOffset;
	u32	fileEntrySize;		//size of all file entries, in bytes
	u32 filenameSize;		//size of all filenames, in bytes
	u32 originFilenameSize;	//filename size before compression
	u32 chunkSize;			//file compress unit
	u32	flag;
	u32 reserved[19];
};

///////////////////////////////////////////////////////////////////////////////////////////////////
struct FileEntry
{
	u64	byteOffset;
	u64	nameHash;
	u32	packSize;	//size in package
	u32 originSize;
	u32 flag;
	u32 chunkSize;	//can be different with chunkSize in package header
	u32 reserved[2];
};

///////////////////////////////////////////////////////////////////////////////////////////////////
class Package : public IPackage
{
	friend class File;
	friend class CompressedFile;
	friend class WriteFile;

public:
	Package(const Char* filename, bool readonly, bool readFilename);
	~Package();

	bool valid() const;

	virtual bool readonly() const;

	virtual const Char* packageFilename() const;

	virtual bool hasFile(const Char* filename) const;
	virtual IReadFile* openFile(const Char* filename);
	virtual void closeFile(IReadFile* file);

	virtual u32 getFileCount() const;
	virtual bool getFileInfo(u32 index, Char* filenameBuffer, u32 filenameBufferSize,
							u32* fileSize = 0, u32* packSize = 0, u32* flag = 0) const;

	virtual bool addFile(const Char* filename, const Char* exterFilename, u32 fileSize, u32 flag,
						u32* outPackSize = 0, u32* outFlag = 0);
	virtual IWriteFile* createFile(const Char* filename, u32 fileSize, u32 packSize,
									u32 chunkSize = 0, u32 flag = 0);
	virtual void closeFile(IWriteFile* file);

	virtual bool removeFile(const Char* filename);
	virtual bool dirty() const;
	virtual void flush();

	virtual u64 countFragmentSize() const;
	virtual bool defrag(Callback callback, void* callbackParam);

private:
	bool readHeader();
	bool readFileEntries();
	bool readFilenames();

	void removeDeletedEntries();

	void writeTables(bool avoidOverwrite);

	bool buildHashTable();
	int getFileIndex(const Char* filename) const;
	int getFileIndex(u64 nameHash) const;
	u32 insertFileEntry(FileEntry& entry, const Char* filename);
	bool insertFileHash(u64 nameHash, u32 entryIndex);

	u64 stringHash(const Char* str, u32 seed) const;

	void fixHashTable(u32 index);

	void writeRawFile(FileEntry& entry, FILE* file);
	void writeCompressFile(FileEntry& entry, FILE* file);

	//for writing file
	u32 getFileAvailableSize(u64 nameHash) const;
	bool setFileAvailableSize(u64 nameHash, u32 size);

private:
	String					m_packageFilename;
	mutable FILE*			m_stream;
	PackageHeader			m_header;
	u32						m_hashBits;
	std::vector<int>		m_hashTable;
	std::vector<FileEntry>	m_fileEntries;
	std::vector<String>		m_filenames;
	u64						m_packageEnd;
	u32						m_hashMask;
	std::vector<u8>			m_chunkData;
	std::vector<u8>			m_compressBuffer;
	std::vector<u32>		m_chunkPosBuffer;
	mutable void*			m_lastSeekFile;
	bool					m_readonly;
	bool					m_readFilename;
	bool					m_dirty;
};

}

#endif
