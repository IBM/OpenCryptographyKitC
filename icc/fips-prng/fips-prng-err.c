/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description:                                                          
//        Error string handling of FIPS 140-2 compliant OpenSSL PRNG           
//        based on openssl crypto/rand/rand_err.c
//
*************************************************************************/


#include "openssl/err.h"
#include "fips-prng-err.h"

static ERR_STRING_DATA fips_prng_str_functs[]= {
  {ERR_PACK(0,RAND_F_FIPS_PRNG_RAND_INIT,0),    "fips_prng_init"},
  {ERR_PACK(0,RAND_F_FIPS_PRNG_RAND_BYTES,0),   "fips_rand_bytes"},
  {ERR_PACK(0,RAND_F_FIPS_PRNG_RAND_SEED,0),    "fips_rand_seed"},
  {ERR_PACK(0,RAND_F_FIPS_PRNG_RAND_CLEANUP,0), "fips_rand_cleanup"},
  {0,NULL}
};

static ERR_STRING_DATA fips_prng_str_reasons[]= {
  {RAND_R_PRNG_NOT_INITIALIZED          ,"FIPS PRNG not initialized"},
  {RAND_R_PRNG_CRYPT_TEST_FAILED        ,"FIPS PRNG cryptographic algorithm test failed"},
  {RAND_R_PRNG_CONTINUOUS_TEST_FAILED   ,"FIPS PRNG continuous test failed"},
  {RAND_R_PRNG_INVALID_ARG              ,"FIPS PRNG invalid args"},
  {RAND_R_PRNG_OUT_OF_MEMORY            ,"FIPS PRNG out of memory"},
  {RAND_R_PRNG_BN_ERROR                 ,"FIPS PRNG failed due to OpenSSL BN error"},
  {RAND_R_PRNG_NOT_IMPLEMENTED          ,"FIPS PRNG function not implemented"},
  {0,NULL}
};

void getPrngErrFuncts(ERR_STRING_DATA (**str)[])
{
    *str = (ERR_STRING_DATA (*)[])(&fips_prng_str_functs);
}

void getPrngErrReasons(ERR_STRING_DATA (**str)[])
{
    *str = (ERR_STRING_DATA (*)[])(&fips_prng_str_reasons);
}
