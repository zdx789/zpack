#ifndef __ZPACK_H__
#define __ZPACK_H__

#include <string>

#if defined (_MSC_VER) && defined (UNICODE)
	#define ZP_USE_WCHAR
#endif

#if defined (_MSC_VER)
	#define ZP_CASE_SENSITIVE	0
#else
	#define ZP_CASE_SENSITIVE	1
#endif

namespace zp
{
#if defined (ZP_USE_WCHAR)
	typedef wchar_t Char;
	#ifndef _T
		#define _T(str) L##str
	#endif
	typedef std::wstring String;
	#define Fopen _wfopen
#else
	typedef char Char;
	#ifndef _T
		#define _T(str) str
	#endif
	typedef std::string String;
	#define Fopen fopen
#endif


typedef unsigned long u32;
typedef unsigned long long u64;

const u32 FLAG_READONLY = 1;
const u32 FLAG_NO_FILENAME = 2;
const u32 FLAG_REPLACE = 4;

typedef bool (*Callback)(const Char* path, void* param);

class IFile;

///////////////////////////////////////////////////////////////////////////////////////////////////
class IPackage
{
public:
	virtual bool readonly() const = 0;

	virtual const Char* packageFilename() const = 0;

	//readonly functions, not available when package is dirty
	virtual bool hasFile(const Char* filename) const = 0;
	virtual IFile* openFile(const Char* filename) = 0;
	virtual void closeFile(IFile* file) = 0;

	virtual u32 getFileCount() const = 0;
	virtual bool getFileInfo(u32 index, Char* filenameBuffer, u32 filenameBufferSize, u32* fileSize = 0) const = 0;

	//package manipulation fuctions
	virtual bool addFile(const Char* filename, void* buffer, u32 size, u32 flag = FLAG_REPLACE) = 0;
	virtual bool removeFile(const Char* filename) = 0;
	virtual bool dirty() const = 0;
	virtual void flush() = 0;	//nothing is really changed untill flush

	virtual u64 countFragmentSize() const = 0;
	virtual bool defrag(Callback callback, void* callbackParam) = 0;	//can be very slow, don't call this all the time

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
IPackage* create(const Char* filename);
IPackage* open(const Char* filename, u32 flag = FLAG_READONLY | FLAG_NO_FILENAME);
void close(IPackage* package);

}

#endif
