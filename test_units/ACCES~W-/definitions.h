/***************************************************************************
                          definitions.h  -  description
                             -------------------
    begin                : Wed Sep 27 2000
    copyright            : (C) 2000 by Nikos Karayannidis
    email                : nikos@dbnet.ntua.gr
 ***************************************************************************/

#ifndef DEF_H
#define DEF_H

//#include <math.h> //for ceil() function
/**
 * Boolean values
 */
//const bool TRUE = 1;
//const bool FALSE = 0;

//the type of a measure value
typedef float measure_t;

/**
 * Stdin thread reply buffer size
 */
const unsigned int thread_reply_buf_size = 256;

/**
 * Disk page size in bytes
 */
 // ****NOTE****
// SSM compiled with a 8192 disk page, can pin the whole record if it has size < 8116 bytes!
// for PAGESIZE = 8192 bucket reading does not work very well (segm. fault at the end) :-)
//	     use 8116 instead!
const unsigned int PAGESIZE = 8192;//16384;//8192;//32768;//8116; // 8K page //65536;

/**
 * Minimum bucket occupancy threshold (in bytes)
 */
const unsigned int BCKT_THRESHOLD = int(PAGESIZE*0.5);

/**
 * Sparseness Threshold : for a compression factor <= 0.75 <=> SPARSENESS_THRSHLD <= 0.75 - (1/(8*entry_size))
 */
const float SPARSENESS_THRSHLD = 0.73; // for entry_size == 8 bytes

/**
 * Max number of characters in a chunk id
 */
const unsigned int MAXCHAR_ID = 100;



#endif // DEF_H
