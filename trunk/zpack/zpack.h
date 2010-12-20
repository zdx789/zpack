#ifndef __ZP_H__
#define __ZP_H__

namespace zp
{

class IFile;

const unsigned long FLAG_READONLY = 1;
const unsigned long FLAG_REPLACE = 2;

///////////////////////////////////////////////////////////////////////////////////////////////////
class IPackage
{
public:
	virtual bool hasFile(const char* filename) = 0;
	virtual IFile* openFile(const char* filename) = 0;
	virtual void closeFile(IFile* file) = 0;
	
	virtual unsigned long getFileCount() = 0;
	virtual bool getFilenameByIndex(char* buffer, unsigned long bufferSize, unsigned long index) = 0;

	virtual bool addFile(const char* externalFilename, const char* filename, unsigned long flag = FLAG_REPLACE) = 0;
	virtual bool removeFile(const char* filename) = 0;
	virtual bool dirty() = 0;
	virtual void flush() = 0;

	virtual void defrag() = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
class IFile
{
public:
	virtual unsigned long getSize() = 0;
	virtual void setPointer(unsigned long pos) = 0;
	virtual unsigned long read(void* buffer, unsigned long size) = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
IPackage* create(const char* filename, unsigned long flag = 0);
IPackage* open(const char* filename, unsigned long flag = FLAG_READONLY);
void close(IPackage* package);

}

#endif
