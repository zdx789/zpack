#include "zpPackage.h"
#include "zpFile.h"
#include "zpCompressedFile.h"
#include <cassert>
#include <sstream>
#include "zlib.h"
//#include "PerfUtil.h"
//#include "windows.h"

//temp
//double g_addFileTime = 0;
//double g_compressTime = 0;

namespace zp
{

const u32 MIN_HASH_BITS = 8;
const u32 MIN_HASH_TABLE_SIZE = (1<<MIN_HASH_BITS);
const u32 MAX_HASH_TABLE_SIZE = 0x100000;
const u32 HASH_SEED = 131;

using namespace std;

///////////////////////////////////////////////////////////////////////////////////////////////////
Package::Package(const Char* filename, bool readonly, bool readFilename)
	: m_stream(NULL)
	, m_packageEnd(0)
	, m_hashMask(0)
	, m_readonly(readonly)
	, m_readFilename(readFilename)
	, m_lastSeekFile(NULL)
	, m_dirty(false)
{
	//require filename to modify package
	if (!readFilename && !readonly)
	{
		return;
	}
	locale loc = locale::global(locale(""));
	if (readonly)
	{
		m_stream = Fopen(filename, _T("rb"));
	}
	else
	{
		m_stream = Fopen(filename, _T("r+b"));
	}
	if (m_stream == NULL)
	{
		return;
	}
	locale::global(loc);
	if (!readHeader() || !readFileEntries() || !readFilenames())
	{
		fclose(m_stream);
		m_stream = NULL;
		return;
	}
	buildHashTable();
	m_packageFilename = filename;
	if (!readonly)
	{
		//for compress output
		m_compressBuffer.resize(m_header.chunkSize);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
Package::~Package()
{
	flush();
	if (m_stream != NULL)
	{
		fclose(m_stream);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool Package::valid() const
{
	return (m_stream != NULL);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool Package::readonly() const
{
	return m_readonly;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
const Char* Package::packageFilename() const
{
	return m_packageFilename.c_str();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool Package::hasFile(const Char* filename) const
{
	if (m_dirty)
	{
		return false;
	}
	return (getFileIndex(filename) >= 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
IFile* Package::openFile(const Char* filename)
{
	if (m_dirty)
	{
		return NULL;
	}
	int fileIndex = getFileIndex(filename);
	if (fileIndex < 0)
	{
		return NULL;
	}
	FileEntry& entry = m_fileEntries[fileIndex];
	if ((entry.flag & FILE_COMPRESS) == 0 || entry.fileSize == entry.originSize)
	{
		return new File(this, entry.byteOffset, entry.fileSize, entry.flag);
	}
	return new CompressedFile(this, entry.byteOffset, entry.fileSize, entry.originSize, entry.flag);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void Package::closeFile(IFile* file)
{
	delete static_cast<File*>(file);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
u32 Package::getFileCount() const
{
	return (u32)m_fileEntries.size();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool Package::getFileInfo(u32 index, Char* filenameBuffer, u32 filenameBufferSize, u32* fileSize, u32* originSize, u32* flag) const
{
	if (index >= m_filenames.size())
	{
		return false;
	}
	if (filenameBuffer != NULL)
	{
	#ifdef ZP_USE_WCHAR
		wcsncpy(filenameBuffer, m_filenames[index].c_str(), filenameBufferSize - 1);
	#else
		strncpy(filenameBuffer, m_filenames[index].c_str(), filenameBufferSize - 1);
	#endif
		filenameBuffer[filenameBufferSize - 1] = 0;
	}
	const FileEntry& entry = m_fileEntries[index];
	if (fileSize != NULL)
	{
		*fileSize = entry.fileSize;
	}
	if (originSize != NULL)
	{
		*originSize = entry.originSize;
	}
	if (flag != NULL)
	{
		*flag = entry.flag;
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool Package::addFile(const Char* filename, const u8* buffer, u32 size, u32 flag)
{
	if (m_readonly)
	{
		return false;
	}

	//__int64 perfBefore, perfAfter, perfFreq;
	//::QueryPerformanceFrequency((LARGE_INTEGER*)&perfFreq);
	//::QueryPerformanceCounter((LARGE_INTEGER*)&perfBefore);

	int fileIndex = getFileIndex(filename);
	if (fileIndex >= 0)
	{
		//file exist
		m_fileEntries[fileIndex].flag |= FILE_DELETE;
	}
	FileEntry entry;
	entry.nameHash = stringHash(filename, HASH_SEED);
	entry.fileSize = size;
	entry.originSize = size;
	entry.flag = flag;
	if (m_header.chunkSize == 0)
	{
		entry.flag &= (~FILE_COMPRESS);
	}
	u32 insertedIndex = insertFile(entry, filename);
	
	if (size > 0)
	{
		writeFileContent(m_fileEntries[insertedIndex], buffer);
	}
	m_dirty = true;

	//::QueryPerformanceCounter((LARGE_INTEGER*)&perfAfter);
	//double perfTime = 1000.0 * (perfAfter - perfBefore) / perfFreq;
	//g_addFileTime += perfTime;

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool Package::removeFile(const Char* filename)
{
	if (m_readonly)
	{
		return false;
	}
	int fileIndex = getFileIndex(filename);
	if (fileIndex < 0)
	{
		return false;
	}
	//hash table doesn't change until flush£¬so we shouldn't remove entry here
	m_fileEntries[fileIndex].flag |= FILE_DELETE;
	m_dirty = true;
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool Package::dirty() const
{
	return m_dirty;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void Package::flush()
{
	if (m_readonly || !m_dirty)
	{
		return;
	}
	if (m_fileEntries.empty())
	{
		m_header.fileEntryOffset = sizeof(m_header);
	}
	else
	{
		FileEntry& last =  m_fileEntries.back();
		u64 lastFileEnd = last.byteOffset + last.fileSize;
		//remove deleted entries
		assert(m_filenames.size() == m_fileEntries.size());
		vector<String>::iterator nameIter = m_filenames.begin();
		for (vector<FileEntry>::iterator iter = m_fileEntries.begin();
			iter != m_fileEntries.end(); )
		{
			FileEntry& entry = *iter;
			if ((entry.flag & FILE_DELETE) != 0)
			{
				iter = m_fileEntries.erase(iter);
				nameIter = m_filenames.erase(nameIter);
				continue;
			}
			++iter;
			++nameIter;
		}
		//avoid overwriting old file entries and filenames
		u32 filenameSize = countFilenameSize();
		if ((lastFileEnd >= m_header.filenameOffset + m_header.filenameSize) ||
			(lastFileEnd + m_fileEntries.size() * sizeof(FileEntry) + filenameSize <= m_header.fileEntryOffset))
		{
			m_header.fileEntryOffset = lastFileEnd;
		}
		else
		{
			m_header.fileEntryOffset = m_header.filenameOffset + m_header.filenameSize;
		}
	}
	_fseeki64(m_stream, m_header.fileEntryOffset, SEEK_SET);
	for (u32 i = 0; i < m_fileEntries.size(); ++i)
	{
		FileEntry& entry = m_fileEntries[i];
		fwrite(&entry, sizeof(FileEntry), 1, m_stream);
	}
	
	m_header.fileCount = (u32)m_fileEntries.size();
	m_header.filenameOffset = m_header.fileEntryOffset + m_header.fileCount * sizeof(FileEntry);
	//filenames
	m_header.filenameSize = 0;
	for (u32 i = 0; i < m_filenames.size(); ++i)
	{
		const String& filename = m_filenames[i];
		fwrite(filename.c_str(), (u32)filename.length() * sizeof(Char), 1, m_stream);
		fwrite(_T("\n"), sizeof(Char), 1, m_stream);
		m_header.filenameSize += (((u32)filename.length() + 1) * sizeof(Char));
	}
	m_packageEnd = m_header.filenameOffset + m_header.filenameSize;
	
	_fseeki64(m_stream, 0, SEEK_SET);
	fwrite(&m_header, sizeof(m_header), 1, m_stream);
	fflush(m_stream);

	buildHashTable();
	m_dirty = false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
u64 Package::countFragmentSize() const
{
	if (m_dirty)
	{
		return 0;
	}
	u64 totalSize = m_header.headerSize + m_header.fileCount * m_header.fileEntrySize + m_header.filenameSize;
	u64 nextPos = m_header.headerSize;
	for (u32 i = 0; i < m_fileEntries.size(); ++i)
	{
		const FileEntry& entry = m_fileEntries[i];
		nextPos += entry.fileSize;
		totalSize += entry.fileSize;
	}
	_fseeki64(m_stream, 0, SEEK_END);
	u64 currentSize = _ftelli64(m_stream);
	return currentSize - totalSize;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool Package::defrag(Callback callback, void* callbackParam)
{
	if (m_readonly || m_dirty)
	{
		return false;
	}
	String tempFilename = m_packageFilename + _T("_");
	FILE* tempFile;
	locale loc = locale::global(locale(""));
	tempFile = Fopen(tempFilename.c_str(), _T("wb"));// std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
	locale::global(loc);
	if (tempFile == NULL)
	{
		return false;
	}
	_fseeki64(tempFile, sizeof(m_header), SEEK_SET);

	vector<char> tempBuffer;
	u64 nextPos = m_header.headerSize;
	//write as chunk instead of file, to speed up
	const u32 MIN_CHUNK_SIZE = 0x100000;
	u64 currentChunkPos = nextPos;
	u64 fragmentSize = 0;
	u32 currentChunkSize = 0;
	for (u32 i = 0; i < m_fileEntries.size(); ++i)
	{
		FileEntry& entry = m_fileEntries[i];
		if (callback != NULL && !callback(m_filenames[i].c_str(), callbackParam))
		{
			//stop
			fclose(tempFile);
			Remove(tempFilename.c_str());
			return false;
		}
		if (entry.fileSize == 0)
		{
			entry.byteOffset = nextPos;
			continue;
		}
		if ((entry.flag & FILE_DELETE) != 0)
		{
			continue;
		}
		if (entry.byteOffset != fragmentSize + nextPos	//new fragment encountered
			|| currentChunkSize > MIN_CHUNK_SIZE)
		{
			if (currentChunkSize > 0)
			{
				tempBuffer.resize(currentChunkSize);
				_fseeki64(m_stream, currentChunkPos, SEEK_SET);
				fread(&tempBuffer[0], currentChunkSize, 1, m_stream);
				fwrite(&tempBuffer[0], currentChunkSize, 1, tempFile);
			}
			fragmentSize = entry.byteOffset - nextPos;
			currentChunkPos = entry.byteOffset;
			currentChunkSize = 0;
		}
		entry.byteOffset = nextPos;
		nextPos += entry.fileSize;
		currentChunkSize += entry.fileSize;
	}
	//one chunk may be left
	if (currentChunkSize > 0)
	{
		tempBuffer.resize(currentChunkSize);
		_fseeki64(m_stream, currentChunkPos, SEEK_SET);
		fread(&tempBuffer[0], currentChunkSize, 1, m_stream);
		fwrite(&tempBuffer[0], currentChunkSize, 1, tempFile);
	}

	fclose(m_stream);
	fclose(tempFile);

	loc = locale::global(locale(""));
	m_stream = Fopen(tempFilename.c_str(), _T("r+b"));//ios_base::in | ios_base::out | ios_base::binary);	//only for flush()
	locale::global(loc);
	assert(m_stream != NULL);
	m_dirty = true;
	flush();	//no need to rebuild hash table here, but it's ok b/c defrag will be slow anyway
	fclose(m_stream);

	Remove(m_packageFilename.c_str());
	Rename(tempFilename.c_str(), m_packageFilename.c_str());
	loc = locale::global(locale(""));
	m_stream = Fopen(m_packageFilename.c_str(), _T("r+b"));//ios_base::in | ios_base::out | ios_base::binary);
	locale::global(loc);
	assert(m_stream != NULL);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool Package::readHeader()
{
	_fseeki64(m_stream, 0, SEEK_END);
	m_packageEnd = _ftelli64(m_stream);
	if (m_packageEnd < sizeof(PackageHeader))
	{
		return false;
	}
	_fseeki64(m_stream, 0, SEEK_SET);
	fread((char*)&m_header, sizeof(PackageHeader), 1, m_stream);
	if (m_header.sign != PACKAGE_FILE_SIGN
		|| m_header.headerSize < sizeof(PackageHeader)
		|| m_header.fileEntrySize < sizeof(FileEntry)
		|| m_header.fileEntryOffset < m_header.headerSize
		|| m_header.fileCount * m_header.fileEntrySize + m_header.fileEntryOffset > m_packageEnd
		|| m_header.filenameOffset + m_header.filenameSize > m_packageEnd)
	{
		return false;
	}
	if (m_header.version != CURRENT_VERSION)
	{
		return false;
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool Package::readFileEntries()
{
	m_fileEntries.resize(m_header.fileCount);
	_fseeki64(m_stream, m_header.fileEntryOffset, SEEK_SET);
	u32 extraDataSize = m_header.fileEntrySize - sizeof(FileEntry);

	u64 nextOffset = m_header.headerSize;
	for (u32 i = 0; i < m_header.fileCount; ++i)
	{
		if (nextOffset > m_header.fileEntryOffset)
		{
			return false;
		}
		FileEntry& entry = m_fileEntries[i];
		fread(&entry, sizeof(FileEntry), 1, m_stream);
		if (entry.byteOffset < nextOffset)
		{
			return false;
		}
		nextOffset = entry.byteOffset + entry.fileSize;
		if (extraDataSize > 0)
		{
			_fseeki64(m_stream, extraDataSize, SEEK_CUR);
		}
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool Package::readFilenames()
{
	if (m_fileEntries.empty() || !m_readFilename)
	{
		return true;
	}
	u32 charCount = m_header.filenameSize / sizeof(Char);
	String names;
	names.resize(charCount + 1);
	//hack
	Char* buffer = const_cast<Char*>(names.c_str());

	m_filenames.resize(m_fileEntries.size());
	_fseeki64(m_stream, m_header.filenameOffset, SEEK_SET);
	fread(buffer, charCount * sizeof(Char), 1, m_stream);
	names[charCount] = 0;
	IStringStream iss(names, IStringStream::in);
	for (u32 i = 0; i < m_fileEntries.size(); ++i)
	{
		Char out[260];
		iss.getline(out, sizeof(out)/sizeof(Char));
		m_filenames[i] = out;
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool Package::buildHashTable()
{
	u32 tableSize = MIN_HASH_TABLE_SIZE / 2;
	u32 hashBits = MIN_HASH_BITS - 1;
	while (tableSize < m_header.fileCount)
	{
		if (tableSize >= MAX_HASH_TABLE_SIZE)
		{
			return false;
		}
		tableSize *= 2;
		++hashBits;
	}
	tableSize *= 4;
	hashBits += 2;
	m_hashMask = (1 << hashBits) - 1;

	bool hashConflict = false;
	m_hashTable.clear();
	m_hashTable.resize(tableSize, -1);
	for (u32 i = 0; i < m_header.fileCount; ++i)
	{
		u32 index = (m_fileEntries[i].nameHash & m_hashMask);
		while (m_hashTable[index] != -1)
		{
			if (!hashConflict && m_fileEntries[m_hashTable[index]].nameHash == m_fileEntries[i].nameHash)
			{
				hashConflict = true;
			}
			if (++index >= tableSize)
			{
				index = 0;
			}
		}
		m_hashTable[index] = i;
	}
	return !hashConflict;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
int Package::getFileIndex(const Char* filename) const
{
	u64 hash = stringHash(filename, HASH_SEED);

	u32 hashIndex = (hash & m_hashMask);
	int fileIndex = m_hashTable[hashIndex];
	while (fileIndex >= 0)
	{
		const FileEntry& entry = m_fileEntries[fileIndex];
		if (entry.nameHash == hash)
		{
			if ((entry.flag & FILE_DELETE) != 0)
			{
				return -1;
			}
			return fileIndex;
		}
		if (++hashIndex >= m_hashTable.size())
		{
			hashIndex = 0;
		}
		fileIndex = m_hashTable[hashIndex];
	}
	return -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
u32 Package::insertFile(FileEntry& entry, const Char* filename)
{
	u32 maxIndex = (u32)m_fileEntries.size();
	u64 lastEnd = m_header.headerSize;

	//file with 0 size will alway be put to the end
	for (u32 fileIndex = 0; fileIndex < maxIndex; ++fileIndex)
	{
		FileEntry& thisEntry = m_fileEntries[fileIndex];
		if (thisEntry.byteOffset >= lastEnd + entry.fileSize
			&& (lastEnd + entry.fileSize <= m_header.fileEntryOffset
			|| lastEnd >= m_header.filenameOffset + m_header.filenameSize))	//don't overwrite old file entries and filenames
		{
			entry.byteOffset = lastEnd;
			m_fileEntries.insert(m_fileEntries.begin() + fileIndex, entry);
			m_filenames.insert(m_filenames.begin() + fileIndex, filename);
			assert(m_filenames.size() == m_fileEntries.size());
			//user may call addFile or removeFile before calling flush, so hash table need to be fixed
			fixHashTable(fileIndex);
			return fileIndex;
		}
		lastEnd = thisEntry.byteOffset + thisEntry.fileSize;
	}

	if (m_header.fileEntryOffset > lastEnd + entry.fileSize)
	{
		entry.byteOffset = lastEnd;
	}
	else
	{
		entry.byteOffset = m_packageEnd;
		m_packageEnd += entry.fileSize;
	}
	m_fileEntries.push_back(entry);
	m_filenames.push_back(filename);
	assert(m_filenames.size() == m_fileEntries.size());
	return m_filenames.size() - 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
u64 Package::stringHash(const Char* str, u32 seed) const
{
	u64 out = 0;
	while (*str)
	{
		Char ch = *(str++);
		if (ch == _T('\\'))
		{
			ch = _T('/');
		}
	#if (ZP_CASE_SENSITIVE)
		out = out * seed + ch;
	#else
		out = out * seed + tolower(ch);
	#endif
	}
	return out;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void Package::fixHashTable(u32 index)
{
	//increase file index which is greater than "index" by 1
	for (u32 i = 0; i < m_hashTable.size(); ++i)
	{
		if (m_hashTable[i] >= (int)index)
		{
			++m_hashTable[i];
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
u32 Package::countFilenameSize() const
{
	u32 total = 0;
	for (u32 i = 0; i < m_filenames.size(); ++i)
	{
		const String& filename = m_filenames[i];
		total += (((u32)filename.length() + 1) * sizeof(Char));
	}
	return total;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void Package::writeFileContent(FileEntry& entry, const u8* buffer)
{
	_fseeki64(m_stream, entry.byteOffset, SEEK_SET);
	if ((entry.flag & FILE_COMPRESS) == 0)
	{
		fwrite(buffer, entry.fileSize, 1, m_stream);
		return;
	}

	assert(m_header.chunkSize > 0);
	u32 chunkCount = entry.originSize / m_header.chunkSize;
	u32 lastChunkSize = entry.originSize % m_header.chunkSize;
	if (lastChunkSize > 0)
	{
		++chunkCount;
	}
	m_chunkPosBuffer.resize(chunkCount);

	entry.fileSize = 0;
	if (chunkCount > 1)
	{
		m_chunkPosBuffer[0] = chunkCount * sizeof(u32);
		fwrite((char*)&m_chunkPosBuffer[0], chunkCount * sizeof(u32), 1, m_stream);
	}
	u32 srcSize = m_header.chunkSize;

	//BEGIN_PERF("compress");

	const u8* srcBuffer = buffer;
	u8* dstBuffer = &m_compressBuffer[0];
	for (u32 i = 0; i < chunkCount; ++i)
	{
		if (i + 1 == chunkCount && lastChunkSize > 0)
		{
			srcSize = lastChunkSize;
		}
		u32 dstSize = m_header.chunkSize;

		//__int64 perfBefore, perfAfter, perfFreq;
		//::QueryPerformanceFrequency((LARGE_INTEGER*)&perfFreq);
		//::QueryPerformanceCounter((LARGE_INTEGER*)&perfBefore);

		int ret = compress2(dstBuffer, &dstSize, srcBuffer, srcSize, Z_DEFAULT_COMPRESSION);

		//::QueryPerformanceCounter((LARGE_INTEGER*)&perfAfter);
		//double perfTime = 1000.0 * (perfAfter - perfBefore) / perfFreq;
		//g_compressTime += perfTime;

		if (ret != Z_OK	|| dstSize >= srcSize)
		{
			//compress failed, write raw data
			fwrite((char*)srcBuffer, srcSize, 1, m_stream);
			dstSize = srcSize;
		}
		else
		{
			fwrite((char*)dstBuffer, dstSize, 1, m_stream);
		}
		if (i + 1 < chunkCount)
		{
			m_chunkPosBuffer[i + 1] = m_chunkPosBuffer[i] + dstSize;
		}
		entry.fileSize += dstSize;
		srcBuffer += srcSize;
	}
	if (entry.fileSize == entry.originSize)
	{
		//none of the chunks is actually compressed, rewrite as non-compressed file
		entry.flag &= (~FILE_COMPRESS);
		//if only 1 chunk, no need to rewrite b/c there's no chunk pos array
		if (chunkCount > 1)
		{
			_fseeki64(m_stream, entry.byteOffset, SEEK_SET);
			fwrite(buffer, entry.fileSize, 1, m_stream);
		}
		return;
	}
	//END_PERF

	if (chunkCount > 1)
	{
		entry.fileSize += chunkCount * sizeof(u32);
		_fseeki64(m_stream, entry.byteOffset, SEEK_SET);
		fwrite((char*)&m_chunkPosBuffer[0], chunkCount * sizeof(u32), 1, m_stream);
	}
}

}