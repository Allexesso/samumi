#ifndef WHODUN_OSHOOK_H
#define WHODUN_OSHOOK_H 1

#include <string>
#include <vector>
#include <stdio.h>
#include <stdint.h>

namespace whodun {

/**iostream is a mess.*/
class OutStream{
public:
	/**Basic setup.*/
	OutStream();
	/**Clean up and close.*/
	virtual ~OutStream();
	/**
	 * Write a byte.
	 * @param toW The byte to write.
	 */
	virtual void writeByte(int toW) = 0;
	/**Close the stream.*/
	virtual void close() = 0;
	/**
	 * Write bytes.
	 * @param toW The bytes to write.
	 * @param numW The number of bytes to write.
	 */
	virtual void writeBytes(const char* toW, uintptr_t numW);
	/**
	 * Write null-terminated bytes (without the null terminator).
	 * @param toW The bytes to write.
	 */
	void writeZTBytes(const char* toW);
	/**Flush waiting bytes.*/
	virtual void flush();
	/**Whether this thing has been closed.*/
	int isClosed;
};

/**iostream is a mess.*/
class InStream{
public:
	/**Basic setup.*/
	InStream();
	/**Clean up and close.*/
	virtual ~InStream();
	/**
	 * Read a byte.
	 * @return The read byte. -1 for eof.
	 */
	virtual int readByte() = 0;
	/**Close the stream.*/
	virtual void close() = 0;
	/**
	 * Read bytes.
	 * @param toR The buffer to store in.
	 * @param numR The maximum number of read.
	 * @return The number actually read. If less than numR, hit end.
	 */
	virtual uintptr_t readBytes(char* toR, uintptr_t numR);
	/**
	 * Read bytes: throw exception if not enough bytes.
	 * @param toR The buffer to store in.
	 * @param numR The maximum number of read.
	 */
	virtual void forceReadBytes(char* toR, uintptr_t numR);
	/**
	 * Read everything from the stream.
	 * @param toFill The place to put it all.
	 */
	virtual void readAll(std::vector<char>* toFill);
	/**Whether this thing has been closed.*/
	int isClosed;
};

/**A random access input stream.*/
class RandaccInStream : public InStream{
public:
	/**
	 * Get the current position in the stream.
	 * @return The current position in the stream.
	 */
	virtual uintmax_t tell() = 0;
	/**
	 * Seek to the given location.
	 * @param toLoc The location to seek to.
	 */
	virtual void seek(uintmax_t toLoc) = 0;
	/**
	 * Get the size of the thing.
	 * @return The size of the thing.
	 */
	virtual uintmax_t size() = 0;
};

/**Concatenate multiple input streams.*/
class ConcatenatedInStream : public InStream{
public:
	/**
	 * Wrap the given streams.
	 * @param strA The first stream.
	 * @param strB The second stream.
	 */
	ConcatenatedInStream(InStream* strA, InStream* strB);
	/**
	 * Wrap the given streams.
	 * @param numStr The number of streams to wrap.
	 * @param theStrs The streams to wrap.
	 */
	ConcatenatedInStream(uintptr_t numStr, InStream** theStrs);
	/**Clean up.*/
	~ConcatenatedInStream();
	uintptr_t readBytes(char* toR, uintptr_t numR);
	int readByte();
	void close();
	/**The waiting streams, in inverse order.*/
	std::vector<InStream*> waitStreams;
};

/**Concatenate multiple input streams.*/
class ConcatenatedRandaccInStream : public RandaccInStream{
public:
	/**
	 * Wrap the given streams.
	 * @param strA The first stream.
	 * @param strB The second stream.
	 */
	ConcatenatedRandaccInStream(RandaccInStream* strA, RandaccInStream* strB);
	/**
	 * Wrap the given streams.
	 * @param numStr The number of streams to wrap.
	 * @param theStrs The streams to wrap.
	 */
	ConcatenatedRandaccInStream(uintptr_t numStr, RandaccInStream** theStrs);
	/**Clean up.*/
	~ConcatenatedRandaccInStream();
	uintptr_t readBytes(char* toR, uintptr_t numR);
	int readByte();
	void close();
	uintmax_t tell();
	void seek(uintmax_t toLoc);
	uintmax_t size();
	/**The current file.*/
	uintptr_t curF;
	/**The current index.*/
	uintmax_t curI;
	/**The wrapped streams.*/
	std::vector<RandaccInStream*> wrapped;
	/**The low indices of each stream.*/
	std::vector<uintmax_t> lowInds;
	/**The high indices of each stream.*/
	std::vector<uintmax_t> highInds;
};

/**Get a subrange of a random access stream.*/
class SubrangeRandaccInStream : public RandaccInStream{
public:
	/**
	 * Wrap a sub-range of a file.
	 * @param toWrap The file to wrap.
	 * @param wrapFrom The low location to wrap.
	 * @param wrapTo The high location to wrap.
	 */
	SubrangeRandaccInStream(RandaccInStream* toWrap, uintmax_t wrapFrom, uintmax_t wrapTo);
	/**Clean up*/
	~SubrangeRandaccInStream();
	uintptr_t readBytes(char* toR, uintptr_t numR);
	int readByte();
	void close();
	uintmax_t tell();
	void seek(uintmax_t toLoc);
	uintmax_t size();
	/**The wrapped stream.*/
	RandaccInStream* wrapped;
	/**The current index.*/
	uintmax_t curI;
	/**The low index.*/
	uintmax_t lowI;
	/**The high index.*/
	uintmax_t higI;
};

/**Wrap memory as a stream.*/
class MemoryInStream : public RandaccInStream{
public:
	/**
	 * Basic setup
	 * @param numBytes The number of bytes.
	 * @param theBytes The bytes.
	 */
	MemoryInStream(uintptr_t numBytes, const char* theBytes);
	/**Clean up and close.*/
	~MemoryInStream();
	uintmax_t tell();
	void seek(uintmax_t toLoc);
	uintmax_t size();
	int readByte();
	uintptr_t readBytes(char* toR, uintptr_t numR);
	void close();
	/**The number of bytes.*/
	uintptr_t numBts;
	/**The bytes in question.*/
	const char* theBts;
	/**The next byte.*/
	uintptr_t nextBt;
};

/**Out to console.*/
class ConsoleOutStream: public OutStream{
public:
	/**Basic setup.*/
	ConsoleOutStream();
	/**Clean up and close.*/
	~ConsoleOutStream();
	void writeByte(int toW);
	void writeBytes(const char* toW, uintptr_t numW);
	void close();
};

/**Out to error.*/
class ConsoleErrStream: public OutStream{
public:
	/**Basic setup.*/
	ConsoleErrStream();
	/**Clean up and close.*/
	~ConsoleErrStream();
	void writeByte(int toW);
	void close();
};

/**In from console.*/
class ConsoleInStream: public InStream{
public:
	/**Basic setup.*/
	ConsoleInStream();
	/**Clean up and close.*/
	~ConsoleInStream();
	int readByte();
	uintptr_t readBytes(char* toR, uintptr_t numR);
	void close();
};

/**The seperator for path elements.*/
extern const char* filePathSeparator;

/**
 * Gets whether a file exists.
 * @param fileName THe name of the file to test.
 * @return Whether it exists.
 */
bool fileExists(const char* fileName);

/**
 * Deletes a file, if it exists.
 * @param fileName THe name of the file to test.
 */
void fileKill(const char* fileName);

/**
 * Gets the size of a file.
 * @param fileName THe name of the file to test.
 * @return The size of said file: -1 on error.
 */
intmax_t fileGetSize(const char* fileName);

/**
 * Like ftell, but future/idiot-proofed (goddamn Windows).
 * @param stream The file in question.
 * @return The position.
 */
intmax_t fileTellFutureProof(FILE* stream);

/**
 * Like fseek, but future/idiot-proofed (goddamn Windows).
 * @param stream The file to seek.
 * @param offset The number of bytes in the file.
 * @param whence The relative location.
 * @return Whether there was a problem.
 */
int fileSeekFutureProof(FILE* stream, intmax_t offset, int whence);

/**Out to file.*/
class FileOutStream: public OutStream{
public:
	/**
	 * Open the file.
	 * @param append Whether to append to a file if it is already there.
	 * @param fileName The name of the file.
	 */
	FileOutStream(int append, const char* fileName);
	/**Clean up and close.*/
	~FileOutStream();
	void writeByte(int toW);
	void writeBytes(const char* toW, uintptr_t numW);
	void close();
	/**The base file.*/
	FILE* baseFile;
	/**The name of the file.*/
	std::string myName;
};

/**In from file.*/
class FileInStream : public RandaccInStream{
public:
	/**
	 * Open the file.
	 * @param fileName The name of the file.
	 */
	FileInStream(const char* fileName);
	/**Clean up and close.*/
	~FileInStream();
	uintmax_t tell();
	void seek(uintmax_t toLoc);
	uintmax_t size();
	int readByte();
	uintptr_t readBytes(char* toR, uintptr_t numR);
	void close();
	/**The base file.*/
	FILE* baseFile;
	/**The name of the file.*/
	std::string myName;
	/**The size of the file.*/
	uintmax_t fileSize;
};

/**
 * Get whether a directory exists.
 * @param dirName The name of the directory.
 * @return Whether it exists.
 */
bool directoryExists(const char* dirName);

/**
 * Make a directory.
 * @param dirName The name of the directory.
 * @return Whether there was a problem.
 */
int directoryCreate(const char* dirName);

/**
 * Delete a directory, if it exists.
 * @param dirName The name of the directory to delete.
 */
void directoryKill(const char* dirName);

/**
 * Open a directory.
 * @param dirName The name of the directory.
 * @return A handle to the directory.
 */
void* directoryOpen(const char* dirName);

/**
 * Read the next entry from a directory.
 * @param dirHand The handle to the directory.
 * @return Whether there is such an entry.
 */
int directoryReadNext(void* dirHand);

/**
 * Get whether the current item is a directory.
 * @param dirHand The handle to the directory.
 * @return Whether it is a directory.
 */
int directoryCurIsDir(void* dirHand);

/**
 * Get the name of the current item.
 * @param dirHand The handle to the directory.
 * @return The name.
 */
const char* directoryCurName(void* dirHand);

/**
 * Close a directory.
 * @param dirHand The directory to the handle.
 */
void directoryClose(void* dirHand);

/**Read the stuff in a directory.*/
class DirectoryReader{
public:
	/**
	 * Open a directory for reading.
	 * @param ofDir The directory of interest.
	 */
	DirectoryReader(const char* ofDir);
	/**Clean up.*/
	~DirectoryReader();
	/**
	 * Load up the next entry.
	 * @return Whether there was a next entry.
	 */
	int loadNext();
	
