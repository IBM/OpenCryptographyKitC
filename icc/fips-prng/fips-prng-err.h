/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description:              
//      Interface to Error string handling of FIPS 140-2 compliant OpenSSL 
//      PRNG
//      Note: export intended only for fips prng package !
//
*************************************************************************/


/* Interface to Error string handling of FIPS 140-2 compliant OpenSSL PRNG */
/* Note: export intended only for fips prng package ! */

#ifndef HEADER_FIPS_PRNG_ERR_H
#define HEADER_FIPS_PRNG_ERR_H

#include "openssl/err.h"


/* Public constants */
/* ================ */

/* additional error/status codes for the RAND functions. */
/* ----------------------------------------------------- */
/* Note: Make sure that below values are disjoint of values defined in
   rand.h and fips-prng-err.c is consistant!! */

/* Function codes. */
typedef enum
{
     RAND_F_FIPS_PRNG_RAND_INIT    =                  200,
     RAND_F_FIPS_PRNG_RAND_BYTES   =                  201,
     RAND_F_FIPS_PRNG_RAND_SEED    =                  202,
     RAND_F_FIPS_PRNG_RAND_CLEANUP =                  203
}  FIPS_PRNG_FUNCTION_CODES;

/* Reason codes. */
typedef enum
{
     RAND_R_PRNG_OK                       =                0,
     RAND_R_PRNG_NOT_INITIALIZED          =              200,
     RAND_R_PRNG_CRYPT_TEST_FAILED        =              201,
     RAND_R_PRNG_CONTINUOUS_TEST_FAILED   =              202,
     RAND_R_PRNG_INVALID_ARG              =              203,
     RAND_R_PRNG_OUT_OF_MEMORY            =              204,
     RAND_R_PRNG_BN_ERROR                 =              205,
     RAND_R_PRNG_NOT_IMPLEMENTED          =              300
}  FIPS_PRNG_REASON_CODES;

/* Prototypes of exported functions */
/* ================================ */

void getPrngErrFuncts(ERR_STRING_DATA (**str)[]);

void getPrngErrReasons(ERR_STRING_DATA (**str)[]);
#endif /* HEADER_FIPS_PRNG_ERR_H */


