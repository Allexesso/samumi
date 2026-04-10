#ifndef WHODUN_SORT_H
#define WHODUN_SORT_H 1

#include <deque>
#include <vector>
#include <stdint.h>

#include "whodun_thread.h"
#include "whodun_string.h"
#include "whodun_compress.h"

namespace whodun {

/**Compare stuff.*/
class RawComparator{
public:
	/**Allow subclasses to implement.*/
	virtual ~RawComparator();
	/**
	 * Get the size of the items this works on.
	 * @return The size of the items this works on.
	 */
	virtual uintptr_t itemSize() = 0;
	/**
	 * Compare with raw memory address.
	 * @param itemA The first item.
	 * @param itemB The second item.
	 * @return Whether itemA should come before itemB.
	 */
	virtual bool compare(const void* itemA, const void* itemB) = 0;
};

//TODO sorting Objects
/*
template<typename T>
class LessComparator : public RawComparator{
public:
	uintptr_t itemSize(){
		return sizeof(T);
	}
	bool compare(const void* itemA, const void* itemB){
		return *((const T*)itemA) < *((const T*)itemB);
	}
};
*/

/**Options for sorting.*/
class SortOptions{
public:
	/** Set up some empty sort options. */
	SortOptions();
	/**
	 * Set up some simple sort options.
	 * @param itemS The size of each item.
	 * @param useU The uniform to use.
	 * @param compM The comparison method.
	 */
	SortOptions(uintptr_t itemS, void* useU, bool(*compM)(void*,void*,void*));
	/**
	 * Set up to use a comparator.
	 * @param useComp The comparator to use.
	 */
	SortOptions(RawComparator* useComp);
	/**
	 * The method to use for comparison (i.e. less than).
	 * @param unif A uniform for the comparison.
	 * @param itemA The first item.
	 * @param itemB The second item.
	 * @return Whther itemA should come before itemB (false if equal).
	 */
	bool (*compMeth)(void* unif, void* itemA,void* itemB);
	/**The uniform to use.*/
	void* useUni;
	/**The size of each item.*/
	uintptr_t itemSize;
};

/**Do merge sorts in memory.*/
class InMemoryMergesort{
public:
	/**
	 * Set up in memory mergesorting.
	 * @param theOpts The options for sorting.
	 */
	InMemoryMergesort(SortOptions* theOpts);
	/**
	 * Set up in memory mergesorting.
	 * @param theOpts The options for sorting.
	 * @param numThr The number of threads to use.
	 * @param mainPool The threads to use.
	 */
	InMemoryMergesort(SortOptions* theOpts, uintptr_t numThr, ThreadPool* mainPool);
	/**Clean up.*/
	~InMemoryMergesort();
	
	/**
	 * Actually sort some stuff.
	 * @param numEntries The number of entries.
	 * @param entryStore The entries.
	 */
	void sort(uintptr_t numEntries, char* entryStore);
	
	/**The sorting options.*/
	SortOptions useOpts;
	/**The amount of space allocated for temporaries.*/
	uintptr_t allocSize;
	/**The temporary space.*/
	char* allocTmp;
	/**The thread pool to use, if any.*/
	ThreadPool* usePool;
	/**The number of threads this should use.*/
	uintptr_t numThread;
	/**Storage for thread uniforms.*/
	void* threadUniStore;
	/**Indirect storage.*/
	std::vector<ThreadTask*> passUnis;
	/**The waiting pieces.*/
	std::deque< SizePtrString > threadPieces;
};

/**
 * Perform a lower_bound-type search.
 * @param numEnts The number of items to search through.
 * @param inMem The items to search through.
 * @param lookFor The thing to look for.
 * @param opts The options for the sort.
 * @return The lower_bound insertion index.
 */
char* chunkyLowerBound(uintptr_t numEnts, char* inMem, char* lookFor, SortOptions* opts);

/**
 * Perform a upper_bound-type search.
 * @param numEnts The number of items to search through.
 * @param inMem The items to search through.
 * @param lookFor The thing to look for.
 * @param opts The options for the sort.
 * @return The upper_bound insertion index.
 */
char* chunkyUpperBound(uintptr_t numEnts, char* inMem, char* lookFor, SortOptions* opts);

/**
 * Find a quantile between a pair of arrays.
 * @param numEntsA The number of entries in the first array.
 * @param inMemA The entries of the first array.
 * @param numEntsB The number of entries in the second array.
 * @param inMemB The entries of the second array.
 * @param tgtQuantile
 * @param opts The options for the sort.
 * @return The number to take from each to get the quantile.
 */
std::pair<uintptr_t,uintptr_t> chunkyPairedQuantile(uintptr_t numEntsA, char* inMemA, uintptr_t numEntsB, char* inMemB, uintptr_t tgtQuantile, SortOptions* opts);

/**Run sorts in external memory.*/
class ExternalMergeSort{
public:
	/**
	 * Set up an external sort.
	 * @param workDirName The working directory for temporary files.
	 * @param theOpts The options for sorting.
	 */
	ExternalMergeSort(const char* workDirName, SortOptions* theOpts);
	/**
	 * Set up an external sort.
	 * @param workDirName The working directory for temporary files.
	 * @param theOpts The options for sorting.
	 * @param numThr The number of threads to use.
	 * @param mainPool The threads to use.
	 */
	ExternalMergeSort(const char* workDirName, SortOptions* theOpts, uintptr_t numThr, ThreadPool* mainPool);
	/**Clean up.*/
	~ExternalMergeSort();
	/**
	 * Add some (sorted) data.
	 * @param numEntries The number of entries.
	 * @param entryStore The entries. Memory can be reused after this call.
	 */
	void addData(uintptr_t numEntries, char* entryStore);
	/**
	 * Merge all the data.
	 * @param toDump The place to write it.
	 */
	void mergeData(OutStream* toDump);
	/**The maximum number of bytes to load.*/
	uintptr_t maxLoad;
	/**The number of created temporary files.*/
	uintptr_t numTemps;
	/**The name of the temporary directory.*/
	std::string tempName;
	/**Whether this made the temporary directory.*/
	int madeTemp;
	/**The sorting options.*/
	SortOptions useOpts;
	/**The thread pool to use, if any.*/
	ThreadPool* usePool;
	/**The number of threads this should use.*/
	uintptr_t numThread;
	/**Storage for thread uniforms.*/
	void* threadUniStore;
	/**Indirect storage.*/
	std::vector<ThreadTask*> passUnis;
	/**Storage for information on temporary files.*/
	std::deque<std::string> allTempBase;
	/**Storage for information on temporary files.*/
	std::deque<std::string> allTempBlock;
	/**
	 * Make a name for a new temporary.
	 * @param toFill The place to put the name.
	 */
	void newTempName(std::string* toFill);
	/**
	 * Do a single merge.
	 * @param tempStore Storage space (for both loading and merging).
	 * @param loadEnts The number of entries to load.
	 * @param toDump The place to write.
	 */
	void mergeSingle(char* tempStore, uintptr_t loadEnts, OutStream* toDump);
};

}

#endif
