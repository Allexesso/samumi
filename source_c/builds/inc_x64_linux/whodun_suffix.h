#ifndef WHODUN_SUFFIX_H
#define WHODUN_SUFFIX_H 1

#include <vector>
#include <stdint.h>

#include "whodun_sort.h"
#include "whodun_string.h"
#include "whodun_thread.h"

#define WHODUN_SUFFIX_ENTRY_SIZE 40
#define WHODUN_SUFFIX_ENTRY_OFF_STRING 0
#define WHODUN_SUFFIX_ENTRY_OFF_CHAR 8
#define WHODUN_SUFFIX_ENTRY_OFF_LCP 16
#define WHODUN_SUFFIX_ENTRY_OFF_RANK 24
#define WHODUN_SUFFIX_ENTRY_OFF_NEXT 32
#define WHODUN_OOM_SUFFIX_ENTRY_SIZE 24

/**For each byte of memory in an initial string, about how many bytes of memory will it take to store the suffix array.*/
#define WHODUN_SUFFIX_STORE_MEMORY_FACTOR 40
/**For each byte of memory in an initial string, about how many bytes of memory will it take to build the suffix array, including storage.*/
#define WHODUN_SUFFIX_BUILD_MEMORY_FACTOR 88

namespace whodun{

/**An entry in a suffix array.*/
typedef struct {
	/**The string index of this entry.*/
	uintmax_t stringInd;
	/**The character index of this entry.*/
	uintmax_t charInd;
	/**The longest common prefix between this and the next entry.*/
	uintmax_t lcpNext;
} SuffixArrayEntry;

/**A suffix array.*/
class SuffixArray{
public:
	/**Allow subclasses*/
	virtual ~SuffixArray();
	/**
	 * Get the number of entries in the suffix array.
	 * @return The number of entries.
	 */
	virtual uintmax_t numEntry() = 0;
	/**
	 * Get an entry from the suffix array.
	 * @param entInd The index to get.
	 * @param storeLoc The place to put the results.
	 */
	virtual void getEntry(uintmax_t entInd, SuffixArrayEntry* storeLoc) = 0;
};

/**The suffix array (and LCP) in memory.*/
class InMemorySuffixArray : public SuffixArray{
public:
	/**Set up an empty suffix array.*/
	InMemorySuffixArray();
	/**Clean up.*/
	~InMemorySuffixArray();
	uintmax_t numEntry();
	void getEntry(uintmax_t entInd, SuffixArrayEntry* storeLoc);
	
	/**The total number of characters.*/
	uintmax_t numEntries;
	/**The amount of space allocated for said storage.*/
	uintptr_t numAlloc;
	/**The suffix array storage: 40 bytes per character, big endian: string index, character position, lcp, rank, next rank*/
	char* allocBuff;
};

/**Do a chunk of work on constructing a suffix array.*/
class SuffixArrayUniform : public ThreadTask {
public:
	void doIt();
	/**The task to do.*/
	int phase;
	
