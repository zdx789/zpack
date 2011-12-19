#include "zpFile.h"

namespace zp
{

File* File::s_lastSeek = NULL;

///////////////////////////////////////////////////////////////////////////////////////////////////
File::File(FILE* stream, u64 offset, u32 size, u32 flag)
	: m_stream(stream)
	, m_offset(offset)
	, m_flag(flag)
	, m_size(size)
	, m_readPos(0)
{
	_fseeki64(m_stream, m_offset, SEEK_SET);
	s_lastSeek = this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
u32 File::size()
{
	return m_size;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void File::seek(u32 pos)
{
	if (pos != m_readPos || s_lastSeek != this)
	{
		m_readPos = pos;
		_fseeki64(m_stream, m_offset + m_readPos, SEEK_SET);
		s_lastSeek = this;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
u32 File::read(void* buffer, u32 size)
{
	if (m_readPos + size > m_size)
	{
		size = m_size - m_readPos;
	}
	if (s_lastSeek != this)
	{
		_fseeki64(m_stream, m_offset + m_readPos, SEEK_SET);
		s_lastSeek = this;
	}
	fread(buffer, size, 1, m_stream);
	m_readPos += size;
	return size;
}

}
