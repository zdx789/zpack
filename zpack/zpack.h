#ifndef __ZPACK_H__
#define __ZPACK_H__

namespace zp
{

#define ZP_CASE_SENSITIVE	0

typedef unsigned long u32;
typedef unsigned __int64 u64;

const u32 FLAG_READONLY = 1;
const u32 FLAG_REPLACE = 2;

class IFile;

///////////////////////////////////////////////////////////////////////////////////////////////////
class IPackage
{
public:
	//readonly functions, not available when package is dirty
	virtual bool hasFile(const char* filename) const = 0;
	virtual IFile* openFile(const char* filename) = 0;
	virtual void closeFile(IFile* file) = 0;
	
	virtual u32 getFileCount() const = 0;
	virtual bool getFileInfoByIndex(u32 index, char* filenameBuffer, u32 filenameBufferSize, u32* fileSize = 0) = 0;

	//package manipulation fuctions
	virtual bool addFile(const char* externalFilename, const char* filename, u32 flag = FLAG_REPLACE) = 0;
	virtual bool removeFile(const char* filename) = 0;
	virtual bool dirty() const = 0;
	virtual void flush() = 0;

	virtual u64 countFragmentSize() = 0;
	virtual bool defrag() = 0;	//can be very slow, don't call this all the time

protected:
	virtual ~IPackage(){}
};

///////////////////////////////////////////////////////////////////////////////////////////////////
class IFile
{
public:
	virtual u32 size() = 0;
	virtual void seek(u32 pos) = 0;
	virtual u32 read(void* buffer, u32 size) = 0;

protected:
	virtual ~IFile(){}
};

///////////////////////////////////////////////////////////////////////////////////////////////////
IPackage* create(const char* filename, u32 flag = 0);
IPackage* open(const char* filename, u32 flag = FLAG_READONLY);
void close(IPackage* package);

}

#endif
