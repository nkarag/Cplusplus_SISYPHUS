#include <vector>
#include <iostream>
#include <string>

/**
 * This is a simple coordinates structure
 * @author: Nikos Karayannidis
 */
struct Coordinates {
	int numCoords; //number of coordinates
	vector<int> cVect;
	/**
	 * default constructor
	 */
	Coordinates() : numCoords(0), cVect(){}

	/**
	 * constructor
	 */
	Coordinates(int nc, vector<int>& cds) : numCoords(nc), cVect(cds){}

	/**
	 * copy constructor
	 */
	Coordinates(Coordinates const& c) : numCoords(c.numCoords), cVect(c.cVect) {}
	/**
	 * It fills in another Coordinates struct the coordinates that this object contains
	 * excluding the pseudo coordinates (i.e. coordinates that correspond to pseudo-levels)
	 *
	 * @param coords        the initially empty coord struct that will be filled with the coordinate values
	 */
        void excludePseudoCoords(Coordinates & coords)const;

};

/**
 * This is a simple range structure over the order-codes of the members of a level
 * @author: Nikos Karayannidis
 */
struct LevelRange {
	// Note: the condition to check for a null range is: ?(leftEnd==rightEnd)
	static const int NULL_RANGE;
	string dimName;
	string lvlName;
	unsigned int leftEnd;
	unsigned int rightEnd;
	//LevelRange() : NULL_RANGE(0) {}
	LevelRange(){}
	/**
	 * assignment operator, because the compiler complains that it can use
	 * the default, due to the const member
	 */
	LevelRange const& operator=(LevelRange const& other);
	/**
	 * copy constructor
	 */
	 LevelRange::LevelRange(const LevelRange& l);
};

/**
 * This struct is used only because we need a struct
 * with a vector of LevelRange(s) member. within the bodu of calcCellOffset,
 * it plays the role of a ChunkHeader struct.
 */
struct DummyHdr{
        vector<LevelRange> vectRange;

        void excludeNULLRanges(vector<LevelRange>& newvectRange)const;
};

unsigned int calcCellOffset(const Coordinates& coords, DummyHdr& hdr);
unsigned int calcCellOffset(const Coordinates& coords, const bit_vector& bmp, bool& isEmpty, DummyHdr& hdr); //THIS THE TESTING CANDIDATE!!!

const int LevelRange::NULL_RANGE=-1;

class Level_Member{
public:
	enum {PSEUDO_CODE = -1};
};


main(){
        cout<<"Give No of dimensions: ";
        int nodim;
        cin>>nodim;

        DummyHdr hdr;
        for(int i = 0; i< nodim; i++){
                LevelRange rng;
                cout<<"\nDimension "<<i<<endl;
                cout<<"Give left edge: ";
                cin>>rng.leftEnd;

                cout<<"Give right edge: ";
                cin>>rng.rightEnd;

                hdr.vectRange.push_back(rng);
        }//end for

        //get the total number of cells
        int numCells = 1;
        for(vector<LevelRange>::iterator lvl_i = hdr.vectRange.begin();
            lvl_i != hdr.vectRange.end(); lvl_i++) {
                // add to product the cardinality of each dimension level
                numCells *= lvl_i->rightEnd - lvl_i->leftEnd + 1;
        }//end for

        //create the compression bitmap
        bit_vector bitv;
        cout<<"Give bitmap - bits separated by whitespace: \n";
        for(int b=0; b<numCells; b++){
                bool bit;
                cin>>bit;
                (bit) ? bitv.push_back(true) : bitv.push_back(false);
        }//end for

        cout<<"\n\nGive some coordinates -separated by whitespace (end with a '.'): \n";
        vector<int> cvect;
        int c;
        while(cin>>c)
                cvect.push_back(c);

        Coordinates coords(cvect.size(),cvect);

        bool isempty;
        int offset = calcCellOffset(coords, bitv, isempty, hdr);
        if(isempty)
                cout<<"EMPTY CELL!!!\n";
        cout<<"The corresponding offset is: "<<offset<<endl;
}

