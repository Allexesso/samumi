#ifndef WHODUN_GEN_TABLE_ALIGN_H
#define WHODUN_GEN_TABLE_ALIGN_H 1

#include <vector>
#include <stdint.h>

#include "whodun_parse.h"
#include "whodun_thread.h"
#include "whodun_string.h"
#include "whodun_compress.h"
#include "whodun_stat_table.h"

namespace whodun {

#define WHODUN_SAM_FLAG_MULTSEG 1
#define WHODUN_SAM_FLAG_ALLALN 2
#define WHODUN_SAM_FLAG_SEGUNMAP 4
#define WHODUN_SAM_FLAG_NEXTSEGUNMAP 8
#define WHODUN_SAM_FLAG_SEQREVCOMP 16
#define WHODUN_SAM_FLAG_NEXTSEQREVCOMP 32
#define WHODUN_SAM_FLAG_FIRST 64
#define WHODUN_SAM_FLAG_LAST 128
#define WHODUN_SAM_FLAG_SECONDARY 256
#define WHODUN_SAM_FLAG_FILTFAIL 512
#define WHODUN_SAM_FLAG_DUPLICATE 1024
#define WHODUN_SAM_FLAG_SUPPLEMENT 2048

/**Information on a record.*/
class CRBSAMFileContents{
public:
	/**Set up an empty.*/
	CRBSAMFileContents();
	/**Allow subclasses to tear down.*/
	~CRBSAMFileContents();
	/**Whether this is a header line.*/
	int lastReadHead;
	
	/**The header line, if it was a header.*/
	SizePtrString headerLine;
	
	/**The name of the last entry. Empty for unknown.*/
	SizePtrString name;
	/**The flag of the last entry.*/
	int flag;
	/**The reference it is on. Empty for unknown.*/
	SizePtrString reference;
	/**The position in the reference of the first base in the sequence. -1 for unmapped.*/
	intptr_t pos;
	/**Mapping quality of the entry. 255 for fail.*/
	unsigned char mapq;
	/**The cigar string for the entry. Empty for unknown.*/
	SizePtrString cigar;
	/**The reference the next part of the template is on. Empty for unknown.*/
	SizePtrString nextReference;
	/**The position in the reference of the first base in the next part of the template. -1 for unmapped.*/
	intptr_t nextPos;
	/**The length of the template (positive for the early read, negative for the late read).*/
	intptr_t tempLen;
	/**The sequence. Empty for not present.*/
	SizePtrString seq;
	/**The phred based quality scores for the last entry. Empty for not present.*/
	SizePtrString qual;
	/**Any extra crap for the entry.*/
	SizePtrString extra;
	
	/**Storage for text.*/
	char* entryStore;
	/**The allocated size of entryStore.*/
	uintptr_t entryAlloc;
	/**
	 * Ensure capacity.
	 * @param numChar The number of characters to support.
	 */
	void ensureCapacity(uintptr_t numChar);
	
	/**
	 * Copy another entry.
	 * @param toCopy The entry to copy.
	 * @return this.
	 */
	CRBSAMFileContents& operator=(const CRBSAMFileContents& toCopy);
};

/**Read from a sam file.*/
class CRBSAMFileReader{
public:
	/**Set up.*/
	CRBSAMFileReader();
	/**ALlow subcalsses to tear down.*/
	virtual ~CRBSAMFileReader();
	
	/**
	 * Get the next entry.
	 * @return The next entry. 0 for end of file.
	 */
	virtual CRBSAMFileContents* read() = 0;
	/** Close anything this thing opened. */
	virtual void close() = 0;
	
	/**
	 * Get space for an entry.
	 * @return A new entry.
	 */
	virtual CRBSAMFileContents* allocate();
	/**
	 * Return an entry: can cut down on allocation.
	 * @param toRet The sequence to return.
	 */
	virtual void release(CRBSAMFileContents* toRet);
	/**A cache for entries.*/
	std::vector<CRBSAMFileContents*> cache;
};

/**Write to a sam file.*/
class CRBSAMFileWriter{
public:
	/**Set up*/
	CRBSAMFileWriter();
	/**Allow subclasses to tear down.*/
	virtual ~CRBSAMFileWriter();
	