	//phase 1 : set rank/next rank to characters
		/**The entries to fill*/
		char* p1SetLoc;
		/**The number of entries to fill*/
		uintptr_t p1NumFill;
		/**The strings to fill with*/
		SizePtrString* p1FillWith;
		/**The string index to start at.*/
		uintptr_t p1StartS;
		/**The character index to start at.*/
		uintptr_t p1StartC;
	//phase 2 : rerank internal, expect sort on rank
	//reuse phase1 stuff
		/**The maximum rank at the end.*/
		uintmax_t p2MaxRank;
	//phase 3 : rerank make consistent between threads
		/**The offset for this set*/
		uintmax_t p3Offset;
	//phase 4 : fill in inverse suffix array
		/**The offsets of each string.*/
		uintmax_t* p4StrOffset;
		/**The place to put the inverse suffix array.*/
		uintmax_t* p4InvFill;
		/**The start location of the entries.*/
		char* p4BaseLoc;
	//phase 5 : next rank, expect sort on string/character index
		/**The number of zeros at the end*/
		uintmax_t p5SkipLen;
		/**The strings: used for length.*/
		SizePtrString* p5OrigStrs;
	//phase 6 : build lcp
		/**The total number of characters.*/
		uintmax_t p6NumEntry;
	//phase 7 : memcpy
		/**The number of bytes to copy.*/
		uintptr_t p7NumCopy;
		/**The source.*/
		char* p7Source;
		/**The target.*/
		char* p7Target;
	//phase 8 : offset
		/**The number of things to offset.*/
		uintptr_t p8NumOffset;
		/**The offset to use.*/
		uintmax_t p8Offset;
		/**The entries to offset.*/
		char* p8Entries;
	//phase 9 : merge
		/**The full number of things in A.*/
		uintptr_t p9FullNumA;
		/**The things in A.*/
		char* p9EntsA;
		/**The full number of things in B.*/
		uintptr_t p9FullNumB;
		/**The things in B.*/
		char* p9EntsB;
		/**The low quantile.*/
		uintptr_t p9QuantileLow;
		/**The high quantile.*/
		uintptr_t p9QuantileHigh;
		/**The place to put the merged sequence.*/
		char* p9EntsDump;
		/**The source of strings.*/
		BulkStringSource* p9Compare;
	//phase 10 : figure LCP without guidance
		/**The place to set the previous LCP.*/
		char* p10PrevSet;
	//phase 11 : pack to remove rank/nextrank
		/**The number of entries to pack.*/
		uintptr_t p11NumPack;
		/**The entries to pack.*/
		char* p11PackSrc;
		/**The place to pack to.*/
		char* p11PackTgt;
		/**The offset to add to string index.*/
		uintmax_t p11Offset;
	//TODO
};

/**Build a suffix array in memory.*/
class InMemorySuffixArrayBuilder {
public:
	/**Set up an empty suffix array.*/
	InMemorySuffixArrayBuilder();
	/**
	 * Set up a suffix array ready to use multithreading.
	 * @param numThr The number of threads to use.
	 * @param mainPool The pool to use.
	 */
	InMemorySuffixArrayBuilder(uintptr_t numThr, ThreadPool* mainPool);
	/**Clean up*/
	~InMemorySuffixArrayBuilder();
	/**
	 * Build a suffix array on the given string.
	 * @param buildOn The string to build on.
	 * @param toStore The place to put the array.
	 */
	void build(SizePtrString buildOn, InMemorySuffixArray* toStore);
	/**
	 * Build a suffix array on multiple strings.
	 * @param numOn The number of strings.
	 * @param buildOn The strings to build on.
	 * @param toStore The place to put the array.
	 */
	void build(uintptr_t numOn, SizePtrString** buildOn, InMemorySuffixArray* toStore);
	/**
	 * Build a suffix array on multiple strings.
	 * @param numOn The number of strings.
	 * @param buildOn The strings to build on.
	 * @param toStore The place to put the array.
	 */
	void build(uintptr_t numOn, SizePtrString* buildOn, InMemorySuffixArray* toStore);
	
