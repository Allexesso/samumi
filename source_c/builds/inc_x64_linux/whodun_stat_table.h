#ifndef WHODUN_STAT_TABLE_H
#define WHODUN_STAT_TABLE_H 1

#include <vector>
#include <stdint.h>

#include "whodun_parse.h"
#include "whodun_thread.h"
#include "whodun_compress.h"

namespace whodun {

//*********************************************************
//Text table

/**A loaded entry in a table.*/
class TextTableRow{
public:
	/**Set up an empty entry.*/
	TextTableRow();
	/**Clean up*/
	~TextTableRow();
	/**The number of entries in the table.*/
	uintptr_t numEntries;
	/**The lengths of those entries.*/
	uintptr_t* entrySizes;
	/**The first characters of those entries.*/
	char** curEntries;
	/**The allocation for columns.*/
	uintptr_t colAlloc;
	/**The allocated size of entryStore.*/
	uintptr_t entryAlloc;
	/**Storage for the text.*/
	char* entryStore;
	/**
	 * Ensure capacity.
	 * @param numCol THe number of columns to support.
	 * @param numChar The number of characters to support.
	 */
	void ensureCapacity(uintptr_t numCol, uintptr_t numChar);
	/**
	 * Trim whitespace off the end.
	 */
	void trim();
	/**
	 * Trim characters off the end.
	 * @param numChar The number of characters to trim.
	 * @param theChar The characters to trim.
	 */
	void trim(uintptr_t numChar, const char* theChar);
	/**
	 * Clone another row.
	 * @param toCopy The thing to copy.
	 */
	void clone(TextTableRow* toCopy);
};

/**Read from a text table.*/
class TextTableReader{
public:
	/**Set up.*/
	TextTableReader();
	/**ALlow subcalsses to tear down.*/
	virtual ~TextTableReader();
	
	/**
	 * Get the next entry.
	 * @return The next entry. 0 for end of file.
	 */
	virtual TextTableRow* read() = 0;
	/** Close anything this thing opened. */
	virtual void close() = 0;
	
	/**
	 * Get space for an entry.
	 * @return A new entry.
	 */
	virtual TextTableRow* allocate();
	/**
	 * Return an entry: can cut down on allocation.
	 * @param toRet The sequence to return.
	 */
	virtual void release(TextTableRow* toRet);
	/**A cache for entries.*/
	std::vector<TextTableRow*> cache;
};

/**Random access to a text table.*/
class RandacTextTableReader : public TextTableReader{
public:
	/**
	 * Get the number of entries in the file.
	 * @return The number of entries.
	 */
	virtual uintmax_t getNumEntries() = 0;
	/**
	 * Read a specific entry.
	 * @param entInd The index.
	 * @return The sequence.
	 */
	virtual TextTableRow* getEntry(uintmax_t endInd) = 0;
};

/**Write rows to a text table.*/
class TextTableWriter{
public:
	/**Set up*/
	TextTableWriter();
	/**Allow subclasses to tear down.*/
	virtual ~TextTableWriter();
	
