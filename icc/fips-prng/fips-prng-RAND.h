/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description:
//    External OpenSSL RAND-based API of FIPS 140-2 compliant PRNG
//
*************************************************************************/


#ifndef HEADER_FIPS_PRNG_RAND_H
#define HEADER_FIPS_PRNG_RAND_H

#include "fips-prng-err.h" 



/* Constants */
/* ========= */

#define RAND_FIPS_MIN_SEED_BYTES FIPS_DSS_PRNG_MIN_SEED_BYTES
#define RAND_FIPS_MAX_SEED_BYTES FIPS_DSS_PRNG_MIN_SEED_BYTES


/* Prototypes of exported functions */
/* ================================ */

/* initialize the OpenSSL integration of the FIPS PRNG */
int RAND_FIPS_init(
	const void			*seed,
	const unsigned int		num);
/* Input:
    - seed: valid pointer to buffer with random seed
    - seed_len: size in bytes of buffer seed. Must be between
      RAND_FIPS_MIN_SEED_BYTES and RAND_FIPS_MAX_SEED_BYTES
      bytes.  Should contain at least 160 bits of entropy.
  Usage constraints:
   - Must be called exactly once during power up and before any of the
     functions retrieved via RAND_FIPS is called ! */


/* retrieve OpenSSL compliant methods implementing the FIPS PRNG */
RAND_METHOD *RAND_FIPS(void);



/*! 
  @brief return the name of the PRNG ICC uses
  @return The name of the PRNG ICC uses
*/
char *GetPRNGName();
  
/*!
  @brief set the global ICC PRNG
  @param prngname - the PRNG "name" SHA224, HMAC-256, AES-256-ECB 
  @return 1 on sucess, 0 otherwise
  @note this must be called before the first ICC_Attach() and
  must specify a PRNG that passes the NIST suite and provides
  256 bits of entropy. The default does that.
  The ONLY reason for allowing this to be changed is so we have
  a fallback at application level if one of the RNG's proves to
  have a flaw.
*/
int SetPRNGName(char *prngname);
/*! 
  @brief return the name of the TRNG ICC uses
  @return The name of TRNG ICC uses
*/
char *GetTRNGName();
  
/*!
  @brief set the global ICC TRNG
  @param trngname - the TRNG "name" SHA224, HMAC-256, AES-256-ECB 
  @return 1 on sucess, 0 otherwise
  @note this must be called before the first ICC_Attach() 
  The ONLY reason for allowing this to be changed is so we have
  a fallback at application level if we run in an environment
  (i.e. virtualized, broken timebase hardware) where the default
  RNG won't work
*/
int SetTRNGName(char *trngname);

/*!
  @brief return the entropy estimates for the core entropy sources
  @note as we have TWO sources, one feeding the PRNG and the other 
  the Seed source, retrieve both, return the minimum
*/
int RAND_FIPS_Entropy();


/*! 
  @brief return the number of RNG instances in use
*/
int GetRNGInstances();

/*!
  @brief set the global ICC TRNG
  @param instances The number of RNG instances (0 < X <= MAX)
  @return 1 on sucess, 0 otherwise
  @note this must be called before the first ICC_Attach() 
  - MAX is currently 256
*/
int SetRNGInstances(int instances);



#endif /* HEADER_FIPS_PRNG_RAND_H */
