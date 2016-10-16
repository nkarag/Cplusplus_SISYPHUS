#include <new>
#include <cmath>

typedef unsigned int WORD; // a bitmap will be represented by an array of WORDS

/**
 * Number of bits per word, e.g., per unsigned integer, if the
 * bitmap is represented by an array of unsigned integers
 */
const unsigned int BITSPERWORD = sizeof(WORD)*8;

/**
 * logarithm of base 2
 */
inline double log2(double x) {
        return log(x)/log(2);
}

/**
 * Used in order to locate the WORD in which a bit has been stored
 */
const unsigned int SHIFT = static_cast<unsigned int>(ceil(log2(BITSPERWORD)));


/**
 * Returns the number of words needed to stored a bitmap
 * of size no_bits bits
 */
inline unsigned int numOfWords(unsigned int no_bits) {
	//return no_bits/BITSPERWORD + 1;
	return static_cast<unsigned int>(ceil(double(no_bits)/double(BITSPERWORD)));
}

/**
 * This mask is used in order to isolate the number of LSBits from an integer i (representing a
 * bit position in a bitmap) that correspond to the position of the bit within a WORD.
 *			                     			        ~~~~~~
 */
inline WORD create_mask(){
        WORD MASK = 1;
        // turn on the #SHIFT LSBits
        for(int b = 1; b<SHIFT; b++){
                MASK |= (1 << b);
        }
        return MASK;
}


/**
 * This structure implements the header of a chunk (dir or data chunk).
 *
 * @author Nikos Karayannidis
 */
struct DiskChunkHeader {
	/**
	 * Define the type of an order-code
	 */
        typedef short ordercode_t;

        typedef struct CidDomain{
        	/**
        	 * pointer to this domain's order-codes
        	 */
        	ordercode_t* ordercodes;
        	
        	CidDomain():ordercodes(0){}
        	~CidDomain() { delete [] ordercodes;}
        } Domain_t;
       	
       	/**
	 * Define the NULL range.
	 * Note: the condition to check for a null range is: ?(leftEnd==rightEnd)
	 */
        enum {null_range = -1};

       	/**
	 * Define the type of an order-code range
	 */
        typedef struct OcRng{
		ordercode_t left;
		ordercode_t right;
		
		OcRng():left(null_range),right(null_range){}
        	} 	OrderCodeRng_t;
        		
	/**
	 * This chunk's chunking depth. Also denotes the length of the outermost
	 * vector of the chunk id (see further on)
	 */
	char depth;

	/**
	 * The number of dimensions of the chunk. Also denotes the length of the innermost
	 * vector of the chunk id and the length of the order-code range vector (see further on)
	 */
	char no_dims;
			
	/**
	 * The number of measures contained inside the cell of this chunk. If this
	 * is a directory chunk then = 0.
	 */		
	char no_measures;
	
	/**
	 * The number of entries of this chunk. If this is a data chunk
	 * then this is the total number of entries (empty cells included), i.e.
	 * it gives us the length of the bitmap.
	 */
        short unsigned int no_entries;
        	        	
	/**
	 * A chunk id is of the form: oc|oc...|oc[.oc|oc...|oc]...,
	 * where "oc" is an order-code corresponding to a specific dimension
	 * and a specific level within this dimension.
	 * We implement the chunk id with a vector of vectors of order-codes.
	 * The outmost vector corresponds to the different domains (i.e. depths)
	 * of the chunk id, while the innermost corresponds to the different dimensions.
	 * This heap structure is laid out on a byte array order-code by order-code prior to
	 * disk storage. Therefore,in order to read it back, special offset computing
	 * routines are needed.
	 */
	//vector<vector<ordercode_t> > chunk_id;
	//ordercode_t** chunk_id;
	Domain_t* chunk_id;
	
