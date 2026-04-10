#ifndef WHODUN_GEN_SEQUENCE_H
#define WHODUN_GEN_SEQUENCE_H 1

#include <stdint.h>
#include <signal.h>

#include "whodun_parse.h"
#include "whodun_thread.h"
#include "whodun_string.h"
#include "whodun_compress.h"

namespace whodun {

/**
 * Turn an ascii phred score to a log (10) probability.
 * @param toConv The phred score to convert.
 * @return The log10 probability.
 */
double phredToLog10Prob(char toConv);

/**
 * Turn a log (10) probability to an ascii phred score.
 * @param toConv The log10 probability.
 * @return The phred score.
 */
char phredLog10ToScore(double toConv);

/**Useful utilities for genetic sequences.*/
class GeneticSequenceUtilities{
public:
	/**Set up*/
	GeneticSequenceUtilities();
	/**Tear down.*/
	virtual ~GeneticSequenceUtilities();
	
	/**
	 * Convert multiple.
	 * @param numConv The number of bases to convert.
	 * @param toConv The scores.
	 * @param toStore The place to put the results.
	 */
	virtual void phredsToLog10Prob(uintptr_t numConv, char* toConv, double* toStore);
	
	/**
	 * Convert multiple.
	 * @param numConv The number of bases to convert.
	 * @param toConv The scores.
	 * @param toStore The place to put the results.
	 */
	virtual void phredsLog10ToScore(uintptr_t numConv, double* toConv, char* toStore);
	
	/**
	 * Reverse complement a sequence.
	 * @param numConv The number of bases.
	 * @param toRev The bases.
	 * @param toRevQ The qualities (if present).
	 */
	virtual void reverseComplement(uintptr_t numConv, char* toRev, double* toRevQ);
};

/**Actually do the stuff in the utilities.*/
class MTGeneticSequenceUtilitiesUniform : public ThreadTask{
public:
	void doIt();
	/**The thing to do.*/
	int forPhase;
	/**The number of things to convert.*/
	uintptr_t numConv;
	/**The relevant double argument.*/
	double* anyDouble;
	/**The relevant bytes.*/
	char* anyBytes;
	/**The relevant double argument.*/
	double* anyDoubleB;
	/**The relevant bytes.*/
	char* anyBytesB;
	/**The thread id.*/
	uintptr_t threadID;
};

/**Multithreded version of the same.*/
class MTGeneticSequenceUtilities : public GeneticSequenceUtilities{
public:
	/**
	 * Set up
	 * @param numThread The number of tasks to spawn.
	 * @param mainPool The threads to use.
	 */
	MTGeneticSequenceUtilities(int numThread, ThreadPool* mainPool);
	/**Tear down.*/
	virtual ~MTGeneticSequenceUtilities();
	virtual void phredsToLog10Prob(uintptr_t numConv, char* toConv, double* toStore);
	virtual void phredsLog10ToScore(uintptr_t numConv, double* toConv, char* toStore);
	virtual void reverseComplement(uintptr_t numConv, char* toRev, double* toRevQ);
	
	/**
	 * Start a bulk op.
	 * @param numConv The number of bases to convert.
	 * @param toConv The scores.
	 * @param toStore The place to put the results.
	 * @param useUni THe uniforms to use.
	 */
	void startPhredsToLog10Prob(uintptr_t numConv, char* toConv, double* toStore, MTGeneticSequenceUtilitiesUniform* useUni);
	/**
	 * Start a bulk op.
	 * @param numConv The number of bases to convert.
	 * @param toConv The scores.
	 * @param toStore The place to put the results.
	 * @param useUni THe uniforms to use.
	 */
	void startPhredsLog10ToScore(uintptr_t numConv, double* toConv, char* toStore, MTGeneticSequenceUtilitiesUniform* useUni);
	/**
	 * Start a bulk op.
	 * @param numConv The number of bases.
	 * @param toRev The bases.
	 * @param toRevQ The qualities (if present).
	 * @param useUni THe uniforms to use.
	 */
	void startReverseComplement(uintptr_t numConv, char* toRev, double* toRevQ, MTGeneticSequenceUtilitiesUniform* useUni);
	
