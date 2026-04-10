#ifndef SAMUMI_H
#define SAMUMI_H 1

#include "whodun_args.h"
#include "whodun_thread.h"
#include "whodun_stat_table.h"
#include "whodun_gen_sequence.h"
#include "whodun_gen_table_align.h"

/**Parse arguments for a samumi task.*/
class SamumiArgumentParser : public whodun::ArgumentParser{
public:
	/**Allow subclasses.*/
	virtual ~SamumiArgumentParser();
	/**Run the stupid thing.*/
	virtual void runThing() = 0;
	
	/**A one line summary.*/
	const char* mySummary;
};

/**Convert formats for a fasta file.*/
class FuzzFinderArgumentParser : public SamumiArgumentParser{
public:
	/**Basic setup.*/
	FuzzFinderArgumentParser();
	/**Tear down.*/
	~FuzzFinderArgumentParser();
	int posteriorCheck();
	void runThing();
	
	/**The output file to write to.*/
	char* outFileN;
	/**The input file to read from.*/
	char* inFileN;
	/**The name of the forward strand file.*/
	char* inForwN;
	/**The name of the backward strand file.*/
	char* inBackN;
	/**Whether the fastq entries are interleaved.*/
	bool fastqInter;
	/**The name of the primer file.*/
	char* primerFileN;
	/**The common sequence.*/
	char* commonSeq;
	/**The number of bases for the umi.*/
	intptr_t umiLen;
	/**The maximum number of differences in the primers.*/
	intptr_t primerFuzz;
	/**The maximum number of differences in the anchors.*/
	intptr_t anchorFuzz;
	/**The maximum number of differences in the common sequence.*/
	intptr_t commonFuzz;
	/**The number of threads to use.*/
	intptr_t numThread;
	/**The number of sequences to load per thread.*/
	intptr_t threadGrain;
	
	/**The reverse complement of the common sequence*/
	std::vector<char> commonSeqR;
	
	/**Reliable storage for the name for stdout/stdin*/
	char defOutFN[2];
};

/**Pack umi families.*/
class FamilyPackArgumentParser : public SamumiArgumentParser{
public:
	/**Basic setup.*/
	FamilyPackArgumentParser();
	/**Tear down.*/
	~FamilyPackArgumentParser();
	int posteriorCheck();
	void runThing();
	
	/**The output file to write to.*/
	char* outFileN;
	/**The input file to read from.*/
	char* inFileN;
	/**The number of threads to use.*/
	intptr_t numThread;
	/**The number of sequences to load per thread.*/
	intptr_t threadGrain;
	
	/**Reliable storage for the name for stdout/stdin*/
	char defOutFN[2];
};

/**Merge nearby UMI families.*/
class UmiHammingMergeArgumentParser : public SamumiArgumentParser{
public:
	/**Basic setup.*/
	UmiHammingMergeArgumentParser();
	/**Tear down.*/
	~UmiHammingMergeArgumentParser();
	int posteriorCheck();
	void runThing();
	
	/**The output file to write to.*/
	char* outFileN;
	/**The input file to read from.*/
	char* inFileN;
	/**The name of the file to write dumped items to.*/
	char* dumpFileN;
	/**The number of threads to use.*/
	intptr_t numThread;
	/**The number of sequences to load per thread.*/
	intptr_t threadGrain;
	/**The maximum haming distance to merge.*/
	intptr_t mergeHam;
	/**The ambiguity mode (multiple UMI in range).*/
	intptr_t umiAmbigMode;
	/**How to handle different haplotypes in a merge.*/
	intptr_t hapAmbigMode;
	
	/**Reliable storage for the name for stdout/stdin*/
	char defOutFN[2];
};

/**Summarize UMI families.*/
class UmiSummarizeArgumentParser : public SamumiArgumentParser{
public:
	/**Basic setup.*/
	UmiSummarizeArgumentParser();
	/**Tear down.*/
	~UmiSummarizeArgumentParser();
	int posteriorCheck();
	void runThing();
	
	/**The output file to write to.*/
	char* outFileN;
	/**The input file to read from.*/
	char* inFileN;
	/**The number of threads to use.*/
	intptr_t numThread;
	/**The number of sequences to load per thread.*/
	intptr_t threadGrain;
	/**The maximum haming distance to note.*/
	intptr_t mergeHam;
	/**The type of this run.*/
	char* readTagInfo;
	/**The name of the primer file.*/
	char* primerFileN;
	
