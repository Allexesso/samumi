#ifndef WHODUN_STRING_H
#define WHODUN_STRING_H 1

#include <iostream>

#include "whodun_thread.h"

namespace whodun {

//pascal flavored strings

/**A size/pointer combination used as a string.*/
typedef struct{
	/**The length*/
	uintptr_t len;
	/**The text*/
	char* txt;
} SizePtrString;
/**
 * Compare two strings in lexicographic order.
 * @param strA The first "string": length and characters.
 * @param strB The second "string": length and characters.
 * @return Whether strA is less than strB.
 */
bool operator < (const SizePtrString& strA, const SizePtrString& strB);
/**
 * Compare two strings in lexicographic order.
 * @param strA The first "string": length and characters.
 * @param strB The second "string": length and characters.
 * @return Whether strA is less than strB.
 */
bool operator > (const SizePtrString& strA, const SizePtrString& strB);
/**
 * Compare two strings in lexicographic order.
 * @param strA The first "string": length and characters.
 * @param strB The second "string": length and characters.
 * @return Whether strA is less than strB.
 */
bool operator <= (const SizePtrString& strA, const SizePtrString& strB);
/**
 * Compare two strings in lexicographic order.
 * @param strA The first "string": length and characters.
 * @param strB The second "string": length and characters.
 * @return Whether strA is less than strB.
 */
bool operator >= (const SizePtrString& strA, const SizePtrString& strB);
/**
 * Compare two strings in lexicographic order.
 * @param strA The first "string": length and characters.
 * @param strB The second "string": length and characters.
 * @return Whether strA is less than strB.
 */
bool operator == (const SizePtrString& strA, const SizePtrString& strB);
/**
 * Compare two strings in lexicographic order.
 * @param strA The first "string": length and characters.
 * @param strB The second "string": length and characters.
 * @return Whether strA is less than strB.
 */
bool operator != (const SizePtrString& strA, const SizePtrString& strB);

/**
 * Trim a string.
 * @param toTrim The thing to trim.
 * @return The trimmed string.
 */
SizePtrString trim(SizePtrString toTrim);

/**
 * Output a size pointer string.
 * @param os The place to write.
 * @param toOut The thing to write.
 * @return the stream.
 */
std::ostream& operator<<(std::ostream& os, SizePtrString const & toOut);

//stuff missing from string.h

/**
 * Return the number of characters in str1 until one of the characters in str2 is encountered (or the end is reached).
 * @param str1 The string to walk along.
 * @param numB1 The length of said string.
 * @param str2 The characters to search for.
 * @param numB2 The number of characters to search for.
 * @return The number of characters until a break (one of the characters in str2 or end of string).
 */
size_t memcspn(const char* str1, size_t numB1, const char* str2, size_t numB2);
/**
 * Return the number of characters in str1 until something not in str2 is encountered.
 * @param str1 The string to walk along.
 * @param numB1 The length of said string.
 * @param str2 The characters to search for.
 * @param numB2 The number of characters to search for.
 * @return The number of characters until a break (something not in str2 or end of string).
 */
size_t memspn(const char* str1, size_t numB1, const char* str2, size_t numB2);
/**
 * Returns a pointer to the first occurence of str2 in str1.
 * @param str1 The string to walk along.
 * @param numB1 The length of said string.
 * @param str2 The string to search for.
 * @param numB2 The length of said string.
 * @return The location of str2 in str1, or null if not present.
 */
char* memmem(const char* str1, size_t numB1, const char* str2, size_t numB2);
/**
 * This will swap, byte by byte, the contents of two (non-overlapping) arrays.
 * @param arrA THe first array.
 * @param arrB The second array.
 * @param numBts The number of bytes to swap.
 */
void memswap(char* arrA, char* arrB, size_t numBts);
/**
 * This will figure the number of bytes until the first difference between the two chunks of memory.
 * @param str1 The first chunk.
 * @param numB1 The length of the first chunk.
 * @param str2 The second chunk.
 * @param numB2 The length of the second chunk.
 * @return The number of bytes to the first difference. End of string is always different.
 */
size_t memdiff(const char* str1, size_t numB1, const char* str2, size_t numB2);

//other string utilities

/**
 * Determine if one string ends with another.
 * @param str1 The large string.
 * @param str2 The suspected ending.
 * @return Whether str1 ends with str2.
 */
int strendswith(const char* str1, const char* str2);

/**
 * Determine if one string starts with another.
 * @param str1 The large string.
 * @param str2 The suspected ending.
 * @return Whether str1 starts with str2.
 */
int strstartswith(const char* str1, const char* str2);

/**
 * Determine if one memory block ends with another.
 * @param str1 The large string.
 * @param numB1 The size of str1.
 * @param str2 The suspected ending.
 * @param numB1 The size of str2.
 * @return Whether str1 ends with str2.
 */
int memendswith(const char* str1, size_t numB1, const char* str2, size_t numB2);

/**
 * Determine if one memory block ends with another.
 * @param str1 The large string.
 * @param numB1 The size of str1.
 * @param str2 The suspected ending.
 * @param numB1 The size of str2.
 * @return Whether str1 ends with str2.
 */
int memstartswith(const char* str1, size_t numB1, const char* str2, size_t numB2);

/**
 * Look for a string inside another, allowing for mismatches.
 * @param lookForS The string to look for.
 * @param lookForE THe end of the string to look for.
 * @param lookInS The string to look in.
 * @param lookInE The end of the string to look in.
 I @param maxFuzz The maximum number of mismatches to allow.
 * @return The location it was found at: null on fail.
 */
const char* fuzzyFind(const char* lookForS, const char* lookForE, const char* lookInS, const char* lookInE, uintptr_t maxFuzz);
/**
 * Look for a string inside another, allowing for mismatches.
 * @param lookForS The string to look for.
 * @param lookForE THe end of the string to look for.
 * @param lookInS The string to look in.
 * @param lookInE The end of the string to look in.
 I @param maxFuzz The maximum number of mismatches to allow.
 * @return The location it was found at: null on fail.
 */
char* fuzzyFind(const char* lookForS, const char* lookForE, char* lookInS, char* lookInE, uintptr_t maxFuzz);

/**Perform an edit distance search.*/
class EditFinder{
public:
	/**Set up.*/
	EditFinder();
	/**Tear down*/
	~EditFinder();
	