	/**The number of threads to spin up.*/
	uintptr_t numThread;
	/**The pool to use.*/
	ThreadPool* usePool;
	/**Sort on rank/next rank.*/
	SortOptions soptsRank;
	/**Sort on rank/next rank.*/
	SortOptions soptsInd;
	/**The sorting method to use.*/
	InMemoryMergesort sortMethod;
	/**Save the uniforms.*/
	std::vector<SuffixArrayUniform> threadUnis;
	/**Pass uniforms.*/
	std::vector<ThreadTask*> passUnis;
	/**The original strings.*/
	std::vector<SizePtrString> allStrings;
	/**The zero index of each string.*/
	std::vector<uintmax_t> allStringC0;
	/**The allocated space for the inverse.*/
	uintptr_t allocInverse;
	/**The allocated space for the inverse.*/
	uintmax_t* inverseBuff;
	/**Whether each thread has an increment with the previous thread.*/
	std::vector<bool> rankChunkBreak;
	/**
	 * Check that the suffix array being built has been sorted to k bases.
	 * @param kCheck The number of bases to check.
	 * @param numOn The number of strings.
	 * @param buildOn The strings to build on.
	 * @param toStore The suffix array in question.
	 */
	void debugCheckSortedToK(uintptr_t kCheck, uintptr_t numOn, SizePtrString* buildOn, InMemorySuffixArray* toStore);
};

/**A random access interface to an out-of-memory suffix array.*/
class ExternalSuffixArray : public SuffixArray{
public:
	/**
	 * Open a suffix array.
	 * @param baseFile The base data file name.
	 * @param blockFile The file name of the block file.
	 */
	ExternalSuffixArray(const char* baseFile, const char* blockFile);
	/**Clean up.*/
	~ExternalSuffixArray();
	uintmax_t numEntry();
	void getEntry(uintmax_t entInd, SuffixArrayEntry* storeLoc);
	/**The number of entries.*/
	uintmax_t numEntries;
	/**The next entry.*/
	uintmax_t nextEntry;
	/**The base stream.*/
	RandaccInStream* myStr;
};

/**Options for an external suffix merge.*/
typedef struct{
	/**The base file of the first array.*/
	const char* baseFA;
	/**The block file of the first array.*/
	const char* blockFA;
	/**The offset for string indices in the first array.*/
	uintmax_t offsetA;
	/**The base file of the second array.*/
	const char* baseFB;
	/**The block file of the second array.*/
	const char* blockFB;
	/**The offset for string indices in the second array.*/
	uintmax_t offsetB;
	/**The base file to write to.*/
	const char* baseOut;
	/**The block file to write to.*/
	const char* blockOut;
} ExternalSuffixMergeOptions;

/**Merge two suffix arrays.*/
class ExternalSuffixArrayMerger{
public:
	/**
	 * Create a merger.
	 * @param maxBytes The maximum number of bytes to use for merging.
	 */
	ExternalSuffixArrayMerger(uintptr_t maxBytes);
	/**
	 * Create a merger.
	 * @param maxBytes The maximum number of bytes to use for merging.
	 * @param numThr The number of threads to use.
	 * @param mainPool The pool to use.
	 */
	ExternalSuffixArrayMerger(uintptr_t maxBytes, uintptr_t numThr, ThreadPool* mainPool);
	/**Clean up.*/
	~ExternalSuffixArrayMerger();
	
	/**
	 * Merge two suffix arrays.
	 * @param opts The options.
	 * @param useCompare The string source.
	 */
	void mergeArrays(ExternalSuffixMergeOptions* opts, BulkStringSource* useCompare);
	
	/**The number of threads to spin up.*/
	uintptr_t numThread;
	/**The pool to use.*/
	ThreadPool* usePool;
	/**Save the uniforms.*/
	std::vector<SuffixArrayUniform> threadUnis;
	/**Pass uniforms.*/
	std::vector<ThreadTask*> passUnis;
	/**The maximum number to load per bank.*/
	uintptr_t maxLoadBank;
	/**The place to load from array A.*/
	char* loadBankA;
	/**The place to load from array B.*/
	char* loadBankB;
	/**The place to store merged banks.*/
	char* mergeBank;
};

/**Build a big suffix array.*/
class ExternalSuffixArrayBuilder{
public:
	/**
	 * Set up a suffix array builder.
	 * @param workingFolder The name of the working folder.
	 * @param useCompare The comparison method to use.
	 */
	ExternalSuffixArrayBuilder(const char* workingFolder, BulkStringSource* useCompare);
	/**
	 * Set up a suffix array builder.
	 * @param workingFolder The name of the working folder.
	 * @param useCompare The comparison method to use.
	 * @param numThr The number of threads to use.
	 * @param mainPool The pool to use.
	 */
	ExternalSuffixArrayBuilder(const char* workingFolder, BulkStringSource* useCompare, uintptr_t numThr, ThreadPool* mainPool);
	/**Clean up*/
	~ExternalSuffixArrayBuilder();
	
	/**
	 * Add a suffix array to the set.
	 * @param numString The number of strings that array is for.
	 * @param toAdd The array to add. Will be mangled.
	 */
	void addSuffixArray(uintmax_t numString, InMemorySuffixArray* toAdd);
	
	/**
	 * Collect the suffix arrays.
	 * @param baseFile The file to pack them into.
	 * @param blockFile The block file for said thing.
	 */
	void collectSuffixArrays(const char* baseFile, const char* blockFile);
	
