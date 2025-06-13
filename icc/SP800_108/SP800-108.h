/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: Definitions for  SP800-108 key derivation modes
//
*************************************************************************/

#if !defined(SP800_108_H)
#define SP800_108_H


typedef struct KDF_data_t KDF;

/*!
  @brief get the list of FIPS compliant 
  SP800-108 modes so we can iterate through them
  (running the self tests) during POST
  @return the list of FIPS complaint KDF modes
*/
const char **get_SP800_108FIPS(void);

/*!
  @brief clear the "tested" field in the structure if
  it wasn't a fail so that the SelfTest code can
  retest the functions
*/  
void SP800_108_clear_tested(void);

/*!
 @brief Return a key derivation context for the specified mode
 Implemented modes are:
 [SHA1..SHA512]-[CTR|FB|DP]
 [[AES|CAMELLIA]-[128|192|256]-[CTR|FB|DP]
 i.e. "SHA384-CTR" "AES-192-DP"
 Non-FIPS allowed modes are not returned in FIPS mode.
 @param pcb A pointer to an ICC library context
 @param kdfname The name of the function to use
 @return A KDF_CTX pointer, or NULL
 @note the KDF API is experimental, and may not be stable between ICC releases
*/
const KDF *SP800_108_get_KDFbyname(ICClib *pcb,char *kdfname);


int SP800_108_KDF(const KDF *xctx,
		  unsigned char *Ki,unsigned int Kilen,
		  unsigned char *Label, unsigned int Llen,
		  unsigned char *Context, unsigned int Clen,
		  unsigned char *K0,unsigned int L);
#endif
