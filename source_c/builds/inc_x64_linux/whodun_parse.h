#ifndef WHODUN_PARSE_H
#define WHODUN_PARSE_H 1

#include "whodun_thread.h"
#include "whodun_string.h"

namespace whodun {

/**Information on how a string was split.*/
class StringSplitData{
public:
	/**The number of splits.*/
	uintptr_t numSplits;
	/**Where the split tokens start.*/
	char** splitStart;
	/**Where the split tokens end.*/
	char** splitEnd;
};

/**Split a string according to some criteria.*/
class StringSplitter{
public:
	/**Set up a splitter.*/
	StringSplitter();
	/**Clean up.*/
	virtual ~StringSplitter();
	/**Storage for the results.*/
	StringSplitData res;
	
	/**
	 * Split as much of the given string as possible.
	 * @param inpLen The length of the string.
	 * @param inpStr The string.
	 * @return The place it stopped looking.
	 */
	virtual char* split(uintptr_t inpLen, char* inpStr) = 0;
	
	/**
	 * Split all of the given string.
	 * @param inpLen The length of the string.
	 * @param inpStr The string.
	 */
	virtual void splitAll(uintptr_t inpLen, char* inpStr) = 0;
};

/**Split a string on a single character.*/
class CharacterSplitter : public StringSplitter{
public:
	/**Set up a splitter.*/
	CharacterSplitter();
	/**Clean up.*/
	virtual ~CharacterSplitter();
	/**The caracter to split on.*/
	char splitOn;
	/**The allocated space for splits.*/
	uintptr_t allocSplit;
	/**The allocated space for the starting locations.*/
	char** allocStarts;
	/**The allocated space for the ending locations.*/
	char** allocEnds;
	virtual char* split(uintptr_t inpLen, char* inpStr);
	virtual void splitAll(uintptr_t inpLen, char* inpStr);
};

class MultithreadCharacterSplitter;

/**A uniform for a multithreaded character search.*/
class MultithreadCharacterSplitterFindUniform : public ThreadTask{
public:
	/**Set up.*/
	MultithreadCharacterSplitterFindUniform();
	/**Tear down*/
	~MultithreadCharacterSplitterFindUniform();
	void doIt();
	/**The main thing.*/
	MultithreadCharacterSplitter* mainCont;
	/**The character to look for.*/
	char splitOn;
	/**The place to look from.*/
	char* lookFrom;
	/**The place to look to.*/
	char* lookTo;
	/**The id of the thread.*/
	uintptr_t threadID;
	/**The locations the character was found at.*/
	std::vector<char*> foundChar;
	/**The previous starting location.*/
	char* prevStart;
	/**The place to pack to (if at that phase).*/
	char** packLocStart;
	/**The place to pack to (if at that phase).*/
	char** packLocEnd;
};

/**A multithreaded character splitter.*/
class MultithreadCharacterSplitter : public CharacterSplitter{
public:
	/**
	 * Set up a splitter.
	 * @param numThread The number of tasks to start.
	 * @param mainPool The threads to use.
	 */
	MultithreadCharacterSplitter(int numThread, ThreadPool* mainPool);
	/**Clean up.*/
	virtual ~MultithreadCharacterSplitter();
	virtual char* split(uintptr_t inpLen, char* inpStr);
	/**The thread pool to use.*/
	ThreadPool* usePool;
	/**The things that will perform the initial search.*/
	std::vector<MultithreadCharacterSplitterFindUniform> findUnis;
};

/**Split a string on a single character (from a set of characters).*/
class CharacterSetSplitter : public StringSplitter{
public:
	/**Set up a splitter.*/
	CharacterSetSplitter();
	/**Clean up.*/
	virtual ~CharacterSetSplitter();
	/**Whether to split on each character.*/
	char splitOn[256];
	/**The allocated space for splits.*/
	uintptr_t allocSplit;
	/**The allocated space for the starting locations.*/
	char** allocStarts;
	/**The allocated space for the ending locations.*/
	char** allocEnds;
	virtual char* split(uintptr_t inpLen, char* inpStr);
	virtual void splitAll(uintptr_t inpLen, char* inpStr);
};

class MultithreadCharacterSetSplitter;

/**A uniform for a multithreaded character search.*/
class MultithreadCharacterSetSplitterFindUniform : public ThreadTask{
public:
	/**Set up.*/
	MultithreadCharacterSetSplitterFindUniform();
	/**Tear down*/
	~MultithreadCharacterSetSplitterFindUniform();
	void doIt();
	/**The main thing.*/
	MultithreadCharacterSetSplitter* mainCont;
	/**The character set to look for.*/
	char* splitOn;
	/**The place to look from.*/
	char* lookFrom;
	/**The place to look to.*/
	char* lookTo;
	/**The id of the thread.*/
	uintptr_t threadID;
	/**The locations the character was found at.*/
	std::vector<char*> foundChar;
	/**The previous starting location.*/
	char* prevStart;
	/**The place to pack to (if at that phase).*/
	char** packLocStart;
	/**The place to pack to (if at that phase).*/
	char** packLocEnd;
};

/**A multithreaded character splitter.*/
class MultithreadCharacterSetSplitter : public CharacterSetSplitter{
public:
	/**
	 * Set up a splitter.
	 * @param numThread The number of tasks to start.
	 * @param mainPool The threads to use.
	 */
	MultithreadCharacterSetSplitter(int numThread, ThreadPool* mainPool);
	/**Clean up.*/
	virtual ~MultithreadCharacterSetSplitter();
	virtual char* split(uintptr_t inpLen, char* inpStr);
	/**The thread pool to use.*/
	ThreadPool* usePool;
	/**The things that will perform the initial search.*/
	std::vector<MultithreadCharacterSetSplitterFindUniform> findUnis;
};

/**Wrap an input stream, spitting out tokens.*/
class SplitterInStream{
public:
	/**
	 * Set up such a stream.
	 * @param fromStream The stream to split up.
	 * @param splitMeth The split method.
	 * @param bufferSize The starting amount of data to load at one time.
	 */
	SplitterInStream(InStream* fromStream, StringSplitter* splitMeth, uintptr_t bufferSize);
	/**
	 * Set up such a stream: use multiple threads for buffer resize.
	 * @param fromStream The stream to split up.
	 * @param splitMeth The split method.
	 * @param bufferSize The starting amount of data to load at one time.
	 * @param numThreads The number of threads to use.
	 * @param mainPool The threads to use.
	 */
	SplitterInStream(InStream* fromStream, StringSplitter* splitMeth, uintptr_t bufferSize, int numThreads, ThreadPool* mainPool);
	/**Clean up.*/
	~SplitterInStream();
	/**
	 * Read more tokens.
	 * @return The split data. Only good until the next call to read.
	 */
	StringSplitData read();
	/**
	 * Increase the size of the buffer and read more: the last call to read is unaffected.
	 * @param lastRet The data returned from the initial call to read: will be updated.
	 * @return Whether anything more was read: 0 for end of file.
	 */
	int readMore(StringSplitData* lastRet);
	/**The stream to read.*/
	InStream* fromStr;
	/**The way to split that stream.*/
	StringSplitter* splMeth;
	/**The ammount of space allocated for the buffer.*/
	uintptr_t buffAlloc;
	/**The buffer.*/
	char* buff;
	/**Whether the previous read left stuff on the table.*/
	char* prevBStart;
	/**Whether the previous read left stuff on the table.*/
	char* prevBEnd;
	/**A buffer for a move.*/
	char* moveBuff;
	/**The size of the move buffer.*/
	uintptr_t threadStageSize;
	/**The method for copying data, if any.*/
	MultithreadedStringH* useCopy;
	/**Whether the end has been hit.*/
	int hitEnd;
};

/**
 * Parse a hex nibble.
 * @param cval The character to parse.
 * @return The parsed value: -1 for a bad input.
 */
int parseHexNibble(char cval);

/**
 * This will parse a hex byte.
 * @param cvalA The first value.
 * @param cvalB The second value.
 * @return The byte, or -1 if invalid.
 */
int parseHexCode(char cvalA, char cvalB);

//TODO

}

#endif