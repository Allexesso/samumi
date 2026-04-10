#ifndef WHODUN_GEN_COORD_H
#define WHODUN_GEN_COORD_H 1

#include <vector>
#include <stdint.h>

#include "whodun_parse.h"
#include "whodun_thread.h"
#include "whodun_string.h"
#include "whodun_compress.h"
#include "whodun_stat_table.h"

namespace whodun {

/**A range of coordinates.*/
typedef struct{
	/**The low point on the range.*/
	uintptr_t start;
	/**The high point on the range.*/
	uintptr_t end;
} CoordinateRange;

/**
 * Compare two coordinates.
 * @param itemA The first coordinate.
 * @param itemB The second coordinate.
 * @return Whether coordinate A begins before coordinate B (or, if same beginning, whether it ends before).
 */
bool coordinateCompareFunc(const CoordinateRange& itemA, const CoordinateRange& itemB);

/**Interpret a tsv as a bed file.*/
class BedFileReader{
public:
	/**
	 * Parse the contents of a bed file.
	 * @param readFrom The tsv to read from (or csv, or whatever).
	 */
	BedFileReader(TextTableReader* readFrom);
	/**Clean tear down.*/
	~BedFileReader();
	/**The chromosomes of each entry.*/
	std::vector<SizePtrString> chromosomes;
	/**The zero based locations of each entry.*/
	std::vector<CoordinateRange> locs;
	/**Save text.*/
	std::vector<char> saveText;
};

/**Parse a UCSC coordinate.*/
class UCSCCoordinateParse{
public:
	/**
	 * Parse a ucsc coordinate.
	 * @param ucsc The coordinate specification.
	 */
	UCSCCoordinateParse(SizePtrString ucsc);
	/**Any errors while building.*/
	const char* lastErr;
	/**The reference of the range.*/
	SizePtrString ref;
	/**The bounds of the range.*/
	CoordinateRange bounds;
};

/**Figure out how a cigar maps coordinates.*/
class CigarCoordinateParse{
public:
	/**
	 * Set up getting information from a cigar.
	 * @param pos The positoin things are relative to.
	 * @param cigar The thing to get information from.
	 */
	CigarCoordinateParse(uintptr_t pos, SizePtrString cigar);
	
	/**
	 * Get the number of read bases the cigar covers.
	 * @return The number of read bases.
	 */
	uintptr_t getBases();
	/**
	 * Get the span the cigar covers.
	 * @param The span. Length zero if no correspondance.
	 */
	CoordinateRange getBounds();
	/**
	 * Get the coordinates of each read base.
	 * @param saveArr The place to put the coordinates. -1 for inserted bases.
	 * @return The number of soft clipped bases: saveArr[0] corresponds to the first soft clipped base, if any.
	 */
	std::pair<uintptr_t,uintptr_t> getCoordinates(intptr_t* saveArr);
	
	/**The position in the reference the operations are in relation to.*/
	uintptr_t startPos;
	/**Save the cigar.*/
	SizePtrString saveCig;
};

}

#endif