	/**Reliable storage for the name for stdout/stdin*/
	char defOutFN[2];
};

/**Take a header and figure out what goes where.*/
class TSVHeaderMap{
public:
	/**
	 * Create the map from the header line.
	 * @param headLine The header line.
	 * @param allNeed The needed names.
	 */
	TSVHeaderMap(whodun::TextTableRow* headLine);
	/**Clean up.*/
	~TSVHeaderMap();
	/**The location of everything in the header.*/
	std::map<std::string,uintptr_t> headOrder;
};

/**Store primer data.*/
class PrimerData{
public:
	/**Set up*/
	PrimerData();
	/**Tear down.*/
	~PrimerData();
	/**
	 * Initialize from a tsv entry.
	 * @param tabD The tsv to load from.
	 */
	void parsePrimerData(whodun::TextTableRow* loadF);
	/**The name of the locus.*/
	std::vector<char> locus;
	/**The chromosome this thing is on.*/
	std::vector<char> chrom;
	/**The position of the stupid thing.*/
	uintptr_t position;
	/**Whether it is reverse complemented (i.e. read 1 is backwards).*/
	int isRC;
	/**The primer sequence (on the + strand).*/
	std::vector<char> primer;
	/**The anchor sequence (on the + strand).*/
	std::vector<char> anchor;
	/**The motif size.*/
	uintptr_t period;
	/**The reverse complemented primer.*/
	std::vector<char> primerRC;
	/**The reverse complemented anchor.*/
	std::vector<char> anchorRC;
	/**Do the reverse complement.*/
	whodun::GeneticSequenceUtilities doRC;
};
/**
 * Output primer data.
 * @param os The place to write.
 * @param toOut The primer to write.
 * @return the stream.
 */
std::ostream& operator<<(std::ostream& os, PrimerData const & toOut);
/**A set of primers.*/
class PrimerSet{
public:
	/**Make an empty primer set.*/
	PrimerSet();
	/**
	 * Parse from a file..
	 * @param fileName The file to load from.
	 */
	PrimerSet(const char* fileName);
	/**
	 * Parse.
	 * @param loadFrom The thing to load from.
	 */
	PrimerSet(whodun::TextTableReader* loadFrom);
	/**Clean up.*/
	~PrimerSet();
	/**
	 * Parse.
	 * @param loadFrom The thing to load from.
	 */
	void parse(whodun::TextTableReader* loadFrom);
	/**All the primers.*/
	std::vector<PrimerData> allPrimer;
};

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

/**Information on a single read.*/
class EntryReadData{
public:
	/**Set up.*/
	EntryReadData();
	/**Tear down.*/
	~EntryReadData();
	/**Empty the entry.*/
	void clear();
	/**
	 * The table row to parse.
	 * @param toParse The row to parse.
	 */
	void parse(whodun::TextTableRow* toParse);
	/**
	 * Dump to a row.
	 * @param toDump The place to dump.
	 */
	void dump(whodun::TextTableRow* toDump);
	/**The name of the query.*/
	std::vector<char> qname;
	/**The canonical primer.*/
	std::vector<char> canonPrimer;
	/**The locus name.*/
	std::vector<char> canonLocus;
	/**The canonical umi*/
	std::vector<char> canonUMI;
	/**The canonical umi*/
	std::vector<char> canonSequence;
	/**The sequence.*/
	std::vector<char> sequence;
	/**The quality phred scores.*/
	std::vector<char> sequencePhred;
	/**The umi.*/
	std::vector<char> umi;
	/**The quality phred scores.*/
	std::vector<char> umiPhred;
	/**The primer.*/
	std::vector<char> primer;
	/**The quality phred scores.*/
	std::vector<char> primerPhred;
	/**The anchor.*/
	std::vector<char> anchor;
	/**The quality phred scores.*/
	std::vector<char> anchorPhred;
	/**The common sequence.*/
	std::vector<char> common;
	/**The quality phred scores.*/
	std::vector<char> commonPhred;
};

/**Parse in bulk.*/
class EntryReadDataBulkParse : public whodun::ParallelForLoop{
public:
	/**
	 * Set up.
	 * @param numThread The number of threads to use.
	 */
	EntryReadDataBulkParse(uintptr_t numThread);
	void doSingle(uintptr_t threadInd, uintptr_t ind);
	/**All the things to parse.*/
	whodun::TextTableRow** allParse;
	/**All the things to parse to.*/
	EntryReadData** allStore;
};

