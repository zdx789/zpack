#ifndef __ZPACK_H__
#define __ZPACK_H__

namespace zp
{

class IFile;

const unsigned long FLAG_READONLY = 1;
const unsigned long FLAG_REPLACE = 2;

///////////////////////////////////////////////////////////////////////////////////////////////////
class IPackage
{
public:
	//readonly functions, not available when package is dirty
	virtual bool hasFile(const char* filename) = 0;
	virtual IFile* openFile(const char* filename) = 0;
	virtual void closeFile(IFile* file) = 0;
	
	virtual unsigned long getFileCount() = 0;
	virtual bool getFilenameByIndex(char* buffer, unsigned long bufferSize, unsigned long index) = 0;

	//package manipulation fuctions
	virtual bool addFile(const char* externalFilename, const char* filename, unsigned long flag = FLAG_REPLACE) = 0;
	virtual bool removeFile(const char* filename) = 0;
	virtual bool dirty() = 0;
	virtual void flush() = 0;

	virtual unsigned __int64 countFragmentSize(unsigned __int64& bytesToMove) = 0;
	virtual bool defrag() = 0;

protected:
	virtual ~IPackage(){}
};

///////////////////////////////////////////////////////////////////////////////////////////////////
class IFile
{
public:
	virtual unsigned long getSize() = 0;
	virtual void setPointer(unsigned long pos) = 0;
	virtual unsigned long read(void* buffer, unsigned long size) = 0;

protected:
	virtual ~IFile(){}
};

///////////////////////////////////////////////////////////////////////////////////////////////////
IPackage* create(const char* filename, unsigned long flag = 0);
IPackage* open(const char* filename, unsigned long flag = FLAG_READONLY);
void close(IPackage* package);

}

#endif
