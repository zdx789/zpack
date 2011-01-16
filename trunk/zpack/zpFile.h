#ifndef __ZP_FILE_H__
#define __ZP_FILE_H__

#include "zpack.h"
#include <fstream>

namespace zp
{

class File : public IFile
{
public:
	File(std::fstream& stream, u64 offset, u32 size, u32 flag);

	virtual u32 size();
	virtual void seek(u32 pos);
	virtual u32 read(void* buffer, u32 size);

private:
	std::fstream&	m_stream;
	u64				m_offset;
	u32				m_flag;
	u32				m_size;
	u32				m_readPos;
	static File*	s_lastSeek;
};

}

#endif