	/**
	 * This vector contains a range of order-codes for each dimension level of the
	 * depth of this chunk.Therefore the length is equal with the number of dimensions.
	 * This range denotes the order-codes covered on each dimension
	 * from this chunk.
	 * This heap structure is laid out on a byte array range by range prior to
	 * disk storage. Therefore,in order to read it back, special offset computing
	 * routines are needed.	
	 */
	 //vector<OrderCodeRng_t> oc_range;
	 OrderCodeRng_t* oc_range;
	 /**
	  * Default constructor
	  */
	 DiskChunkHeader(): chunk_id(0),oc_range(0) {}
	
	 /**
	  * copy constructor
	  */
	  DiskChunkHeader(const DiskChunkHeader& h):
	  	depth(h.depth),no_dims(h.no_dims),no_measures(h.no_measures),no_entries(h.no_entries),
	  	chunk_id(h.chunk_id),oc_range(h.oc_range) {}
	
	  /*
	   * Destructor. It has to free space pointed to by the chunk id
	   * and oc_range arrays
	   */
	 ~DiskChunkHeader() {
	 	/*for(int d = 1; d<=depth; i++){
	 		delete [] chunk_id[d-1];
	 	} */
	 	delete [] chunk_id;
	 	delete [] oc_range;
	 }	
}; //end of DiskChunkHeader

struct DiskDataChunk {
	/**
	 * Define the type of a data chunk entry.
	 */
        typedef struct Entry {
        	//vector<measure_t> measures;
        	measure_t* measures;
        	
        	Entry():measures(0){}
        	~Entry(){delete [] measures;}        	
        	} 	DataEntry_t;

	/**
	 * The chunk header
	 */        		        	
	DiskChunkHeader	hdr;
	
	/**
	 * The number of non-empty cells. I.e. the number of 1's in
	 * the bitmap.
	 */
	 unsigned int no_ace;
	
	/**
	 * This unsigned integer dynamic array will represent the compression bitmap.
	 * It will be created on the heap from the corresponding bit_vector of class DataChunk
	 * and then copied to our byte array, prior to disk storage. The total length of this bitmap
	 * can be found from the DiskChunkHeader.no_entries attribute.
	 */
	WORD* bitmap;        		
	
	/**
	 * Vector of entries. Note that this is essentially a vector of vectors.
	 * This heap structure is laid out on a byte array entry by entry and measure by measure
	 * prior to disk storage. Therefore,in order to read it back, special offset computing
	 * routines are needed.	
	 */	
	//vector<DataEntry_t> entry;
	DataEntry_t* entry;
	
	/**
	 * Default constructor
	 */	
	DiskDataChunk(): bitmap(0), entry(0){}
	
	/**
	 * constructor
	 */
	 DiskDataChunk(const DiskChunkHeader& h): hdr(h), bitmap(0), entry(0){}
	
	 /**
	  * constructor
	  */
	  DiskDataChunk(const DiskChunkHeader& h, unsigned int i, WORD* const bmp, DataEntry_t * const e):
	  	 hdr(h), no_ace(i), bitmap(bmp), entry(e){}
	  	
	/**
	 * Destructor
	 */  	
	~DiskDataChunk() { delete [] bitmap; delete [] entry; }
	
	/**
	 * Allocate WORDS for storing a bitmap of size n
	 */
	 inline allocBmp(int n){
	 	try{
			bitmap = new WORD[numOfWords(n)];
		}
		catch(bad_alloc){
			throw bad_alloc();		
		}
	}	
	
	/**
	 * turn on bit i
	 */
	inline void setbit(int i){
		WORD MASK = ::create_mask();
	        bitmap[i>>SHIFT] |= (1<<(i & MASK));
	}
	
	/**
	 * turn off bit i
	 */
	inline void clearbit(int i){
	        WORD MASK = ::create_mask();
	        bitmap[i>>SHIFT] &= ~(1<<(i & MASK));
	}
	
	/**
	 * test bit i. Returns 0 if bit i is 0 and 1 if it is 1.
	 */
	inline int testbit(int i) const {
	        WORD MASK = ::create_mask();
        	return bitmap[i>>SHIFT] & (1<<(i & MASK));	
	}
};