#ifndef __ZP_FILE_H__
#define __ZP_FILE_H__

#include "zpack.h"

namespace zp
{

class Package;

class File : public IFile
{
public:
	File(Package* package, u64 offset, u32 size, u32 flag);

	virtual u32 size();

	virtual void seek(u32 pos);

	virtual u32 read(u8* buffer, u32 size);

private:
	void seekInPackage();

private:
	Package*	m_package;
	u64			m_offset;
	u32			m_flag;
	u32			m_size;
	u32			m_readPos;
};

}

#endif