	/**
	 * Write out an entry.
	 * @param nextOut The next entry to write.
	 */
	virtual void write(TextTableRow* nextOut) = 0;
	/**
	 * Write out multiple entries.
	 * @param numWrite the number of entries to write.
	 * @param allOut The entries to write.
	 */
	virtual void write(uintptr_t numWrite, TextTableRow** allOut);
	/** Close anything this thing opened. */
	virtual void close() = 0;
};

/**Do actions for parsing/packing delimited tables.*/
class DelimitedTableUniform : public ThreadTask{
public:
	/**Do the action.*/
	void doIt();
	/**The action to do.*/
	int actDo;
	/**The number of things to manage.*/
	uintptr_t numManage;
	/**The raw input string start.*/
	char** bulkStrStart;
	/**The raw input string end.*/
	char** bulkStrEnd;
	/**Temporary pack storage.*/
	std::vector<char> tmpPack;
	/**The data in a class.*/
	std::deque<TextTableRow*>::iterator bulkTTR;
	/**The data in a class.*/
	TextTableRow** bulkoTTR;
	/**The escape prefix.*/
	char escapePref;
	/**The escape sequences.*/
	short* replaceForw;
	/**Split columns.*/
	CharacterSplitter doSplit;
	/**Remember the row/column delimiter.*/
	char rowDelim;
	/**Remember the row/column delimiter.*/
	char colDelim;
};

/**Read a delimited table.*/
class DelimitedTableReader : public TextTableReader{
public:
	/**
	 * Set up a delimited input.
	 * @param rowDelim The delimiter for rows.
	 * @param colDelim The delimiter for columns.
	 * @param mainFrom The stream to delimit.
	 */
	DelimitedTableReader(char rowDelim, char colDelim, InStream* mainFrom);
	/**
	 * Set up a delimited input.
	 * @param rowDelim The delimiter for rows.
	 * @param colDelim The delimiter for columns.
	 * @param mainFrom The stream to delimit.
	 * @param numThread The number of tasks to spawn.
	 * @param mainPool The threads to use.
	 */
	DelimitedTableReader(char rowDelim, char colDelim, InStream* mainFrom, int numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~DelimitedTableReader();
	TextTableRow* read();
	void close();
	/**Whether to use escapes.*/
	int useEscapes;
	/**The escape prefix.*/
	char escapePref;
	/**Packed replacement info: if you see an escape, pass the following character to this to get the replacement.*/
	short replacePack[256];
	/**Entries already loaded and waiting to return.*/
	std::deque<TextTableRow*> waitEnts;
	/**The thing to parse.*/
	InStream* theStr;
	/**Split on newlines.*/
	CharacterSplitter* newlineSpl;
	/**Read newlines.*/
	SplitterInStream* newlineStr;
	/**The maximum number of threads to spin up.*/
	uintptr_t numThr;
	/**The pool to use, if any.*/
	ThreadPool* usePool;
	/**Active tasks.*/
	std::vector<DelimitedTableUniform> liveTask;
	/**Allocated task uniform table.*/
	std::vector<DelimitedTableUniform*> passTask;
	/**Save delimiter.*/
	char saveRD;
	/**Save delimiter.*/
	char saveCD;
	/**Whether the uniforms need initialization.*/
	int needInitTask;
};

/**Write a delimited table.*/
class DelimitedTableWriter : public TextTableWriter{
public:
	/**
	 * Set up a delimited input.
	 * @param rowDelim The delimiter for rows.
	 * @param colDelim The delimiter for columns.
	 * @param mainFrom The stream to delimit.
	 */
	DelimitedTableWriter(char rowDelim, char colDelim, OutStream* mainFrom);
	/**
	 * Set up a delimited input.
	 * @param rowDelim The delimiter for rows.
	 * @param colDelim The delimiter for columns.
	 * @param mainFrom The stream to delimit.
	 * @param numThread The number of tasks to spawn.
	 * @param mainPool The threads to use.
	 */
	DelimitedTableWriter(char rowDelim, char colDelim, OutStream* mainFrom, int numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~DelimitedTableWriter();
	void write(TextTableRow* nextOut);
	void write(uintptr_t numWrite, TextTableRow** allOut);
	void close();
	/**Whether to use escapes.*/
	int useEscapes;
	/**The escape prefix.*/
	char escapePref;
	/**Packed replacement info: if you see one of these characters, output an escape sequence using the replacement.*/
	short replacePack[256];
	/**The thing to parse.*/
	OutStream* theStr;
	/**The maximum number of threads to spin up.*/
	uintptr_t numThr;
	/**The pool to use, if any.*/
	ThreadPool* usePool;
	/**Active tasks.*/
	std::vector<DelimitedTableUniform> liveTask;
	/**Allocated task uniform table.*/
	std::vector<DelimitedTableUniform*> passTask;
	/**Save delimiter.*/
	char saveRD;
	/**Save delimiter.*/
	char saveCD;
	/**Whether the uniforms need initialization.*/
	int needInitTask;
	/**A buffer for writing.*/
	uintptr_t packAlloc;
	/**A buffer for writing.*/
	char* packBuffer;
	/**Cut the buffer into pieces.*/
	std::vector<char*> packCut;
};

/**Set up a TSV reader.*/
class TSVTableReader : public DelimitedTableReader{
public:
	/**
	 * Set up a delimited input.
	 * @param mainFrom The stream to delimit.
	 */
	TSVTableReader(InStream* mainFrom);
	/**
	 * Set up a delimited input.
	 * @param mainFrom The stream to delimit.
	 * @param numThread The number of tasks to spawn.
	 * @param mainPool The threads to use.
	 */
	TSVTableReader(InStream* mainFrom, int numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~TSVTableReader();
};

/**Set up a TSV writer.*/
class TSVTableWriter : public DelimitedTableWriter{
public:
	/**
	 * Set up a delimited input.
	 * @param mainFrom The stream to delimit.
	 */
	TSVTableWriter(OutStream* mainFrom);
	/**
	 * Set up a delimited input.
	 * @param mainFrom The stream to delimit.
	 * @param numThread The number of tasks to spawn.
	 * @param mainPool The threads to use.
	 */
	TSVTableWriter(OutStream* mainFrom, int numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~TSVTableWriter();
};

/**Do multithreaded block compress tsv things.*/
class BlockCompTextTableUniform : public ThreadTask{
public:
	void doIt();
	/**The thing to do.*/
	int taskStage;
	/**The row to pack.*/
	TextTableRow* toPack;
	/**The offset into the bulk data.*/
	uintptr_t packInd;
	/**The bulk data to pack from.*/
	char* packFrom;
	/**The id of the running thread.*/
	uintptr_t threadID;
};

/**Quick sequential access to a bctsv file.*/
class BlockCompTextTableReader : public TextTableReader{
public:
	/**
	 * Open a gail file.
	 * @param fileName The name of the annotation file.
	 * @param numThread The number of threads to use.
	 * @param mainPool The threads to use.
	 */
	BlockCompTextTableReader(const char* fileName, int numThread, ThreadPool* mainPool);
	/**Clean up*/
	~BlockCompTextTableReader();
	TextTableRow* read();
	void close();
	/**The next index to report.*/
	uintmax_t focusInd;
	/**The number of entries in the thing.*/
	uintmax_t numEntry;
	/**Entries already loaded and waiting to return.*/
	std::deque<TextTableRow*> waitEnts;
	/**The maximum number of threads to spin up.*/
	uintptr_t numThr;
	/**The pool to use, if any.*/
	ThreadPool* usePool;
	/**Active tasks.*/
	std::vector<BlockCompTextTableUniform> liveTask;
	/**Allocated tasks.*/
	std::vector<BlockCompTextTableUniform*> passTask;
	/**Allocated storage for loading.*/
	uintptr_t loadAlloc;
	/**Storage for loading.*/
	char* loadBuffer;
	/**Storage for loaded annotations.*/
	char* annotBuffer;
	/**The index data.*/
	InStream* indStr;
	/**The tsv data.*/
	InStream* tsvStr;
};

/**Random access to a bctsv file.*/
class RandacBlockCompTextTableReader : public RandacTextTableReader{
public:
	/**
	 * Open a gail file.
	 * @param fileName The name of the annotation file.
	 */
	RandacBlockCompTextTableReader(const char* fileName);
	/**Clean up*/
	~RandacBlockCompTextTableReader();
	TextTableRow* read();
	void close();
	uintmax_t getNumEntries();
	TextTableRow* getEntry(uintmax_t endInd);
	/**Whether the streams need a seek to happen (i.e. a random access function was called).*/
	int needSeek;
	/**The next index to report.*/
	uintmax_t focusInd;
	/**The number of entries in the thing.*/
	uintmax_t numEntry;
	/**The index data.*/
	FileInStream* indStr;
	/**The name data.*/
	BlockCompInStream* tsvStr;
};

/**Write a bctsv file.*/
class BlockCompTextTableWriter : public TextTableWriter{
public:
	/**
	 * Open a gail file.
	 * @param fileName The name of the annotation file.
	 */
	BlockCompTextTableWriter(const char* fileName);
	/**
	 * Open a gail file.
	 * @param fileName The name of the annotation file.
	 * @param numThread The number of threads to use.
	 * @param mainPool The threads to use.
	 */
	BlockCompTextTableWriter(const char* fileName, int numThread, ThreadPool* mainPool);
	/**Clean up*/
	~BlockCompTextTableWriter();
	void write(TextTableRow* nextOut);
	void write(uintptr_t numWrite, TextTableRow** allOut);
	void close();
	/**Allocated storage for quality data.*/
	uintptr_t qualityAlloc;
	/**Storage for big endian quality data.*/
	char* qualityBuffer;
	/**The total written data.*/
	uintmax_t totalOut;
	/**The maximum number of threads to spin up.*/
	uintptr_t numThr;
	/**The pool to use.*/
	ThreadPool* usePool;
	/**Active tasks.*/
	std::vector<BlockCompTextTableUniform> liveTask;
	/**Allocated tasks.*/
	std::vector<BlockCompTextTableUniform*> passTask;
	/**The index data.*/
	OutStream* indStr;
	/**The name data.*/
	OutStream* tsvStr;
};

/**
 * Get the file extensions ExtensionTextTableReader/Writer will recognize.
 * @param modeWrite WHether we will be writing.
 * @param toFill The place to put the extensions.
 */
void getTextTableExtensions(int modeWrite, std::set<std::string>* toFill);

/**
 * Get the file extensions ExtensionRandacTextTableReader will recognize.
 * @param toFill The place to put the extensions.
 */
void getRandacTextTableExtensions(std::set<std::string>* toFill);

/**Open a file based on a file name.*/
class ExtensionTextTableReader : public TextTableReader{
public:
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 */
	ExtensionTextTableReader(const char* fileName);
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 * @param numThread The number of threads to use for the base reader.
	 * @param mainPool The threads to use for reading.
	 */
	ExtensionTextTableReader(const char* fileName, int numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~ExtensionTextTableReader();
	
	TextTableRow* read();
	void close();
	TextTableRow* allocate();
	void release(TextTableRow* toRet);
	
	/**The base streams.*/
	std::vector<InStream*> baseStrs;
	/**The wrapped reader.*/
	TextTableReader* wrapStr;
};

/**Open up a random access text table by extension.*/
class ExtensionRandacTextTableReader : public RandacTextTableReader{
public:
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 */
	ExtensionRandacTextTableReader(const char* fileName);
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 * @param numThread The number of threads to use for the base reader.
	 * @param mainPool The threads to use for reading.
	 */
	ExtensionRandacTextTableReader(const char* fileName, int numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~ExtensionRandacTextTableReader();
	
	TextTableRow* read();
	void close();
	TextTableRow* allocate();
	void release(TextTableRow* toRet);
	uintmax_t getNumEntries();
	TextTableRow* getEntry(uintmax_t endInd);
	
	/**The base streams.*/
	std::vector<InStream*> baseStrs;
	/**The wrapped reader.*/
	RandacTextTableReader* wrapStr;
};

/**Open a file based on a file name.*/
class ExtensionTextTableWriter : public TextTableWriter{
public:
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 */
	ExtensionTextTableWriter(const char* fileName);
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 * @param numThread The number of threads to use for the base reader.
	 * @param mainPool The threads to use for reading.
	 */
	ExtensionTextTableWriter(const char* fileName, int numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~ExtensionTextTableWriter();
	
	void write(TextTableRow* nextOut);
	void write(uintptr_t numWrite, TextTableRow** allOut);
	void close();
	
	/**The base streams.*/
	std::vector<OutStream*> baseStrs;
	/**The wrapped writer.*/
	TextTableWriter* wrapStr;
};

/**Cache reads from a text table.*/
class TextTableCache : public RandacTextTableReader{
public:
	/**
	 * Set up.
	 * @param maxCacheSize The maximum size of the cache.
	 * @param baseArray The underlying array to cache.
	 */
	TextTableCache(uintptr_t maxCacheSize, RandacTextTableReader* baseArray);
	/**Clean up.*/
	~TextTableCache();
	