	/**
	 * Wait on a task to finish.
	 * @param useUni The uniforms used.
	 */
	void wait(MTGeneticSequenceUtilitiesUniform* useUni);
	
	/**The threads to use.*/
	ThreadPool* usePool;
	/**Storage for the one-shot methods.*/
	std::vector<MTGeneticSequenceUtilitiesUniform> helpUnis;
};

/**A loaded sequence.*/
class SequenceEntry{
public:
	/**Set up an empty entry.*/
	SequenceEntry();
	/**Clean up*/
	~SequenceEntry();
	/**The length of the name.*/
	uintptr_t nameLen;
	/**The shortened name length, if any.*/
	uintptr_t shortNameLen;
	/**The name.*/
	char* name;
	/**The sequence length.*/
	uintptr_t seqLen;
	/**The sequence.*/
	char* seq;
	/**Whether the sequence has a quality.*/
	int haveQual;
	/**The quality, if any.*/
	double* qual;
	/**Ammount of space allocated for the name.*/
	uintptr_t allocName;
	/**Ammount of space allocated for the sequence.*/
	uintptr_t allocSeq;
	/**Ammount of space allocated for the quality.*/
	uintptr_t allocQual;
	/**
	 * Try and preallocate: will clobber data.
	 * @param needName The bytes needed for a name.
	 * @param needSeq The bytes needed for sequence.
	 * @param needQual Whether quality should also be checked.
	 */
	void preallocate(uintptr_t needName, uintptr_t needSeq, int needQual);
};

/**Read sequences from a file.*/
class SequenceReader{
public:
	/**Set up.*/
	SequenceReader();
	/**ALlow subcalsses to tear down.*/
	virtual ~SequenceReader();
	
	/**
	 * Get the next sequence.
	 * @return The next sequence. 0 for end of file.
	 */
	virtual SequenceEntry* read() = 0;
	/** Close anything this thing opened. */
	virtual void close() = 0;
	
	/**
	 * Get space for an entry.
	 * @return A new entry.
	 */
	virtual SequenceEntry* allocate();
	/**
	 * Return an entry: can cut down on allocation.
	 * @param toRet The sequence to return.
	 */
	virtual void release(SequenceEntry* toRet);
	/**A cache for entries.*/
	std::vector<SequenceEntry*> cache;
};

/**Write sequences to a file.*/
class SequenceWriter{
public:
	/**Set up*/
	SequenceWriter();
	/**Allow subclasses to tear down.*/
	virtual ~SequenceWriter();
	
