/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*
// Description:
//        This incorporates ICC into the error facilities of OpenSSL.
*/


#include "openssl/err.h"
#include "iccversion.h"
#include "platform.h"
#include "iccerr.h"



/*! @brief ICC library error strings */
static ERR_STRING_DATA ICC_str_libraries[]=
{
  {ERR_PACK(ICC_ERR_L_ICC,0,0), ICC_LIB_NAME}, /*!< Error is in the ICC shared lib */
  {0,NULL}  /*!< terminate */
};


/*! @brief ICC function error strings */
static ERR_STRING_DATA ICC_str_functions[]=
{
  {ERR_PACK(0,ICC_ERR_F_ICCVERIFYRSAKEY,0),  "iccVerifyRSAKey"},  /*!< RSA/DSA verification */
  {ERR_PACK(0,ICC_ERR_F_ICCRANDBYTESFIPS,0), "iccRandBytesFIPS"}, /*!< RNG initialization */
  {ERR_PACK(0,ICC_ERR_F_GENERATERANDOMSEED,0), "GenerateRandomSeed"}, /* TRNG */ 
  {0,NULL}  /*!< terminate */
};

/*! @brief ICC reason error strings */
static ERR_STRING_DATA ICC_str_reasons[]=
{
  {ICC_ERR_R_RSA_KEY_CONSISTENCY, 
   "The RSA key consistency test failed on validation of a signature."}, /*!< as stated */
  {ICC_ERR_R_RNG_CONT_TEST,
   "The continuous RNG test failed because duplicate consecutive random numbers were generated."},   /*!< as stated */
  {ICC_ERR_R_TRNG_LOW_ENTROPY,
   "The continuous TRNG entropy test failed because insufficient entropy was detected."}, /*!< as stated */
  {0,NULL}  /*!< terminate */
};

void get_ICC_str_libraries(ERR_STRING_DATA (**str)[])
{
  *str = (ERR_STRING_DATA (*)[])(&ICC_str_libraries);
}
void get_ICC_str_functions(ERR_STRING_DATA (**str)[])
{
  *str = (ERR_STRING_DATA (*)[])(&ICC_str_functions);
}
void get_ICC_str_reasons(ERR_STRING_DATA (**str)[])
{
  *str = (ERR_STRING_DATA (*)[])(&ICC_str_reasons);
}
