#include "zpFile.h"

namespace zp
{

File* File::s_lastSeek = NULL;

///////////////////////////////////////////////////////////////////////////////////////////////////
File::File(std::fstream& stream, u64 offset, u32 size)
	: m_stream(stream)
	, m_offset(offset)
	, m_size(size)
	, m_readPos(0)
{
	m_stream.seekg(m_offset, std::ios::beg);
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
		m_stream.seekg(m_offset + m_readPos, std::ios::beg);
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
		m_stream.seekg(m_offset + m_readPos, std::ios::beg);
		s_lastSeek = this;
	}
	m_stream.read((char*)buffer, size);
	m_readPos += size;
	return size;
}

}
