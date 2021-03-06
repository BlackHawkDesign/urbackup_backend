#pragma once

#include <string>
#include <memory>

#include "../Interface/Server.h"
#include "../Interface/File.h"
#include "LRUMemCache.h"


class CompressedFile : public IFile, public ICacheEvictionCallback
{
public:
	CompressedFile(std::wstring pFilename, int pMode);
	CompressedFile(IFile* file, bool openExisting, bool readOnly);
	~CompressedFile();

	virtual std::string Read(_u32 tr);
	virtual _u32 Read(char* buffer, _u32 bsize);
	virtual _u32 Write(const std::string &tw);
	virtual _u32 Write(const char* buffer, _u32 bsize);
	virtual bool Seek(_i64 spos);
	virtual _i64 Size(void);

	virtual std::string getFilename(void);
	virtual std::wstring getFilenameW(void);

	bool hasError();

	bool finish();

	bool hasNoMagic();

private:
	void readHeader();
	void readIndex();
	bool fillCache(__int64 offset, bool errorMsg);
	virtual void evictFromLruCache(const SCacheItem& item);
	void writeHeader();
	void writeIndex();

	_u32 readFromFile(char* buffer, _u32 bsize);
	_u32 writeToFile(const char* buffer, _u32 bsize);

	__int64 filesize;
	__int64 index_offset;
	_u32 blocksize;

	__int64 currentPosition;

	std::vector<__int64> blockOffsets;

	IFile* uncompressedFile;
	
	std::auto_ptr<LRUMemCache> hotCache;

	std::vector<char> compressedBuffer;

	bool error;

	bool finished;

	bool readOnly;

	bool noMagic;
};