	/**The handle to the directory.*/
	void* dirHandle;
	/**Whether the currently loaded item is a folder.*/
	int curItemFolder;
	/**The name of the current item.*/
	const char* curItemName;
};

/**
 * This will start a thread.
 * @param callFun The thread function.
 * @param callUniform The thing to pass to said function.
 * @return A handle to the thread.
 */
void* threadStart(void(*callFun)(void*), void* callUniform);

/**
 * Joins on a thread and drops it.
 * @param tHandle THe thread to join on.
 */
void threadJoin(void* tHandle);

/**A task to a thread.*/
class ThreadTask{
public:
	/**Set up.*/
	ThreadTask();
	/**Clean up.*/
	virtual ~ThreadTask();
	/**Perform the task.*/
	virtual void doIt() = 0;
	/**Whether the task had an error.*/
	int wasErr;
	/**The error message from the task.*/
	char* errMess;
};

/**Create and manage threads.*/
class OSThread{
public:
	/**
	 * Start a thread for the given task.
	 * @param toDo The task to do.
	 */
	OSThread(ThreadTask* toDo);
	/**Clean up.*/
	~OSThread();
	/**Join on the thread.*/
	void join();
	/**The handle to the thread, if not already joined.*/
	void* threadHandle;
	/**Save the task.*/
	ThreadTask* saveDo;
};

/**
 * Make a mutex for future use.
 * @return The created mutex.
 */
void* mutexMake();

/**
 * Get a lock.
 * @param toLock The lock to get.
 */
void mutexLock(void* toLock);

/**
 * Release a lock.
 * @param toUnlock The lock to get.
 */
void mutexUnlock(void* toUnlock);

/**
 * Delete a mutex.
 * @param toKill The lock to delete.
 */
void mutexKill(void* toKill);

/**A managed mutex.*/
class OSMutex{
public:
	/**Set up the mutex.*/
	OSMutex();
	/**Destroy the muted.*/
	~OSMutex();
	/**Lock the mutex.*/
	void lock();
	/**Unlock the mutex.*/
	void unlock();
	/**The mutex.*/
	void* myMut;
};

/**
 * Make a condition variable.
 * @param forMutex The lock to make it for.
 * @return The cretated condition variable.
 */
void* conditionMake(void* forMutex);

/**
 * Wait on a condition.
 * @param forMutex The relevant lock.
 * @param forCondition The condition in question.
 */
void conditionWait(void* forMutex, void* forCondition);

/**
 * Start one waiting on condition.
 * @param forMutex The relevant lock.
 * @param forCondition The condition in question.
 */
void conditionSignal(void* forMutex, void* forCondition);

/**
 * Start all waiting on condition.
 * @param forMutex The relevant lock.
 * @param forCondition The condition in question.
 */
void conditionBroadcast(void* forMutex, void* forCondition);

/**
 * Delete a condition variable.
 * @param forCondition The condition to kill.
 */
void conditionKill(void* forCondition);

/**A managed condition.*/
class OSCondition{
public:
	/**Make a condition around a mutex.*/
	OSCondition(OSMutex* baseMut);
	/**Destroy.*/
	~OSCondition();
	/**Wait on the condition.*/
	void wait();
	/**Signal one from the condition.*/
	void signal();
	/**Signal all from the condition.*/
	void broadcast();
	/**The mutex.*/
	void* saveMut;
	/**The condition.*/
	void* myCond;
};

/**
 * Load a dll.
 * @param loadFrom The name of the dll.
 * @return A handle to it.
 */
void* dllLoadIn(const char* loadFrom);

/**
 * Get the location of a loaded item in a dll.
 * @param fromDLL The dll to get from.
 * @param locName The name.
 */
void* dllGetLocation(void* fromDLL, const char* locName);

/**
 * Unload a dll.
 * @param toKill The dll to load.
 */
void dllUnload(void* toKill);

/**A managed dll.*/
class OSDLLSO{
public:
	/**
	 * Load the dll.
	 * @param dllName The name of the dll to load.
	 */
	OSDLLSO(const char* dllName);
	/**Unload.*/
	~OSDLLSO();
	/**
	 * Get a location from this dll.
	 * @param locName The name of the thing to get.
	 * @return The location, or null if not found.
	 */
	void* get(const char* locName);
	/**The loaded dll.*/
	void* myDLL;
};

/**Nothin from nothin leave nothin*/
#define WHODUN_SUBPROCESS_NULL 0
/**Get/send data to a file.*/
#define WHODUN_SUBPROCESS_FILE 1
/**Will pipe between two processes.*/
#define WHODUN_SUBPROCESS_PIPE 2

/**The options to pass to a sub-process*/
typedef struct{
	/**The name of the program to run.*/
	char* progName;
	/**The number of command line arguments.*/
	uintptr_t numArgs;
	/**The command line arguments to pass.*/
	char** progArgs;
	/**How to handle stdin.*/
	int stdinMeans;
	/**How to handle stdout.*/
	int stdoutMeans;
	/**How to handle stderr.*/
	int stderrMeans;
	/**The file to use for stdin, if relevant.*/
	char* stdinFile;
	/**The file to use for stdout, if relevant.*/
	char* stdoutFile;
	/**The file to use for stderr, if relevant.*/
	char* stderrFile;
} SubProcessArguments;

/**
 * Spin up a subprocess.
 * @param theArgs The information for the subprocess.
 * @return The subprocess handle.
 */
void* subprocStart(SubProcessArguments* theArgs);

/**
 * Note that nothing else is coming over stdin.
 * @param theProc The process in question.
 */
void subprocInFinish(void* theProc);

/**
 * Write to the standard input of a process.
 * @param theProc The process.
 * @param toW The bytes to write.
 * @param numW The number of bytes to write.
 */
void subprocInWrite(void* theProc, const char* toW, uintptr_t numW);

/**
 * Read from the standard output of a process.
 * @param theProc The process.
 * @param toR The place to store the bytes.
 * @param numR The number of bytes to read.
 * @return The number of bytes actually read.
 */
uintptr_t subprocOutRead(void* theProc, char* toR, uintptr_t numR);

/**
 * Read from the standard output of a process.
 * @param theProc The process.
 * @param toR The place to store the bytes.
 * @param numR The number of bytes to read.
 * @return The number of bytes actually read.
 */
uintptr_t subprocErrRead(void* theProc, char* toR, uintptr_t numR);

/**
 * Wait for the process to end.
 * @param theProc The process to wait on.
 * @return The return value of the process.
 */
int subprocWait(void* theProc);

/**Manage a subprocess.*/
class SubProcess{
public:
	/**
	 * Spin up a subprocess.
	 * @param theArgs The arguments to the subprocess.
	 */
	SubProcess(SubProcessArguments* theArgs);
	/**Clean up.*/
	~SubProcess();
	/**
	 * Wait on the process.
	 * @return The return code.
	 */
	int wait();
	
	/**Standard in for the process, if available.*/
	OutStream* stdIn;
	/**Standard out for the process, if available.*/
	InStream* stdOut;
	/**Standard error for the process, if available.*/
	InStream* stdErr;
	
	/**Save the process.*/
	void* saveProc;
	/**Whether it has waited.*/
	int haveWait;
	/**The return code from the process.*/
	int retCode;
};

}

#endif
