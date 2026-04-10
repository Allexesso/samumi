#ifndef WHODUN_ARGS_H
#define WHODUN_ARGS_H 1

#include <map>
#include <set>
#include <string>
#include <vector>
#include <iostream>

#include "whodun_string.h"
#include "whodun_stat_table.h"

namespace whodun {

/**Align pairs of linear sequences.*/
class LinearPairwiseSequenceAlignment{
public:
	/**Basic setup.*/
	LinearPairwiseSequenceAlignment();
	/**Subclass cleanup.*/
	virtual ~LinearPairwiseSequenceAlignment();
	
	/**
	 * Prepare alignment data for a new pair of strings.
	 * @param refString The reference sequence.
	 * @param readString The read string.
	 */
	virtual void alignSequences(SizePtrString refString, SizePtrString readString) = 0;
	
	/**
	 * This will find some number of alignment scores.
	 * @param numFind The maximum number of scores to look for.
	 * @param storeCost The place to put the results.
	 * @param minScore The minimum score to entertain.
	 * @param maxDupDeg The maximum number of times to hit a duplicate degraded score. Zero for no such limit.
	 * @return The number of found scores.
	 */
	int findAlignmentScores(int numFind, intptr_t* storeCost, intptr_t minScore, intptr_t maxDupDeg);
	/**
	 * Start an iteration through the optimal alignments.
	 */
	virtual void startOptimalIteration() = 0;
	/**
	 * (Re)Start a non-optimal iteration.
	 * @param minimumScore The minimum score to accept.
	 * @param maxDupDeg The maximum number of times to hit a duplicate degraded score. Zero for no such limit.
	 * @param maxNumScore The maximum number of scores to keep track of: if more are encountered, the minimum score will rise.
	 */
	virtual void startFuzzyIteration(intptr_t minimumScore, intptr_t maxDupDeg, intptr_t maxNumScore) = 0;
	/**
	 * Get the next alignment.
	 * @return Whether there was an alignment.
	 */
	virtual int getNextAlignment() = 0;
	/**
	 * Update the new minimum score.
	 * @param newMin The new minimum score.
	 */
	virtual void updateMinimumScore(intptr_t newMin) = 0;
	
	/**The characters of the first (reference) sequence.*/
	SizePtrString seqA;
	/**The characters of the second sequence.*/
	SizePtrString seqB;
	/**The score of the most recent alignment.*/
	intptr_t alnScore;
	/**The length of the alignment indices.*/
	uintptr_t alnLength;
	/**The visited indices in sequence A.*/
	intptr_t* aInds;
	/**The visited indices in sequence B.*/
	intptr_t* bInds;
};

//TODO edit distance type alignment

/**An affine gap cost.*/
class AlignCostAffine{
public:
	/**Set up an empty cost.*/
	AlignCostAffine();
	/**Free any allocations.*/
	~AlignCostAffine();
	/**The costs of each interaction pair.*/
	int** allMMCost;
	/**The number of character entries in the cost table.*/
	int numLiveChars;
	/**The cost to open a gap in the reference (deletion).*/
	int openRefCost;
	/**The cost to extend a gap in the reference.*/
	int extendRefCost;
	/**The cost to close a gap in the reference.*/
	int closeRefCost;
	/**The cost to open a gap in the read (insertion).*/
	int openReadCost;
	/**The cost to extend a gap in the read.*/
	int extendReadCost;
	/**The cost to close a gap in the read.*/
	int closeReadCost;
	/**Map from character values to indices.*/
	short charMap[256];
};

/**Output for debug.*/
std::ostream& operator<<(std::ostream& os, const AlignCostAffine& toOut);

//TODO uniform affine gap cost alignment

/**A focus point in an iteration.*/
typedef struct{
	/**The i index.*/
	intptr_t focI;
	/**The j index.*/
	intptr_t focJ;
	/**The directions left to consider.*/
	intptr_t liveDirs;
	/**The score for this path.*/
	intptr_t pathScore;
	/**How this location was approached.*/
	intptr_t howGot;
	/**Whether the default path for this entry has been hit.*/
	intptr_t seenDef;
} AGLPFocusStackEntry;

/**The cost at a position.*/
typedef struct{
	/**The cost to eat the base.*/
	int eatCost;
	/**The cost to open a gap at the position (deletion).*/
	int openRefCost;
	/**The cost to extend a gap at the position.*/
	int extendRefCost;
	/**The cost to close a gap at the position.*/
	int closeRefCost;
	/**The cost to open a gap at the position (insertion).*/
	int openReadCost;
	/**The cost to extend a gap at the position.*/
	int extendReadCost;
	/**The cost to close a gap at the position.*/
	int closeReadCost;
} PositionCost;

/**Costs managled for each relevant position.*/
class PositionalCostTable{
public:
	/**Set up an empty table.*/
	PositionalCostTable();
	/**Clean up.*/
	~PositionalCostTable();
	/**The number of bases in the reference sequence.*/
	uintptr_t numCutsRef;
	/**The number of bases in the read sequence.*/
	uintptr_t numCutsRead;
	/**The grid of costs: (numCutsRef+1) * (numCutsRead+1).*/
	PositionCost* allCosts;
	
