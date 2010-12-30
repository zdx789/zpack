#ifndef __ZPACK_H__
#define __ZPACK_H__

namespace zp
{

extern const bool CASE_SENSITIVE;

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
	virtual bool hasFile(const char* filename) = 0;
	virtual IFile* openFile(const char* filename) = 0;
	virtual void closeFile(IFile* file) = 0;
	
	virtual u32 getFileCount() = 0;
	virtual bool getFilenameByIndex(char* buffer, u32 bufferSize, u32 index) = 0;

	//package manipulation fuctions
	virtual bool addFile(const char* externalFilename, const char* filename, u32 flag = FLAG_REPLACE) = 0;
	virtual bool removeFile(const char* filename) = 0;
	virtual bool dirty() = 0;
	virtual void flush() = 0;

	virtual u64 countFragmentSize(u64& bytesToMove) = 0;
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