	TextTableRow* read();
	void close();
	TextTableRow* allocate();
	void release(TextTableRow* toRet);
	uintmax_t getNumEntries();
	TextTableRow* getEntry(uintmax_t entInd);
	
	/**The maximum number of cachelines to store.*/
	uintptr_t maxCache;
	/**The wrapped source.*/
	RandacTextTableReader* baseArr;
	
	/**Lock access to the TextTableRow cache.*/
	OSMutex allocLock;
	/**Save the number of entries.*/
	uintmax_t saveNumEnt;
	/**Lock access to the cache and file.*/
	ReadWriteLock cacheLock;
	/**The cached sequences.*/
	std::map<uintmax_t,TextTableRow*> saveSeqs;
	/**The current cache size.*/
	uintptr_t curCacheSize;
};

//*********************************************************
//Data table

/**Categorical data.*/
#define WHODUN_DATA_CAT 1
/**Integer data.*/
#define WHODUN_DATA_INT 2
/**Real data.*/
#define WHODUN_DATA_REAL 3
/**(Fixed-length) string data.*/
#define WHODUN_DATA_STR 4

/**An entry in a data table.*/
typedef struct{
	/**Whether the entry is not known.*/
	int isNA;
	union{
		/**The value, for categorical entries.*/
		uint64_t valC;
		/**The value, for integer entries.*/
		int64_t valI;
		/**The value, for real entries.*/
		double valR;
		/**The value, for string entries.*/
		char* valS;
	};
} DataTableEntry;

/**A header for a data table.*/
class DataTableDescription{
public:
	/**Set up.*/
	DataTableDescription();
	/**Tear down.*/
	~DataTableDescription();
	
