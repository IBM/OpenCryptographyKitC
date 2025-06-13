/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: Collects all the bits and pieces and sets them up
//              for use within ICC
//
*************************************************************************/


#ifndef ICC_NRBG_H
#define ICC_NRBG_H
#include "entropy_estimator.h"
#include "TRNG/TRNG_FIPS.h"
#include "TRNG/TRNG_ALT.h"
#include "TRNG/TRNG_ALT4.h"


 /*!
   @brief Which TRNG (seed source) are we using ?
   @return index of TRNG_ARRAY
 */
TRNG_TYPE GetDefaultTrng(void);
 /*!
   @brief Select the TRNG (seed source) to use
   @param  trng index of TRNG_ARRAY
  @return index of TRNG_ARRAY
 */

TRNG_TYPE SetDefaultTrng(TRNG_TYPE trng);

/*!
  @brief Return the name of the TRNG given the TRNG_TYPE
  @param trng TRNG enum
  @return NULL or the name of the TRNG
*/
const char * GetTRNGNameR(TRNG_TYPE trng);

/*! 
  @brief return the number of API accessable TRNG types
  @return the number of API accessable TRNG types 
*/
int TRNG_Count();


/*! 
  @brief
  Default personalization string for ICC
  This is supposed to be "per-instance" unique
  @param buffer A pre-allocate buffer to fill with unique personalization data
  @return if buffer is NULL, return the length that buffer needs to be passed in
  otherwise returns 0 on sucess, or 1 on failure.
  \FIPS
  Used in initializion of the TRNG's and SP800-90 PRNG's.
  This returns  timestamp + CPU count + pid + tid + the first 80 bytes of the
  machine name + padding obtained from CPU event counters
  The time/date fields are first as this data may be truncated in some modes
*/
unsigned int Personalize(unsigned char *buffer);


/*! @brief
  Return the running estimate of the global TRNG entropy
  @return current Global TRNG entropy estimate
  @note This is a crude approximation
*/

int GetGlobalEntropy();


/*! @brief
  Cleanup the TRNG instance and any global state used by OpenSSL
*/  
void CleanupGlobalTRNG(void);

/*! @brief
  Return a TRNG context
  @return NULL or a valid (tested) TRNG context
*/
TRNG *TRNG_new(TRNG_TYPE type);

/*! @brief
  Initialize a TRNG context
  @param T TRNG context
  @param type type of TRNG 
  @return TRNG_OK on sucess, an error otherwise
*/
TRNG_ERRORS TRNG_TRNG_Init(TRNG *T,TRNG_TYPE type);

/*!
  @brief
  Clear/free a TRNG context
  @param T the context to free
*/
void TRNG_free(TRNG *T);

/*!
  @brief return the type of a TRNG 
  @return the TRNG type
*/
TRNG_TYPE TRNG_type(TRNG *T);

/*!
 @brief
  This is the code path ICC uses INTERNALLY for seeds, i.e. ones
  fed into the SP800-90 PRNG's
  @param T a pointer to a TRNG structure
  @param seedLength the number of bytes requested
  @param seed a pointer to the buffer to fill
  @return status
*/
TRNG_ERRORS TRNG_GenerateRandomSeed(TRNG *T, int seedLength, void * seed);

/*!
 @brief
  This is the code path ICC uses externally, loops down into redirected OpenSSL RNG's
  @param num the number of bytes requested
  @param buf a pointer to the buffer to fill
  @return 1 on sucess, 0 on failure
*/
int my_GenerateRandomSeed(int num, unsigned char * buf);


/*! @brief return the entropy each NRBG type thinks it can deliver
    i.e. bytes required to deliver 1 byte of entropy
    @param T a pointer to a TRNG structure
    @return bytes needed, defaults to 2 (2:1 compression required)
*/
unsigned int TRNG_guarantee(TRNG *T);

#endif

