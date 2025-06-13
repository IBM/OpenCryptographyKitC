/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: High level entropy check. This code
//              checks the TRNG outputs and provides a failsafe
//              to shut down the API if 'something bad' (tm) happens
//
*************************************************************************/



#include "platform.h"
#include "zlib.h"
#include "TRNG/entropy_to_NRBGi.h"
#include "induced.h"
static const char * TRNG_E_MEASUREtag = "E_MEASURE";

/*!
  @brief zlib compatable calloc wrapper 
  @param opaque - apparently just that
  @param items number of blocks
  @param size size of each block
  @return Z_NULL or a valid pointer  
*/
static void *izcalloc(void *opaque,unsigned items, unsigned size)
{
  return ICC_Calloc(items,size,__FILE__,__LINE__);
}

static void izfree(void *opaque,void *ptr)
{
  ICC_Free(ptr);
}

/*!
  @brief our entropy estimator function
  This uses the zlib compressor on streaming data on the smallest possible
  block size to give us an estimator for entropy
  Larger block sizes would be better, but the typical 32k would take too long
  to get results.
  @param trng The trng instance to use
  @param data the input data buffer
  @param n the number of input bytes
  @return 1 if more data is needed to get an estimate,
  this only happens at startup when we need at least one block of input before
  we can produce an estimate of the available entropy. 
  0 otherwise.
  @note 
  - We keep a running average of bytes in/bytes out to avoid overflow problems.
  - The idea of using a compression function as an estimator is good but
  it takes a lot of data before good estimates are available, since we mix
  this with TOD down to nS resolution the first set of data is known good
  - Bytes out always trail the bytes in by a significant margin. 
  Rather than wait until we have enough data to flush the output automatically, 
  we flush/reset it every n bytes so that we can measure the compression ratio
  reliably. 
  - Life is full of compromises - but this will reliably detect a TRNG failure.
  \FIPS This is the FIPS 140-3 TRNG entropy estimator
*/

int EntropyEstimator(TRNG *trng,unsigned char *data,int n)
{  
  int rv = 0; 
  int i,j;

  do {
    i = n;
    j = trng->e.Tbytesin - 1024;
    if( j > n ) {
      i = j;
    }  
    /** \induced 201: Entropy test, force  the incoming data to be a constant
      which should lower the entropy to the point where the TRNG entropy test trips. 
      Note that by the time we get to trip this by setting the induced failure
      we already have data buffered.
    */
    if( 201 == icc_failure) {
      memset(data,0xA5,i);
    }

    trng->e.strm.avail_in = i;
    trng->e.strm.next_in = data;
    deflate(&trng->e.strm,Z_NO_FLUSH);
    trng->e.Tbytesin += i;
    if(trng->e.Tbytesin >= 1024) {
      deflate(&(trng->e.strm),Z_SYNC_FLUSH);
      memset(trng->e.out,0,sizeof(trng->e.out)); /* Don't use the compressed data, so zero this now */
      trng->e.Tbytesout = 2048 - trng->e.strm.avail_out;
      /* recalc the entropy - we have ~ 47 bytes overhead with uncompressable data */
      trng->e.EntropyEstimate = ((trng->e.Tbytesout - 48) * 100)/trng->e.Tbytesin;
      trng->e.Tbytesin = 0;
      trng->e.Tbytesout = 0;
      trng->e.strm.avail_out = 2048;
      trng->e.strm.next_out = &(trng->e.out[0]);
    }
    n -= i;
    data += i;
  } while (n > 0);
  return rv;
}

/*! @brief Return the design entropy for the TRNG 
            Used to adjust the amount of data fed to PRNG's
  @param T The TRNG context pointer to get the design entropy of
  @return The entropy guarantee for this TRNG type
  @note if this is ever set to zero we'll get a /0 trap in EntropyOK
        Live with that, because it's better than continuing.
  @note The fixed 2 is correct here. We've compressed the data with HMAC
        to a nominal 50% level BEFORE this stage. We don't claim 100% simply
        because we can't measure entropy levels that high in any reasonable
        time. We can check 50% quickly      
*/
int GetDesignEntropy(TRNG *T)
{
  return(2);
}
/*! @brief Return the running estimate of the TRNG entropy
  @param T The TRNG pointer
  @return current TRNG entropy estimate
  @note This is a crude approximation.
  In the event that a TRNG isn't avilable, or the Estimator 
  isn't initialized (by generating data) it'll return 0.
*/
int GetEntropy(TRNG *T)
{
  int rv = 0;
  if( (NULL == T) || (1 != T->e.EntropyState)) {
    rv = 0;
  } else {
    rv = T->e.EntropyEstimate;
  }
  return rv;
}
/*! @brief Check the entropy meets the entropy guarantee
  @param T The TRNG to check
  @return 0 on failure, !0 if within bounds compared to design low entropy bound
*/
int EntropyOK(TRNG *T)
{
  /* Rewrite to make debug easier */
  int rv = 0;
  int ent = 0;
  int targ = 0;

  ent =  GetEntropy(T);
  targ = 100/GetDesignEntropy(T);
  if( ent > targ ) {
    rv = !0;
  } else {
    rv = 0; /* just so there's something to set a breakpoint on */
  }
  return rv;
}

TRNG_ERRORS InitEntropyEstimator(TRNG *trng)
{
  TRNG_ERRORS rv = TRNG_OK;
  if(NULL != trng) {
    trng->e.strm.zalloc = izcalloc;
    trng->e.strm.zfree = izfree;
    trng->e.strm.opaque = Z_NULL;
    deflateInit2(&trng->e.strm,Z_DEFAULT_COMPRESSION,Z_DEFLATED,9,1,Z_DEFAULT_STRATEGY);
    trng->e.strm.avail_out = 2048;
    trng->e.strm.next_out = &(trng->e.out[0]);
    trng->e.EntropyState = 1;
    trng->e.EntropyEstimate = 100; /* Until we have better information ... */
    trng->e.id = TRNG_E_MEASUREtag;
  } else {
    rv = TRNG_INIT;
  }
  return rv;
}
/*! 
   @brief cleanup so we don't leak the mutex and compression structures
*/
void CleanupEntropyEstimator(TRNG *trng)
{

  deflateEnd(&trng->e.strm);
  trng->e.EntropyState = 0;
}