	/**The types of data in each column.*/
	std::vector<int> colTypes;
	/**The names of the columns.*/
	std::vector<std::string> colNames;
	/**For categorical data, the valid levels.*/
	std::vector< std::map<std::string,int> > factorColMap;
	/**The lengths of any string variables.*/
	std::vector<uintptr_t> strLengths;
	
	/**Once the above are decided, fill in the following.*/
	void initialize();
	/**The number of bytes for a row.*/
	uintptr_t rowBytes;
	/**The offset for a given column.*/
	std::vector<uintptr_t> colOffset;
};

/**An item in a data table.*/
class DataTableRow{
public:
	/**Set up a completely blank row: dangerous.*/
	DataTableRow();
	/**
	 * Set up an empty row.
	 * @param useD The type of table to house.
	 */
	DataTableRow(DataTableDescription* useD);
	/**
	 * Copy another row.
	 * @param useD The type of table to house.
	 * @param toCopy The row to copy.
	 */
	DataTableRow(DataTableDescription* useD, DataTableRow* toCopy);
	/**Clean up.*/
	~DataTableRow();
	/**
	 * Get whether a given entry is present (is.na from R).
	 * @param entInd THe entry index.
	 * @param useD The data description: yes, this is prone to error, why do you ask?
	 * @return The entry.
	 */
	DataTableEntry getEntry(uintptr_t entInd, DataTableDescription* useD);
	/**
	 * Set an entry in the table.
	 * @param entInd THe entry index.
	 * @param useD The data description.
	 * @param toSet The new value.
	 */
	void setEntry(uintptr_t entInd, DataTableDescription* useD, DataTableEntry toSet);
	/**The packed data.*/
	char* packData;
};

/**Read from a data table.*/
class DataTableReader{
public:
	/**Set up.*/
	DataTableReader();
	/**ALlow subcalsses to tear down.*/
	virtual ~DataTableReader();
	/**
	 * Get the header of this table.
	 * @param toFill The place to put the description.
	 */
	void getDescription(DataTableDescription* toFill);
	/**
	 * Get the next entry.
	 * @return The next entry. 0 for end of file.
	 */
	virtual DataTableRow* read() = 0;
	/** Close anything this thing opened. */
	virtual void close() = 0;
	