	/**The number of bytes to load at a time.*/
	uintptr_t maxByteLoad;
	/**Whether this made the temporary folder.*/
	int madeTemp;
	/**The folder to work in.*/
	std::string workFold;
	/**The number of threads to spin up.*/
	uintptr_t numThread;
	/**The pool to use.*/
	ThreadPool* usePool;
	/**The way to compare strings.*/
	BulkStringSource* useComp;
	/**The total number of strings in the thing.*/
	uintmax_t totNumString;
	/**Save the uniforms.*/
	std::vector<SuffixArrayUniform> threadUnis;
	/**Pass uniforms.*/
	std::vector<ThreadTask*> passUnis;
	/**Storage for information on temporary files.*/
	std::vector<std::string> allTempBase;
	/**Storage for information on temporary files.*/
	std::vector<std::string> allTempBlock;
	/**Storage for string offsets for temporary files.*/
	std::vector<uintmax_t> allTempOffset;
	/**A place to cram data.*/
	char* marshalBuff;
	
	/**The number of created temporary files.*/
	uintptr_t numTemps;
	/**
	 * Make a name for a new temporary.
	 * @param toFill The place to put the name.
	 */
	void newTempName(std::string* toFill);
};

/**Perform searches.*/
class SuffixArraySearch{
public:
	/**Make an uninitialized search.*/
	SuffixArraySearch();
	/**
	 * Start a search over an entire array.
	 * @param inArray The array to search through.
	 * @param withStrings The strings corresponding to that array.
	 */
	SuffixArraySearch(SuffixArray* inArray, BulkStringSource* withStrings);
	
	/**
	 * Limit the range to suffixes matching the given string.
	 * @param toStr The string to extend matches with.
	 */
	void limitMatch(SizePtrString toStr);
	
	/**The array to search through.*/
	SuffixArray* inArr;
	/**The corresponding strings.*/
	BulkStringSource* withStrs;
	/**The number of characters matched so far.*/
	uintptr_t charInd;
	/**The index to start from.*/
	uintmax_t fromInd;
	/**The index to end at.*/
	uintmax_t toInd;
};

/**
 * Caching searches may be useful.
 * @param searchA The first search.
 * @param searchB The second search.
 * @return Whether to consider searchA before searchB.
 */
bool operator<(const SuffixArraySearch& searchA, const SuffixArraySearch& searchB);

/**
 * Compare suffix array entries, first by rank/next-rank, then by string/character (to impose a total order).
 * @param unif Ignored
 * @param itemA The first entry.
 * @param itemB The second entry.
 */
bool internalSuffixRankCompare(void* unif, void* itemA, void* itemB);

/**Cache results from a suffix array.*/
class SuffixArrayCache : public SuffixArray{
public:
	/**
	 * Open a suffix array.
	 * @param maxCacheSize The maximum size of the cache.
	 * @param baseArray The underlying array to cache.
	 */
	SuffixArrayCache(uintptr_t maxCacheSize, SuffixArray* baseArray);
	/**Clean up.*/
	~SuffixArrayCache();
	uintmax_t numEntry();
	void getEntry(uintmax_t entInd, SuffixArrayEntry* storeLoc);
	/**The maximum number of cachelines to store.*/
	uintptr_t maxCache;
	/**The base array.*/
	SuffixArray* baseArr;
	
	/**Lock access to the cache and file.*/
	ReadWriteLock cacheLock;
	/**The actual cache.*/
	std::map<uintmax_t,SuffixArrayEntry> cacheRes;
};

/**Cache updates to a suffix array search.*/
class SuffixArraySearchCache{
public:
	/**
	 * Create an empty cache.
	 * @param maxCacheSize The maximum size of the cache.
	 */
	SuffixArraySearchCache(uintptr_t maxCacheSize);
	/**Clean up.*/
	~SuffixArraySearchCache();
	/**
	 * Update a match, using the cache if it is available.
	 * @param prevSearch The current state of the search.
	 * @param updateWith The character to update the search with.
	 * @return The next state of the search.
	 */
	SuffixArraySearch updateMatch(SuffixArraySearch prevSearch, char updateWith);
	/**The bytes of ram to use for a cache.*/
	uintptr_t maxCache;
	/**Lock access to the cache and file.*/
	ReadWriteLock cacheLock;
	/**A cache for searches.*/
	std::map< std::pair<SuffixArraySearch,char>, SuffixArraySearch > arraySearchCache;
};

}

#endif