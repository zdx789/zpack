#include "zpPackage.h"
#include "zpFile.h"
#include <cassert>
#include <sstream>

namespace zp
{

const u32 MIN_HASH_TABLE_SIZE = 256;
const u32 MAX_HASH_TABLE_SIZE = 0x80000;
const u32 HASH_SEED0 = 31;	//not a good idea to change these numbers, must be identical between reading and writing code
const u32 HASH_SEED1 = 131;
const u32 HASH_SEED2 = 1313;

using namespace std;

///////////////////////////////////////////////////////////////////////////////////////////////////
Package::Package(const Char* filename, bool readonly)
	: m_fileEnd(0)
	, m_readonly(readonly)
	, m_dirty(false)
{
	locale loc = locale::global(locale(""));
	m_stream.open(filename, ios_base::in | ios_base::out | ios_base::binary);
	locale::global(loc);
	if (!m_stream.is_open()
		|| !readHeader()
		|| !readFileEntries()
		|| !readFilenames())
	{
		m_stream.close();
		return;
	}
	buildHashTable();
	if (!readonly)
	{
		m_packageName = filename;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
Package::~Package()
{
	flush();
	if (m_stream.is_open())
	{
		m_stream.close();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool Package::valid() const
{
	return m_stream.is_open();
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
	return new File(m_stream, entry.byteOffset, entry.fileSize, entry.flag);
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
bool Package::getFileInfo(u32 index, Char* filenameBuffer, u32 filenameBufferSize, u32* fileSize) const
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
	if (fileSize != NULL)
	{
		*fileSize = m_fileEntries[index].fileSize;
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool Package::addFile(const Char* externalFilename, const Char* filename, u32 flag, u32* fileSize)
{
	if (m_readonly)
	{
		return false;
	}
	fstream stream;
	locale loc = locale::global(locale(""));
	stream.open(externalFilename, ios_base::in | ios_base::binary);
	locale::global(loc);
	if (!stream.is_open())
	{
		return false;
	}
	int fileIndex = getFileIndex(filename);
	if (fileIndex >= 0)
	{
		//file exist
		if ((flag & FLAG_REPLACE) == 0)
		{
			return false;
		}
		m_fileEntries[fileIndex].flag |= FILE_FLAG_DELETED;
	}
	FileEntry entry;
	entry.flag = 0;
	entry.hash0 = stringHash(filename, HASH_SEED0);
	entry.hash1 = stringHash(filename, HASH_SEED1);
	entry.hash2 = stringHash(filename, HASH_SEED2);
	stream.seekg(0, ios::end);
	entry.fileSize = static_cast<u32>(stream.tellg());
	void* content = new unsigned char[entry.fileSize];
	stream.seekg(0, ios::beg);
	stream.read((char*)content, entry.fileSize);
	stream.close();

	insertFile(entry, filename);

	m_stream.seekg(entry.byteOffset, ios::beg);
	m_stream.write((char*)content, entry.fileSize);
	delete[] content;
	
	m_dirty = true;
	if (fileSize != NULL)
	{
		*fileSize = entry.fileSize;
	}
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
	m_fileEntries[fileIndex].flag |= FILE_FLAG_DELETED;
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
	//file entries
	assert(m_filenames.size() == m_fileEntries.size());
	vector<String>::iterator nameIter = m_filenames.begin();
	for (vector<FileEntry>::iterator iter = m_fileEntries.begin();
		iter != m_fileEntries.end(); )
	{
		FileEntry& entry = *iter;
		if ((entry.flag & FILE_FLAG_DELETED) != 0)
		{
			iter = m_fileEntries.erase(iter);
			nameIter = m_filenames.erase(nameIter);
			continue;
		}
		++iter;
		++nameIter;
	}
	m_header.fileCount = (u32)m_fileEntries.size();
	if (m_fileEntries.empty())
	{
		 m_header.fileEntryOffset = sizeof(m_header);
	}
	else
	{
		FileEntry& last =  m_fileEntries.back();
		m_header.fileEntryOffset = last.byteOffset + last.fileSize;
	}
	m_stream.seekg(m_header.fileEntryOffset, ios::beg);
	for (u32 i = 0; i < m_fileEntries.size(); ++i)
	{
		FileEntry& entry = m_fileEntries[i];
		m_stream.write((char*)&entry, sizeof(FileEntry));
	}
	m_header.filenameOffset = m_header.fileEntryOffset + m_header.fileCount * sizeof(FileEntry);
	//filenames
	m_header.filenameSize = 0;
	for (u32 i = 0; i < m_filenames.size(); ++i)
	{
		const String& filename = m_filenames[i];
		m_stream.write((char*)filename.c_str(), (u32)filename.length() * sizeof(Char));
		m_stream.write((char*)_T("\n"), sizeof(Char));
		m_header.filenameSize += (((u32)filename.length() + 1) * sizeof(Char));
	}
	m_fileEnd = m_header.filenameOffset + m_header.filenameSize;
	m_stream.seekg(0, ios::beg);
	m_stream.write((char*)&m_header, sizeof(m_header));
	m_stream.flush();
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
	m_stream.seekg(0, ios::end);
	u64 currentSize = m_stream.tellg();
	return currentSize - totalSize;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool Package::defrag(Callback callback, void* callbackParam)
{
	if (m_readonly || m_dirty)
	{
		return false;
	}
	String tempFilename = m_packageName + _T("_");
	fstream tempFile;
	locale loc = locale::global(locale(""));
	tempFile.open(tempFilename.c_str(), std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
	locale::global(loc);
	if (!tempFile.is_open())
	{
		return false;
	}
	tempFile.seekg(sizeof(m_header), ios::beg);

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
			tempFile.close();
			Remove(tempFilename.c_str());
			return false;
		}
		if (entry.fileSize == 0)
		{
			entry.byteOffset = nextPos;
			continue;
		}
		if ((entry.flag & FILE_FLAG_DELETED) != 0)
		{
			continue;
		}
		if (entry.byteOffset != fragmentSize + nextPos	//new fragment encountered
			|| currentChunkSize > MIN_CHUNK_SIZE)
		{
			if (currentChunkSize > 0)
			{
				tempBuffer.resize(currentChunkSize);
				m_stream.seekg(currentChunkPos, ios::beg);
				m_stream.read(&tempBuffer[0], currentChunkSize);
				tempFile.write(&tempBuffer[0], currentChunkSize);
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
		m_stream.seekg(currentChunkPos, ios::beg);
		m_stream.read(&tempBuffer[0], currentChunkSize);
		tempFile.write(&tempBuffer[0], currentChunkSize);
	}

	m_stream.close();
	tempFile.close();

	loc = locale::global(locale(""));
	m_stream.open(tempFilename.c_str(), ios_base::in | ios_base::out | ios_base::binary);	//only for flush()
	locale::global(loc);
	assert(m_stream.is_open());
	m_dirty = true;
	flush();	//no need to rebuild hash table here, but it's ok b/c defrag will be slow anyway
	m_stream.close();

	Remove(m_packageName.c_str());
	Rename(tempFilename.c_str(), m_packageName.c_str());
	loc = locale::global(locale(""));
	m_stream.open(m_packageName.c_str(), ios_base::in | ios_base::out | ios_base::binary);
	locale::global(loc);
	assert(m_stream.is_open());
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool Package::readHeader()
{
	m_stream.seekg(0, ios::end);
	m_fileEnd = m_stream.tellg();
	if (m_fileEnd < sizeof(PackageHeader))
	{
		return false;
	}
	m_stream.seekg(0, ios::beg);
	m_stream.read((char*)&m_header, sizeof(PackageHeader));
	if (m_header.sign != PACKAGE_FILE_SIGN
		|| m_header.headerSize < sizeof(PackageHeader)
		|| m_header.fileEntrySize < sizeof(FileEntry)
		|| m_header.fileEntryOffset < m_header.headerSize
		|| m_header.fileCount * m_header.fileEntrySize + m_header.fileEntryOffset > m_fileEnd
		|| m_header.filenameOffset + m_header.filenameSize > m_fileEnd)
	{
		return false;
	}
	if (m_header.version != CURRENT_VERSION)
	{
		m_readonly = true;
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool Package::readFileEntries()
{
	m_fileEntries.resize(m_header.fileCount);
	m_stream.seekg(m_header.fileEntryOffset, ios::beg);
	u32 extraDataSize = m_header.fileEntrySize - sizeof(FileEntry);

	u64 nextOffset = m_header.headerSize;
	for (u32 i = 0; i < m_header.fileCount; ++i)
	{
		if (nextOffset > m_header.fileEntryOffset)
		{
			return false;
		}
		FileEntry& entry = m_fileEntries[i];
		m_stream.read((char*)&entry, sizeof(FileEntry));
		if (entry.byteOffset < nextOffset)
		{
			return false;
		}
		nextOffset = entry.byteOffset + entry.fileSize;
		if (extraDataSize > 0)
		{
			m_stream.seekg(extraDataSize, ios::cur);
		}
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool Package::readFilenames()
{
	if (m_fileEntries.empty() || m_readonly)
	{
		return true;
	}
	u32 charCount = m_header.filenameSize / sizeof(Char);
	String names;
	names.resize(charCount + 1);
	//hack
	Char* buffer = const_cast<Char*>(names.c_str());

	m_filenames.resize(m_fileEntries.size());
	m_stream.seekg(m_header.filenameOffset, ios::beg);
	m_stream.read((char*)buffer, charCount * sizeof(Char));
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
	while (tableSize < m_header.fileCount)
	{
		if (tableSize >= MAX_HASH_TABLE_SIZE)
		{
			return false;
		}
		tableSize *= 2;
	}
	tableSize *= 2;
	m_hashTable.clear();
	m_hashTable.resize(tableSize, -1);
	for (u32 i = 0; i < m_header.fileCount; ++i)
	{
		u32 index = m_fileEntries[i].hash0 % tableSize;
		while (m_hashTable[index] != -1)
		{
			if (++index >= tableSize)
			{
				index = 0;
			}
		}
		m_hashTable[index] = i;
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
int Package::getFileIndex(const Char* filename) const
{
	u32 hash0 = stringHash(filename, HASH_SEED0);
#if (ZP_HASH_NUM > 1)
	u32 hash1 = stringHash(filename, HASH_SEED1);
#endif
#if (ZP_HASH_NUM > 2)
	u32 hash2 = stringHash(filename, HASH_SEED2);
#endif
	u32 hashIndex = hash0 % m_hashTable.size();
	int fileIndex = m_hashTable[hashIndex];
	while (fileIndex >= 0)
	{
		const FileEntry& entry = m_fileEntries[fileIndex];
		if (entry.hash0 == hash0
#if (ZP_HASH_NUM > 1)
			&& entry.hash1 == hash1
#endif
#if (ZP_HASH_NUM > 2)
			&& entry.hash2 == hash2
#endif
			)
		{
			if ((entry.flag & FILE_FLAG_DELETED) != 0)
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
void Package::insertFile(FileEntry& entry, const Char* filename)
{
	u32 maxIndex = (u32)m_fileEntries.size();
	u64 lastEnd = m_header.headerSize;
	if (entry.fileSize != 0)
	{
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
				return;
			}
			lastEnd = thisEntry.byteOffset + thisEntry.fileSize;
		}
	}
	else
	{
		lastEnd = m_fileEnd;
	}
	if (m_header.fileEntryOffset > lastEnd + entry.fileSize)
	{
		entry.byteOffset = lastEnd;
	}
	else
	{
		entry.byteOffset = m_fileEnd;
		m_fileEnd += entry.fileSize;
	}
	m_fileEntries.push_back(entry);
	m_filenames.push_back(filename);
	assert(m_filenames.size() == m_fileEntries.size());
	return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
u32 Package::stringHash(const Char* str, u32 seed) const
{
	u32 out = 0;
	while (*str)
	{
	#if (ZP_CASE_SENSITIVE)
		out = out * seed + *(str++);
	#else
		out = out * seed + tolower(*(str++));
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

}