	/**
	 * Write out an entry.
	 * @param nextOut The next entry to write.
	 */
	virtual void write(SequenceEntry* nextOut) = 0;
	/**
	 * Write out multiple entries.
	 * @param numWrite the number of entries to write.
	 * @param allOut The entries to write.
	 */
	virtual void write(uintptr_t numWrite, SequenceEntry** allOut);
	/** Close anything this thing opened. */
	virtual void close() = 0;
};

/**Random access to a sequence file.*/
class RandacSequenceReader : public SequenceReader{
public:
	/**Set up.*/
	RandacSequenceReader();
	/**
	 * Get the number of entries in the file.
	 * @return The number of entries.
	 */
	virtual uintmax_t getNumEntries() = 0;
	/**
	 * Get the length of an entry.
	 * @param entInd The index.
	 * @return The number of characters in the entry.
	 */
	virtual uintptr_t getEntryLength(uintmax_t entInd) = 0;
	/**
	 * Read a piece of an entry.
	 * @param entInd The index.
	 * @param fromBase The base to start from.
	 * @param toBase The base to end at.
	 * @return The sequence.
	 */
	virtual SequenceEntry* getEntry(uintmax_t endInd, uintptr_t fromBase, uintptr_t toBase) = 0;
	/**
	 * Read a specific entry.
	 * @param entInd The index.
	 * @return The sequence.
	 */
	virtual SequenceEntry* getEntry(uintmax_t endInd);
};

/**Peform the tasks for a sequence reader.*/
class FastAQSequenceReaderUniform : public ThreadTask{
public:
	/**Set up.*/
	FastAQSequenceReaderUniform();
	/**Tear down.*/
	~FastAQSequenceReaderUniform();
	void doIt();
	/**What to do.*/
	int taskID;
	/**The length of the source string.*/
	uintptr_t srcLen;
	/**The source string.*/
	char* srcLoc;
	/**The entry being written to.*/
	SequenceEntry* tgtEnt;
	/**The offset in the target.*/
	uintptr_t tgtOffset;
	/**Do the phred score stuff.*/
	GeneticSequenceUtilities phredMeDo;
	/**The thread index.*/
	uintptr_t threadID;
};

/**Read sequences from a fastA / fastQ file.*/
class FastAQSequenceReader : public SequenceReader{
public:
	/**
	 * Set up a parser for the thing.
	 * @param mainFrom The thing to parse.
	 */
	FastAQSequenceReader(InStream* mainFrom);
	/**
	 * Set up a parser for the thing.
	 * @param mainFrom The thing to parse.
	 * @param numThread The number of tasks to spawn.
	 * @param mainPool The threads to use.
	 */
	FastAQSequenceReader(InStream* mainFrom, int numThread, ThreadPool* mainPool);
	/**Clean up*/
	~FastAQSequenceReader();
	SequenceEntry* read();
	void close();
	/**The thing to parse.*/
	InStream* theStr;
	/**Split on newlines.*/
	CharacterSplitter* newlineSpl;
	/**Read newlines.*/
	SplitterInStream* newlineStr;
	/**Entries already loaded and waiting to return.*/
	std::deque<SequenceEntry*> waitEnts;
	/**The mode the last build left off at.*/
	uintptr_t curMode;
	/**The entry currently being built.*/
	SequenceEntry* curBuild;
	/**The maximum number of threads to spin up.*/
	uintptr_t numThr;
	/**The pool to use, if any.*/
	ThreadPool* usePool;
	/**Active tasks.*/
	std::vector<FastAQSequenceReaderUniform*> liveTask;
	/**Allocated tasks.*/
	std::vector<FastAQSequenceReaderUniform*> cacheTask;
	/**Help with memcpy.*/
	MultithreadedStringH* useStringOp;
	/**
	 * Allocate a uniform.
	 * @return The allocated uniform.
	 */
	FastAQSequenceReaderUniform* allocUni();
	/**
	 * Pack a name into a sequence entry.
	 * @param packTo The place to put the name in.
	 * @param namS The first character of the name.
	 * @param namE The last character of the name.
	 */
	void packName(SequenceEntry* packTo, char* namS, char* namE);
	/**
	 * Start packing a sequence.
	 * @param packTo The place to put the sequence in.
	 * @param seqS The first character of the sequence.
	 * @param seqE The last character of the sequence.
	 * @param useOff The offset into the sequence.
	 */
	void packSeq(SequenceEntry* packTo, char* seqS, char* seqE, uintptr_t useOff);
	/**
	 * Start packing a quality.
	 * @param packTo The place to put the quality in.
	 * @param seqS The first character of the quality.
	 * @param seqE The last character of the quality.
	 * @param useOff The offset into the quality.
	 */
	void packQual(SequenceEntry* packTo, char* qualS, char* qualE, uintptr_t useOff);
};

/**Peform the tasks for a sequence writer.*/
class FastAQSequenceWriterUniform : public ThreadTask{
public:
	/**Set up.*/
	FastAQSequenceWriterUniform();
	/**Tear down.*/
	~FastAQSequenceWriterUniform();
	void doIt();
	/**What to do.*/
	int taskID;
	/**The amount of data to handle.*/
	uintptr_t dstLen;
	/**The target.*/
	char* dstLoc;
	/**The offset in the source.*/
	uintptr_t srcOffset;
	/**The entry being written to.*/
	SequenceEntry* srcEnt;
	/**Do the phred score stuff.*/
	GeneticSequenceUtilities phredMeDo;
	/**The thread index.*/
	uintptr_t threadID;
};

/**Write fastq data.*/
class FastAQSequenceWriter : public SequenceWriter{
public:
	/**
	 * Set up a parser for the thing.
	 * @param mainTo The thing to write to.
	 */
	FastAQSequenceWriter(OutStream* mainTo);
	/**
	 * Set up a parser for the thing.
	 * @param mainTo The thing to write to.
	 * @param numThread The number of tasks to spawn.
	 * @param mainPool The threads to use.
	 */
	FastAQSequenceWriter(OutStream* mainTo, int numThread, ThreadPool* mainPool);
	/**Tear down.*/
	~FastAQSequenceWriter();
	void write(SequenceEntry* nextOut);
	void write(uintptr_t numWrite, SequenceEntry** allOut);
	void close();
	/**The thing to actually write to.*/
	OutStream* theStr;
	/**Storage for packed output.*/
	uintptr_t storeAlloc;
	/**Storage for packed output.*/
	char* storeHelp;
	/**The maximum number of threads to spin up.*/
	uintptr_t numThr;
	/**The pool to use, if any.*/
	ThreadPool* usePool;
	/**Active tasks.*/
	std::vector<FastAQSequenceWriterUniform*> liveTask;
	/**Allocated tasks.*/
	std::vector<FastAQSequenceWriterUniform*> cacheTask;
	/**
	 * Allocate a uniform.
	 * @return The allocated uniform.
	 */
	FastAQSequenceWriterUniform* allocUni();
	/**Do the phred score stuff.*/
	GeneticSequenceUtilities phredMeDo;
};

/**Do multithreaded gail things.*/
class GailSequenceUniform : public ThreadTask{
public:
	void doIt();
	/**The thing to do.*/
	int taskStage;
	/**The number of values to handle.*/
	uintptr_t numVal;
	/**Input bytes*/
	char* boperA;
	/**Output bytes*/
	char* boperB;
	/**Doubles to work with/over.*/
	double* doper;
	/**The id of the running thread.*/
	uintptr_t threadID;
};

/**Quick sequential access to a gail file.*/
class GailSequenceReader : public SequenceReader{
public:
	/**
	 * Open a gail file.
	 * @param fileName The name of the annotation file.
	 * @param numThread The number of threads to use.
	 * @param mainPool The threads to use.
	 */
	GailSequenceReader(const char* fileName, int numThread, ThreadPool* mainPool);
	/**Clean up*/
	~GailSequenceReader();
	SequenceEntry* read();
	void close();
	/**The next index to report.*/
	uintmax_t focusInd;
	/**The number of entries in the thing.*/
	uintmax_t numEntry;
	/**Entries already loaded and waiting to return.*/
	std::deque<SequenceEntry*> waitEnts;
	/**The maximum number of threads to spin up.*/
	uintptr_t numThr;
	/**The pool to use, if any.*/
	ThreadPool* usePool;
	/**Active tasks.*/
	std::vector<GailSequenceUniform*> liveTask;
	/**Allocated tasks.*/
	std::vector<GailSequenceUniform*> cacheTask;
	/**Allocated storage for loading.*/
	uintptr_t loadAlloc;
	/**Storage for loading.*/
	char* loadBuffer;
	/**Storage for loaded annotations.*/
	char* annotBuffer;
	/**The index data.*/
	InStream* indStr;
	/**The name data.*/
	InStream* namStr;
	/**The sequence data.*/
	InStream* seqStr;
	/**The quality data.*/
	InStream* qualStr;
};

/**Random access to a gail file.*/
class RandacGailSequenceReader : public RandacSequenceReader{
public:
	/**
	 * Open a gail file.
	 * @param fileName The name of the annotation file.
	 */
	RandacGailSequenceReader(const char* fileName);
	/**Clean up*/
	~RandacGailSequenceReader();
	SequenceEntry* read();
	void close();
	/**
	 * Get the number of entries in the file.
	 * @return The number of entries.
	 */
	uintmax_t getNumEntries();
	/**
	 * Get the length of an entry.
	 * @param entInd The index.
	 * @return The number of characters in the entry.
	 */
	uintptr_t getEntryLength(uintmax_t entInd);
	/**
	 * Read a piece of an entry.
	 * @param entInd The index.
	 * @param fromBase The base to start from.
	 * @param toBase The base to end at.
	 * @return The sequence.
	 */
	SequenceEntry* getEntry(uintmax_t endInd, uintptr_t fromBase, uintptr_t toBase);
	/**Whether the streams need a seek to happen (i.e. a random access function was called).*/
	int needSeek;
	/**The next index to report.*/
	uintmax_t focusInd;
	/**The number of entries in the thing.*/
	uintmax_t numEntry;
	/**Allocated storage for quality data.*/
	uintptr_t qualityAlloc;
	/**Storage for big endian quality data.*/
	char* qualityBuffer;
	/**The index data.*/
	FileInStream* indStr;
	/**The name data.*/
	BlockCompInStream* namStr;
	/**The sequence data.*/
	BlockCompInStream* seqStr;
	/**The quality data.*/
	BlockCompInStream* qualStr;
};

/**Write a gail file.*/
class GailSequenceWriter : public SequenceWriter{
public:
	/**
	 * Open a gail file.
	 * @param fileName The name of the annotation file.
	 */
	GailSequenceWriter(const char* fileName);
	/**
	 * Open a gail file.
	 * @param fileName The name of the annotation file.
	 * @param numThread The number of threads to use.
	 * @param mainPool The threads to use.
	 */
	GailSequenceWriter(const char* fileName, int numThread, ThreadPool* mainPool);
	/**Clean up*/
	~GailSequenceWriter();
	void write(SequenceEntry* nextOut);
	void write(uintptr_t numWrite, SequenceEntry** allOut);
	void close();
	/**Allocated storage for quality data.*/
	uintptr_t qualityAlloc;
	/**Storage for big endian quality data.*/
	char* qualityBuffer;
	/**The total written name data.*/
	uintmax_t totNameData;
	/**The total written sequence data.*/
	uintmax_t totSeqsData;
	/**The total written quality data.*/
	uintmax_t totQualData;
	/**The maximum number of threads to spin up.*/
	uintptr_t numThr;
	/**The pool to use.*/
	ThreadPool* usePool;
	/**Active tasks.*/
	std::vector<GailSequenceUniform*> liveTask;
	/**Allocated tasks.*/
	std::vector<GailSequenceUniform*> cacheTask;
	/**The index data.*/
	OutStream* indStr;
	/**The name data.*/
	OutStream* namStr;
	/**The sequence data.*/
	OutStream* seqStr;
	/**The quality data.*/
	OutStream* qualStr;
};

/**
 * Get the file extensions ExtensionSequenceReader/Writer will recognize.
 * @param modeWrite WHether we will be writing.
 * @param toFill The place to put the extensions.
 */
void getGeneticSequenceExtensions(int modeWrite, std::set<std::string>* toFill);

/**
 * Get the file extensions ExtensionRandacSequenceReader will recognize.
 * @param modeWrite WHether we will be writing.
 * @param toFill The place to put the extensions.
 */
void getRandacGeneticSequenceExtensions(std::set<std::string>* toFill);

/**Open a file based on a file name.*/
class ExtensionSequenceReader : public SequenceReader{
public:
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 */
	ExtensionSequenceReader(const char* fileName);
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 * @param numThread The number of threads to use for the base reader.
	 * @param mainPool The threads to use for reading.
	 */
	ExtensionSequenceReader(const char* fileName, int numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~ExtensionSequenceReader();
	
	SequenceEntry* read();
	void close();
	SequenceEntry* allocate();
	void release(SequenceEntry* toRet);
	
	/**The base streams.*/
	std::vector<InStream*> baseStrs;
	/**The wrapped reader.*/
	SequenceReader* wrapStr;
};

/**Open a file based on a file name.*/
class ExtensionRandacSequenceReader : public RandacSequenceReader{
public:
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 */
	ExtensionRandacSequenceReader(const char* fileName);
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 * @param numThread The number of threads to use for the base reader.
	 * @param mainPool The threads to use for reading.
	 */
	ExtensionRandacSequenceReader(const char* fileName, int numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~ExtensionRandacSequenceReader();
	
	SequenceEntry* read();
	void close();
	SequenceEntry* allocate();
	void release(SequenceEntry* toRet);
	uintmax_t getNumEntries();
	uintptr_t getEntryLength(uintmax_t entInd);
	SequenceEntry* getEntry(uintmax_t endInd, uintptr_t fromBase, uintptr_t toBase);
	SequenceEntry* getEntry(uintmax_t endInd);
	
	/**The base streams.*/
	std::vector<InStream*> baseStrs;
	/**The wrapped reader.*/
	RandacSequenceReader* wrapStr;
};

/**Open a file based on a file name.*/
class ExtensionSequenceWriter : public SequenceWriter{
public:
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 */
	ExtensionSequenceWriter(const char* fileName);
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 * @param numThread The number of threads to use for the base reader.
	 * @param mainPool The threads to use for reading.
	 */
	ExtensionSequenceWriter(const char* fileName, int numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~ExtensionSequenceWriter();
	
	void write(SequenceEntry* nextOut);
	void write(uintptr_t numWrite, SequenceEntry** allOut);
	void close();
	
	/**The base streams.*/
	std::vector<OutStream*> baseStrs;
	/**The wrapped writer.*/
	SequenceWriter* wrapStr;
};

/**Cache random access to a sequence file.*/
class RandacSequenceCache : public RandacSequenceReader{
public:
	/**
	 * Set up.
	 * @param maxCacheSize The maximum size of the cache.
	 * @param cacheChunk The cacheline size.
	 * @param useSource The real source.
	 */
	RandacSequenceCache(uintptr_t maxCacheSize, uintptr_t cacheChunk, RandacSequenceReader* useSource);
	/**Clean up.*/
	~RandacSequenceCache();
	
