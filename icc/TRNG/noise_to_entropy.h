/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: Higher level noise conditioning routines for ICC
//
*************************************************************************/

#if !defined(NOISE_TO_ENTROPY_H)
#define NOISE_TO_ENTROPY_H

#include "TRNG/timer_entropy.h"
#include "TRNG/nist_algs.h"

#define RNG_BUFLEN E_ESTB_BUFLEN /* Needs to be locked to the entropy check buffer */

/*                                         *** NOTE ***
  PROC_MEM and PROC_DEBIAS are attempts to improve the distribution characteristics in problem
  cases (mainly Z). Code, including a default dummy no-op variant can be found in ext_filter.c
  They are defined at BUILD time, i.e. in the Makefile or test code as we need to do A->B 
  comparisons on the captured data. Which means that ideally we aim to build multiple variants
  so we can find which is best. 


  Note: PROC_DEBIAS appears to be an unconditional win and well worth the performance hit
  and is enabled in the ICC Makefile when compiling timer_fips.c
*/



typedef struct TRNG_t TRNG;

 /*! @brief
   ENUM's for TRNG types
   MUST match the ordering of the TRNG array in ICC_NRBG.c
 */

typedef enum {
   TRNG_HW,     /*!< Pure hardware source */
   TRNG_OS,     /*!< RNG from the OS */
   TRNG_FIPS,   /*!< FIPS compliant version */ 
 } NOISE_TYPE;

#define TRNG_TYPE NOISE_TYPE

/*!
  @brief
  ENUM's for TRNG/Entropy source internal errors
*/
typedef enum {
TRNG_OK = 0,        /*!< No error */
  TRNG_CONT_TEST = 1, /*!< 'Stuck at' test failed */
  TRNG_ENTROPY = 2,   /*!< Entropy is below guarantee */
  TRNG_INIT = 3,      /*!< Couldn't initialize TRNG */
  TRNG_REQ_SIZE = 4,  /*!< Could not handle request size, 0, too large */
  TRNG_MEM = 5,        /*!< Out of memory in the TRNG (allocation failure) */
  TRNG_RESTART = 6  /*!< Warning, we've errored and the return from SetRNGError
                         says I've changed the TRNG under you, do your best to recover
	            */
} TRNG_ERRORS;


/*! @brief sizeof retained/personalization data */
#define TRNG_RD 64

typedef struct E_SOURCE_t E_SOURCE;

/*================================================================
  Prototypes for callback functions for the noise
  sources and related processing to become a
  conditioned entropy source
  =================================================================*/

/*! @brief Callback for read from the noise source 
  @param  lcl is for other state data used by the method
  @param buf buffer to fill with data
  @param len number of bytes to read
*/
typedef TRNG_ERRORS (*GETBYTES)(E_SOURCE *myentropy,unsigned char *buf,int len);

/*! @brief Init callback prototype */
typedef TRNG_ERRORS (*NOISE_INIT_F)(E_SOURCE *myentropy, unsigned char *pers, int perl);

/*! @brief Cleanup callback prototype */
typedef TRNG_ERRORS (*NOISE_CLN_F)(E_SOURCE *myentropy);

/*! @brief Pre-init callback. (Global initialization) 
    if reinit != 0, force reinitialization
    (To cope with hot migration)
*/
typedef void  (*NOISE_PRE_INIT_F)(int reinit);

/*! @brief Availabiliy callback. i.e.is hardware present for HW modes */
typedef int  (*NOISE_AVAIL_F)(void);

/*! @brief Data structure for entropy sources of a particular type 
  This is the static part of the structure
*/
typedef struct ENTROPY_IMPL_t {
  const char *name;          /*!< Common "name" of the TRNG */
  const NOISE_TYPE type;     /*!< TRNG type */
  int e_guarantee;           /*!< bits we need to use to have 1 bit of entropy from the noise source */
  GETBYTES gb;                /*!< Fill a buffer on length len with the above entropy */
  NOISE_INIT_F init;         /*!< Initialization routine (optional) */
  NOISE_CLN_F  cleanup;      /*!< Cleanup routine (optional) */
  NOISE_PRE_INIT_F preinit;  /*!< Pre-init routine - global setup */
  NOISE_AVAIL_F avail;       /*!< source is available (HW,/dev/urandom etc) */
  const char *id;            /*!< Debug */
  int fips;                  /*!< 1 allowed in FIPS mode */
} ENTROPY_IMPL;