	/**The amount of space allocated for the cost packing.*/
	uintptr_t allocSpace;
	/**
	 * Allocate space.
	 * @param numRef The number of cuts in the reference.
	 * @param numRead The number of cuts in the read.
	 */
	void allocate(uintptr_t numRef, uintptr_t numRead);
};

/**Costs specified along a reference.*/
class ReferencePositionalCostTable{
public:
	/**Set up an empty set of costs.*/
	ReferencePositionalCostTable();
	/**Tear down.*/
	~ReferencePositionalCostTable();
	
	/**All the costs to look over.*/
	std::vector<AlignCostAffine> allCosts;
	/**The relevant ranges for those costs.*/
	std::vector< triple<intptr_t,intptr_t,uintptr_t> > allRanges;
	
	/**
	 * Parse from a tsv.
	 * @param numEntry The number of entries.
	 * @param theEnts The entries.
	 */
	void parse(uintptr_t numEntry, TextTableRow** theEnts);
	/**
	 * Fill in a cost table.
	 * @param toFill The table to fill.
	 * @param refOffset The offset of the first character in the reference.
	 * @param refString The reference string.
	 * @param readOffset The offset of the first character in the read.
	 * @param readString The read string.
	 * @param defCost The default cost.
	 */
	void fill(PositionalCostTable* toFill, uintptr_t refOffset, SizePtrString refString, uintptr_t readOffset, SizePtrString readString, AlignCostAffine* defCost);
};

/**A linear pairwise sequence alignment with position dependent cost.*/
class PositionDependentCostLPSA : LinearPairwiseSequenceAlignment{
public:
	/**
	 * Basic setup.
	 * @param numSeqEnds The type of alignment to produce: 0 for local, 2 for semi-global and 4 for global.
	 * @param useCost The costs to use for the alignment.
	 */
	PositionDependentCostLPSA(int numSeqEnds, PositionalCostTable* useCost);
	/**Cleanup.*/
	~PositionDependentCostLPSA();
	
	/**
	 * Prepare alignment data for a new pair of strings, with new setup.
	 * @param refString The reference sequence.
	 * @param readString The read string.
	 * @param numSeqEnds The type of alignment to produce: 0 for local, 2 for semi-global and 4 for global.
	 * @param useCost The costs to use for the alignment.
	 */
	void alignSequences(SizePtrString refString, SizePtrString readString, int numSeqEnds, PositionalCostTable* useCost);
	void alignSequences(SizePtrString refString, SizePtrString readString);
	void startOptimalIteration();
	void startFuzzyIteration(intptr_t minimumScore, intptr_t maximumDupDeg, intptr_t maximumNumScore);
	int getNextAlignment();
	void updateMinimumScore(intptr_t newMin);
private:
	/**The type of alignment to make.*/
	int numEnds;
	/**The costs for the alignment.*/
	PositionalCostTable* cost;
	/**The allocated cost table.*/
	intptr_t** costTable;
	/**The size of the saved allocation.*/
	uintptr_t saveSize;
	
	/**The minimum score to entertain.*/
	intptr_t minScore;
	/**The maximum number of times to entertain a duplicate score: zero for infinite.*/
	intptr_t maxDupDeg;
	/**The maximum number of scores to keep track of.*/
	intptr_t maxNumScore;
	/**The scores.*/
	std::vector<intptr_t> allScore;
	/**The number of times each has been seen.*/
	std::vector<intptr_t> allScoreSeen;
	/**The waiting starting positions.*/
	std::vector< AGLPFocusStackEntry > waitingStart;
	/**The size of the alignment stack.*/
	intptr_t alnStackSize;
	/**The alignment stack.*/
	AGLPFocusStackEntry* alnStack;
	/**The score for ending at the current location. Always valid, though not always useful.*/
	intptr_t dieHereScore;
	/**Whether the last move involved a path change (initial state, push to new score, or push to another path with same score).*/
	bool lastIterChange;
	/**The number of bytes allocated for the stack.*/
	uintptr_t alnStackAlloc;
	/**Find all the starting locations, in ascending order of score.*/
	void findStartingLocations();
	/**
	 * Returns whether there are any more paths.
	 * @return Whether there is still a path being looked at.
	 */
	bool hasPath();
	/**
	 * Returns whether the current path has hit its end.
	 * @return Whether the current path is terminal.
	 */
	bool pathTerminal();
	/**
	 * Get the score of the current path.
	 * @return The score of the current path.
	 */
	intptr_t currentPathScore();
	/**
	 * Get the score of the current path, if it ends here. Only useful for local alignment.
	 * @return The score of the current path.
	 */
	intptr_t currentPathEndScore();
	/**
	 * Move forward to the next path step.
	 */
	void iterateNextPathStep();
	/**
	 * The current path does not match some specification, drop it. Also calls iterateNextPathStep.
	 */
	void abandonPath();
	/**
	 * This will dump out the current path.
	 * @param aInds The place to store the one-based indices in sequence a.
	 * @param bInds The place to store the one-based indices in sequence b.
	 * @return The number of entries in said path.
	 */
	void dumpPath();
	/**
	 * Gets whether the last iteration changed the path being followed..
	 * @return Whether a new starting location was added, a lower score was encountered, or a duplicate score was hit.
	 */
	bool lastIterationChangedPath();
};

}

#endif