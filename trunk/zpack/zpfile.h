#ifndef __ZP_FILE_H__
#define __ZP_FILE_H__

#include "zpack.h"
#include <fstream>

namespace zp
{

class File : public IFile
{
public:
	File(std::fstream& stream, unsigned __int64 offset, unsigned long size);

	virtual unsigned long getSize();
	virtual void setPointer(unsigned long pos);
	virtual unsigned long read(void* buffer, unsigned long size);

private:
	std::fstream&		m_stream;
	unsigned __int64	m_offset;
	unsigned long		m_size;
	unsigned long		m_pointer;
};

}

#endif
