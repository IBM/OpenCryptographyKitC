/** \file icc.h
 * @brief  Main ICC API header file (ICCSDK)
 * This is the file you should include to use ICC
 * This includes icc_a.h and iccglobals.h internally
 */

/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: Header file for the icc static library                         
//
*************************************************************************/

    
#ifndef INCLUDED_ICC
#define INCLUDED_ICC

#include <stdio.h>
#include <stddef.h>
#include <errno.h>
#include "iccglobals.h" /*global definitions*/


#ifdef __cplusplus
extern "C" {
#endif

struct ICC_EVP_MD_t;           
struct ICC_EVP_MD_CTX_t;
struct ICC_EVP_CIPHER_t;
struct ICC_EVP_CIPHER_CTX_t;
struct ICC_EVP_ENCODE_CTX_t;
struct ICC_EVP_PKEY_t;
struct ICC_RSA_t;
struct ICC_DH_t;
struct ICC_DSA_t;
struct ICC_BIGNUM_t;
struct ICC_BN_CTX_t;
struct ICC_HMAC_CTX_t;
struct ICC_EC_METHOD_t;
struct ICC_ECDH_METHOD_t;
struct ICC_ECDSA_METHOD_t;
struct ICC_EC_KEY_t;
struct ICC_ECDSA_SIG_t;
struct ICC_EC_POINT_t;
struct ICC_EC_GROUP_t;
struct ICC_X509_ALGOR_t;
struct ICC_PKCS8_PRIV_KEY_INFO_t;
struct ICC_KDF_t;
struct ICC_DSA_SIG_t;
struct ICC_CMAC_CTX_t;
struct ICC_AES_GCM_CTX_t;
struct ICC_EVP_PKEY_CTX_t;
struct ICC_ASN1_OBJECT_t;
struct ICC_EVP_KEM_t;
struct ICC_EVP_KDF_t;
struct ICC_EVP_KDF_CTX_t;

/*! @brief 
  - Placeholder for message digest types. 
  - Must be allocated/freed using ICC API's only.
  - No user accessable components inside.
*/
typedef struct ICC_EVP_MD_t         ICC_EVP_MD;       

/*! @brief 
  - Placeholder for message digest contexts. 
  - Must be allocated/freed using ICC API's only   
  - No user accessable components inside.
*/
typedef struct ICC_EVP_MD_CTX_t     ICC_EVP_MD_CTX;  
    
/*! @brief 
  - Placeholder for symetric cipher types. 
  - Must be allocated/freed using ICC API's only   
  - No user accessable components inside.
*/
typedef struct ICC_EVP_CIPHER_t     ICC_EVP_CIPHER;

/*! @brief 
  - Placeholder for symetric cipher contexts.
  - Must be allocated/freed using ICC API's only   
  - No user accessable components inside.
*/
typedef struct ICC_EVP_CIPHER_CTX_t ICC_EVP_CIPHER_CTX;

/*! @brief 
  - Placeholder for BASE64 encode/decode contexts. 
  - Must be allocated/freed using ICC API's only   
  - No user accessable components inside.
*/
typedef struct ICC_EVP_ENCODE_CTX_t ICC_EVP_ENCODE_CTX;

/*! @brief 
  - Placeholder for generic asymetric key contexts. 
  - Must be allocated/freed using ICC API's only   
  - No user accessable components inside.
*/
typedef struct ICC_EVP_PKEY_t       ICC_EVP_PKEY;

/*! @brief 
  - Placeholder for RSA contexts. 
  - Must be allocated/freed using ICC API's only   
  - No user accessable components inside.
*/
typedef struct ICC_RSA_t            ICC_RSA;

/*! @brief 
  - Placeholder for Diffie-Hellman contexts. 
  - Must be allocated/freed using ICC API's only   
  - No user accessable components inside.
*/
typedef struct ICC_DH_t		    ICC_DH;

/*! @brief 
  - Placeholder for DSA (Digital Signature Agreement) contexts. 
  - Must be allocated/freed using ICC API's only   
  - No user accessable components inside.
*/
typedef struct ICC_DSA_t            ICC_DSA;

/*! @brief  
   - Placeholder for BIGNUM structures
   - We DON'T support the full API.
   - Must be allocated/freed using ICC API's only.    
   - No user accessable components inside.
   - Note BIGNUM support is only for key portability in DH functions. 
*/   
typedef struct ICC_BIGNUM_t         ICC_BIGNUM;

/*! @brief  
   - Placeholder for BN_CTX structures
   - We DON'T support the full API.
   - Must be allocated/freed using ICC API's only.    
   - No user accessable components inside.
   - Note BIGNUM support is only for key access functions. 
*/
typedef struct ICC_BN_CTX_t         ICC_BN_CTX;

/*! @brief  
   - Placeholder for HMAC_CTX structures
   - Must be allocated/freed using ICC API's only.    
   - No user accessable components inside.
*/   
typedef struct ICC_HMAC_CTX_t         ICC_HMAC_CTX;

/*! @brief  
   - Placeholder for ECDH_METHOD structures
   - Must be allocated/freed using ICC API's only.    
   - No user accessable components inside.
*/   
typedef struct ICC_ECDH_METHOD_t         ICC_ECDH_METHOD;

/*! @brief  
   - Placeholder for EC_METHOD structures
   - Must be allocated/freed using ICC API's only.    
   - No user accessable components inside.
*/   
typedef struct ICC_EC_METHOD_t         ICC_EC_METHOD;

/*! @brief  
   - Placeholder for ECDSA_METHOD structures
   - Must be allocated/freed using ICC API's only.    
   - No user accessable components inside.
*/   
typedef struct ICC_ECDSA_METHOD_t         ICC_ECDSA_METHOD;

/*! @brief  
   - Placeholder for ECDSA_SIG structures
   - Must be allocated/freed using ICC API's only.    
   - No user accessable components inside.
*/   
typedef struct ICC_ECDSA_SIG_t         ICC_ECDSA_SIG;

/*! @brief  
   - Placeholder for EC_KEY structures
   - Must be allocated/freed using ICC API's only.    
   - No user accessable components inside.
*/   
typedef struct ICC_EC_KEY_t         ICC_EC_KEY;

/*! @brief  
   - Placeholder for EC_POINT structures
   - Must be allocated/freed using ICC API's only.    
   - No user accessable components inside.
*/   
typedef struct ICC_EC_POINT_t         ICC_EC_POINT;

/*! @brief  
   - Placeholder for EC_GROUP structures
   - Must be allocated/freed using ICC API's only.    
   - No user accessable components inside.
*/   
typedef struct ICC_EC_GROUP_t         ICC_EC_GROUP;


/*! @brief
  - Placeholder for the X509_ALGOR structure used in the
  - PKCS#12 PBE routines
  - Must be allocated/free'd using ICC API's only.
  - No user accessable components inside.
*/
typedef struct ICC_X509_ALGOR_t      ICC_X509_ALGOR;

/*! @brief
  - Placeholder for the PKCS8_PRIV_KEY_INFO structure used in the
  - PKCS#8 conversion routines
  - Must be allocated/free'd using ICC API's only.
  - No user accessable components inside.
*/
typedef struct ICC_PKCS8_PRIV_KEY_INFO_t      ICC_PKCS8_PRIV_KEY_INFO;

/*! @brief
  - Placeholder for the KDF structure used in the
  - SP800-108 key derivation routines
  - This structure is from an internal table, do not free it
  - No user accessable components inside.
*/
typedef struct ICC_KDF_t ICC_KDF;

/*! @brief  
   - Placeholder for CMAC_CTX structures
   - Must be allocated/freed using ICC API's only.    
   - No user accessable components inside.
*/   
typedef struct ICC_CMAC_CTX_t         ICC_CMAC_CTX;

/*! @brief  
   - Placeholder for AES_GCM structures
   - Must be allocated/freed using ICC API's only.    
   - No user accessable components inside.
*/   
typedef struct ICC_AES_GCM_CTX_t         ICC_AES_GCM_CTX;

/*! @brief  
   - Placeholder for DSA_SIG structures
   - Must be allocated/freed using ICC API's only.    
   - No user accessable components inside.
*/   
typedef struct ICC_DSA_SIG_t         ICC_DSA_SIG;

/*! @brief  
   - Placeholder for EVP_PKEY_CTX structures
   - Must be allocated/freed using ICC API's only.    
   - No user accessable components inside.
*/   
typedef struct ICC_EVP_PKEY_CTX_t         ICC_EVP_PKEY_CTX;

/*! @brief  
   - Placeholder for ASN1_OBJECT structures
   - Must be allocated/freed using ICC API's only.    
   - No user accessable components inside.
*/   
typedef struct ICC_ASN1_OBJECT_t         ICC_ASN1_OBJECT;

/*! @brief  
   - Placeholder for EC_builtin_curve structures
   - Must be allocated/freed using ICC API's only.    
   - No user accessable components inside.
*/   
typedef struct ICC_EC_builtin_curve_t         ICC_EC_builtin_curve;


typedef unsigned char ICC_DES_cblock[8]; /*!< What a des key looks like */

#define ICC_EVP_OpenUpdate(x,a,b,c,d,e)         ICC_EVP_DecryptUpdate(x,a,b,c,d,e)
#define ICC_EVP_SealUpdate(x,a,b,c,d,e)         ICC_EVP_EncryptUpdate(x,a,b,c,d,e)	
#define	ICC_EVP_VerifyInit(x,a,b)               ICC_EVP_DigestInit(x,a,b)
#define	ICC_EVP_VerifyUpdate(x,a,b,c)           ICC_EVP_DigestUpdate(x,a,b,c)
#define ICC_EVP_SignInit(x,a,b)                 ICC_EVP_DigestInit(x,a,b)
#define ICC_EVP_SignUpdate(x,a,b,c)             ICC_EVP_DigestUpdate(x,a,b,c)

/*! @brief
  Macro to return the nymber of bytes in a BIGNUM
*/
#define ICC_BN_num_bytes(a,b) ((ICC_BN_num_bits(a,b)+7)/8) 

typedef struct ICC_EVP_KEM_t         ICC_EVP_KEM;
typedef struct ICC_EVP_KDF_t         ICC_EVP_KDF;
typedef struct ICC_EVP_KDF_CTX_t     ICC_EVP_KDF_CTX;

/* Include autogenerated API prototypes/defines */
#include "icc_a.h"

#ifdef __cplusplus
}
#endif

#endif /*INCLUDED_ICC*/