	SequenceEntry* read();
	void close();
	SequenceEntry* allocate();
	void release(SequenceEntry* toRet);
	uintmax_t getNumEntries();
	uintptr_t getEntryLength(uintmax_t entInd);
	SequenceEntry* getEntry(uintmax_t endInd, uintptr_t fromBase, uintptr_t toBase);
	
	/**The maximum number of cachelines to store.*/
	uintptr_t maxCache;
	/**The number of bases to save at a time.*/
	uintptr_t cacheLine;
	/**The wrapped source.*/
	RandacSequenceReader* useSrc;
	
	/**Lock access to the SequenceEntry cache.*/
	OSMutex allocLock;
	/**Save the number of entries.*/
	uintmax_t saveNumEnt;
	/**Lock access to the cache and file.*/
	ReadWriteLock cacheLock;
	/**Save the lengths of the sequences.*/
	std::map<uintmax_t,uintptr_t> saveLens;
	/**The cached sequences.*/
	std::map< std::pair<uintmax_t,uintptr_t>, SequenceEntry* > saveSeqs;
};

/**Using sequences as a source of strings.*/
class SequenceStringSource : public BulkStringSource{
public:
	/**
	 * Wrap a sequence reader for use in string libraries.
	 * @param useSource THe file to use.
	 */
	SequenceStringSource(RandacSequenceReader* useSource);
	/**Clean up.*/
	~SequenceStringSource();
	uintmax_t numStrings();
	uintmax_t stringLength(uintmax_t strInd);
	void getString(uintmax_t strInd, uintmax_t fromC, uintmax_t toC, char* saveC);
	/**The sequence source.*/
	RandacSequenceReader* useSrc;
};

}

#endif