unsigned int calcCellOffset(const Coordinates& coords, const bit_vector& bmp, bool& isEmpty, DummyHdr& hdr)
// precondition:
//	coords contain a valid set of coordinates in a data chunk. The order of the coords in the
//	vector member from left to right corresponds to major-to-minor positioning of the cells. Only the non-empty
//	cells have been allocated and we use the bitmap bmp to locate empty cells.
// postcondition:
//	the corresponding cell offset is returned. This offset is a value in the range [0..realNumCells-1] and
//	can be used as an index value for access of the cell in an array (or vector) of the form: array[realNumCells]
//      The flag isEmpty is set to true if the requested cell corresponds to a 0 bit in the bitmap.
{
	//ASSERTION1: not an empty vector
	if(coords.cVect.empty())
		throw "DataChunk::(static)calcCellOffset ==> ASSERTION1: empty coord vector\n";

	//ASSERTION2: not an empty vector
	if(bmp.empty())
		throw "DataChunk::(static)calcCellOffset ==> ASSERTION2: empty bitmap\n";

        //ASSERTION3: this is a data chunk therefore no pseudo coords must exist
        for(int c = 0; c<coords.numCoords; c++){
                  // if this is a pseudo coordinate
                  if(coords.cVect[c] == Level_Member::PSEUDO_CODE){
                        //then the corresponding range should be NULL
                                throw "DataChunk::calcCellOffset ==> ASSERTION3: pseudo coord in data chunk coordinates!\n";
                  }//end if
        }//end for


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// comprOffset(Cn,Cn-1,...,C1) = offset(Cn,Cn-1,...,C1) - number_of_zeros(offset(Cn,Cn-1,...,C1))
	//
	// where
	// offset(Cn,Cn-1,...,C1) = Cn*card(Dn-1)*...*card(D1) + Cn-1*card(Dn-2)*...*card(D1) + ... + C2*card(D1) + C1
	// card(Di) = the number of values along dimension Di
	// number_of_zeros(i) = the number of zeros in the bitmap from the beginning up to the ith bit (not included)
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//init offset
        unsigned int offset = 0;
        try{
	        offset = calcCellOffset(coords, hdr);
        }
        catch(const char* msg){
		string m("DataChunk::calcCellOffset ==> ");//"Exception from Chunk::scanFileForPrefix, in Chunk::createCostTree : ");
		m += msg;
                throw m.c_str();
        }

	// set empty-cell flag
	(!bmp[offset]) ? isEmpty = true : isEmpty = false;

	//ASSERTION4: proper offset in bitmap
	if(offset > bmp.size())
		throw "DataChunk::(static)calcCellOffset ==> ASSERTION4: wrong offset in bitmap\n";

	//calculate number of zeros
	int numZeros = 0;
	for(int bit=0; bit<offset; bit++){
		// if current bit is zero add one to the zero counter
		if (bmp[bit] == false)
		        numZeros++;
	}//end for

	return (offset - numZeros);
}// end of DataChunk::calcCellOffset(const Coordinates& coords, const bit_vector& bmp)


