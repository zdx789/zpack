#include "zpWriteFile.h"
#include "zpPackage.h"

namespace zp
{

///////////////////////////////////////////////////////////////////////////////////////////////////
WriteFile::WriteFile(Package* package, u64 offset, u32 size, u32 flag, u64 nameHash)
	: m_package(package)
	, m_offset(offset)
	, m_size(size)
	, m_flag(flag)
	, m_nameHash(nameHash)
	, m_writePos(0)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
WriteFile::~WriteFile()
{
	if (m_package->m_lastSeekFile == this)
	{
		m_package->m_lastSeekFile = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
u32 WriteFile::flag() const
{
	return m_flag;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
u32 WriteFile::write(const u8* buffer, u32 size)
{
	if (m_writePos + size > m_size)
	{
		size = m_size - m_writePos;
	}
	if (size == 0)
	{
		return 0;
	}
	if (m_package->m_lastSeekFile != this)
	{
		seekInPackage();
	}
	fwrite(buffer, size, 1, m_package->m_stream);
	m_writePos += size;

	if (!m_package->setFileAvailableSize(m_nameHash, m_writePos))
	{
		//something wrong, stop writing
		m_size = 0;
		return 0;
	}
	return size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void WriteFile::seekInPackage()
{
	_fseeki64(m_package->m_stream, m_offset + m_writePos, SEEK_SET);
	m_package->m_lastSeekFile = this;
}

}
