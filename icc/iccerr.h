/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*
// Description:
//    This incoporates ICC into the error facilities of openSSL. 
*/

#ifndef INCLUDED_ICCERR
#define INCLUDED_ICCERR


/*! @brief Error code number base for the ICC functions. */
#define ICC_ERR_F_BASE   128
/*! @brief Error code number base for the ICC reasons. */
#define ICC_ERR_R_BASE   100

/*! @brief ICC library errors */
typedef enum
{
  ICC_ERR_L_ICC = ERR_LIB_USER + 1   /*!< ICC library error */
}   ICC_ERR_L_ENUM;

/*! @brief ICC Function error codes. */
typedef enum
{
  ICC_ERR_F_ICCVERIFYRSAKEY     =  ICC_ERR_F_BASE + 0, /*!< RSA/DSA verification */
  ICC_ERR_F_ICCRANDBYTESFIPS    =  ICC_ERR_F_BASE + 1, /*!< RNG initialization */
  ICC_ERR_F_GENERATERANDOMSEED                         /*!< TRNG */
}   ICC_ERR_F_ENUM;

/*! @brief ICC Reason error codes. */
typedef enum
{
  ICC_ERR_R_RSA_KEY_CONSISTENCY   =  ICC_ERR_R_BASE+1, /*!< RSA/DSA key failed consistancy test */
  ICC_ERR_R_RNG_CONT_TEST         =  ICC_ERR_R_BASE+2, /*!< Continuous RNG test failed */
  ICC_ERR_R_TRNG_LOW_ENTROPY      =  ICC_ERR_R_BASE+3  /*!< TRNG failure, below min entropy */
}   ICC_ERR_R_ENUM;

void get_ICC_str_libraries(ERR_STRING_DATA (**str)[]);
void get_ICC_str_functions(ERR_STRING_DATA (**str)[]);
void get_ICC_str_reasons(ERR_STRING_DATA (**str)[]);

#endif
