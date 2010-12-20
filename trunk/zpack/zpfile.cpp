#include "zpfile.h"

namespace zp
{

///////////////////////////////////////////////////////////////////////////////////////////////////
File::File(std::fstream& stream, unsigned __int64 offset, unsigned long size)
	: m_stream(stream)
	, m_offset(offset)
	, m_size(size)
	, m_pointer(0)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
unsigned long File::getSize()
{
	return m_size;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void File::setPointer(unsigned long pos)
{
	m_pointer = pos;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
unsigned long File::read(void* buffer, unsigned long size)
{
	if (m_pointer + size > m_size)
	{
		size = m_size - m_pointer;
	}
	m_stream.seekg(m_offset + m_pointer, std::ios::beg);
	m_stream.read((char*)buffer, size);
	m_pointer += size;
	return size;
}

}