/* Manifest constants for the FIPS TRNG */

/* Number of samples in a read burst */
#define TE_BUFLEN 256
/* 
    More samples than this in a single bucket cannot plausibly be noise 
    ~2SD assuming gaussian. Conceded, that's a stretch but it's the best we have
    Note that we express this as a ratio because we COULD make the sample buffer
    longer. We probably won't becuase the filter uses a lot of memory, but we could.
    Note that this isn't fine grained because of rounding. It's only done this 
    way because it makes this track TE_BUFLEN.
*/
#define TE_MAXB (TE_BUFLEN/32) /* Maximum number of values in buckets we'll treat as possible noise */


/* The first two buckets don't typically contain any noise 
    The first definately won't, the second usually won't as the instruction rate
    doesn't generally lock step exactly with timed reads and we bounce between
    two values.
*/
#define MIN_BUCKETS 3 


typedef struct
{
    ICC_INT64 values[TE_MAXB]; /*values when that happened, we won't classify more than (TE_MAXB-1) values as noise and we'll throw away excess data early as it won't be used */
    ICC_INT64 v;               /* differences in counter values */
    int freq;             /* Frequency with which that difference occured */
} DIST;

/* Parameters for distribution squeezing */
#define HISTSZ 4096
#define BASELINE (HISTSZ/256) /* value if we had a uniform distribution */
#define FREEDOM 2             /* Slack we'll allow above the expected average value */
#define TOO_HIGH (BASELINE+FREEDOM)  /* Higher than the expected average */


typedef struct T_FILTER_t {   
    ICC_UINT64 samples[TE_BUFLEN]; /* Initial sample's and at end of processing a sorted list of nnoise samples that were plauibly after noise events */
    DIST dist[TE_BUFLEN];   /* Because it's a large object, and shouldn't be on the stack */
    int done;               /* Tuned */
    int deadcnt;            /* Number of sample batches aquired with no useful data */
    unsigned int lindex;     /* tuning parameter, index into table of delay between samples */
    int nnoise;             /* Number of timer samples in deltas at the end of processing which are used  */
    int nentropy;           /* Number of timer samples in deltas which were noise */
    int counter;            /* Counter for byte merging */
    char arry[256];         /* frequency array, max value here is TOO_HIGH (18) so char is large enough */
    unsigned char fifo[HISTSZ]; /*FIFO */
    int idx;                /* Current end of history buffer */
    int fifo_init;          /* FIFO has been initialized */
    unsigned int totl;      /* number of entries accumulated in history. Maximum is HISTSZ */
    const char *id;         /* ID tag, used for debug */
} T_FILTER;
 


/*! @brief
   Collected data structures for an Entropy source (one byte at a time reads)
   Some of the structs are unused in some implementations but this is easier to
   debug than dereferenced anonymous pointers.
   @note T_FILTER is large but the code is so complex I gave up on dynamically 
   allocating it.
*/
struct E_SOURCE_t {		
  ENTROPY_IMPL impl;    /*!< Implementation of this TRNG's entropy source (type/class) */	  
  ENTROPY_HT hti;       /*!< Health tests for input entropy in the TRNG core */
  T_FILTER tf;          /*!< State data (i.e. for the FIPS timer source RBG's), make it a union if need be */
  unsigned char nbuf[E_ESTB_BUFLEN]; /*!< I'd rather not do this, but there's a mismatch between what the NIST algs need and what we need later */
  int cnt;              /*!< Number of bytes left in the buffer */
  const char *id;       /*!< Debug string */
};





/*! 
  @brief
  - This routine implements common processing for noise
  sources ICC uses.
  - Checks the entropy
  - Applies the NIST Adapative Prediction and Repeat Count
  tests
  - turns a stream of bytes into a buffer full of bytes
  with the entopy guarantee met
  - DOES NOT perform further conditioning
  @param E pointer to data for the entropy source
  @param data buffer for returned value
  @param len length of returned value
  @return 0 on sucess, 1 on fail (Health test failed repeatedly)
  @note 
  - Our entropy guarantee at this point is 0.5bits/bit and
  that's hardwired into the tables in the AP and RC tests.
  - There's a final sanity check on output data stream later, a compression
  test, which doesn't appear in the NIST tests
*/
int trng_raw(E_SOURCE* E, 
	      unsigned char* data, 
	      unsigned int len);



#endif
