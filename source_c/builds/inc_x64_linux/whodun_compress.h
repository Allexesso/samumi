#ifndef WHODUN_COMPRESS_H
#define WHODUN_COMPRESS_H 1

#include <string>
#include <vector>
#include <stdint.h>

#include <zlib.h>

#include "whodun_oshook.h"
#include "whodun_string.h"
#include "whodun_thread.h"

namespace whodun {

/**A method for compressing data.*/
class CompressionMethod{
public:
	/**Allow subclasses to deconstruct.*/
	virtual ~CompressionMethod();
	/**Decompress the data stored in the compData into theData.*/
	virtual void decompressData() = 0;
	/**Compress the data in theData into compData.*/
	virtual void compressData() = 0;
	/**The data to compress and/or the decompressed data.*/
	std::vector<char> theData;
	/**The compressed data and/or data to decompress.*/
	std::vector<char> compData;
};

/**Generate CompressionMethods*/
class CompressionFactory{
public:
	/**Allow subclasses to tear down.*/
	virtual ~CompressionFactory();
	/**
	 * Make a new compression method.
	 * @return The allocated item.
	 */
	virtual CompressionMethod* instantiate() = 0;
};

/**Compress by doing nothing.*/
class RawCompressionMethod : public CompressionMethod{
public:
	/**Simple clean.*/
	~RawCompressionMethod();
	void decompressData();
	void compressData();
};
/**Create RawCompressionMethods*/
class RawCompressionFactory : public CompressionFactory{
public:
	CompressionMethod* instantiate();
};

/**Compress using gzip.*/
class GZipCompressionMethod : public CompressionMethod{
public:
	/**Simple clean.*/
	~GZipCompressionMethod();
	void decompressData();
	void compressData();
};
/**Create RawCompressionMethods*/
class GZipCompressionFactory : public CompressionFactory{
public:
	CompressionMethod* instantiate();
};

/**Simple run length encoding.*/
class RunLengthCompressionMethod : public CompressionMethod{
public:
	void decompressData();
	void compressData();
};
/**Create RunLengthCompressionMethod*/
class RunLengthCompressionFactory : public CompressionFactory{
public:
	CompressionMethod* instantiate();
};

/**Out to gzip file.*/
class GZipOutStream : public OutStream{
public:
	/**
	 * Open the file.
	 * @param append Whether to append to a file if it is already there.
	 * @param fileName The name of the file.
	 */
	GZipOutStream(int append, const char* fileName);
	/**Clean up and close.*/
	~GZipOutStream();
	void writeByte(int toW);
	void writeBytes(const char* toW, uintptr_t numW);
	void close();
	/**The base file.*/
	gzFile baseFile;
	/**The name of the file.*/
	std::string myName;
};

/**In from gzip file.*/
class GZipInStream : public InStream{
public:
	/**
	 * Open the file.
	 * @param fileName The name of the file.
	 */
	GZipInStream(const char* fileName);
	/**Clean up and close.*/
	~GZipInStream();
	int readByte();
	uintptr_t readBytes(char* toR, uintptr_t numR);
	void close();
	/**The base file.*/
	gzFile baseFile;
	/**The name of the file.*/
	std::string myName;
};

/**Bytes for an annotation entry.*/
#define BLOCKCOMP_ANNOT_ENTLEN 32
/**The number of entries to load in memory: less jumping.*/
#define BLOCKCOMPIN_LASTLINESEEK 64

/**Block compress output.*/
class BlockCompOutStream : public OutStream{
public:
	/**
	 * Open up the file to dump to.
	 * @param append Whether to append to a file if it is already there.
	 * @param blockSize The size of the compressed blocks.
	 * @param mainFN The name of the data file.
	 * @param annotFN The name of the annotation file.
	 * @param compMeth The compression method to use for the blocks.
	 */
	BlockCompOutStream(int append, uintptr_t blockSize, const char* mainFN, const char* annotFN, CompressionFactory* compMeth);
	/**Clean up.*/
	~BlockCompOutStream();
	void writeByte(int toW);
	void writeBytes(const char* toW, uintptr_t numW);
	void flush();
	void close();
	/**Get the number of (uncompressed) bytes already written.*/
	uintmax_t tell();
	/**The number of bytes to accumulate before dumping a block.*/
	uintptr_t chunkSize;
	/**The total number of uncompressed bytes written to the start of the current block.*/
	uintmax_t preCompBS;
	/**The total number of compressed bytes written to the start of the current block.*/
	uintmax_t postCompBS;
	/**The data file.*/
	FileOutStream* mainF;
	/**The annotation file. Quads of pre-comp address, post-comp address, pre-comp len, post-comp len.*/
	FileOutStream* annotF;
	/**The compression method this uses.*/
	CompressionMethod* myComp;
};

/**Read a block compressed input stream.*/
class BlockCompInStream : public RandaccInStream{
public:
	/**
	 * Open up a blcok compressed file.
	 * @param mainFN The name of the data file.
	 * @param annotFN The name of the annotation file.
	 * @param compMeth The compression method to use for the blocks.
	 */
	BlockCompInStream(const char* mainFN, const char* annotFN, CompressionFactory* compMeth);
	/**Clean up and close.*/
	~BlockCompInStream();
	int readByte();
	uintptr_t readBytes(char* toR, uintptr_t numR);
	void close();
	void seek(uintmax_t toAddr);
	uintmax_t tell();
	uintmax_t size();
	/**The number of blocks in the file.*/
	uintmax_t numBlocks;
	/**The annotation index at the start of lastLineBuff.*/
	uintmax_t lastLineBI0;
	/**The number of blocks accounted for in lastLineBuff.*/
	uintmax_t numLastLine;
	/**Temporary storage for a seek.*/
	char* lastLineBuff;
	/**The pre-compressed addresses of those block infos.*/
	uintmax_t* lastLineAddrs;
	/**The data file.*/
	FileInStream* mainF;
	/**The annotation file. Quads of pre-comp address, post-comp address, pre-comp len, post-comp len.*/
	FileInStream* annotF;
	/**The current location in the file.*/
	uintmax_t totalReads;
	/**The index in the decompressed data the next read should return.*/
	uintptr_t nextReadI;
	/**The compression method this uses.*/
	CompressionMethod* myComp;
};

/**Compress a single block in a compression stream*/
class MultithreadBlockCompOutStreamUniform : public ThreadTask{
public:
	/**Set up clean.*/
	MultithreadBlockCompOutStreamUniform();
	/**Clean up.*/
	~MultithreadBlockCompOutStreamUniform();
	void doIt();
	/**The number of data to work over.*/
	uintptr_t numData;
	/**The data to compress.*/
	const char* compData;
	/**The compression method this uses.*/
	CompressionMethod* myComp;
	/**The id of the compress task.*/
	uintptr_t threadID;
};

/**Block compress output.*/
class MultithreadBlockCompOutStream : public OutStream{
public:
	/**
	 * Open up the file to dump to.
	 * @param append Whether to append to a file if it is already there.
	 * @param blockSize The size of the compressed blocks.
	 * @param mainFN The name of the data file.
	 * @param annotFN The name of the annotation file.
	 * @param compMeth The compression method to use for the blocks.
	 * @param numThreads The number of threads to spawn.
	 * @param useThreads The threads to use.
	 */
	MultithreadBlockCompOutStream(int append, uintptr_t blockSize, const char* mainFN, const char* annotFN, CompressionFactory* compMeth, int numThreads, ThreadPool* useThreads);
	/**Clean up and close.*/
	~MultithreadBlockCompOutStream();
	void writeByte(int toW);
	void writeBytes(const char* toW, uintptr_t numW);
	void flush();
	void close();
	/**Get the number of (uncompressed) bytes already written.*/
	uintmax_t tell();
	/**The number of bytes to accumulate before dumping a block.*/
	uintptr_t chunkSize;
	/**The total number of uncompressed bytes written to the start of the current block.*/
	uintmax_t preCompBS;
	/**The total number of compressed bytes written to the start of the current block.*/
	uintmax_t postCompBS;
	/**The running total of bytes written: preCompBS can get out of sync with this.*/
	uintmax_t totalWrite;
	/**The data file.*/
	FileOutStream* mainF;
	/**The annotation file. Quads of pre-comp address, post-comp address, pre-comp len, post-comp len.*/
	FileOutStream* annotF;
	/**The threads to use for compression.*/
	ThreadPool* compThreads;
	/**The maximum number of bytes that can be marshalled.*/
	uintptr_t maxMarshal;
	/**The number of bytes marshalled.*/
	uintptr_t numMarshal;
	/**A place to store data before compression.*/
	char* chunkMarshal;
	/**Compression tasks.*/
	std::vector<MultithreadBlockCompOutStreamUniform> threadUnis;
	/**Compression tasks.*/
	std::vector<ThreadTask*> threadPass;
	/**The multithread utility.*/
	MultithreadedStringH stringDumpHandle;
	/**
	 * Compress everything in and dump.
	 * @param numDump The number of bytes to dump.
	 * @param dumpFrom The bytes to dump.
	 */
	void compressAndOut(uintptr_t numDump, const char* dumpFrom);
};

/**Decompress a single block in a compression stream*/
class MultithreadBlockCompInStreamUniform : public ThreadTask{
public:
	/**Set up clean.*/
	MultithreadBlockCompInStreamUniform();
	/**Clean up.*/
	~MultithreadBlockCompInStreamUniform();
	void doIt();
	/**The compression method this uses.*/
	CompressionMethod* myComp;
	/**The id of the compress task.*/
	uintptr_t compressID;
};

/**Read a block compressed input stream: does NOT allow random access.*/
class MultithreadBlockCompInStream : public InStream{
public:
	/**
	 * Open up a blcok compressed file.
	 * @param mainFN The name of the data file.
	 * @param annotFN The name of the annotation file.
	 * @param compMeth The compression method to use for the blocks.
	 * @param numThreads The number of threads to spawn.
	 * @param useThreads The threads to use.
	 */
	MultithreadBlockCompInStream(const char* mainFN, const char* annotFN, CompressionFactory* compMeth, int numThreads, ThreadPool* useThreads);
	/**Clean up and close.*/
	~MultithreadBlockCompInStream();
	int readByte();
	uintptr_t readBytes(char* toR, uintptr_t numR);
	void close();
	/**The number of blocks in the file.*/
	uintmax_t numBlocks;
	/**The data file.*/
	FileInStream* mainF;
	/**The annotation file. Quads of pre-comp address, post-comp address, pre-comp len, post-comp len.*/
	FileInStream* annotF;
	/**The threads to use for compression.*/
	ThreadPool* compThreads;
	/**The number of read blocks.*/
	uintmax_t numReadBlocks;
	/**A place to store stuff for each action.*/
	std::vector<MultithreadBlockCompInStreamUniform> threadUnis;
	/**The next uniform to load from.*/
	uintptr_t nextOutUni;
	/**The byte in said uniform to load from.*/
	uintptr_t nextOutByte;
	/**The multithread utility.*/
	MultithreadedStringH stringDumpHandle;
	/**The uniforms for bulk memcpys.*/
	std::vector<MultithreadedStringMemcpyTask> stringDumpUnis;
	/**Whether memcopies were invoked.*/
	std::vector<int> stringDumpHot;
	/**Load (and decompress) stuff from the file.*/
	void fillBuffer();
};



}

#endif
