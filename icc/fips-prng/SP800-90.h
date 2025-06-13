/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: Definitions for  SP800-90 RNG modes
//
*************************************************************************/


/* Note that while the standard specifies quantities in bits
   this code uses bytes, and truncates values at ~ 2^32-1

   Thread safety. The PRNG "object" is not safe to share
   across threads as it contains retained state. 
   Use one/thread.

*/

#if !defined(SP800_90_H)
#define SP800_90_H

#include "iccglobals.h"

/*! PRNG's supported 
  Note that we also support the TRNG through this interface
  IS_TRNG is a flag to specify that this is a TRNG not a PRNG and doesn't
  need reseed, doesn't have a TRNG backing it etc
*/
#define IS_TRNG 0x8000  
 
typedef enum {
  SP800_SHA1 = 1,           /*!< Real SP800 modes */
  SP800_SHA224,
  SP800_SHA256,
  SP800_SHA384,
  SP800_SHA512,
  SP800_HMAC_SHA1,
  SP800_HMAC_SHA224,
  SP800_HMAC_SHA256,
  SP800_HMAC_SHA384,
  SP800_HMAC_SHA512,
  SP800_CTR_3DES,
  SP800_CTR_AES128,
  SP800_CTR_AES192,
  SP800_CTR_AES256,
  SP800_TRNG_ALT,         /*!< ALT RBG */
  SP800_TRNG_ALT_ETAP,    /*!  ALT Entropy source  - Test tap ONLY */
  SP800_TRNG_ALT_NOISE,   /*!  ALT Noise source  - Test tap ONLY */
  SP800_TRNG_OS,
  SP800_TRNG_OS_NOISE,
  SP800_TRNG_OS_ETAP,
  SP800_TRNG_ALT4_NOISE,  /*!  ALT4 Noise source  - *may* be usable directly */
  SP800_TRNG_ALT4_ETAP,   /*!  ALT4 Entropy source  - Test tap ONLY */
  SP800_TRNG_ALT4,        /*!< ALT4 RBG */
  SP800_TRNG_HW,
  SP800_TRNG_HW_NOISE,
  SP800_TRNG_HW_ETAP,
  SP800_TRNG_FIPS,        /*!< FIPS TRNG */
  SP800_TRNG_FIPS_NOISE,  /*!< FIPS TRNG, noise tap */
  SP800_TRNG_FIPS_ETAP,   /*!< FIPS TRNP, Entropy tap */
} SP800_90PRNG_mode;


/* 
   Abstract types for the PRNG_CTX and PRNG objects 
*/

/*struct PRNG_CTX; */
typedef struct SP800_90PRNG_Data_t PRNG_CTX;

/* struct PRNG;*/
typedef struct SP800_90PRNG_t PRNG;

/*! @brief return the list of FIPS compliant supported RNGS.
  @return a pointer to the internal (text) list of RNG's
*/
const char **get_SP800_90FIPS(void);


/*!
  @brief Get PRNG method by name
  @param algname algorithm name
  @param fips are we in FIPS mode ?. If so algorithms known to be
  non-FIPS compliant won't return a PRNG
  @return a PRNG method for the provided algorithm
  */
PRNG *  get_RNGbyname(const char *algname, int fips);

PRNG_CTX *RNG_CTX_new_no_TRNG();
PRNG_CTX *RNG_CTX_new();

void RNG_CTX_free(PRNG_CTX *ctx);
SP800_90STATE  RNG_CTX_Init(PRNG_CTX *ctx,PRNG *alg, 
			       unsigned char *person, unsigned int personal,
			       unsigned int strength, int prediction_resistance
			    );

SP800_90STATE RNG_ReSeed(PRNG_CTX *ctx,unsigned char *adata,unsigned int adatal);
SP800_90STATE RNG_Generate(PRNG_CTX *ctx,
			   unsigned char *buffer,unsigned int n,
			   unsigned char *adata,unsigned int adatal);

SP800_90STATE RNG_CTX_ctrl(PRNG_CTX *ctx,SP800_90CTRL type,int arg, void *ptr);

void Set_rng_exclude(char *list);
#endif
