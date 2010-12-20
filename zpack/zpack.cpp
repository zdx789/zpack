#include "zpack.h"
#include "zppackage.h"
#include "zpfile.h"

namespace zp
{

///////////////////////////////////////////////////////////////////////////////////////////////////
IPackage* open(const char* filename, u32 flag)
{
	Package* package = new Package(filename, (flag & FLAG_READONLY) != 0);
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
IPackage* create(const char* filename, u32 flag)
{
	std::fstream stream;
	stream.open(filename, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);

	PackageHeader header;
	header.sign = PACKAGE_FILE_SIGN;
	header.version = CURRENT_VERSION;
	header.headerSize = sizeof(PackageHeader);
	header.fileEntrySize = sizeof(FileEntry);
	header.fileCount = 0;
	header.fileEntryOffset = sizeof(PackageHeader);
	header.filenameOffset = sizeof(PackageHeader);
	header.filenameSize = 0;
	header.flag = flag;
	header.reserved = 0;

	stream.write((char*)&header, sizeof(header));
	stream.close();

	return open(filename, 0);
}

}
