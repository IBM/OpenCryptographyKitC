
/*
// Copyright IBM Corp. 2023
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*/
/* Description: Data tables for SP800-90 Cipher based PRNG structures and self test
*/

#include "icclib.h"
#include "ds.h"
#include "SP800-90.h"
#include "SP800-90i.h"
#include "utils.h"

/*!
  @brief maximum output blocksize we have to deal with
*/
#define MAX_OBL 16
static const unsigned char K[32] = { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
				     0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
				     0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
				     0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
};
static const unsigned char ZERO[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
				       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static const unsigned char C01[1] = {0x01};
static const unsigned char C80[1] = {0x80};

/*!
  @brief SP800-90 Block cipher chaining function.
  Note: This is a compression function, it takes multiple blocks of input, and only returns one block
  of output
  @param pctx an internal PRNG context
  @param ctx An initialized cipher context
  @param in A Data chaining structure containing the input data
  @param out a pointer to the output buffer
  @note key initialization is done outside this code. (Is expensive !)
*/

static void BCC(SP800_90PRNG_Data_t *pctx,EVP_CIPHER_CTX *ctx,DS *in,unsigned char *out)
{
  int outl = 0;
  unsigned i,n;
  unsigned char T[MAX_OBL];
  /* Chaining value = 0^outlen */
  memset(out,0,pctx->prng->OBL); 
  DS_Reset(in);
  /* Exactly as specified, 
     Note input is always 0 padded to a full block 
     so partical blocks of input aren't a concern
  */
  n = in->total / pctx->prng->OBL;
  for(i = 1; i <= n; i++) { 
    DS_Copy(in,T,pctx->prng->OBL); /* Note that DS_Copy() 0 pads */    
    /* input_block (T) = chaining_value (out) ^ block_i (in) */
    xor(T,T,out,pctx->prng->OBL);
    /* Chaining value = Block_Encrypt(key,input_block) */
    if( 1 !=EVP_EncryptUpdate(ctx,out,&outl,T,(int)pctx->prng->OBL) || 
	(outl != (int)pctx->prng->OBL) ) {
      pctx->error_reason = ERRAT("Encrypt Update failed");
      pctx->state = SP800_90ERROR;
      return;
    }
  }
  /* output block (1 block) = chaining value, it's already there */
}
/*!
  @brief Extract a new K & V
  @param pctx an internal PRNG context
  @note data which forms the new K,V is always seedlen bytes in pctx->T
*/
void SetKV(SP800_90PRNG_Data_t *pctx)
{
  int keylen;
  keylen = EVP_CIPHER_key_length(pctx->alg.cipher);
  memcpy(pctx->K,pctx->T,keylen); /* K is leftmost keylen bytes */
  /* F.3.1: (Otherwise the XOR with provided (seedlen) data doesn't cover all
     of K & V)
  */
  memcpy(pctx->V,(pctx->T)+keylen,pctx->prng->OBL); 
  /* And set up the new key */
  EVP_CIPHER_CTX_cleanup(pctx->ctx.cctx);
  if(1 != EVP_EncryptInit(pctx->ctx.cctx,pctx->alg.cipher,pctx->K,NULL)  ) {
    pctx->error_reason = ERRAT("Encrypt Init failed");
    pctx->state = SP800_90ERROR;
    return;
  } 

}
/*!
  @brief SP800-90 Block cipher Update (SP800-90 10.2.1.2)
  @param pctx a pointer to an internal PRNG ctx structure
  @note Key and V are contained within the pctx data,
  pctx->C contains the provided data which is NOT cleared after use
  by this function.
  This updates the key and generator data by incrementing the 
  block sized generator (V) and encrypting the result enough 
  times to generate a new V and a new key (K)
*/
static void Update(SP800_90PRNG_Data_t *pctx)
{
  unsigned int k = 0;
  int outl = 0;
  unsigned char *buf = pctx->T;
  unsigned int blen = pctx->prng->seedlen;
  
  
  while(blen > 0) {
    Add(pctx->V,pctx->V,pctx->prng->OBL,(unsigned char *)C01,1);
    if( 1 != EVP_EncryptUpdate(pctx->ctx.cctx,buf,&outl,pctx->V,(int)pctx->prng->OBL) ||
	(outl != (int)pctx->prng->OBL)  ) {
      pctx->error_reason = ERRAT("Encrypt Update failed");
      pctx->state = SP800_90ERROR;
      return;
    }
    k = (blen > pctx->prng->OBL) ? pctx->prng->OBL : blen;
    buf += k;
    blen -= k;
  }
  /* XOR in any provided data, which must be seedlen long */
  xor(pctx->T,pctx->T,pctx->C,pctx->prng->seedlen);
  /* Copy K & V from pctx->T and setup the new key */
  SetKV(pctx);  
  memset(pctx->T,0,pctx->prng->seedlen);
}
/*!
  @brief SP800-90 Cipher derivation function
  @param pctx a PRNG context
  @param dsin a data chain containing the input data
  @note Output always comes out in pctx->C and is pctx->prng->seedlen bytes
  ASSUME that this is supposed to not change the stored state 
  (Encryption key, pctx->K, pctx->V), so Cipher_df uses it's own EVP_CIPHER_CTX
*/

static void Cipher_df(SP800_90PRNG_Data_t *pctx,DS *dsin)
{

  unsigned int i,k;

  unsigned char IV[MAX_OBL]; 
  unsigned char L[4]; /* Maximum blocksize */
  unsigned char N[4];
  /*  unsigned char I[4]; */
  unsigned char *ptr = NULL;
  unsigned int outl = pctx->prng->seedlen;
  int eol = 0;
  EVP_CIPHER_CTX *ctx = NULL;

  ctx = EVP_CIPHER_CTX_new();
  
  memset(IV,0,pctx->prng->OBL);
  DS_Reset(dsin);

  /* Note the ordering,insert IV after L after N with the resulting 
     order
     IV || L || N || (input) || 0x80 || ZERO_PAD
     When in the inner loop
  */
  /* The length of the input data in bytes 
     Note that we have to calculate this BEFORE we start adding data to dsin
  */
  uint2BS(dsin->total,L);

  uint2BS(outl,N);  /* The length of the output data in bytes */

  DS_Insert(dsin,4,N);
  DS_Insert(dsin,4,L);
  DS_Insert(dsin,pctx->prng->OBL,IV);

  DS_Append(dsin,1,C80);

  i = pctx->prng->OBL - ( dsin->total % pctx->prng->OBL);
  /* explicitly pad to a full block */
  DS_Append(dsin,i,ZERO);


  /* Set the encryption key 0x00,0x01 ....*/
  if( 1 != EVP_EncryptInit(ctx,pctx->alg.cipher,(unsigned char *)K,NULL)  ) {
    pctx->error_reason = ERRAT("Encrypt Init failed");
    pctx->state = SP800_90ERROR;
    return;
  }
  outl = pctx->prng->seedlen; 
  ptr = pctx->T;
  i = 0;
  while(outl > 0) {
    uint2BS(i,IV);  /* Set the block # field in the IV */
    DS_Reset(dsin);
    BCC(pctx,ctx,dsin,ptr);
    k = (outl > pctx->prng->OBL) ? pctx->prng->OBL: outl;
    ptr += k;
    outl -= k;
    i = i + 1;
  }
  EVP_CIPHER_CTX_cleanup(ctx);
  /*  K = Leftmost keylen bits of temp (pctx->T)*/
  if( 1 != EVP_EncryptInit(ctx,pctx->alg.cipher,pctx->T,NULL)  ) {
    pctx->error_reason = ERRAT("Encrypt Init failed");
    pctx->state = SP800_90ERROR;
    return;
  }
  i = EVP_CIPHER_key_length(pctx->alg.cipher);
  /* X = next outlen bits of temp (move to the start of pctx->T) */
  memmove(pctx->T,pctx->T + i,pctx->prng->OBL);
  /* This always generates seedlen bytes in pctx->C */
  outl = pctx->prng->seedlen;
  ptr = pctx->C;
  while( outl > 0 ) {
    /* X = Block_Encrypt(K,X) */
    if( 1 != EVP_EncryptUpdate(ctx,ptr,&eol,pctx->T,(int)pctx->prng->OBL) ||
	(eol != (int)pctx->prng->OBL)  ) {
      pctx->error_reason = ERRAT("Encrypt Update failed");
      pctx->state = SP800_90ERROR;
      return;
    }
    k = (outl > pctx->prng->OBL) ? pctx->prng->OBL: outl;
    memcpy(pctx->T,ptr,pctx->prng->OBL);
    ptr += k;
    outl -= k;
  }
  /* And clear our scratch area */
  memset(pctx->T,0,pctx->prng->OBL);
  EVP_CIPHER_CTX_cleanup(ctx);
  EVP_CIPHER_CTX_free(ctx);
}

/*! 
   Cipher based PRNG's 
*/
/*!
  @brief Instantiate function for Cipher based PRNG's
  @param ctx a partially initialized PRNG context
  @param ein a pointer to the entropy input buffer. (May be NULL)
  @param einl a pointer to the length of the provided entropy
  @param nonce additional entropy
  @param nonl a pointer to the length of the provided entropy
  @param person a pointer to the personalization data
  @param perl a pointer to the length of the personalization data
 */
SP800_90STATE CIPHER_Instantiate(PRNG_CTX *ctx,
			      unsigned char *ein, unsigned int einl,
			      unsigned char *nonce, unsigned int nonl,
			      unsigned char *person,unsigned int perl)
{
  DS seedDS;
  SP800_90PRNG_Data_t *pctx = (SP800_90PRNG_Data_t *)ctx;
  
  /* Yes, this is thread safe, it always gets set to the same value, so 
     even in a race - no harm done 
  */
  if(NULL == pctx->alg.cipher) {
    pctx->alg.cipher = EVP_get_cipherbyname(pctx->prng->specific);
    if( NULL == pctx->alg.cipher) {
      pctx->error_reason = ERRAT("Could not obtain cipher");
      pctx->state = SP800_90ERROR;
      return pctx->state;
    }
  }
  if(NULL == pctx->ctx.cctx) {
    pctx->ctx.cctx = EVP_CIPHER_CTX_new();
  }
  DS_Init(&seedDS);
  DS_Append(&seedDS,einl,ein);
  DS_Append(&seedDS,nonl,nonce);
  DS_Append(&seedDS,perl,person);
  /* Compress all the seed inputs to a single "seedlen" buffer (pctx->C) */
  Cipher_df(pctx,&seedDS);
  memset(pctx->K,0,32);
  memset(pctx->V,0,pctx->prng->OBL);
  /* Set the initial encryption key */
  EVP_CIPHER_CTX_cleanup(pctx->ctx.cctx);
  if( 1 != EVP_EncryptInit(pctx->ctx.cctx,pctx->alg.cipher,pctx->K,NULL)  ) {
    pctx->error_reason = ERRAT("Encrypt Init failed");
    pctx->state = SP800_90ERROR;
    return pctx->state;
  }  
  /* Run "Update" with the provided seed */
  Update(pctx);
  /* And clean up the supplied AAD */
  memset(pctx->C,0,pctx->prng->seedlen);
  return pctx->state;
}
/*!
  @brief ReSeed function for Cipher based PRNG's
  @param ctx an initialized PRNG context
  @param ein a pointer to the entropy input buffer. (May be NULL)
  @param einl a pointer to the length of the provided entropy
  @param adata additional user provided data
  @param adatal a pointer to the length of the provided data

*/
SP800_90STATE CIPHER_ReSeed(PRNG_CTX *ctx,
			 unsigned char *ein, unsigned int einl,
			 unsigned char *adata,unsigned int adatal)
{
  SP800_90PRNG_Data_t *pctx = (SP800_90PRNG_Data_t *)ctx;
  DS ds;
  DS_Init(&ds);
  DS_Append(&ds,einl,ein);
  DS_Append(&ds,adatal,adata);
  Cipher_df(pctx,&ds);
  Update(pctx);
  /* And clean up the supplied data */
  memset(pctx->C,0,pctx->prng->seedlen);
  return pctx->state;
 
}

/*!
  @brief Generate function for Cipher based PRNG's
  @param ctx an initialized PRNG context
  @param buffer a pointer to the PRNG data destination
  @param blen Number of bytes of PRNG data to supply
  @param adata additional user provided data
  @param adatal a pointer to the length of the provided data

*/

SP800_90STATE CIPHER_Generate(PRNG_CTX *ctx,
			      unsigned char *buffer,unsigned blen,
			      unsigned char *adata,unsigned adatal)
{
 
  SP800_90PRNG_Data_t *pctx = (SP800_90PRNG_Data_t *)ctx;

  int j = 0;
  int outl = 0;
  DS seedDS;
  /* Note that the ReSeed logic, state validation 
     and prediction resistance
     is handled in SP800_90.c
  */
  
  memset(pctx->C,0,pctx->prng->seedlen);
  if( adatal ) { /* If there was adata feed that it */
    DS_Init(&seedDS);
    DS_Append(&seedDS,adatal,adata);
    Cipher_df(pctx,&seedDS);
    Update(pctx);
  }  /*else { adata = 0^seedlen; } ,  pctx->C is 0 by default */   

  /*
    Data is generated by encrypting V and incrementing V for the next block
  */
  while(blen > 0) {
    Add(pctx->V,pctx->V,pctx->prng->OBL,(unsigned char *)C01,1);      
    if( 1 != EVP_EncryptUpdate(pctx->ctx.cctx,pctx->T,&outl,pctx->V,(int)pctx->prng->OBL) ||
	(outl != (int)pctx->prng->OBL)  ) {
      pctx->error_reason = ERRAT("Encrypt Update failed");
      pctx->state = SP800_90ERROR;
      return pctx->state;
    }
    j = (blen > pctx->prng->OBL) ?  pctx->prng->OBL: blen; 
    memcpy(buffer,pctx->T,j);
    buffer += j;
    blen -= j;
  }
  /*
    Update K,V , with additional input if any fed through
    As noted above additional data is in pctx->C, either made to be seedlen bytes
    using Cipher_df() or zero.
  */

  Update(pctx);
  /* 
     Clear our temporary output buffer, 
  */
  memset(pctx->T,0,pctx->prng->OBL);
  /* And clean up the supplied data */
  memset(pctx->C,0,pctx->prng->seedlen); 
  return pctx->state;
}
/*!
  @brief Cleanup function for Cipher based PRNG's
  @param ctx The PRNG_CTX to cleanup
  All allocated data is released. (Only the EVP_MD_CTX)
*/
SP800_90STATE CIPHER_Cleanup(PRNG_CTX *ctx)
{
  SP800_90PRNG_Data_t *pctx = (SP800_90PRNG_Data_t *)ctx;

  if(NULL != pctx->ctx.cctx) {
    EVP_CIPHER_CTX_cleanup(pctx->ctx.cctx);
    EVP_CIPHER_CTX_free(pctx->ctx.cctx);
    pctx->ctx.cctx = NULL;
  }
  return pctx->state; 
}

#if defined(NODF_ENABLED)
/*!
  @brief Instantiate function for Cipher based PRNG's
  This variant is mssing the derivation function
  @param ctx a partially initialized PRNG context
  @param ein a pointer to the entropy input buffer. (May be NULL)
  @param einl a pointer to the length of the provided entropy
  @param nonce additional entropy
  @param nonl a pointer to the length of the provided entropy
  @param person a pointer to the personalization data
  @param perl a pointer to the length of the personalization data
 */
SP800_90STATE CIPHER_InstantiateNoDF(PRNG_CTX *ctx,
			      unsigned char *ein, unsigned int einl,
			      unsigned char *nonce, unsigned int nonl,
			      unsigned char *person,unsigned int perl)
{
  SP800_90PRNG_Data_t *pctx = (SP800_90PRNG_Data_t *)ctx;
  
  /* Yes, this is thread safe, it always gets set to the same value, so 
     even in a race - no harm done 
  */
  if(NULL == pctx->alg.cipher) {
    pctx->lg.cipher = EVP_get_cipherbyname(pctx->prng->specific);
  }
  if(NULL == pctx->ctx.cctx) {
    pctx->ctx.cctx = EVP_CIPHER_CTX_new();
  }
  memset(pctx->C,0, pctx->prng->seedlen);
  if(perl > pctx->prng->seedlen) perl = pctx->prng->seedlen;
  if((NULL != person) && (perl != 0)) { /* Has already been range checked */
    memcpy(pctx->C,person,perl);
  }
  if((NULL != ein) && (einl != 0)) {
    xor(pctx->C,pctx->C,ein,pctx->prng->seedlen);
  }
  memset(pctx->K,0,32);
  memset(pctx->V,0,pctx->prng->OBL);
  /* In other modes, this is done by Cipher_df */
  EVP_CIPHER_CTX_cleanup(pctx->ctx.cctx);
  EVP_EncryptInit(pctx->ctx.cctx,pctx->prng->alg.cipher,pctx->K,(unsigned char *)ZERO);
  Update(pctx);	  
  memset(pctx->C,0, pctx->prng->seedlen);
  return pctx->state;
}
/*!
  @brief ReSeed function for Cipher based PRNG's
  This variant is mssing the derivation function
  @param ctx an initialized PRNG context
  @param ein a pointer to the entropy input buffer. (May be NULL)
  @param einl a pointer to the length of the provided entropy
  @param adata additional user provided data
  @param adatal a pointer to the length of the provided data

*/
SP800_90STATE CIPHER_ReSeedNoDF(PRNG_CTX *ctx,
			 unsigned char *ein, unsigned int einl,
			 unsigned char *adata,unsigned int adatal)
{
  SP800_90PRNG_Data_t *pctx = (SP800_90PRNG_Data_t *)ctx;

  memset(pctx->C,0, pctx->prng->seedlen);
  if((NULL != adata) && (adatal != 0)  ) { /* Has already been range checked */
    memcpy(pctx->C,adata,adatal);
  }
  if((NULL != ein) && (einl != 0)) {
    xor(pctx->C,pctx->C,ein,pctx->prng->seedlen);
  }
  Update(pctx);  
  memset(pctx->C,0, pctx->prng->seedlen);
  return pctx->state;
 
}

/*!
  @brief Generate function for Cipher based PRNG's
  This variant is mssing the derivation function
  @param ctx an initialized PRNG context
  @param buffer a pointer to the PRNG data destination
  @param blen Number of bytes of PRNG data to supply
  @param adata additional user provided data
  @param adatal a pointer to the length of the provided data

*/

SP800_90STATE CIPHER_GenerateNoDF(PRNG_CTX *ctx,
			      unsigned char *buffer,unsigned blen,
			      unsigned char *adata,unsigned adatal)
{
 
  SP800_90PRNG_Data_t *pctx = (SP800_90PRNG_Data_t *)ctx;

  int j = 0;
  int outl = 0;

  /* Note that the ReSeed logic,parameter checks, 
     state validation and prediction resistance
     is handled in SP800_90.c
  */
  if(adatal > pctx->prng->seedlen) adatal = pctx->prng->seedlen;

  memset(pctx->C,0,pctx->prng->seedlen);
  if((NULL != adata) && (adatal != 0)) {
    memcpy(pctx->C,adata,adatal);
    Update(pctx);
  } 
  /*
    Data is generated by encrypting V and incrementing V for the next block
  */
  while(blen > 0) {
    Add(pctx->V,pctx->V,pctx->prng->OBL,(unsigned char *)C01,1);      
    EVP_EncryptUpdate(pctx->ctx.cctx,pctx->T,&outl,pctx->V,pctx->prng->OBL);
    j = (blen > pctx->prng->OBL) ?  pctx->prng->OBL: blen;
    memcpy(buffer,pctx->T,j);
    buffer += j;
    blen -= j;
  }
  /*
    Update K,V , with additional input if any fed through
    As noted above additional data is in pctx->C, which is 0 if no AAD.
  */
  Update(pctx);
  /* 
     Clear our temporary output buffer, 
  */
  memset(pctx->T,0,pctx->prng->OBL);
  memset(pctx->C,0,pctx->prng->seedlen);
  return pctx->state;
}
#endif
/*!
  Representation of no input data for PRNG self tests
*/
static const StringBuf NONE =
  {0,0,
   {
     0x00
   }
  };


/*! \known AES-128 PRNG known answer test data at 128 bit strength
  INSTANTIATE CTR_DRBG AES-128 use df with NO PREDICTION RESISTANCE
*/

static const StringBuf AES128_128IntEin = 
  {0,16,
   {
     0x52,0x64,0x2e,0xe0,0x3f,0xfc,0x52,0xe9,
     0x12,0xbd,0xcf,0x5c,0x05,0xce,0xbe,0xed
   }
  };
static const StringBuf AES128_128IntNon = 
  {0,8,
   {
     0xf0,0x5d,0x85,0x99,0xe5,0x15,0x04,0x61
   }
  };
static const StringBuf AES128_128IntPers = 
  {0,16,
   {
     0x6c,0x57,0xd0,0xf5,0x83,0x69,0xd7,0x21,
     0xd0,0x2a,0xd5,0x22,0x8f,0x6a,0x00,0xa2
   }
  };
/* Truncated from 1792 bits */
static const StringBuf AES128_128Result =
  {0,64,
   {
     0x83,0x20,0x73,0x43,0x9e,0xfa,0xa4,0xf7,
     0x91,0xc6,0x8b,0xe2,0xd9,0x4b,0xea,0xb6,
     0xf5,0x7c,0x9b,0x69,0x55,0x89,0x17,0x92,
     0x15,0x73,0x2c,0xd1,0x7b,0x25,0xb5,0x70,
     0xc2,0x2c,0x5d,0x6b,0x43,0xdc,0x7a,0x78,
     0x10,0x27,0x8a,0xa0,0x3d,0x3c,0xe1,0xe1,
     0x1a,0x65,0xab,0xe1,0xf4,0xeb,0x5b,0x1f,
     0x13,0x6e,0x21,0xae,0x1d,0xca,0xfe,0x25,
   }
  };


/*!
  \FIPS Data structure defining the capabilities and limits
  of the AES-128 CTR_DRBG
*/
SP800_90PRNG_t aes128PRNG = {
  SP800_CTR_AES128,  /*!< Type, internal use */
  256/8,      /*!< Seed length */
  (1<<27),    /*!< max nonce allowed */
  (1<<27),    /*!< max Personalization string */
  (1<<27),    /*!< Max AAD allowed */
  (1<<11),    /*!< Max bytes/request */
  0x10000000, /*!< Max requests/reseed, less than max allowed ! */
  128/8,      /*!< Output block size */
  (1<<27),    /*!< Max entropy input allowed */
  {128,
   0,
   0,
   0
  },          /*!< Supported security strengths */
  "AES-128-ECB",
  "AES-128-ECB",  /*!< Algorithm "name" used by ICC_get_RNGbyname() also the cipher mode */
  1,              /*!< Has a derivation function */
  Inst,
  Res,
  Gen,
  Cln,
  CIPHER_Instantiate, /*!< Instantiate method */
  CIPHER_ReSeed,      /*!< ReSeed method */
  CIPHER_Generate,    /*!< Generate method */
  CIPHER_Cleanup,     /*!< Cleanup method */
  SP800_IS_FIPS,     /*!< Are we a FIPS approved mode (do we pass the NIST tests) */
  SELF_TEST_AT,       /*!< health check interval */
  0,                  /*!< Health check counter for this method, counts down, triggers at zero */
  {
    {
      &AES128_128IntEin,
      &AES128_128IntNon,
      &AES128_128IntPers,
      &NONE,
      &NONE,
      &AES128_128Result
    },
    {
      NULL,      
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
    },
    {
      NULL,      
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
    },
    {
      NULL,      
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
    },
  }
};
 /*! \known AES192 PRNG known answer test data */

static const StringBuf AES192_192IntEin =
  {0,24,
   {
     0x49,0x03,0x60,0x24,0xee,0x3e,0xad,0x55,
     0xda,0xb0,0x78,0x9d,0x8d,0x80,0x9a,0xda,
     0xcb,0x10,0x82,0x0c,0x50,0x1c,0x80,0x93
  }
  };
static const StringBuf AES192_192IntNon =
  {0,16, 
   {
     0xff,0xce,0x8b,0x0d,0xc0,0x0a,0x0e,0x37,
     0xe4,0x1f,0x2d,0x6d,0xd1,0x3b,0x6b,0x56
   }
  };
static const StringBuf AES192_192GenEin = 
  {0,24,
   {
     0xd3,0x54,0x05,0x98,0x40,0x90,0xca,0xb1,
     0x2f,0xca,0xba,0xae,0xba,0x4b,0x4e,0x36,
     0xb9,0x18,0x69,0x78,0x2e,0xd8,0x63,0x27
   }
  };

static const StringBuf AES192_192Result = 
  {0,8,
   {
     0x7a,0x57,0xcd,0xc3,0x13,0x38,0x3a,0xa2
   }
  };


/*!
  \FIPS Data structure defining the capabilities and limits
  of the AES 192 CTR_DRBG
*/

SP800_90PRNG_t aes192PRNG = {
  SP800_CTR_AES192, /*!< Type, internal use */
  320/8,            /*!< Seed length */
  (1<<27),          /*!< Max Nonce */
  (1<<27),          /*!< Max Personalization string */
  (1<<27),          /*!< Max AAD allowed */
  (1<<11),          /*!< Max bytes/request */
  0x10000000,       /*!< Max requests/reseed, less than max allowed ! */
  128/8,            /*!< Output block size */
  (1<<27),          /*!< Max entropy input allowed */
  {192,
   0,
   0,
   0
  },                /*!< Supported security strengths */
  "AES-192-ECB",
  "AES-192-ECB",    /*!< Algorithm "name" used by ICC_get_RNGbyname() */
  1,              /*!< Has a derivation function */
  Inst,
  Res,
  Gen,
  Cln,
  CIPHER_Instantiate,  /*!< Instantiate method */
  CIPHER_ReSeed,       /*!< ReSeed method */
  CIPHER_Generate,     /*!< Generate method */  
  CIPHER_Cleanup,      /*!< Cleanup method */
  SP800_IS_FIPS,      /*!< Are we a FIPS approved mode (do we pass the NIST tests) */
  SELF_TEST_AT,        /*!< Health check counter */
  0,
  {
    {
      &AES192_192IntEin,
      &AES192_192IntNon,
      &NONE,
      &NONE,
      &AES192_192GenEin,
      &AES192_192Result
    },
    {
      NULL,      
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
    },
    {
      NULL,      
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
    },
    {
      NULL,      
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
    },
  }
};

/*! \known AES_256 PRNG known answer test data, only one security level */
static const StringBuf AES256_256IntEin =
  {0,32,
   {
     0xac,0xd4,0xd2,0xed,0x56,0x72,0x6b,0x52,
     0x34,0xb0,0x06,0xf9,0xe5,0x92,0xc6,0x92,
     0x7f,0x4c,0x6b,0x97,0x29,0x0c,0x0a,0xbb,
     0xf9,0x05,0x0d,0x96,0x0a,0xbd,0x6c,0x90

   }
  };
static const StringBuf AES256_256IntNon =
  {0,16,
   {
     0x58,0xc7,0xba,0x7f,0x78,0xcc,0x56,0x2b,
     0xb3,0x18,0xdd,0xc2,0x00,0x3f,0x42,0xac
   }
  };
static const StringBuf AES256_256GenEin =
  {0,32,
   {
     0xf2,0x3e,0xc2,0x23,0x47,0x4a,0x38,0xcc,
     0x89,0xcb,0xb7,0x7f,0xe1,0x04,0x0e,0x70,
     0xf6,0x0d,0x1b,0xd1,0x1f,0x6e,0x96,0x8a,
     0xc8,0xfe,0x1c,0x36,0x23,0xfc,0x7e,0x5f
   }
  };

static const StringBuf AES256_256Result =
  {0,8,
   {
     0x22,0x0a,0x32,0x05,0x17,0x00,0x85,0x0b
   }
  };


/*!
  \FIPS Data structure defining the capabilities and limits
  of the AES-256 CTR_DRBG

*/

SP800_90PRNG_t aes256PRNG = {
  SP800_CTR_AES256, /*!< Type, internal use */
  384/8,      /*!< Seed length */
  (1<<27),    /*!< Max nonce */
  (1<<27),    /*!< Max Personalization string */
  (1<<27),    /*!< Max AAD allowed */
  (1<<11),    /*!< Max bytes/request */
  0x10000000, /*!< Max requests/reseed, less than max allowed ! */
  128/8,      /*!< Output block size */
  (1<<27),    /*!< Max entropy input allowed */
  {256,
   0,
   0,
   0
  },          /*!< Supported security strengths */
  "AES-256-ECB",
  "AES-256-ECB",  /*!< Algorithm "name"  */
  1,              /*!< Has a derivation function */
  Inst,
  Res,
  Gen,
  Cln,
  CIPHER_Instantiate,  /*!< Instantiate method */
  CIPHER_ReSeed,       /*!< ReSeed method */
  CIPHER_Generate,     /*!< Generate method */ 
  CIPHER_Cleanup,      /*!< Cleanup method */
  SP800_IS_FIPS,      /*!< Are we a FIPS approved mode (do we pass the NIST tests) */
  SELF_TEST_AT,        /*!< health check interval */    
  0,                   /*!< Health check counter */
  {
    {
      &AES256_256IntEin,
      &AES256_256IntNon,
      &NONE,
      &NONE,
      &AES256_256GenEin,
      &AES256_256Result
    },
    {
      NULL,      
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
    },
    {
      NULL,      
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
    },
    {
      NULL,      
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
    }
  }
 };

#if defined(NODF_ENABLED)

/*! \known AES-128 no DF PRNG known answer test data at 128 bit strength

*/
 
static const StringBuf AES128_128_NoDF_IntEin = 
  {0,32,
   {
     0xad,0x0e,0x88,0x11,0xea,0x21,0x51,0x9e,
     0x6e,0x08,0x30,0x77,0x68,0xec,0x3b,0x4f,
     0x97,0x75,0x62,0x91,0x27,0xce,0x65,0x0c,
     0xce,0xf5,0x15,0xe0,0x25,0x46,0xce,0xaa
   }
  };

static const StringBuf AES128_128_NoDF_GenEin = 
  {0,32,
   {
     0x97,0x2c,0xfa,0x70,0xe9,0xc6,0xd8,0x8b,
     0x0c,0x87,0xbe,0x0e,0x94,0xa5,0x3f,0x03,
     0x4d,0x41,0x9f,0x8e,0xfb,0xd2,0xb1,0xf8,
     0x56,0xd9,0xf2,0xff,0x43,0xc1,0xe0,0x5b
   }
  };



static const StringBuf AES128_128_NoDF_Result =
  {0,16,
   {
     0x31,0x60,0xdf,0x9a,0x7b,0xb6,0xed,0x01,
     0xfc,0xfe,0x0b,0x74,0x19,0x95,0x9b,0xf2
   }
  };


/*!
  \FIPS Data structure defining the capabilities and limits
  of the AES-128 CTR_DRBG without derivation function
  @note These were included for testing, but I suspect can't be used
  in FIPS mode in ICC as we can't guarantee a sufficiently
  strong entropy source.
*/
SP800_90PRNG_t aes128PRNG_NoDF = {
  SP800_CTR_AES128_NODF,  /*!< Type, internal use */
  256/8,      /*!< Seed length */
  0,          /*!< max nonce allowed */
  256/8,      /*!< max Personalization string */
  256/8,      /*!< Max AAD allowed */
  (1<<11),    /*!< Max bytes/request */
  0x10000000, /*!< Max requests/reseed, less than max allowed ! */
  128/8,      /*!< Output block size */
  (1<<27),    /*!< Max entropy input allowed */
  {112,
   0,
   0,
   0
  },                  /*!< Supported security strengths */
  "AES-128-ECB",  
  "AES-128-ECB-NODF",      /*!< Algorithm "name" used by ICC_get_RNGbyname()*/
  0,              /*!< Has a derivation function */
  Inst,
  Res,
  Gen,
  Cln,
  CIPHER_InstantiateNoDF, /*!< Instantiate method */
  CIPHER_ReSeedNoDF,      /*!< ReSeed method */
  CIPHER_GenerateNoDF,    /*!< Generate method */
  CIPHER_Cleanup,     /*!< Cleanup method */
  SP800_IS_FIPS,     /*!< Are we a FIPS approved mode (do we pass the NIST tests) */
  SELF_TEST_AT,       /*!< health check interval */
  0,                  /*!< Health check counter for this method, counts down, triggers at zero */
  {
    {
      &AES128_128_NoDF_IntEin,
      &NONE,
      &NONE,
      &NONE,
      &AES128_128_NoDF_GenEin,
      &AES128_128_NoDF_Result
    },
    {
      NULL,      
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
    },
    {
      NULL,      
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
    },
    {
      NULL,      
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
    },
  }
};
 /*! \known AES192 No DF PRNG known answer test data 
   INSTANTIATE CTR_DRBG AES-192 no df with PREDICTION RESISTANCE ENABLED
   Note the bug in the KAT data. A nonce is specified, but this
   mode doesn't use one
*/

static const StringBuf AES192_192_NoDF_IntEin =
  {0,40,
   {
     0xe2,0x63,0x76,0xf2,0x58,0xc8,0x1e,0x1a,
     0x5c,0xb9,0xcb,0xda,0xde,0xb8,0xb6,0x6f,
     0xaf,0xa3,0xfb,0xcb,0x0e,0x36,0xc0,0x30,
     0x74,0xc3,0xa5,0x0f,0xa3,0x2f,0x6e,0x21,
     0xeb,0x2f,0xed,0xc5,0xbe,0xc2,0x34,0x9b,
   }
  };



static const StringBuf AES192_192_NoDF_GenEin = 
  {0,40,
   {
     0xea,0xe3,0x48,0xeb,0xd2,0xc4,0x03,0xa3,
     0x73,0x3c,0x7e,0x05,0x57,0x05,0x9f,0x26,
     0xc4,0x3f,0xd6,0x49,0x9f,0x86,0x16,0x81,
     0x88,0x32,0x10,0xa4,0x32,0xef,0xb1,0x72,
     0x01,0x80,0xdd,0x04,0xe9,0xce,0x6c,0x79
   }
  };
/* Not the NIST data, but the test cases come with 
   a nonce which the standard doesn't support in
   this mode
*/
static const StringBuf AES192_192_NoDF_Result = 
  {0,16,
   {
     0xa5,0xe2,0x14,0xb0,0x95,0x7c,0x6d,0x6b,
     0x3e,0xe8,0xf7,0x6b,0x8e,0xf8,0x06,0xfc
   }

  };
/*!
  \FIPS Data structure defining the capabilities and limits
  of the AES-192 CTR_DRBG (Without derivation functions)
*/

SP800_90PRNG_t aes192PRNG_NoDF = {
  SP800_CTR_AES192_NODF, /*!< Type, internal use */
  320/8,            /*!< Seed length */
  0,                /*!< Max Nonce */
  320/8,            /*!< Max Personalization string */
  320/8,            /*!< Max AAD allowed */
  (1<<11),          /*!< Max bytes/request */
  0x10000000,       /*!< Max requests/reseed, less than max allowed ! */
  128/8,            /*!< Output block size */
  (1<<27),          /*!< Max entropy input allowed */
  {
    144,
    0,
    0,
    0
  },                /*!< Supported security strengths */
  "AES-192-ECB",
  "AES-192-ECB-NODF",    /*!< Algorithm "name" used by ICC_get_RNGbyname() also the cipher mode */
  0,              /*!< Has a derivation function */
  Inst,
  Res,
  Gen,
  Cln,
  CIPHER_InstantiateNoDF,  /*!< Instantiate method */
  CIPHER_ReSeedNoDF,       /*!< ReSeed method */
  CIPHER_GenerateNoDF,     /*!< Generate method */  
  CIPHER_Cleanup,      /*!< Cleanup method */
  SP800_IS_FIPS,      /*!< Are we a FIPS approved mode (do we pass the NIST tests) */
  SELF_TEST_AT,        /*!< Health check counter */
  0,
  {
    {
      &AES192_192_NoDF_IntEin,
      &NONE,
      &NONE, /*&AES192_192_NoDF_IntPer,*/
      &NONE,
      &AES192_192_NoDF_GenEin,
      &AES192_192_NoDF_Result
    },
    {
      NULL,      
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
    },
    {
      NULL,      
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
    },
    {
      NULL,      
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
    },
  }
};

/*! \known AES_256 PRNG known answer test data, only one security level 
  No derivation function
  INSTANTIATE CTR_DRBG AES-256 no df with PREDICTION RESISTANCE ENABLED
  Note: The known answer data has a bug. Specifies "Nonce" where the
  standard species "Personalization string"
*/
static const StringBuf AES256_256_NoDF_IntEin =
  {0,48,
   {
     0xed,0x40,0xca,0xf5,0xee,0x1a,0x6a,0x15,
     0x4e,0xd0,0xc3,0x31,0xb9,0x0d,0xc0,0x08,
     0xea,0x22,0xc2,0xe1,0xc3,0x56,0xd6,0x72,
     0xaf,0x68,0x36,0x0c,0x36,0x68,0x04,0x0a,
     0xe1,0xb6,0x44,0x7a,0x4f,0x59,0x19,0x0c,
     0x96,0x84,0x43,0x39,0xde,0x65,0xd7,0xb6,
   }
  };



static const StringBuf AES256_256_NoDF_GenEin =
  {0,48,
   {
     0x91,0x68,0x41,0x61,0xe7,0x9a,0xb9,0x21,
     0x6f,0xb1,0x77,0xd1,0x6c,0xd2,0xe9,0xb4,
     0x16,0x23,0x7c,0x08,0xff,0x5b,0x1a,0x3d,
     0xa7,0x89,0xe1,0x56,0x5d,0x22,0x27,0x9e,
     0x57,0x50,0xac,0x8e,0x13,0x78,0x20,0xfc,
     0xee,0x0b,0xd8,0x87,0x04,0x91,0xca,0x41,
   }
  };

static const StringBuf AES256_256_NoDF_Result =
  {0,16,
   {   
     0x64,0x84,0xd7,0x7a,0x42,0x30,0x71,0x45,
     0x88,0xb9,0xd3,0xc7,0xcd,0x7e,0xa3,0x4b
   }
  };

/*!
  \FIPS Data structure defining the capabilities and limits
  of the AES-256 CTR_DRBG

*/

SP800_90PRNG_t aes256PRNG_NoDF = {
  SP800_CTR_AES256_NODF, /*!< Type, internal use */
  384/8,      /*!< Seed length */
  0,          /*!< Max nonce */
  384/8,      /*!< Max Personalization string */
  384/8,      /*!< Max AAD allowed */
  (1<<11),    /*!< Max bytes/request */
  0x10000000, /*!< Max requests/reseed, less than max allowed ! */
  128/8,      /*!< Output block size */
  (1<<27),    /*!< Max entropy input allowed */
  {192,
   0,
   0,
   0
  },          /*!< Supported security strengths */ 
  "AES-256-ECB",
  "AES-256-ECB-NODF", /*!< Algorithm "name"  */
  0,              /*!< Has a derivation function */
  Inst,
  Res,
  Gen,
  Cln,
  CIPHER_InstantiateNoDF,  /*!< Instantiate method */
  CIPHER_ReSeedNoDF,       /*!< ReSeed method */
  CIPHER_GenerateNoDF,     /*!< Generate method */ 
  CIPHER_Cleanup,      /*!< Cleanup method */
  SP800_IS_FIPS,      /*!< We pass self test, but our entropy input isn't good enough */
  SELF_TEST_AT,        /*!< health check interval */    
  0,                   /*!< Health check counter */
  {
    {
      &AES256_256_NoDF_IntEin,
      &NONE,
      &NONE,
      &NONE,
      &AES256_256_NoDF_GenEin,
      &AES256_256_NoDF_Result
    },
    {
      NULL,      
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
    },
    {
      NULL,      
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
    },
    {
      NULL,      
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
    }
  }
 };
#endif

