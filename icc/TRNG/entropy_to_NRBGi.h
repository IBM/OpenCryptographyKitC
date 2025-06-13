/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: Removes the 'prvate' parts of the TRNG structures
// from wide exposure - see the notes below as to why this was needed.
//
*************************************************************************/

/* This is here because there's a wierd interaction between zlib.h
   and the OpenSSL headers on AIX (only). Only a few of the C
   files need to know the internals of the TRNG structures, so we
   restrict the problem to these files where it's managable by pulling
   zlib dependencies into this file and #including it only where
   required.
*/


#include "entropy_to_NRBG.h"
#include "TRNG/entropy_estimator.h"
#include "zlib.h"

/* zlib.h includes zconf.h that defines away 'const' !
* It probably needs STDC to be defined in zconf.h
* For now we will just put const back again
*/
#ifdef const
#undef const
#endif


/*! @brief Collected data structure for our long term entropy measuring routine */
typedef struct E_MEASURE_t {
  z_stream strm;         /*!< The compression structure used by the estimator */
  int EntropyState;      /*!< Initialization state of the estimator */
  int EntropyEstimate;   /*!< The current entropy estimate from this PRNG instance */
  int Tbytesin;          /*!< Byte count in for this estimator */
  int Tbytesout;         /*!< Byte count out for this estimator */
  Bytef out[2048]; 	     /*!< Compression buffer */
  const char *id;        /*!< Debug */
} E_MEASURE;


/*! @brief TRNG structure 
   This is the data used to convert a noise source into 
   a conditioned IID entropy source meeting our entopy guarantee
   Note that processing is in two phases
   - convert to an uncondition IID entropy source meeting our entropy guarantees
     Entropy estimation, AP and RC checks occur in phase 1.
   - convert to a conditioned source with a final compression test as a safeguard
*/
struct TRNG_t {
  E_SOURCE econd;       /* Data structures for noise source conditioning */
  unsigned char lastdigest[SHA_DIGEST_SIZE]; /*!< Digest of the last data returned by this TRNG instance */
  int initialized;      /*!< Initializations state of this TRNG instance */
  ENTROPY_HT ht;        /*!< Health tests for the CONDITIONED data */
  E_MEASURE e;          /*!< Compression test data structures */
  TRNG_COND cond;       /*!< Conditioning data structure (HMAC compressor working data set) */
  TRNG_MEASURE_F estin; /*!< Method to feed data into a continuous entropy measurement function  */
  TRNG_GET_MEASURE_F estout; /*!< Method to get the current entropy measurement (0-100) */ 
  EVP_MD_CTX *md_ctx;   /*!< Working digest CTX for CRNG test */
  const EVP_MD *md;     /*!< Digest we are using */
  int type;             /*!< Type of TRNG instantiated */
  const char *id;             /*!< Debug */
};