	/**
	 * Write out an entry.
	 * @param nextOut The next entry to write.
	 */
	virtual void write(CRBSAMFileContents* nextOut) = 0;
	/**
	 * Write out multiple entries.
	 * @param numWrite the number of entries to write.
	 * @param allOut The entries to write.
	 */
	virtual void write(uintptr_t numWrite, CRBSAMFileContents** allOut);
	/** Close anything this thing opened. */
	virtual void close() = 0;
};

/**Do the threading stuff in this thing.*/
class CRBSAMUniform : public ThreadTask{
public:
	/**Set up.*/
	CRBSAMUniform();
	/**Tear down.*/
	~CRBSAMUniform();
	void doIt();
	/**The action to do.*/
	int actDo;
	/**Cut down on space.*/
	union{
		/**Stuff for sam conversion.*/
		struct{
			/**As a tsv.*/
			TextTableRow* asText;
			/**As a sam.*/
			CRBSAMFileContents* asParse;
		} samConv;
		/**Stuff for bam parse*/
		struct{
			/**The binary data.*/
			SizePtrString asData;
			/**As a sam.*/
			CRBSAMFileContents* asParse;
			/**Names of the reference sequences.*/
			std::vector<std::string>* refNames;
			/**An error message from the last run.*/
			const char* lastErr;
		} bamConv;
	};
	//TODO
	/**The id for the running thread.*/
	uintptr_t threadID;
	/**Split on the way back.*/
	CharacterSplitter backSpl;
};

/**Read from a sam file.*/
class SAMFileReader: public CRBSAMFileReader{
public:
	/**
	 * Set up.
	 * @param baseStr The base stream to read from.
	 */
	SAMFileReader(TextTableReader* baseStr);
	/**
	 * Set up.
	 * @param baseStr The base stream to read from.
	 * @param numThread The number of tasks to spawn.
	 * @param mainPool The threads to use.
	 */
	SAMFileReader(TextTableReader* baseStr, int numThread, ThreadPool* mainPool);
	/**Tear down.*/
	~SAMFileReader();
	
	CRBSAMFileContents* read();
	void close();
	
	/**Entries already loaded and waiting to return.*/
	std::deque<CRBSAMFileContents*> waitEnts;
	/**The thing to parse.*/
	TextTableReader* theStr;
	/**The maximum number of threads to spin up.*/
	uintptr_t numThr;
	/**The pool to use, if any.*/
	ThreadPool* usePool;
	/**Active tasks.*/
	std::vector<CRBSAMUniform> liveTask;
	/**Active tasks.*/
	std::vector<CRBSAMUniform*> passTask;
	/**Temporary storage for loaded entities.*/
	std::vector<TextTableRow*> subEnts;
};

/**Write to a sam file.*/
class SAMFileWriter : public CRBSAMFileWriter{
public:
	/**
	 * Set up.
	 * @param baseStr The base stream to write to.
	 */
	SAMFileWriter(TextTableWriter* baseStr);
	/**
	 * Set up.
	 * @param baseStr The base stream to write to.
	 * @param numThread The number of tasks to spawn.
	 * @param mainPool The threads to use.
	 */
	SAMFileWriter(TextTableWriter* baseStr, int numThread, ThreadPool* mainPool);
	/**Tear down.*/
	~SAMFileWriter();
	
	void write(CRBSAMFileContents* nextOut);
	void write(uintptr_t numWrite, CRBSAMFileContents** allOut);
	void close();
	
