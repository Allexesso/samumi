#ifndef WHODUN_CACHE_H
#define WHODUN_CACHE_H 1

#include <set>
#include <vector>
#include <string.h>

#include "whodun_oshook.h"

namespace whodun {

/**Keep track of unique IDs.*/
class UniqueIDSet{
public:
	/**The next ID to return.*/
	uintptr_t seekID;
	/**The IDs in use.*/
	std::set<uintptr_t> liveIDs;
	/**Setup.*/
	UniqueIDSet();
	/**Teardown.*/
	~UniqueIDSet();
	/**
	 * Retire an ID.
	 * @param toKill The id to kill.
	 */
	void retireID(uintptr_t toKill);
	/**
	 * Get a new ID.
	 * @return The ID.
	 */
	uintptr_t reserveID();
};

/**A faster deque for structures/pod classes.*/
template <typename OfT>
class StructDeque{
public:
	/**
	 * Set up an empty circular array.
	 */
	StructDeque(){
		offset0 = 0;
		curSize = 0;
		arenaSize = 1;
		saveArena = (OfT*)malloc(arenaSize * sizeof(OfT));;
	}
	/**Clean up.*/
	~StructDeque(){
		if(saveArena){ free(saveArena); }
	}
	/**
	 * Get the number of things in the thing.
	 * @return The thing.
	 */
	size_t size(){
		return curSize;
	}
	/**
	 * Get an item.
	 * @param itemI The index to get.
	 * @return The relevant item.
	 */
	OfT* getItem(uintptr_t itemI){
		return saveArena + ((itemI + offset0) % arenaSize);
	}
	/**
	 * Get the number of things that can be pushed back in one go and still be contiguous.
	 * @return The number of things that can be pushed back and have all of them contiguous.
	 */
	uintptr_t pushBackSpan(){
		uintptr_t sizeMax = arenaSize - curSize;
		uintptr_t loopMax = arenaSize - ((offset0 + curSize)%arenaSize);
		return std::min(sizeMax, loopMax);
	}
	/**
	 * Mark some number of spots as occupied and get the address of the first thing.
	 * @param numPush THe number of spots to occupy.
	 * @return The address of the first item. Note that the items may be discontinuous.
	 */
	OfT* pushBack(uintptr_t numPush){
		//reallocate if necessary
		if((curSize + numPush) > arenaSize){
			uintptr_t numAlloc = std::max((arenaSize << 1) + 1, (curSize + numPush));
			uintptr_t origSize = curSize;
			OfT* newAlloc = (OfT*)malloc(numAlloc * sizeof(OfT));
			OfT* cpyAlloc = newAlloc;
			while(curSize){
				uintptr_t maxGet = popFrontSpan();
				OfT* curTgt = popFront(maxGet);
				memcpy(cpyAlloc, curTgt, maxGet * sizeof(OfT));
				cpyAlloc += maxGet;
			}
			if(saveArena){ free(saveArena); }
			saveArena = newAlloc;
			offset0 = 0;
			curSize = origSize;
			arenaSize = numAlloc;
		}
		//start pushing
		OfT* toRet = getItem(curSize);
		curSize += numPush;
		return toRet;
	}
	/**
	 * Note how many contiguous items can be popped from the front.
	 * @return The number of contiguous items at the front.
	 */
	uintptr_t popFrontSpan(){
		uintptr_t sizeMax = curSize;
		uintptr_t loopMax = arenaSize - offset0;
		return std::min(sizeMax, loopMax);
	}
	/**
	 * Pop some items from the front and get their memory address (it's still good until YOU do something).
	 * @param numPop The number to pop.
	 * @return The address of the first popped thing. Note that the items may be discontinuous.
	 */
	OfT* popFront(uintptr_t numPop){
		OfT* toRet = getItem(0);
		offset0 = (offset0 + numPop) % arenaSize;
		curSize -= numPop;
		return toRet;
	}
	/**The offset to the first item.*/
	uintptr_t offset0;
	/**The number of items currently present.*/
	uintptr_t curSize;
	/**The amount of space in the arena.*/
	uintptr_t arenaSize;
	/**The memory for the stuff.*/
	OfT* saveArena;
};

/**Allocate containers in a reusable manner: can cut down on allocation.*/
template <typename OfT>
class ReusableContainerCache{
public:
	/**Make an empty cache.*/
	ReusableContainerCache(){}
	/**Destroy the allocated stuff.*/
	~ReusableContainerCache(){
		for(uintptr_t i = 0; i<canReuse.size(); i++){
			delete(canReuse[i]);
		}
	}
	/**
	 * Allocate a container.
	 * @return The allocated container. Must be returned with dealloc.
	 */
	OfT* alloc(){
		OfT* toRet = 0;
		if(canReuse.size()){
			toRet = canReuse[canReuse.size()-1];
			canReuse.pop_back();
		}
		else{
			toRet = new OfT();
		}
		return toRet;
	}
	
	/**
	 * Return a container.
	 * @param toDe The container to return.
	 */
	void dealloc(OfT* toDe){
		canReuse.push_back(toDe);
	}
	
	/**All the containers waiting to be reused.*/
	std::vector<OfT*> canReuse;
};

/**Allocate containers in a threadsafe, reusable manner.*/
template <typename OfT>
class ThreadsafeReusableContainerCache{
public:
	/**Get the lock ready.*/
	ThreadsafeReusableContainerCache(){}
	/**Destroy the allocated stuff.*/
	~ThreadsafeReusableContainerCache(){}
	/**
	 * Allocate a container.
	 * @return The allocated container. Must be returned with dealloc.
	 */
	OfT* alloc(){
		myMut.lock();
		OfT* toRet = actCache.alloc();
		myMut.unlock();
		return toRet;
	}
	/**
	 * Return a container.
	 * @param toDe The container to return.
	 */
	void dealloc(OfT* toDe){
		myMut.lock();
		actCache.dealloc(toDe);
		myMut.unlock();
	}
	
	/**The mutex for this thing.*/
	OSMutex myMut;
	/**The actual cache.*/
	ReusableContainerCache<OfT> actCache;
};

/**Like std::pair.*/
template <typename TA, typename TB, typename TC>
class triple{
public:
	/**The first thing.*/
	TA first;
	/**The second thing.*/
	TB second;
	/**The third thing.*/
	TC third;
	
	/**Set up an uniniailized tuple.*/
	triple(){}
	
	/**
	 * Set up a tuple with the given values.
	 * @param myF The first value.
	 * @param myS The second value.
	 * @param myT The third value.
	 */
	triple(TA myF, TB myS, TC myT){
		first = myF;
		second = myS;
		third = myT;
	}
	
	/**Compare element by element.*/
	bool operator < (const triple<TA,TB,TC>& compTo) const{
		if(first < compTo.first){ return true; }
		if(compTo.first < first){ return false; }
		if(second < compTo.second){ return true; }
		if(compTo.second < second){ return false; }
		if(third < compTo.third){ return true; }
		return false;
	}
};

}

#endif