unsigned int calcCellOffset(const Coordinates& coords, DummyHdr& hdr)
// precondition:
//	coords contain a valid set of coordinates in a dir chunk. The order of the coords in the
//	vector member from left to right corresponds to major-to-minor positioning of the cells
// postcondition:
//	the corresponding cell offset is returned.  This offset is a value in the range [0..totNumCells-1] and
//	can be used as an index value for access of the cell in an array (or vector) of the form: array[totNumCells]
{
	//ASSERTION1: not an empty vector
	if(coords.cVect.empty())
		throw "DirChunk::calcCellOffset ==> ASSERTION1: empty coord vector\n";

        //ASSERTION2: if there are pseudo levels, then the NULL ranges must correspond to the pseudo coordinates
        for(int c = 0; c<coords.numCoords; c++){
                  // if this is a pseudo coordinate
                  if(coords.cVect[c] == Level_Member::PSEUDO_CODE){
                        //then the corresponding range should be NULL
                        if(hdr.vectRange[c].leftEnd != LevelRange::NULL_RANGE ||
                                                hdr.vectRange[c].rightEnd != LevelRange::NULL_RANGE)
                                throw "DirChunk::calcCellOffset ==> ASSERTION2: mismatch in position of pseudo levels\n";
                  }//end if
        }//end for

        //first remove Pseudo-code coordinate values
        Coordinates newcoords;
        coords.excludePseudoCoords(newcoords);

        //Also remove null level-ranges
        vector<LevelRange> newvectRange;
        hdr.excludeNULLRanges(newvectRange);

        //ASSERTION3: same number of pseudo levels found
        if(newvectRange.size() != newcoords.numCoords || newvectRange.size() != newcoords.cVect.size())
                throw "DirChunk::calcCellOffset ==> ASSERTION3: mismatch in no of pseudo levels\n";

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// offset(Cn,Cn-1,...,C1) = Cn*card(Dn-1)*...*card(D1) + Cn-1*card(Dn-2)*...*card(D1) + ... + C2*card(D1) + C1
	// where
	// card(Di) = the number of values along dimension Di
        // NOTE: Cn,Cn-1, etc., are assumed to be normalized to reflect as origin the coordinate 0
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//init offset
	unsigned int offset = 0;
	//for each coordinate
	for(int d=0; d<newcoords.numCoords; d++){
		//compute product of cardinalities
		// init product total
		unsigned int prod = newcoords.cVect[d] - newvectRange[d].leftEnd; // normalize coordinate to 0 origin
		//for each dimension (except for d) get cardinality and multiply total product
		for(int dd = d+1; dd<newcoords.numCoords; dd++){
			//ASSERTION4: proper level ranges
			if(newvectRange[dd].rightEnd < newvectRange[dd].leftEnd)
				throw "DirChunk::calcCellOffset ==> ASSERTION4: invalid level range\n";
			//ASSERTION5: not null range
			if(newvectRange[dd].leftEnd == LevelRange::NULL_RANGE ||
                                                newvectRange[dd].rightEnd == LevelRange::NULL_RANGE)
				throw "DirChunk::calcCellOffset ==> ASSERTION5: NULL level range\n";
			//get cardinality for this dimension
			int card = newvectRange[dd].rightEnd - newvectRange[dd].leftEnd + 1;
			// multiply total with current cardinality
			prod *= card;
		}//end for

		//add product to the offset
		offset += prod;
	}//end for
	return offset;
}// end DirChunk::calcCellOffset

inline void Coordinates::excludePseudoCoords(Coordinates & newcoords) const
// precondition: newcoords contains an empty vector of coordinates. (*this) contains a set of coordinate values,
//      which some of them might be pseudo order codes.
// postcondition:
//      newcoords contains all the coordinates of (*this) with the same order, without the pseudo coordinate values.
//      The state of "this" object remains unchanged!
//
{
        //ASSERTION1: newcoords is empty
        if(!newcoords.cVect.empty())
                throw "Coordinates::excludePseudoCoords ==> ASSERTION1: non-empty vector\n";
        for(vector<int>::const_iterator i = cVect.begin(); i != cVect.end(); i++){
                  // if this is not a pseudo coordinate
                  if(*i != Level_Member::PSEUDO_CODE){
                        //include it
                        newcoords.cVect.push_back(*i);
                        newcoords.numCoords++;
                  }//end if
        }//end for
}// end Coordinates::excludePseudoCoords(const Coordinates & coords)

inline void DummyHdr::excludeNULLRanges(vector<LevelRange>& newvectRange) const
// precondition: newvectRange is an empty vector. (*this).vectRange contains a set of level ranges,
//      which some of them might be NULL (i.e., ranges fro pseudo levels)
// postcondition:
//      newvectRange contains all the ranges of (*this).vectRanges with the same order,
//      without the pseudo ranges. The state of "this" object remains unchanged!
//
{
        //ASSERTION1: newVectRange is empty
        if(!newvectRange.empty())
                throw "DummyHdr::excludePseudoRanges ==> ASSERTION1 : non-empty vector\n";
        for(vector<LevelRange>::const_iterator rng_i = vectRange.begin(); rng_i != vectRange.end(); rng_i++){
                  if(rng_i->leftEnd != LevelRange::NULL_RANGE && rng_i->rightEnd != LevelRange::NULL_RANGE)
                        newvectRange.push_back(*rng_i);
        }//end for
}


LevelRange::LevelRange(const LevelRange& l)
	: dimName(l.dimName),lvlName(l.lvlName),leftEnd(l.leftEnd),rightEnd(l.rightEnd)
{
}
LevelRange const& LevelRange::operator=(LevelRange const& other)
{
	dimName = other.dimName;
	lvlName = other.lvlName;
	leftEnd = other.leftEnd;
	rightEnd = other.rightEnd;
	return (*this);
}