	/**The thing to parse.*/
	TextTableWriter* theStr;
	/**The maximum number of threads to spin up.*/
	uintptr_t numThr;
	/**The pool to use, if any.*/
	ThreadPool* usePool;
	/**Active tasks.*/
	std::vector<CRBSAMUniform> liveTask;
	/**Active tasks.*/
	std::vector<CRBSAMUniform*> passTask;
	/**Temporary storage for loaded entities.*/
	std::vector<TextTableRow> subEnts;
	/**Temporary storage for loaded entities.*/
	std::vector<TextTableRow*> passEnts;
};

/**Read a binary sam file.*/
class BAMFileReader : public CRBSAMFileReader{
public:
	/**
	 * Read a bam.
	 * @param toParse The binary source to read: decompressed.
	 */
	BAMFileReader(InStream* toParse);
	/**
	 * Read a bam.
	 * @param toParse The binary source to read: decompressed.
	 * @param numThread The number of tasks to spawn.
	 * @param mainPool The threads to use.
	 */
	BAMFileReader(InStream* toParse, int numThread, ThreadPool* mainPool);
	/**Tear down.*/
	~BAMFileReader();
	CRBSAMFileContents* read();
	void close();
	/**Entries already loaded and waiting to return.*/
	std::deque<CRBSAMFileContents*> waitEnts;
	/**The file to parse.*/
	InStream* fromSrc;
	/**The maximum number of threads to spin up.*/
	uintptr_t numThr;
	/**The pool to use, if any.*/
	ThreadPool* usePool;
	/**Names of the reference sequences.*/
	std::vector<std::string> refNames;
	/**Use for copying.*/
	MultithreadedStringH* useCopy;
	/**Old stuff.*/
	char* stageArena;
	/**Allocated space for stage area.*/
	uintptr_t stageAlloc;
	/**Amount of stuff in stage area.*/
	uintptr_t stageSize;
	/**Alternate stage arena.*/
	char* copyArena;
	/**Prepare the header.*/
	void parseHeader();
	/**Active tasks.*/
	std::vector<CRBSAMUniform> liveTask;
	/**Active tasks.*/
	std::vector<CRBSAMUniform*> passTask;
};

//TODO BAMFileWriter

/**
 * Get the file extensions ExtensionCRBSAMReader/Writer will recognize.
 * @param modeWrite WHether we will be writing.
 * @param toFill The place to put the extensions.
 */
void getCRBSamExtensions(int modeWrite, std::set<std::string>* toFill);

/**Open a file based on a file name.*/
class ExtensionCRBSAMReader : public CRBSAMFileReader{
public:
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 */
	ExtensionCRBSAMReader(const char* fileName);
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 * @param numThread The number of threads to use for the base reader.
	 * @param mainPool The threads to use for reading.
	 */
	ExtensionCRBSAMReader(const char* fileName, int numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~ExtensionCRBSAMReader();
	
	CRBSAMFileContents* read();
	void close();
	CRBSAMFileContents* allocate();
	void release(CRBSAMFileContents* toRet);
	
	/**The base streams.*/
	std::vector<InStream*> baseStrs;
	/**The base streams.*/
	std::vector<TextTableReader*> baseTsvStrs;
	/**The wrapped reader.*/
	CRBSAMFileReader* wrapStr;
};

/**Open a file based on a file name.*/
class ExtensionCRBSAMWriter : public CRBSAMFileWriter{
public:
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 */
	ExtensionCRBSAMWriter(const char* fileName);
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 * @param numThread The number of threads to use for the base reader.
	 * @param mainPool The threads to use for reading.
	 */
	ExtensionCRBSAMWriter(const char* fileName, int numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~ExtensionCRBSAMWriter();
	
	void write(CRBSAMFileContents* nextOut);
	void write(uintptr_t numWrite, CRBSAMFileContents** allOut);
	void close();
	
	/**The base streams.*/
	std::vector<OutStream*> baseStrs;
	/**The base streams.*/
	std::vector<TextTableWriter*> baseTsvStrs;
	/**The wrapped writer.*/
	CRBSAMFileWriter* wrapStr;
};


#define WHODUN_SAM_HEAD_META_SORT_UNK 0
#define WHODUN_SAM_HEAD_META_SORT_NONE 1
#define WHODUN_SAM_HEAD_META_SORT_NAME 2
#define WHODUN_SAM_HEAD_META_SORT_COORD 3

#define WHODUN_SAM_HEAD_META_GROUP_NONE 0
#define WHODUN_SAM_HEAD_META_GROUP_NAME 1
#define WHODUN_SAM_HEAD_META_GROUP_REF 2

/**Description headers.*/
typedef struct{
	/**The major version of this thing.*/
	int majorVersion;
	/**The minor version of this thing.*/
	int minorVersion;
	/**The sorting order of this thing.*/
	int sortOrder;
	/**How things are grouped together.*/
	int groupOrder;
	/**An sub-order for sorting.*/
	SizePtrString subSortOrder;
} CRBSAMHeaderHDinfo;

#define WHODUN_SAM_HEAD_REF_TOP_LINE 0
#define WHODUN_SAM_HEAD_REF_TOP_CIRCLE 1

/**Reference header.*/
typedef struct{
	/**The name of the reference.*/
	SizePtrString name;
	/**The length of the reference.*/
	uintptr_t len;
	/**Whether this is an alternate.*/
	int isAlt;
	/**Description of the alternative location.*/
	SizePtrString altDesc;
	/**Other names for this reference.*/
	SizePtrString altNames;
	/**Genome assembly identifier.*/
	SizePtrString genAssID;
	/**A description.*/
	SizePtrString desc;
	/**Checksum.*/
	SizePtrString md5;
	/**Species of the reference.*/
	SizePtrString species;
	/**The topology of the reference.*/
	int topology;
	/**URL for the thing.*/
	SizePtrString url;
} CRBSAMHeaderSQinfo;

/**Read group information*/
typedef struct{
	/**The id of this group.*/
	SizePtrString id;
	/**The used barcode sequence.*/
	SizePtrString barcode;
	/**Where this thing was made.*/
	SizePtrString center;
	/**A description.*/
	SizePtrString desc;
	/**When this thing was made.*/
	SizePtrString date;
	/**The flow order.*/
	SizePtrString flow;
	/**The key sequence.*/
	SizePtrString key;
	/**The library.*/
	SizePtrString library;
	/**The program used on this read group.*/
	SizePtrString progData;
	/**The expected insert size: 0 if missing.*/
	uintptr_t insertSize;
	/**The category of machine used to make the thing.*/
	SizePtrString platform;
	/**The model of machine used.*/
	SizePtrString platformModel;
	/**A unit ID for the exact machine.*/
	SizePtrString platformUnit;
	/**The name of the sample that made this group.*/
	SizePtrString sampleName;
} CRBSAMHeaderRGinfo;

/**A description of a program.*/
typedef struct{
	/**The id of this program.*/
	SizePtrString id;
	/**The name of this program.*/
	SizePtrString name;
	/**The command line used to run the program.*/
	SizePtrString comLine;
	/**The previous program id.*/
	SizePtrString prevID;
	/**A description.*/
	SizePtrString desc;
	/**The version of the program.*/
	SizePtrString version;
} CRBSAMHeaderPGinfo;

#define WHODUN_SAM_HEAD_OTHER 1
#define WHODUN_SAM_HEAD_CO 1
#define WHODUN_SAM_HEAD_HD 2
#define WHODUN_SAM_HEAD_SQ 3
#define WHODUN_SAM_HEAD_RG 4
#define WHODUN_SAM_HEAD_PG 5

/**Parse a header line.*/
class CRBSAMHeaderParser{
public:
	/**Set up the stuff.*/
	CRBSAMHeaderParser();
	/**Tear down the stuff.*/
	~CRBSAMHeaderParser();
	
