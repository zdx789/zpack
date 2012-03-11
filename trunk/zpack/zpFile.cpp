#include "zpFile.h"
#include "zpPackage.h"
#include <cassert>

namespace zp
{

////////////////////////////////////////////////////////////////////////////////////////////////////
File::File(Package* package, u64 offset, u32 size, u32 flag)
	: m_package(package)
	, m_offset(offset)
	, m_flag(flag)
	, m_size(size)
	, m_readPos(0)
{
	assert(package != NULL);
	assert(package->m_stream != NULL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
u32 File::size()
{
	return m_size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void File::seek(u32 pos)
{
	m_readPos = pos;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
u32 File::read(u8* buffer, u32 size)
{
	if (m_readPos + size > m_size)
	{
		size = m_size - m_readPos;
	}
	if (size == 0)
	{
		return 0;
	}
	if (m_package->m_lastSeekFile != this)
	{
		seekInPackage();
	}
	fread(buffer, size, 1, m_package->m_stream);
	m_readPos += size;
	return size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void File::seekInPackage()
{
	_fseeki64(m_package->m_stream, m_offset + m_readPos, SEEK_SET);
	m_package->m_lastSeekFile = this;
}

}
