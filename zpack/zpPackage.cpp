#include "zpPackage.h"
#include "zpFile.h"
#include "zpCompressedFile.h"
#include "zpWriteFile.h"
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

const u32 HASH_TABLE_SCALE = 4;
const u32 MIN_HASH_BITS = 8;
const u32 MIN_HASH_TABLE_SIZE = (1<<MIN_HASH_BITS);
const u32 MAX_HASH_TABLE_SIZE = 0x100000;
const u32 MIN_CHUNK_SIZE = 0x1000;

const u32 HASH_SEED = 131;

using namespace std;

///////////////////////////////////////////////////////////////////////////////////////////////////
Package::Package(const Char* filename, bool readonly, bool readFilename)
	: m_stream(NULL)
	, m_hashBits(MIN_HASH_BITS)
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
	if (!readHeader() || !readFileEntries() || !readFilenames())
	{
		fclose(m_stream);
		m_stream = NULL;
		return;
	}

	if (!buildHashTable())
	{
		fclose(m_stream);
		m_stream = NULL;
		return;
	}
	m_packageFilename = filename;
	if (!readonly)
	{
		//for compress output
		m_compressBuffer.resize(m_header.chunkSize);
		m_chunkData.resize(m_header.chunkSize);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
Package::~Package()
{
	if (m_stream != NULL)
	{
		removeDeletedEntries();
		flush();
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
	//if (m_dirty)
	//{
	//	return false;
	//}
	return (getFileIndex(filename) >= 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
IReadFile* Package::openFile(const Char* filename)
{
	//if (m_dirty)
	//{
	//	return NULL;
	//}
	int fileIndex = getFileIndex(filename);
	if (fileIndex < 0)
	{
		return NULL;
	}
	FileEntry& entry = m_fileEntries[fileIndex];
	if ((entry.flag & FILE_COMPRESS) == 0)
	{
		return new File(this, entry.byteOffset, entry.packSize, entry.flag, entry.nameHash);
	}
	u32 chunkSize = entry.chunkSize == 0 ? m_header.chunkSize : entry.chunkSize;
	CompressedFile* file = new CompressedFile(this, entry.byteOffset, entry.packSize, entry.originSize,
												chunkSize, entry.flag, entry.nameHash);
	if ((file->flag() & FILE_DELETE) != 0)
	{
		delete file;
		file = NULL;
	}
	return file;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void Package::closeFile(IReadFile* file)
{
	if ((file->flag() & FILE_COMPRESS) == 0)
	{
		delete static_cast<File*>(file);
	}
	else
	{
		delete static_cast<CompressedFile*>(file);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void Package::closeFile(IWriteFile* file)
{
	delete static_cast<WriteFile*>(file);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
u32 Package::getFileCount() const
{
	return (u32)m_fileEntries.size();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool Package::getFileInfo(u32 index, Char* filenameBuffer, u32 filenameBufferSize, u32* fileSize, u32* packSize, u32* flag) const
{
	if (index >= m_filenames.size())
	{
		return false;
	}
	if (filenameBuffer != NULL)
	{
		Strcpy(filenameBuffer, filenameBufferSize, m_filenames[index].c_str());
		filenameBuffer[filenameBufferSize - 1] = 0;
	}
	const FileEntry& entry = m_fileEntries[index];
	if (fileSize != NULL)
	{
		*fileSize = entry.originSize;
	}
	if (packSize != NULL)
	{
		*packSize = entry.packSize;
	}
	if (flag != NULL)
	{
		*flag = entry.flag;
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool Package::addFile(const Char* filename, const Char* externalFilename, u32 fileSize, u32 flag,
						u32* outPackSize, u32* outFlag)
{
	if (m_readonly)
	{
		return false;
	}

	FILE* file = Fopen(externalFilename, _T("rb"));
	if (file == NULL)
	{
		return false;
	}

	m_dirty = true;

	int fileIndex = getFileIndex(filename);
	if (fileIndex >= 0)
	{
		//file exist
		m_fileEntries[fileIndex].flag |= FILE_DELETE;
	}
	FileEntry entry;
	entry.nameHash = stringHash(filename, HASH_SEED);
	entry.packSize = fileSize;
	entry.originSize = fileSize;
	entry.flag = flag;
	entry.chunkSize = m_header.chunkSize;
	memset(entry.reserved, 0, sizeof(entry.reserved));

	u32 insertedIndex = insertFileEntry(entry, filename);

	if (!insertFileHash(entry.nameHash, insertedIndex))
	{
		//may be hash confliction
		m_fileEntries[insertedIndex].flag |= FILE_DELETE;
		return false;
	}
	
	if (fileSize == 0)
	{
		entry.flag &= (~FILE_COMPRESS);
	}
	else
	{
		if ((entry.flag & FILE_COMPRESS) == 0)
		{
			writeRawFile(m_fileEntries[insertedIndex], file);
		}
		else
		{
			writeCompressFile(m_fileEntries[insertedIndex], file);
		}
	}
	fclose(file);

	if (outPackSize != NULL)
	{
		*outPackSize = m_fileEntries[insertedIndex].packSize;
	}
	if (outFlag != NULL)
	{
		*outFlag = m_fileEntries[insertedIndex].flag;
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
IWriteFile* Package::createFile(const Char* filename, u32 fileSize, u32 packSize, u32 chunkSize, u32 flag)
{
	if (m_readonly)
	{
		return NULL;
	}
	m_dirty = true;

	int fileIndex = getFileIndex(filename);
	if (fileIndex >= 0)
	{
		//file exist
		m_fileEntries[fileIndex].flag |= FILE_DELETE;
	}

	FileEntry entry;
	entry.nameHash = stringHash(filename, HASH_SEED);
	entry.packSize = packSize;
	entry.originSize = fileSize;
	flag |= FILE_WRITING;
	entry.flag = flag;
	if ((entry.flag & FILE_COMPRESS) == 0)
	{
		chunkSize = 0;
	}
	entry.chunkSize = chunkSize;
	memset(entry.reserved, 0, sizeof(entry.reserved));

	u32 insertedIndex = insertFileEntry(entry, filename);

	if (!insertFileHash(entry.nameHash, insertedIndex))
	{
		//hash confliction
		m_fileEntries[insertedIndex].flag |= FILE_DELETE;
		return NULL;
	}

	return new WriteFile(this, entry.byteOffset, entry.packSize, entry.flag, entry.nameHash);
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
	m_lastSeekFile = NULL;

	writeTables(true);

	//header
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
	m_lastSeekFile = NULL;

	u64 totalSize = m_header.headerSize + m_fileEntries.size() * sizeof(FileEntry) + m_header.filenameSize;
	for (u32 i = 0; i < m_fileEntries.size(); ++i)
	{
		const FileEntry& entry = m_fileEntries[i];
		totalSize += entry.packSize;
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
	m_lastSeekFile = NULL;

	String tempFilename = m_packageFilename + _T("_");
	FILE* tempFile;
	tempFile = Fopen(tempFilename.c_str(), _T("wb"));
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
		if (callback != NULL && !callback(m_filenames[i].c_str(), entry.originSize, callbackParam))
		{
			//stop
			fclose(tempFile);
			Remove(tempFilename.c_str());
			return false;
		}
		if (entry.packSize == 0)
		{
			entry.byteOffset = nextPos;
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
		nextPos += entry.packSize;
		currentChunkSize += entry.packSize;
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

	m_stream = Fopen(tempFilename.c_str(), _T("r+b"));//ios_base::in | ios_base::out | ios_base::binary);	//only for flush()
	assert(m_stream != NULL);

	//write file entries, filenames and header
	writeTables(false);
	_fseeki64(m_stream, 0, SEEK_SET);
	fwrite(&m_header, sizeof(m_header), 1, m_stream);
	fflush(m_stream);

	fclose(m_stream);

	Remove(m_packageFilename.c_str());
	Rename(tempFilename.c_str(), m_packageFilename.c_str());
	m_stream = Fopen(m_packageFilename.c_str(), _T("r+b"));//ios_base::in | ios_base::out | ios_base::binary);
	assert(m_stream != NULL);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool Package::readHeader()
{
	_fseeki64(m_stream, 0, SEEK_END);
	u64 packageSize = _ftelli64(m_stream);
	if (packageSize < sizeof(PackageHeader))
	{
		return false;
	}
	_fseeki64(m_stream, 0, SEEK_SET);
	fread(&m_header, sizeof(PackageHeader), 1, m_stream);
	if (m_header.sign != PACKAGE_FILE_SIGN
		|| m_header.headerSize != sizeof(PackageHeader)
		|| m_header.fileEntryOffset < m_header.headerSize
		|| m_header.fileEntryOffset + m_header.fileEntrySize > packageSize
		|| m_header.filenameOffset < m_header.fileEntryOffset + m_header.fileEntrySize
		|| m_header.filenameOffset + m_header.filenameSize > packageSize
		|| m_header.chunkSize < MIN_CHUNK_SIZE)
	{
		return false;
	}
	if (m_header.version != CURRENT_VERSION && !m_readonly)
	{
		return false;
	}
	m_packageEnd = m_header.filenameOffset + m_header.filenameSize;
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool Package::readFileEntries()
{
	m_fileEntries.resize(m_header.fileCount);
	if (m_header.fileCount == 0)
	{
		return true;
	}
	_fseeki64(m_stream, m_header.fileEntryOffset, SEEK_SET);
	if (m_header.fileEntrySize == m_header.fileCount * sizeof(FileEntry))
	{
		//not compressed
		fread(&m_fileEntries[0], m_header.fileEntrySize, 1, m_stream);
	}
	else
	{
		vector<u8> srcBuffer(m_header.fileEntrySize);
		fread(&srcBuffer[0], m_header.fileEntrySize, 1, m_stream);
		u32 dstBufferSize = m_header.fileCount * sizeof(FileEntry);
		int ret = uncompress((u8*)&m_fileEntries[0], &dstBufferSize, &srcBuffer[0], m_header.fileEntrySize);
		if (ret != Z_OK || dstBufferSize != m_header.fileCount * sizeof(FileEntry))
		{
			return false;
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
	if (m_header.filenameSize == 0)
	{
		return false;
	}
	_fseeki64(m_stream, m_header.filenameOffset, SEEK_SET);
	vector<u8> dstBuffer(m_header.originFilenameSize);
	if (m_header.filenameSize == m_header.originFilenameSize)
	{
		//not compressed
		fread(&dstBuffer[0], m_header.filenameSize, 1, m_stream);
	}
	else
	{
		vector<u8> tempBuffer(m_header.filenameSize);
		fread(&tempBuffer[0], m_header.filenameSize, 1, m_stream);
		u32 originSize = m_header.originFilenameSize;
		int ret = uncompress(&dstBuffer[0], &originSize, &tempBuffer[0], m_header.filenameSize);
		if (ret != Z_OK || originSize != m_header.originFilenameSize)
		{
			return false;
		}
	}
	
	String names;
	names.assign((Char*)&dstBuffer[0], m_header.originFilenameSize / sizeof(Char));
	
	m_filenames.resize(m_fileEntries.size());
	
	IStringStream iss(names, IStringStream::in);
	for (u32 i = 0; i < m_fileEntries.size(); ++i)
	{
		Char out[MAX_FILENAME_LEN];
		iss.getline(out, MAX_FILENAME_LEN);
		m_filenames[i] = out;
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void Package::removeDeletedEntries()
{
	if (m_readonly)
	{
		return;
	}
	assert(m_fileEntries.size() == m_filenames.size());

	//m_header.fileCount and m_header.filenameSize will not change
	vector<String>::iterator nameIter = m_filenames.begin();
	for (vector<FileEntry>::iterator iter = m_fileEntries.begin();
		iter != m_fileEntries.end(); )
	{
		FileEntry& entry = *iter;
		if ((entry.flag & FILE_DELETE) != 0)
		{
			iter = m_fileEntries.erase(iter);
			nameIter = m_filenames.erase(nameIter);
			m_dirty = true;
			continue;
		}
		++iter;
		++nameIter;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void Package::writeTables(bool avoidOverwrite)
{
	//compress file entries
	u32 srcEntrySize = m_fileEntries.size() * sizeof(FileEntry);
	u32 dstEntrySize = srcEntrySize;

	vector<u8> dstEntryBuffer(dstEntrySize);
	int ret = compress(&dstEntryBuffer[0], &dstEntrySize, (u8*)&m_fileEntries[0], srcEntrySize);
	if (ret != Z_OK || dstEntrySize >= srcEntrySize)
	{
		dstEntrySize = srcEntrySize;
	}

	//compress filenames
	String srcFilename;
	for (u32 i = 0; i < m_filenames.size(); ++i)
	{
		srcFilename += m_filenames[i];
		srcFilename += _T("\n");
	}
	u32 srcFilenameSize = srcFilename.length() * sizeof(Char);
	u32 dstFilenameSize = srcFilenameSize;

	vector<u8> dstFilenameBuffer(dstFilenameSize);
	ret = compress(&dstFilenameBuffer[0], &dstFilenameSize, (const u8*)srcFilename.c_str(), srcFilenameSize);
	if (ret != Z_OK || dstFilenameSize >= srcFilenameSize)
	{
		dstFilenameSize = srcFilenameSize;
	}

	//find pos to write
	if (m_fileEntries.empty())
	{
		m_header.fileEntryOffset = sizeof(m_header);
	}
	else
	{
		FileEntry& last =  m_fileEntries.back();
		u64 lastFileEnd = last.byteOffset + last.packSize;
		if (avoidOverwrite)
		{
			if ((lastFileEnd >= m_header.filenameOffset + m_header.filenameSize) ||
				(lastFileEnd + dstEntrySize + dstFilenameSize <= m_header.fileEntryOffset))
			{
				m_header.fileEntryOffset = lastFileEnd;
			}
			else
			{
				m_header.fileEntryOffset = m_header.filenameOffset + m_header.filenameSize;
			}
		}
		else
		{
			m_header.fileEntryOffset = lastFileEnd;
		}
	}

	_fseeki64(m_stream, m_header.fileEntryOffset, SEEK_SET);
	if (dstEntrySize == srcEntrySize)
	{
		fwrite(&m_fileEntries[0], srcEntrySize, 1, m_stream);
	}
	else
	{
		fwrite(&dstEntryBuffer[0], dstEntrySize, 1, m_stream);
	}
	if (dstFilenameSize == srcFilenameSize)
	{
		fwrite(&srcFilename[0], srcFilenameSize, 1, m_stream);
	}
	else
	{
		fwrite(&dstFilenameBuffer[0], dstFilenameSize, 1, m_stream);
	}

	m_header.fileCount = (u32)m_fileEntries.size();
	m_header.fileEntrySize = dstEntrySize;
	m_header.filenameOffset = m_header.fileEntryOffset + m_header.fileEntrySize;
	m_header.filenameSize = dstFilenameSize;
	m_header.originFilenameSize = srcFilenameSize;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool Package::buildHashTable()
{
	u32 requireSize = m_fileEntries.size() * HASH_TABLE_SCALE;
	u32 tableSize = MIN_HASH_TABLE_SIZE;
	m_hashBits = MIN_HASH_BITS;
	while (tableSize < requireSize)
	{
		if (tableSize >= MAX_HASH_TABLE_SIZE)
		{
			return false;
		}
		tableSize *= 2;
		++m_hashBits;
	}
	m_hashMask = (1 << m_hashBits) - 1;

	bool wrong = false;
	m_hashTable.clear();
	m_hashTable.resize(tableSize, -1);
	for (u32 i = 0; i < m_fileEntries.size(); ++i)
	{
		const FileEntry& currentEntry = m_fileEntries[i];
		u32 index = (currentEntry.nameHash & m_hashMask);
		while (m_hashTable[index] != -1)
		{
			const FileEntry& conflictEntry = m_fileEntries[m_hashTable[index]];
			if (!wrong && (conflictEntry.flag & FILE_DELETE) == 0
				&& conflictEntry.nameHash == currentEntry.nameHash)
			{
				wrong = true;
			}
			if (++index >= tableSize)
			{
				index = 0;
			}
		}
		m_hashTable[index] = i;
	}
	return !wrong;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
int Package::getFileIndex(const Char* filename) const
{
	u64 nameHash = stringHash(filename, HASH_SEED);

	return getFileIndex(nameHash);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
int Package::getFileIndex(u64 nameHash) const
{
	u32 hashIndex = (nameHash & m_hashMask);
	int fileIndex = m_hashTable[hashIndex];
	while (fileIndex >= 0)
	{
		const FileEntry& entry = m_fileEntries[fileIndex];
		if (entry.nameHash == nameHash)
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
u32 Package::insertFileEntry(FileEntry& entry, const Char* filename)
{
	u32 maxIndex = (u32)m_fileEntries.size();
	u64 lastEnd = m_header.headerSize;

	//file with 0 size will alway be put to the end
	for (u32 fileIndex = 0; fileIndex < maxIndex; ++fileIndex)
	{
		FileEntry& thisEntry = m_fileEntries[fileIndex];
		if (thisEntry.byteOffset >= lastEnd + entry.packSize
			&& (lastEnd + entry.packSize <= m_header.fileEntryOffset
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
		lastEnd = thisEntry.byteOffset + thisEntry.packSize;
	}

	if (m_fileEntries.empty() || m_header.fileEntryOffset > lastEnd + entry.packSize)
	{
		entry.byteOffset = lastEnd;
		if (entry.byteOffset + entry.packSize > m_packageEnd)
		{
			m_packageEnd = entry.byteOffset + entry.packSize;
		}
	}
	else
	{
		entry.byteOffset = m_packageEnd;
		m_packageEnd += entry.packSize;
	}
	m_fileEntries.push_back(entry);
	m_filenames.push_back(filename);
	assert(m_filenames.size() == m_fileEntries.size());
	return m_filenames.size() - 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool Package::insertFileHash(u64 nameHash, u32 entryIndex)
{
	u32 requireSize = m_fileEntries.size() * HASH_TABLE_SCALE;
	u32 tableSize = m_hashTable.size();
	if (tableSize < requireSize)
	{
		//file entry hash been inserted, just rebuild hash table
		return buildHashTable();
	}
	u32 index = (nameHash & m_hashMask);
	while (m_hashTable[index] != -1)
	{
		const FileEntry& entry = m_fileEntries[m_hashTable[index]];
		if ((entry.flag & FILE_DELETE) == 0 && entry.nameHash == nameHash)
		{
			//it's possible that file is added back right after it's deleted
			return false;
		}
		if (++index >= tableSize)
		{
			index = 0;
		}
	}
	m_hashTable[index] = entryIndex;
	return true;
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
void Package::writeRawFile(FileEntry& entry, FILE* file)
{
	_fseeki64(m_stream, entry.byteOffset, SEEK_SET);

	u32 chunkCount = (entry.originSize + m_header.chunkSize - 1) / m_header.chunkSize;
	for (u32 i = 0; i < chunkCount; ++i)
	{
		u32 curChunkSize = m_header.chunkSize;
		if (i == chunkCount - 1 && entry.originSize % m_header.chunkSize != 0)
		{
			curChunkSize = entry.originSize % m_header.chunkSize;
		}
		fread(&m_chunkData[0], curChunkSize, 1, file);
		fwrite(&m_chunkData[0], curChunkSize, 1, file);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void Package::writeCompressFile(FileEntry& entry, FILE* file)
{
	_fseeki64(m_stream, entry.byteOffset, SEEK_SET);

	u32 chunkCount = (entry.originSize + m_header.chunkSize - 1) / m_header.chunkSize;
	m_chunkPosBuffer.resize(chunkCount);

	entry.packSize = 0;
	if (chunkCount > 1)
	{
		m_chunkPosBuffer[0] = chunkCount * sizeof(u32);
		fwrite(&m_chunkPosBuffer[0], chunkCount * sizeof(u32), 1, m_stream);
	}

	//BEGIN_PERF("compress");

	u8* dstBuffer = &m_compressBuffer[0];
	for (u32 i = 0; i < chunkCount; ++i)
	{
		u32 curChunkSize = m_header.chunkSize;
		if (i == chunkCount - 1 && entry.originSize % m_header.chunkSize != 0)
		{
			curChunkSize = entry.originSize % m_header.chunkSize;
		}
		fread(&m_chunkData[0], curChunkSize, 1, file);

		//__int64 perfBefore, perfAfter, perfFreq;
		//::QueryPerformanceFrequency((LARGE_INTEGER*)&perfFreq);
		//::QueryPerformanceCounter((LARGE_INTEGER*)&perfBefore);

		u32 dstSize = m_header.chunkSize;
		int ret = compress(dstBuffer, &dstSize, &m_chunkData[0], curChunkSize);

		//::QueryPerformanceCounter((LARGE_INTEGER*)&perfAfter);
		//double perfTime = 1000.0 * (perfAfter - perfBefore) / perfFreq;
		//g_compressTime += perfTime;

		if (ret != Z_OK	|| dstSize >= curChunkSize)
		{
			//compress failed or compressed size greater than origin, write raw data
			fwrite(&m_chunkData[0], curChunkSize, 1, m_stream);
			dstSize = curChunkSize;
		}
		else
		{
			fwrite(dstBuffer, dstSize, 1, m_stream);
		}
		if (i + 1 < chunkCount)
		{
			m_chunkPosBuffer[i + 1] = m_chunkPosBuffer[i] + dstSize;
		}
		entry.packSize += dstSize;
	}
	
	//END_PERF

	if (chunkCount > 1)
	{
		entry.packSize += chunkCount * sizeof(u32);
		_fseeki64(m_stream, entry.byteOffset, SEEK_SET);
		fwrite(&m_chunkPosBuffer[0], chunkCount * sizeof(u32), 1, m_stream);
	}
	else if (entry.packSize == entry.originSize)
	{
		//only 1 chunk and not compressed, entire file should not be compressed
		entry.flag &= (~FILE_COMPRESS);
	}
	//temp
	if (m_packageEnd == entry.byteOffset + entry.originSize)
	{
		m_packageEnd = entry.byteOffset + entry.packSize;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
u32 Package::getFileAvailableSize(u64 nameHash) const
{
	int fileIndex = getFileIndex(nameHash);
	if (fileIndex < 0)
	{
		return 0;
	}
	const FileEntry& entry = m_fileEntries[fileIndex];
	if ((entry.flag & FILE_WRITING) == 0)
	{
		return entry.packSize;
	}
	return entry.flag & 0xfffff000;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool Package::setFileAvailableSize(u64 nameHash, u32 size)
{
	int fileIndex = getFileIndex(nameHash);
	if (fileIndex < 0)
	{
		return false;
	}
	FileEntry& entry = m_fileEntries[fileIndex];
	if ((entry.flag & FILE_WRITING) == 0)
	{
		return false;
	}
	entry.flag = (entry.flag & 0x00000fff) | (size & 0xfffff000);
	if (size == entry.packSize)
	{
		//done
		entry.flag &= ~FILE_WRITING;
		entry.flag &= 0x00000fff;
	}
	return true;
}

}