/**Parse in bulk.*/
class EntryReadDataBulkDump : public whodun::ParallelForLoop{
public:
	/**
	 * Set up.
	 * @param numThread The number of threads to use.
	 */
	EntryReadDataBulkDump(uintptr_t numThread);
	void doSingle(uintptr_t threadInd, uintptr_t ind);
	/**All the things to parse.*/
	whodun::TextTableRow** allParse;
	/**All the things to parse to.*/
	EntryReadData** allStore;
};

/**
 * Read every entry read from the given file.
 * @param fileName The file to read.
 * @param allData The place to put the read data.
 * @param numThread The number of threads to use.
 * @param useThread The threads to use.
 * @param threadGrain The numebr of things to load per thread.
 */
void fullReadEntryReadData(const char* fileName, std::vector<EntryReadData*>* allData, uintptr_t numThread, whodun::ThreadPool* useThread, uintptr_t threadGrain);

/**
 * Write every entry read to the given file.
 * @param fileName The file to read.
 * @param allData The place to put the read data.
 * @param numThread The number of threads to use.
 * @param useThread The threads to use.
 * @param threadGrain The numebr of things to load per thread.
 */
void fullWriteEntryReadData(const char* fileName, std::vector<EntryReadData*>* allData, uintptr_t numThread, whodun::ThreadPool* useThread, uintptr_t threadGrain);

/**The identifier for a UMI family.*/
typedef std::pair<whodun::SizePtrString,whodun::SizePtrString> UmiFamilyMarker;
/**A umi family broken down by allele.*/
typedef whodun::triple<whodun::SizePtrString,whodun::SizePtrString,whodun::SizePtrString> PrimerUmiAlleleMarker;

/**
 * Only compare primer.
 * @param entA The first entry.
 * @param entB The second entry.
 * @return Whether entA is less than entB.
 */
bool primerOnlyCompareFunc(const PrimerUmiAlleleMarker& entA, const PrimerUmiAlleleMarker& entB);

/**
 * Only compare umi.
 * @param entA The first entry.
 * @param entB The second entry.
 * @return Whether entA is less than entB.
 */
bool umiOnlyCompareFunc(const PrimerUmiAlleleMarker& entA, const PrimerUmiAlleleMarker& entB);

/**
 * Compare primer and umi, but not haplotype.
 * @param entA The first entry.
 * @param entB The second entry.
 * @return Whether entA is less than entB.
 */
bool familyOnlyCompareFunc(const PrimerUmiAlleleMarker& entA, const PrimerUmiAlleleMarker& entB);

/**
 * Get the hamming distance between two strings.
 * @param strA The first string.
 * @param strB The second string.
 * @return The Hamming distance.
 */
uintptr_t hammingDistance(whodun::SizePtrString strA, whodun::SizePtrString strB);

/**Compare a specific character in the UMI.*/
class UmiCharacterCompareFunc{
public:
	/**The index to compare.*/
	uintptr_t charInd;
	/**The comparison method.*/
	bool operator()(const PrimerUmiAlleleMarker& entA, const PrimerUmiAlleleMarker& entB);
};

/**Compartmentalize the UMI neighborhood.*/
class UmiFamilyNeighborhood{
public:
	/**
	 * Set up the neighborhood.
	 * @param numReads The number of reads.
	 * @param allReads All the reads.
	 * @param numThread The number of threads to use.
	 * @param useThread The threads to use.
	 */
	UmiFamilyNeighborhood(uintptr_t numReads, EntryReadData** allReads, uintptr_t numThread, whodun::ThreadPool* useThread);
	/**Clean up.*/
	~UmiFamilyNeighborhood();
	/**
	 * Find UMI entries within a given range.
	 * @param primer The primer to look within.
	 * @param onUmi The umi to look around.
	 * @param maxHam The maximum haming distance.
	 * @param storeInds The place to put the found indices.
	 */
	void findUmiWithinRange(whodun::SizePtrString primer, whodun::SizePtrString onUmi, uintptr_t maxHam, std::vector<uintptr_t>* storeInds);
	/**The amount of data in the neighborhood.*/
	uintptr_t numReadData;
	/**The reads in the neighborhood.*/
	EntryReadData** allReadData;
	/**The sorted information on the neighborhood.*/
	std::vector<PrimerUmiAlleleMarker> allUmiEntries;
	/**The original indices of the neighborhood.*/
	std::vector<uintptr_t> umiEntryInds;
	//TODO family bounds
};

#endif
