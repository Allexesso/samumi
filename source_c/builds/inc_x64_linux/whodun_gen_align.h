#ifndef WHODUN_GEN_ALIGN_H
#define WHODUN_GEN_ALIGN_H 1

#include <set>
#include <map>
#include <deque>
#include <stdint.h>

#include "whodun_cache.h"
#include "whodun_oshook.h"
#include "whodun_string.h"

namespace whodun {

#define WHODUN_CHANGE_MUTATE 1
#define WHODUN_CHANGE_INSERT 2
#define WHODUN_CHANGE_DELETE 3

/**Information on a sequence change.*/
typedef struct{
	/**The index this occurs before.*/
	uintptr_t seqInd;
	/**The type of thing.*/
	int changeType;
	union{
		/**Mutation data.*/
		struct{
			/**The character to mutate to.*/
			char mutTo;
		} mut;
		/**Insertion data.*/
		struct{
			/**The string to insert.*/
			SizePtrString insStr;
		} ins;
		/**Deletion data.*/
		struct{
			/**The number of characters to delete.*/
			int numDel;
		} del;
	};
} SequenceChangeEntry;

/**A set of changes.*/
typedef struct{
	/**The number of changes to make.*/
	uintptr_t numChange;
	/**The changes to make.*/
	SequenceChangeEntry* theChange;
	/**The probability the changed set properly represents the truth, given the whole thing isn't spurious.*/
	double log10AltRight;
} SequenceChangeSet;

/**Information on an alignment.*/
typedef struct{
	/**The index of the relevant sequence changes*/
	uintptr_t changeI;
	/**The probability of mutating the reference sequence to this sequence.*/
	double log10PSeqFromRef;
	/**The probability of this particular alignment (without regard for the other reads, or the probability of the read itself).*/
	double log10PAlign;
	/**The alignment score.*/
	intptr_t alignScore;
	/**Whether each base is unrelated to the reference. One bit per base.*/
	char* isInsert;
	/**The index each base maps to.*/
	uintptr_t* alignBases;
} SequenceAlignmentEntry;

/**A set of a alignments.*/
class SequenceAlignmentSet{
public:
	/**Set up an empty set.*/
	SequenceAlignmentSet();
	/**Clean up.*/
	~SequenceAlignmentSet();
	
	/**The name of these sequences.*/
	SizePtrString queryName;
	
	/**The number of sequences.*/
	uintptr_t numSeqs;
	/**The sequences (usually as they came off the sequencer).*/
	SizePtrString* sequences;
	/**The probability this set of sequences is spurious.*/
	double log10SeqSpurious;
	/**The probability each sequence misrepresents the truth (given it's not spurious).*/
	double* log10SeqWrong;
	/**The number of alternate sequences for each sequence.*/
	uintptr_t* numAlts;
	/**The changes sets for the alternative sequences.*/
	SequenceChangeSet** altSeqs;
	
	/**The number of tag sequences.*/
	uintptr_t numTags;
	/**The names of each tag.*/
	SizePtrString* tagNames;
	/**The tag sequences.*/
	SizePtrString* tags;
	/**The probability each tag misrepresents the truth.*/
	double* log10TagWrong;
	/**The number of alternate sequences for each tag.*/
	uintptr_t* numAltTag;
	/**The changes sets for the alternative tag.*/
	SequenceChangeSet** altTags;
	
	/**The number of alignment sets.*/
	uintptr_t numAligns;
	/**The alignments: each alignment has one entry per sequence.*/
	SequenceAlignmentEntry** allAligns;
	/**The joint probabilities of the alignment positioning.*/
	double* log10AlignJointProb;
	
	/**The number of names/mark data.*/
	uintptr_t numMarks;
	/**Extra mark data.*/
	SizePtrString* markData;
	
	std::vector<char> allocStr;
	std::vector<double> allocDbl;
	std::vector<uintptr_t> allocInt;
	std::vector<SizePtrString> allocSps;
	std::vector<SequenceChangeEntry> allocSce;
	std::vector<SequenceChangeSet> allocScs;
	std::vector<SequenceChangeSet*> allocScsP;
	std::vector<SequenceAlignmentEntry> allocSae;
	std::vector<SequenceAlignmentEntry*> allocSaeP;
};

//TODO read, write, randacc, sort on location, sort on qname, sort on specific tag, sort on data

}

#endif