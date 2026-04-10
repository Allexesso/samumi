#ifndef WHODUN_THREAD_H
#define WHODUN_THREAD_H 1

#include <set>
#include <map>
#include <deque>
#include <stdint.h>

#include "whodun_cache.h"
#include "whodun_oshook.h"

namespace whodun {

class ThreadPool;

/**The actual task to run for thread pool.*/
class ThreadPoolLoopTask : public ThreadTask{
public:
	void doIt();
	/**Remember the actual thread pool.*/
	ThreadPool* mainPool;
};

/**Internal storage for a task.*/
typedef struct{
	/**The task to do.*/
	ThreadTask* taskDo;
	/**The condition to notify on completion.*/
	OSCondition* waitCond;
	/**Whether the task has been finished.*/
	int isDone;
	/**Whether the task has been joined.*/
	int hasJoin;
} ThreadPoolTaskInfo;

/**A pool of reusable threads.*/
class ThreadPool{
public:
	/**
	 * Set up the threads.
	 * @param numThread The number of threads.
	 */
	ThreadPool(int numThread);
	/**Kill the threads.*/
	~ThreadPool();
	/**
	 * Add a task to run.
	 * @param toDo The thing to do.
	 * @return The ID of the task. Will need to join at some point.
	 */
	uintptr_t addTask(ThreadTask* toDo);
	/**
	 * Wait for a task to finish.
	 * @param taskID The ID of the task.
	 */
	void joinTask(uintptr_t taskID);
	/**
	 * Add multiple tasks in one go.
	 * @param numAdd The number of tasks to add.
	 * @param toDo The tasks to add.
	 * @return The ID of the first task: the IDs are sequential.
	 */
	uintptr_t addTasks(uintptr_t numAdd, ThreadTask** toDo);
	/**
	 * Join multiple tasks in one go.
	 * @param fromID The ID of the first task.
	 * @param toID The ID after the last task.
	 */
	void joinTasks(uintptr_t fromID, uintptr_t toID);
	
	/**Call at the start of a bulk add/join.*/
	void bigStart();
	/**
	 * Add a task to run as part of a bulk operation.
	 * @param toDo The thing to do.
	 * @return The ID of the task
	 */
	uintptr_t bigAdd(ThreadTask* toDo);
	/**
	 * Wait for a task to finish as part of a bulk operation.
	 * @param taskID The ID of the task.
	 */
	void bigJoin(uintptr_t taskID);
	/**Whether any calls were made to join in the bulk operation.*/
	int bigAnyJoined;
	/**Whether any calls were made to add in the bulk operation.*/
	int bigAnyAdded;
	/**Call at the end of a bulk operation.*/
	void bigEnd();
	
	/**Whether the pool is live.*/
	bool poolLive;
	/**The number of threads in this pool.*/
	int numThr;
	/**The task mutex.*/
	OSMutex taskMut;
	/**The task conditions.*/
	OSCondition taskCond;
	
	/**The offset of the openTasks[0].*/
	uintptr_t idOffset;
	/**The next ID to do.*/
	uintptr_t nextDoID;
	/**All the tasks on the thing.*/
	StructDeque<ThreadPoolTaskInfo> openTasks;
	/**Avoid allocating many conditions.*/
	std::vector<OSCondition*> saveConds;
	
	/**The real task uniform storage.*/
	std::vector<ThreadPoolLoopTask> uniStore;
	/**The live threads.*/
	std::vector<OSThread*> liveThread;
};

/**Collect stuff between threads.*/
template <typename OfT>
class ThreadProdComCollector{
public:
	/**Whether end has been called.*/
	bool queueEnded;
	/**The maximum size of the queue.*/
	uintptr_t maxQueue;
	/**Lock for edit.*/
	OSMutex myMut;
	/**Wait for more things.*/
	OSCondition myConMore;
	/**Wait for less things.*/
	OSCondition myConLess;
	/**Things added and waiting to be handled.*/
	std::deque<OfT*> waitQueue;
	/**Protect by a lock.*/
	ThreadsafeReusableContainerCache<OfT> taskCache;
	
