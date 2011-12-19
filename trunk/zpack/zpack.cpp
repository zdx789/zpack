#include "zpack.h"
#include "zpPackage.h"
#include "zpFile.h"
#include <fstream>

using namespace std;

namespace zp
{

///////////////////////////////////////////////////////////////////////////////////////////////////
IPackage* open(const Char* filename, u32 flag)
{
	Package* package = new Package(filename, 
									(flag & FLAG_READONLY) != 0,
									(flag & FLAG_NO_FILENAME) == 0);
	if (!package->valid())
	{
		delete package;
		package = NULL;
	}
	return package;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void close(IPackage* package)
{
	delete static_cast<Package*>(package);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
IPackage* create(const Char* filename)
{
	fstream stream;
	locale loc = locale::global(locale(""));
	stream.open(filename, ios_base::out | ios_base::trunc | ios_base::binary);
	locale::global(loc);
	if (!stream.is_open())
	{
		return NULL;
	}
	PackageHeader header;
	header.sign = PACKAGE_FILE_SIGN;
	header.version = CURRENT_VERSION;
	header.headerSize = sizeof(PackageHeader);
	header.fileEntrySize = sizeof(FileEntry);
	header.fileCount = 0;
	header.fileEntryOffset = sizeof(PackageHeader);
	header.filenameOffset = sizeof(PackageHeader);
	header.filenameSize = 0;
	header.flag = 0;
	header.reserved = 0;

	stream.write((char*)&header, sizeof(header));
	stream.close();

	return open(filename, 0);
}

}