	/**
	 * Get space for an entry.
	 * @return A new entry.
	 */
	virtual DataTableRow* allocate();
	/**
	 * Return an entry: can cut down on allocation.
	 * @param toRet The sequence to return.
	 */
	virtual void release(DataTableRow* toRet);
	/**A cache for entries.*/
	std::vector<DataTableRow*> cache;
	/**The description of the table.*/
	DataTableDescription tabDesc;
};

/**Random access to data tables.*/
class RandacDataTableReader : public DataTableReader{
public:
	/**Set up.*/
	RandacDataTableReader();
	/**
	 * Get the number of entries in the file.
	 * @return The number of entries.
	 */
	virtual uintmax_t getNumEntries() = 0;
	/**
	 * Read a specific entry.
	 * @param entInd The index.
	 * @return The sequence.
	 */
	virtual DataTableRow* getEntry(uintmax_t entInd) = 0;
};

/**Write rows to a data table.*/
class DataTableWriter{
public:
	/**Set up*/
	DataTableWriter();
	/**Allow subclasses to tear down.*/
	virtual ~DataTableWriter();
	
	/**
	 * Write out an entry.
	 * @param nextOut The next entry to write.
	 */
	virtual void write(DataTableRow* nextOut) = 0;
	/**
	 * Write out multiple entries.
	 * @param numWrite the number of entries to write.
	 * @param allOut The entries to write.
	 */
	virtual void write(uintptr_t numWrite, DataTableRow** allOut);
	/** Close anything this thing opened. */
	virtual void close() = 0;
	/**The description of the table.*/
	DataTableDescription tabDesc;
};

/**Mangle data.*/
class BinaryDataTableUniform : public ThreadTask{
public:
	void doIt();
	/**The action to do.*/
	int actDo;
	/**The number of things to manage.*/
	uintptr_t numManage;
	/**The data in a class.*/
	std::deque<DataTableRow*>::iterator bulkTTR;
	/**The data in a class.*/
	DataTableRow** bulkoTTR;
	/**The bytes to work over.*/
	char* packLoc;
	/**The size of a row.*/
	uintptr_t rowBytes;
};

/**Read from a binary file*/
class BinaryDataTableReader : public DataTableReader{
public:
	/**
	 * Read from stream.
	 * @param mainFrom The stream to read from.
	 */
	BinaryDataTableReader(InStream* mainFrom);
	/**
	 * Read from stream.
	 * @param mainFrom The stream to read from.
	 * @param numThread The number of tasks to spawn.
	 * @param mainPool The threads to use.
	 */
	BinaryDataTableReader(InStream* mainFrom, int numThread, ThreadPool* mainPool);
	/**ALlow subcalsses to tear down.*/
	~BinaryDataTableReader();
	DataTableRow* read();
	void close();
	/**The maximum number of threads to spin up.*/
	uintptr_t numThr;
	/**The pool to use, if any.*/
	ThreadPool* usePool;
	/**The thing to parse.*/
	InStream* theStr;
	/**Entries already loaded and waiting to return.*/
	std::deque<DataTableRow*> waitEnts;
	/**Active tasks.*/
	std::vector<BinaryDataTableUniform> liveTask;
	/**Allocated task uniform table.*/
	std::vector<BinaryDataTableUniform*> passTask;
	/**Size of the buffer.*/
	uintptr_t bufferSize;
	/**Staging buffer.*/
	char* stageBuff;
};

/**Random access to a binary data table.*/
class RandacBinaryDataTableReader : public RandacDataTableReader{
public:
	/**
	 * Read from stream.
	 * @param mainFrom The stream to read from.
	 */
	RandacBinaryDataTableReader(RandaccInStream* mainFrom);
	/**ALlow subcalsses to tear down.*/
	~RandacBinaryDataTableReader();
	DataTableRow* read();
	void close();
	uintmax_t getNumEntries();
	DataTableRow* getEntry(uintmax_t entInd);
	/**The thing to parse.*/
	RandaccInStream* theStr;
	/**The number of entries.*/
	uintmax_t numEnts;
	/**The next entry to spit out.*/
	uintmax_t nextSpit;
	/**The location of the first entry.*/
	uintmax_t baseLoc;
};

/**Write rows to a data table.*/
class BinaryDataTableWriter : public DataTableWriter{
public:
	/**
	 * Set up
	 * @param forDat The data to manage.
	 * @param mainFrom The place to write.
	 */
	BinaryDataTableWriter(DataTableDescription* forDat, OutStream* mainFrom);
	/**
	 * Set up
	 * @param forDat The data to manage.
	 * @param mainFrom The place to write.
	 * @param numThread The number of tasks to spawn.
	 * @param mainPool The threads to use.
	 */
	BinaryDataTableWriter(DataTableDescription* forDat, OutStream* mainFrom, int numThread, ThreadPool* mainPool);
	/**Allow subclasses to tear down.*/
	~BinaryDataTableWriter();
	void write(DataTableRow* nextOut);
	void write(uintptr_t numWrite, DataTableRow** allOut);
	void close();
	/**The maximum number of threads to spin up.*/
	uintptr_t numThr;
	/**The pool to use, if any.*/
	ThreadPool* usePool;
	/**The thing to parse.*/
	OutStream* theStr;
	/**Active tasks.*/
	std::vector<BinaryDataTableUniform> liveTask;
	/**Allocated task uniform table.*/
	std::vector<BinaryDataTableUniform*> passTask;
	/**Size of the buffer.*/
	uintptr_t bufferSize;
	/**Staging buffer.*/
	char* stageBuff;
};

//TODO block compression

/**
 * Get the file extensions ExtensionDataTableReader/Writer will recognize.
 * @param modeWrite WHether we will be writing.
 * @param toFill The place to put the extensions.
 */
void getBinaryTableExtensions(int modeWrite, std::set<std::string>* toFill);

/**
 * Get the file extensions ExtensionDataTableReader/Writer will recognize.
 * @param toFill The place to put the extensions.
 */
void getRandacBinaryTableExtensions(std::set<std::string>* toFill);

/**Open a file based on a file name.*/
class ExtensionDataTableReader : public DataTableReader{
public:
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 */
	ExtensionDataTableReader(const char* fileName);
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 * @param numThread The number of threads to use for the base reader.
	 * @param mainPool The threads to use for reading.
	 */
	ExtensionDataTableReader(const char* fileName, int numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~ExtensionDataTableReader();
	
	DataTableRow* read();
	void close();
	DataTableRow* allocate();
	void release(DataTableRow* toRet);
	
	/**The base streams.*/
	std::vector<InStream*> baseStrs;
	/**The wrapped reader.*/
	DataTableReader* wrapStr;
};

/**Open a file based on a file name.*/
class ExtensionRandacDataTableReader : public RandacDataTableReader{
public:
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 */
	ExtensionRandacDataTableReader(const char* fileName);
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 * @param numThread The number of threads to use for the base reader.
	 * @param mainPool The threads to use for reading.
	 */
	ExtensionRandacDataTableReader(const char* fileName, int numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~ExtensionRandacDataTableReader();
	
	DataTableRow* read();
	void close();
	DataTableRow* allocate();
	void release(DataTableRow* toRet);
	uintmax_t getNumEntries();
	DataTableRow* getEntry(uintmax_t entInd);
	
	/**The base streams.*/
	std::vector<InStream*> baseStrs;
	/**The wrapped reader.*/
	RandacDataTableReader* wrapStr;
};

/**Open a file based on a file name.*/
class ExtensionDataTableWriter : public DataTableWriter{
public:
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 */
	ExtensionDataTableWriter(DataTableDescription* forDat, const char* fileName);
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 * @param numThread The number of threads to use for the base reader.
	 * @param mainPool The threads to use for reading.
	 */
	ExtensionDataTableWriter(DataTableDescription* forDat, const char* fileName, int numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~ExtensionDataTableWriter();
	
	void write(DataTableRow* nextOut);
	void write(uintptr_t numWrite, DataTableRow** allOut);
	void close();
	
	/**The base streams.*/
	std::vector<OutStream*> baseStrs;
	/**The wrapped writer.*/
	DataTableWriter* wrapStr;
};

/**Cache reads from a data table.*/
class DataTableCache : public RandacDataTableReader{
public:
	/**
	 * Set up.
	 * @param maxCacheSize The maximum size of the cache.
	 * @param baseArray The underlying array to cache.
	 */
	DataTableCache(uintptr_t maxCacheSize, RandacDataTableReader* baseArray);
	/**Clean up.*/
	~DataTableCache();
	
	DataTableRow* read();
	void close();
	DataTableRow* allocate();
	void release(DataTableRow* toRet);
	uintmax_t getNumEntries();
	DataTableRow* getEntry(uintmax_t entInd);
	
	/**The maximum number of cachelines to store.*/
	uintptr_t maxCache;
	/**The wrapped source.*/
	RandacDataTableReader* baseArr;
	
	/**Lock access to the DataTableRow cache.*/
	OSMutex allocLock;
	/**Save the number of entries.*/
	uintmax_t saveNumEnt;
	/**Lock access to the cache and file.*/
	ReadWriteLock cacheLock;
	/**The cached sequences.*/
	std::map<uintmax_t,DataTableRow*> saveSeqs;
};

}

#endif