	/**The type of header this is.*/
	int headType;
	/**The full header line.*/
	SizePtrString line;
	union{
		/**Description headers.*/
		CRBSAMHeaderHDinfo headHD;
		/**Reference header.*/
		CRBSAMHeaderSQinfo headSQ;
		/**Read group information*/
		CRBSAMHeaderRGinfo headRG;
		/**A description of a program.*/
		CRBSAMHeaderPGinfo headPG;
	};
	
	/**
	 * Parse the given header line.
	 * @param toParse The thing to parse.
	 * @return The header type.
	 */
	int parseHeader(CRBSAMFileContents* toParse);
	
	/**Used to split lines.*/
	CharacterSplitter subSplit;
};

/**Whether things are present.*/
typedef struct{
	/**The smallest template independent mapping quality in the template.*/
	char minTempMapQuality;
	/**The alignment score for this thing.*/
	char alignScore;
	/**An offset to subtract from each base's phred score.*/
	char alignQualityOffset;
	/**Reference of the next hit.*/
	char nextHitReference;
	/**Leftmost coordinate of the next hit.*/
	char nextHitCoord;
	/**2nd most likely base calls.*/
	char alternateBaseCalls;
	/**Phred of 2nd most likely base calls.*/
	char alternateBaseQuals;
	/**Index of this segment in the template.*/
	char templateIndex;
	/**Suffix of the segment.*/
	char segmentSuffix;
	/**Number of perfect hits.*/
	char hitCountPerfect;
	/**Number of off by one hits.*/
	char hitCountByOne;
	/**Number of off by two hits.*/
	char hitCountByTwo;
	/**Index of this hit.*/
	char hitIndex;
	/**Number of alignments in the file for this query.*/
	char numAlignmentsIH;
	/**Cigar for the next segment.*/
	char pairCigar;
	/**A sequence that can be used to get the original reference sequence.*/
	char referenceDiff;
	/**Mapping quality of the next segment.*/
	char pairMapQuality;
	/**Number of alignments in the file for this query.*/
	char numAlignmentsNH;
	/**Number of differences ("edit distance").*/
	char numDifference;
	/**Phred of the template given a correct mapping.*/
	char mappedPhred;
	/**Phred of the next segment.*/
	char pairQuality;
	/**The sequence of the next segment.*/
	char pairSequence;
	/**Alternate canonical alignments.*/
	char chimericAlts;
	/**What the mapping quality would be if not paired.*/
	char unpairedMapQuality;
	/**Number of segments in the template.*/
	char templateCount;
	/**The strand the read has been mapped to.*/
	char strand;
	/**Phred score of segment, given a correct mapping.*/
	char segmentQual;
	/**The read groupd this is part of.*/
	char readGroup;
	/**The library this is a part of.*/
	char library;
	/**A program used to mangle this thing.*/
	char program;
	/**The unit that made this.*/
	char platformUnit;
	/**The library barcode sequence.*/
	char barcodeSeq;
	/**The library barcode quality.*/
	char barcodeQual;
	/**The alternate barcode ID (i.e. corrected).*/
	char cellID;
	/**The alternate barcode sequence.*/
	char cellSeq;
	/**The alternate barcode quality.*/
	char cellQual;
	/**Umi(s) for this thing (corrected, file unique).*/
	char umiID;
	/**Umi(s) for this thing.*/
	char umiSeq;
	/**Umi(s) for this thing.*/
	char umiQual;
	/**Umi(s) for this thing.*/
	char umiRawSeq;
	/**Umi(s) for this thing.*/
	char umiRawQual;
	/**Previous alignments.*/
	char previousAlignments;
	/**Previous quality.*/
	char previousQuality;
	/**Read annotation.*/
	char readAnnotations;
	/**Read annotation.*/
	char readPaddedAnnotations;
	/**The edit distance to the color.*/
	char colorEditDistance;
	/**Color sequence.*/
	char colorSeq;
	/**Color sequence.*/
	char colorQual;
	/**Encoded flow intensity.*/
	char flowIntensity;
} CRBSAMExtraEntryDataPresent;

/**Extra stuff crammed onto the end of an entry.*/
typedef struct{
	/**The smallest template independent mapping quality in the template.*/
	intptr_t minTempMapQuality;
	/**The alignment score for this thing.*/
	intptr_t alignScore;
	/**An offset to subtract from each base's phred score.*/
	SizePtrString alignQualityOffset;
	/**Reference of the next hit.*/
	SizePtrString nextHitReference;
	/**Leftmost coordinate of the next hit.*/
	intptr_t nextHitCoord;
	/**2nd most likely base calls.*/
	SizePtrString alternateBaseCalls;
	/**Phred of 2nd most likely base calls.*/
	SizePtrString alternateBaseQuals;
	/**Index of this segment in the template.*/
	intptr_t templateIndex;
	/**Suffix of the segment.*/
	SizePtrString segmentSuffix;
	/**Number of perfect hits.*/
	intptr_t hitCountPerfect;
	/**Number of off by one hits.*/
	intptr_t hitCountByOne;
	/**Number of off by two hits.*/
	intptr_t hitCountByTwo;
	/**Index of this hit.*/
	intptr_t hitIndex;
	/**Number of alignments in the file for this query.*/
	intptr_t numAlignmentsIH;
	/**Cigar for the next segment.*/
	SizePtrString pairCigar;
	/**A sequence that can be used to get the original reference sequence.*/
	SizePtrString referenceDiff;
	/**Mapping quality of the next segment.*/
	intptr_t pairMapQuality;
	/**Number of alignments in the file for this query.*/
	intptr_t numAlignmentsNH;
	/**Number of differences ("edit distance").*/
	intptr_t numDifference;
	/**Phred of the template given a correct mapping.*/
	SizePtrString mappedPhred;
	/**Phred of the next segment.*/
	SizePtrString pairQuality;
	/**The sequence of the next segment.*/
	SizePtrString pairSequence;
	/**Alternate canonical alignments.*/
	SizePtrString chimericAlts;
	/**What the mapping quality would be if not paired.*/
	intptr_t unpairedMapQuality;
	/**Number of segments in the template.*/
	intptr_t templateCount;
	/**The strand the read has been mapped to.*/
	char strand;
	/**Phred score of segment, given a correct mapping.*/
	intptr_t segmentQual;
	/**The read groupd this is part of.*/
	SizePtrString readGroup;
	/**The library this is a part of.*/
	SizePtrString library;
	/**A program used to mangle this thing.*/
	SizePtrString program;
	/**The unit that made this.*/
	SizePtrString platformUnit;
	/**The library barcode sequence.*/
	SizePtrString barcodeSeq;
	/**The library barcode quality.*/
	SizePtrString barcodeQual;
	/**The alternate barcode ID (i.e. corrected).*/
	SizePtrString cellID;
	/**The alternate barcode sequence.*/
	SizePtrString cellSeq;
	/**The alternate barcode quality.*/
	SizePtrString cellQual;
	/**Umi(s) for this thing (corrected, file unique).*/
	SizePtrString umiID;
	/**Umi(s) for this thing.*/
	SizePtrString umiSeq;
	/**Umi(s) for this thing.*/
	SizePtrString umiQual;
	/**Umi(s) for this thing.*/
	SizePtrString umiRawSeq;
	/**Umi(s) for this thing.*/
	SizePtrString umiRawQual;
	/**Previous alignments.*/
	SizePtrString previousAlignments;
	/**Previous quality.*/
	SizePtrString previousQuality;
	/**Read annotation.*/
	SizePtrString readAnnotations;
	/**Read annotation.*/
	SizePtrString readPaddedAnnotations;
	/**The edit distance to the color.*/
	intptr_t colorEditDistance;
	/**Color sequence.*/
	SizePtrString colorSeq;
	/**Color sequence.*/
	SizePtrString colorQual;
	/**Encoded flow intensity.*/
	SizePtrString flowIntensity;
} CRBSAMExtraEntryData;

/**Parse extra stuff in an entry.*/
class CRBSAMExtraParser{
public:
	/**Set up the stuff.*/
	CRBSAMExtraParser();
	/**Tear down the stuff.*/
	~CRBSAMExtraParser();
	