	/**
	 * Set up a producer consumer thing.
	 * @param maxQueueSize The maximum number of things in the queue.
	 */
	ThreadProdComCollector(uintptr_t maxQueueSize) : myConMore(&myMut), myConLess(&myMut){
		queueEnded = false;
		maxQueue = maxQueueSize;
	}
	
	~ThreadProdComCollector(){
		for(uintptr_t i = 0; i<waitQueue.size(); i++){ taskCache.dealloc(waitQueue[i]); }
	}
	
	/**
	 * Add the thing to the job list.
	 * @param toAdd The thing to add.
	 */
	void addThing(OfT* toAdd){
		myMut.lock();
			while(waitQueue.size() >= maxQueue){ if(queueEnded){ break; } myConLess.wait(); }
			waitQueue.push_back(toAdd);
			myConMore.signal();
		myMut.unlock();
	}
	
	/**
	 * Get a thing.
	 * @return The got thing: null if end called and nothing left.
	 */
	OfT* getThing(){
		myMut.lock();
			while(waitQueue.size() == 0){ if(queueEnded){ break; } myConMore.wait(); }
			OfT* nxtThing = 0;
			if(waitQueue.size()){
				nxtThing = waitQueue[0];
				waitQueue.pop_front();
				myConLess.signal();
			}
		myMut.unlock();
		return nxtThing;
	}
	
	/**
	 * Get a thing, if any.
	 * @return The got thing.
	 */
	OfT* tryThing(){
		myMut.lock();
			OfT* nxtThing = 0;
			if(waitQueue.size()){
				nxtThing = waitQueue[0];
				waitQueue.pop_front();
				myConLess.signal();
			}
		myMut.unlock();
		return nxtThing;
	}
	
	/**Wake up ALL waiters, let all know things are ending.*/
	void end(){
		if(queueEnded){ return; }
		myMut.lock();
			queueEnded = true;
			myConMore.broadcast();
			myConLess.broadcast();
		myMut.unlock();
	}
};

class ParallelForLoop;

/**Manages threading for a parallel for loop.*/
class ParallelForLoopUniform : public ThreadTask{
public:
	void doIt();
	/**The main loop.*/
	ParallelForLoop* mainLoop;
	/**The index to run from.*/
	uintptr_t fromI;
	/**The index to run to.*/
	uintptr_t toI;
	/**The thread this is for.*/
	uintptr_t threadInd;
};

/**Run a for loop in parallel.*/
class ParallelForLoop{
public:
	/**
	 * Set up.
	 * @param numThread The number of threads to use.
	 */
	ParallelForLoop(uintptr_t numThread);
	/**Allow subclasses to tear down.*/
	virtual ~ParallelForLoop();
	/**
	 * Actually run the stupid thing.
	 * @param inPool The pool to run in.
	 */
	void doIt(ThreadPool* inPool);
	/**
	 * Run a range of indices.
	 * @param threadInd The thread this is for.
	 * @param fromI The index to start at.
	 * @param toI The index to run to.
	 */
	virtual void doRange(uintptr_t threadInd, uintptr_t fromI, uintptr_t toI);
	/**
	 * Run a single index.
	 * @param threadInd The thread this is for.
	 * @param ind The index to run.
	 */
	virtual void doSingle(uintptr_t threadInd, uintptr_t ind) = 0;
	/**The threads to run.*/
	std::vector<ParallelForLoopUniform> allUni;
	/**The thing to actually pass.*/
	std::vector<ThreadTask*> passUni;
	/**The index to run from.*/
	uintptr_t startI;
	/**The index to run to*/
	uintptr_t endI;
};

/**Allow many things to read, but only one to write.*/
class ReadWriteLock{
public:
	/**Setup*/
	ReadWriteLock();
	/**Teardown*/
	~ReadWriteLock();
	/**Lock for reading.*/
	void lockRead();
	/**Unlock for reading.*/
	void unlockRead();
	/**Lock for writing.*/
	void lockWrite();
	/**Unlock for writing.*/
	void unlockWrite();
	/**The number of things reading.*/
	uintptr_t numR;
	/**The number of things waiting to write.*/
	uintptr_t numW;
	/**For counts.*/
	OSMutex countLock;
	/**For writers.*/
	OSCondition waitW;
	/**For readers.*/
	OSCondition waitR;
};

}

#endif