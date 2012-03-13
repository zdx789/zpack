#ifndef __ZP_COMPRESSED_FILE_H__
#define __ZP_COMPRESSED_FILE_H__

#include "zpack.h"

namespace zp
{

class Package;

class CompressedFile : public IFile
{
public:
	CompressedFile(Package* package, u64 offset, u32 compressedSize, u32 originSize, u32 flag);
	virtual ~CompressedFile();

	//from IFiled
	virtual u32 size();

	virtual void seek(u32 pos);

	virtual u32 read(u8* buffer, u32 size);

private:
	void seekInPackage(u32 offset);

	u32 oneChunkRead(u8* buffer, u32 size);

	bool readChunk(u32 chunkIndex, u32 offset, u32 readSize, u8* buffer);

private:
	Package*	m_package;
	u64			m_offset;
	u32			m_flag;
	u32			m_compressedSize;
	u32			m_originSize;

	u32			m_readPos;
	u32			m_chunkCount;
	u32*		m_chunkPos;
	u8*			m_fileData;		//available when there's only 1 chunk
	u8**		m_chunkData;	//available when there's more than 1 chunk
};

}

#endif