	/**Whether each piece of extra data is present.*/
	CRBSAMExtraEntryDataPresent extDatP;
	/**The parsed extra data.*/
	CRBSAMExtraEntryData extDat;
	
	/**
	 * Parse the given header line.
	 * @param toParse The thing to parse.
	 * @return The header type.
	 */
	void parseExtra(CRBSAMFileContents* toParse);
	
	/**Used to split lines.*/
	CharacterSplitter subSplit;
};

/**Cache sequences until their pair is found.*/
class CRBSAMPairedEndCache{
public:
	/**Set up an empty cache*/
	CRBSAMPairedEndCache();
	/**Clean up.*/
	~CRBSAMPairedEndCache();
	
	/**
	 * Get whether the given sequence is a primary alignment.
	 * @param toExamine The sequence to examine.
	 * @return WHether the entry is a primary alignment.
	 */
	bool entryIsPrimary(CRBSAMFileContents* toExamine);

	/**
	 * Get whether the given alignment represents an alignment.
	 * @param toExamine THe sequence to examine.
	 * @return Whether it was an alignment: mapped, cigar, and not all soft clipped.
	 */
	bool entryIsAlign(CRBSAMFileContents* toExamine);

	/**
	 * Determine if the entry needs matching with a pair.
	 * @param toExamine The sam entry.
	 * @return Whether it is waiting on a pair.
	 */
	bool entryNeedPair(CRBSAMFileContents* toExamine);
	
