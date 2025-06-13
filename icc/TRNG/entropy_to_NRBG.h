/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: Takes our entropy sources, turns them into TRNG's
//
*************************************************************************/


#ifndef ENTROPY_TO_TRNG_H
#define ENTROPY_TO_TRNG_H


#include "openssl/evp.h"
#include "openssl/cmac.h"
#include "openssl/hmac.h"
#include "noise_to_entropy.h"

/*! @brief the digest used to check for repeated outputs, SHA512/64 bytes to fit better with underlying chunk sizes 
  slightly better performance on 64 systems can't hurt either
*/
#define TRNG_DIGEST "SHA256"

/*! @brief sizeof SHA-256 */
#define SHA_DIGEST_SIZE 32

/*! @brief number of retries to allow on matching hashes */

#define TRNG_RETRIES 5

TRNG_ERRORS SetRNGError(const char *a,const char *file,int line);



/* TRNG functions */

/*! @brief Long term entropy measurement callback (data input) */
typedef int (* TRNG_MEASURE_F) (TRNG * mytrng, unsigned char *data,int n);

/*! @brief Long term entropy measurement callback (get measurement) */
typedef int (*TRNG_GET_MEASURE_F)();






/*! @brief Collected data structures for noise conditioning 
    Note that in theory we could use the HMAC or CMAC conditioner
    Our HMAC code is faster, so we use that.
 */
typedef struct TRNG_COND_t {
  unsigned char key[SHA_DIGEST_SIZE];   /*!< HMAC/CMAC key */
  unsigned char rdata[SHA_DIGEST_SIZE]; /*!< Residual personalization data/IV for the conditioner */              
  HMAC_CTX *hctx;                       /*!< Conditioning/compression key */
  const char *id;                       /*!< Debug */
} TRNG_COND;



/*!
  @brief Run the entropy source through the conditioning function
  to produce a byte stream of nominally 1 bit/bit entropy
  @param T The TRNG
  @param outbuf output buffer
  @param len length of buffer
  @return 0 if O.K., 1 if an error ocurred. (Multiple ht fails)
  @note we prefer the HMAC version simply because our CMAC code is slow
*/
int conditioner(TRNG *T, unsigned char* outbuf, unsigned len);

/*! 
  @brief
  - This routine implements the TRNG code, but is actually layered on 
    top of multiple low level 'TRNG' routines which provide entropy
  - Enforces the entropy guarantee made by the underlying sources
  - Performs the NIST Adaptive Proportion and Repeat Count tests
    This code is used by all the TRNG's which are accessible by applications.
  - the gb() callback provides raw entropic bits from the 'TRNG's
  @param T  Pointer to a TRNG, structure to hold TRNG data
  @param data buffer for returned value
  @param len length of returned value
  @note FIPS 140-3 TRNG entropy estimator is called here
  \FIPS VE07.13.01 Resistance of the seed generator (TRNG)
  - Initial state:
    The initial internal  state of the TRNG is determined by the function 
    efPersonalize() which incorporates date/time, machine ID, 
    process ID/Thread ID and high resolution timing data. 
    This only guarantees uniqueness.
  - Internal state:
    The current internal state of the TRNG is used to initialize a buffer into
    which samples from the raw entropy source are mixed before a primary 
    distribution check is carried out.
    Data from the entropy source (which passed the primary distribution test) 
    is mixed back into this internal state buffer.
  - Entropy guarantee
    - Short term, the entropy sources coming into here have passed the NIST
      entropy estimation tests. The Adaptive prediction tests, the Repitition
      Count test, and have an entropy estimate (PMAX) >= to the entropy
      guarantee.
    - Long term A compression function is used to estimate the entropy 
      in the TRNG, if that falls below the design minimum the TRNG shuts down. 
      In the event that's the global TRNG hooked into OpenSSL, ICC shuts down.
*/

  
TRNG_ERRORS Entropy_to_TRNG(TRNG * T, unsigned char* data, unsigned int len);



/*!
  @brief Compress things like personalization data
  to squash them into our working buffers
  @param T a TRNG context
  @param outbuf the field to be updated
  @param in the input data to be compressed
  @param len the length of the input data
*/

void xcompress(TRNG *T,unsigned char outbuf[SHA_DIGEST_SIZE],unsigned char *in, int len);

#endif