	/**
	 * Start an edit distance search.
	 * @param lookForS The string to look for.
	 * @param lookForE THe end of the string to look for.
	 * @param lookInS The string to look in.
	 * @param lookInE The end of the string to look in.
	 * @param maxEdit The maximum number of edits to allow.
	 */
	void startSearch(char* lookForS, char* lookForE, char* lookInS, char* lookInE, uintptr_t maxEdit);
	/**
	 * Get the next match.
	 * @return Whether something was found.
	 */
	int findNext();
	/**
	 * Get the match with the lowest edit score. If multiple, pick one.
	 * @param biasEnd If multiple best, pick the latest one. By default, picks earliest.
	 * @return Whether something was found.
	 */
	int findBest(int biasEnd);
	
	/**Where the last match was found at.*/
	uintptr_t foundAt;
	/**Where the last match ended.*/
	uintptr_t foundTo;
	/**The edit distance for that match.*/
	uintptr_t foundEdit;
	
	/**The maximum edit distance.*/
	uintptr_t lookEdit;
	/**The thing to look for.*/
	SizePtrString lookFor;
	/**The thing to look in.*/
	SizePtrString lookIn;
	/**A stack for values.*/
	std::vector<uintptr_t> lookStack;
};

//single packing operations

/**
 * Pack a value into memory (big endian).
 * @param toPrep The value to pack.
 * @param toBuffer The place to pack it.
 */
void packBE64(uint64_t toPrep, char* toBuffer);
/**
 * Pack a value into memory (big endian).
 * @param toPrep The value to pack.
 * @param toBuffer The place to pack it.
 */
void packBE32(uint32_t toPrep, char* toBuffer);
/**
 * Pack a value into memory (big endian).
 * @param toPrep The value to pack.
 * @param toBuffer The place to pack it.
 */
void packBE16(uint16_t toPrep, char* toBuffer);

/**
 * Pack a value into memory (little endian).
 * @param toPrep The value to pack.
 * @param toBuffer The place to pack it.
 */
void packLE64(uint64_t toPrep, char* toBuffer);
/**
 * Pack a value into memory (little endian).
 * @param toPrep The value to pack.
 * @param toBuffer The place to pack it.
 */
void packLE32(uint32_t toPrep, char* toBuffer);
/**
 * Pack a value into memory (little endian).
 * @param toPrep The value to pack.
 * @param toBuffer The place to pack it.
 */
void packLE16(uint16_t toPrep, char* toBuffer);

/**
 * Unpack a value from memory (big endian).
 * @param toBuffer The memory to unpack.
 * @return The unpacked value.
 */
uint64_t unpackBE64(const char* toBuffer);
/**
 * Unpack a value from memory (big endian).
 * @param toBuffer The memory to unpack.
 * @return The unpacked value.
 */
uint32_t unpackBE32(const char* toBuffer);
/**
 * Unpack a value from memory (big endian).
 * @param toBuffer The memory to unpack.
 * @return The unpacked value.
 */
uint16_t unpackBE16(const char* toBuffer);

/**
 * Unpack a value from memory (big endian).
 * @param toBuffer The memory to unpack.
 * @return The unpacked value.
 */
uint64_t unpackLE64(const char* toBuffer);
/**
 * Unpack a value from memory (big endian).
 * @param toBuffer The memory to unpack.
 * @return The unpacked value.
 */
uint32_t unpackLE32(const char* toBuffer);
/**
 * Unpack a value from memory (big endian).
 * @param toBuffer The memory to unpack.
 * @return The unpacked value.
 */
uint16_t unpackLE16(const char* toBuffer);

/**
 * Get the bits of a double.
 * @param toPack The double to get bits from.
 * @return The bits.
 */
uint64_t packDbl(double toPack);
/**
 * Get the bits of a double.
 * @param toPack The double to get bits from.
 * @return The bits.
 */
uint32_t packFlt(float toPack);

/**
 * Turn bits into a double.
 * @param toPack The bits.
 * @return The double.
 */
double unpackDbl(uint64_t toPack);
/**
 * Turn bits into a double.
 * @param toPack The bits.
 * @return The double.
 */
float unpackFlt(uint32_t toPack);

//multithreaded operations

#define WHODUN_TASK_UTILITY(tname, needVs) class tname : public ThreadTask { public: void doIt(); uintptr_t myID; needVs };

WHODUN_TASK_UTILITY(MultithreadedStringMemcpyTask, void* cpyTo; const void* cpyFrom; size_t copyNum;)
WHODUN_TASK_UTILITY(MultithreadedStringMemsetTask, void* setP; int value; size_t numBts;)
WHODUN_TASK_UTILITY(MultithreadedStringMemswapTask, char* arrA; char* arrB; size_t numBts;)

#undef WHODUN_TASK_UTILITY

/**Mange multithreaded string operations.*/
class MultithreadedStringH{
public:
	/**
	 * Set up a manager.
	 * @param numThread The number of threads to use.
	 * @param mainPool The thread pool to use.
	 */
	MultithreadedStringH(uintptr_t numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~MultithreadedStringH();
	
	/**
	 * Start running a memcpy.
	 * @param cpyTo The place to copy to.
	 * @param cpyFrom The place to copy from.
	 * @param copyNum The number of bytes to copy.
	 * @param saveID The thread uniform storage.
	 */
	void initMemcpy(void* cpyTo, const void* cpyFrom, size_t copyNum, MultithreadedStringMemcpyTask* saveID);
	
	/**
	 * Start running a memset.
	 * @param setP The first byte to set.
	 * @param value The avlue to set to.
	 * @param numBts The number of bytes to set.
	 * @param saveID The thread uniform storage.
	 */
	void initMemset(void* setP, int value, size_t numBts, MultithreadedStringMemsetTask* saveID);
	
	/**
	 * Initialize a memswap.
	 * @param arrA The first array.
	 * @param arrB The second array.
	 * @param numBts The number of bytes to swap.
	 * @param saveID The thread uniform storage.
	 */
	void initMemswap(char* arrA, char* arrB, size_t numBts, MultithreadedStringMemswapTask* saveID);
	
	/**
	 * Wait on a task to finish.
	 * @param allID All The IDs to wait on.
	 */
	void wait(MultithreadedStringMemcpyTask* allID);
	
	/**
	 * Wait on a task to finish.
	 * @param allID All The IDs to wait on.
	 */
	void wait(MultithreadedStringMemsetTask* allID);
	
	/**
	 * Wait on a task to finish.
	 * @param allID All The IDs to wait on.
	 */
	void wait(MultithreadedStringMemswapTask* allID);
	
	/**
	 * Run a memcpy.
	 * @param cpyTo The place to copy to.
	 * @param cpyFrom The place to copy from.
	 * @param copyNum The number of bytes to copy.
	 */
	void memcpy(void* cpyTo, const void* cpyFrom, size_t copyNum);
	
	/**
	 * Run a memset.
	 * @param setP The first byte to set.
	 * @param value The avlue to set to.
	 * @param numBts The number of bytes to set.
	 */
	void memset(void* setP, int value, size_t numBts);
	
	/**
	 * Run a memswap.
	 * @param arrA The first array.
	 * @param arrB The second array.
	 * @param numBts The number of bytes to swap.
	 */
	void memswap(char* arrA, char* arrB, size_t numBts);
	
	/**The number of tasks to allocate.*/
	uintptr_t numThr;
	/**The thread pool to use.*/
	ThreadPool* usePool;
	/**The IDs to wait on for the blocking calls.*/
	std::vector<uintptr_t> blockIDs;
	/**The stuff to run.*/
	std::vector<MultithreadedStringMemcpyTask> saveUniMCpy;
	/**The stuff to run.*/
	std::vector<MultithreadedStringMemsetTask> saveUniMSet;
	/**The stuff to run.*/
	std::vector<MultithreadedStringMemswapTask> saveUniMSwap;
};

/**A buffer of memory: can coordinate bulk writes.*/
class PistonInStream : public InStream{
public:
	/**
	 * Create the stream to use.
	 * @param buffSize The size of the internal buffer.
	 * @param numThread The number of threads to use for transfers.
	 * @param mainPool The thread pool to use.
	 */
	PistonInStream(uintptr_t buffSize, uintptr_t numThread, ThreadPool* mainPool);
	/**Clean up*/
	~PistonInStream();
	int readByte();
	void close();
	uintptr_t readBytes(char* toR, uintptr_t numR);
	/**
	 * Wait for the buffer to be read through, and get the lock.
	 */
	void waitForEmpty();
	/**
	 * Note that the buffer is full: give the lock back.
	 */
	void noteFull();
	/**
	 * Note that no more data is coming: give the lock back.
	 */
	void noteEnd();
	/**Has the source ended.*/
	int haveEnd;
	/**The allocated size of the buffer.*/
	uintptr_t allocBuff;
	/**The buffer.*/
	char* saveBuff;
	/**The next byte in the buffer to report.*/
	uintptr_t nextByte;
	/**The number of bytes left in the buffer.*/
	uintptr_t numByte;
	/**Lock*/
	OSMutex useMut;
	/**Produce wait*/
	OSCondition prodCond;
	/**Consumer wait*/
	OSCondition consCond;
	/**Use for bulk copies.*/
	MultithreadedStringH useCopy;
};

/**Coordinate bulk writes*/
class PistonOutStream : public OutStream{
public:
	/**
	 * Create the stream to use.
	 * @param buffSize The size of the internal buffer.
	 * @param numThread The number of threads to use for transfers.
	 * @param mainPool The thread pool to use.
	 */
	PistonOutStream(uintptr_t buffSize, uintptr_t numThread, ThreadPool* mainPool);
	/**Clean up*/
	~PistonOutStream();
	void writeByte(int toW);
	void close();
	void writeBytes(const char* toW, uintptr_t numW);
	/**
	 * Wait for the buffer to be filled, and get the lock.
	 */
	void waitForFull();
	/**
	 * Note that the buffer is empty, give the lock back.
	 */
	void noteEmpty();
	/**The allocated size of the buffer.*/
	uintptr_t allocBuff;
	/**The buffer.*/
	char* saveBuff;
	/**The next byte in the buffer to report.*/
	uintptr_t nextByte;
	/**Lock*/
	OSMutex useMut;
	/**Produce wait*/
	OSCondition prodCond;
	/**Consumer wait*/
	OSCondition consCond;
	/**Use for bulk copies.*/
	MultithreadedStringH useCopy;
};

/**A source of strings.*/
class BulkStringSource{
public:
	/**Allow subclasses.*/
	virtual ~BulkStringSource();
	/**
	 * Get the number of strings in the thing.
	 * @return The number of strings.
	 */
	virtual uintmax_t numStrings() = 0;
	/**
	 * Get the length of a string.
	 * @param strInd The string index.
	 * @return The length of that string.
	 */
	virtual uintmax_t stringLength(uintmax_t strInd) = 0;
	/**
	 * Get the sequence of a string.
	 * @param strInd The string index.
	 * @param fromC The character to start at.
	 * @param toC The character to end at.
	 * @param saveC The place to put the sequence.
	 */
	virtual void getString(uintmax_t strInd, uintmax_t fromC, uintmax_t toC, char* saveC) = 0;
};

/**A source of strings from SizePtrStrings in memory.*/
class SizePtrBulkStringSource : public BulkStringSource{
public:
	/**
	 * Create a bulk string source.
	 * @param numStrings The number of strings.
	 * @param theStrings The strings of interest.
	 */
	SizePtrBulkStringSource(uintptr_t numStrings, SizePtrString* theStrings);
	/**Clean up.*/
	~SizePtrBulkStringSource();
	
	uintmax_t numStrings();
	uintmax_t stringLength(uintmax_t strInd);
	void getString(uintmax_t strInd, uintmax_t fromC, uintmax_t toC, char* saveC);
	
	/**The strings of interest.*/
	std::vector<SizePtrString> saveStr;
};

/***
 * See if the file name ends with one of the approved extensions.
 * @param fileName The file name in question.
 * @param extSet The approved extensions.
 * @param Whether fileName ends with one of those extensions.
 */
bool checkFileExtensions(const char* fileName, std::set<std::string>* extSet);

}

#endif