	/**
	 * Is pair data present for the given entry.
	 * @param toExamine The entry to examine.
	 * @return Whether its pair is present.
	 */
	bool havePair(CRBSAMFileContents* toExamine);
	
	/**
	 * Get the pair data and remove from cache.
	 * @param forAln The alignment to get the pair of.
	 * @return The ID for the cached entry, and the cached entry.
	 */
	std::pair<uintptr_t,CRBSAMFileContents*> getPair(CRBSAMFileContents* forAln);
	
	/**
	 * Store the entry until its pair comes up.
	 * @param alnID An ID to associate with the entry.
	 * @param forAln The entry to store.
	 */
	void waitForPair(uintptr_t alnID, CRBSAMFileContents* forAln);
	
	/**
	 * Get the remaining elements.
	 * @param saveID THe place to put the outstanding ids.
	 * @param saveAln The place to put the outstanding alignments.
	 */
	void getOutstanding(std::vector<uintptr_t>* saveID, std::vector<CRBSAMFileContents*>* saveAln);
	
	/**Names of sequences waiting for a pair.*/
	std::map<SizePtrString, uintptr_t> waitingPair;
	/**The sequence alignments waiting for a pair.*/
	std::map<uintptr_t, CRBSAMFileContents* > pairData;
};

}

#endif
