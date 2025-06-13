/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: Implementation of the upper API levels of SP800-108 
//              key derivation functions
//
*************************************************************************/

/*! \IMPLEMENT SP800-108.c
  Implements SP800-108 key derivation functions.
  <p><b>Thread safety</b><p> 
  The KDF "object" is not safe to share
  across threads as it contains retained state. 
  Use one/thread.
*/

/*
#define DEBUG_KDF 1
*/

#include "fips.h"
#include "openssl/hmac.h"
#include "openssl/cmac.h"
#include "icclib.h"


/** \known Data: Initial key, Label and context for SP800-108
    KDF POST tests
*/
static const unsigned char KAD[33] = {0,1,2,3,4,5,6,7,8,9,
				      0,1,2,3,4,5,6,7,8,9,
				      0,1,2,3,4,5,6,7,8,9,
				      0,1,2};

/*! '00' constant used in the KDF functions */
static const unsigned char C00[1] = {0x00};

extern void uint2BS(unsigned n,unsigned char N[4]);

/*! \IMPLEMENT SP800-108 Implementation
  @note this code wasn't written with efficiency in mind
  but we don't expect KDF functions to require extreme speeds
*/

/*!
  @brief the enum used to tell us we have an HMAC or CMAC based
  KDF
*/
typedef enum {
  IS_HMAC = 0,
  IS_CMAC = 1
} KDF_MODE;

/*!
  @brief typedef for the low level KDF function 
*/
typedef int (*KDF_Func)(void *method,
			unsigned char *Ki,unsigned int Kilen,
			unsigned char *Label, unsigned int Llen,
			unsigned char *Context, unsigned int Clen,
			unsigned char *K0,unsigned int L);

/*!
  @brief KDF code control structure
*/
typedef struct {
  const char *name;    /*!< The user provided KDF name */
  const char *algname; /*!< The derived algorithm name for HMAC/CMAC */
  int fips;            /*!< FIPS allowed mode if !0 */
  KDF_MODE mode;       /*!< CMAC or HMAC ? */
  KDF_Func kdf;        /*!< The KDF handle returned to the application */
  void *handle;        /*!< A pointer to the underlying hash/cmac algorithm object */
  int tested;          /*!< A flag to indicate that we tested this KDF 
			 0 untested
			 1 tested and passed
			 -1 tested and failed (unusable)
		       */
  unsigned char kadata[16];  /*!< The known answer data for this KDF */ 
} KDF_data_t;





/*!
  \FIPS NIST SP800-108 modes
  A known answer test is carried out on first use of each KDF mode
  and consists of 10 iterations of the KDF, 
  the first 16 bytes of the final key are compared with known data.
  @note No FIPS specified known answer test was specified at the time this was
  written.
*/

/*!
  \known Code: FIPS known answer KDF test. Performed the first time a KDF is used
  @brief KDF known answer testing
  @param kdf a KDF structure
  @return 1 if self test passed, -1 if it failed
  @note this also sets internal state if it's a FIPS context/FIPS KDF
*/

static int KDF_KA(KDF_data_t *kdf) {
  unsigned char tmp[32];
  unsigned char tmpo[32];
  int i = 0;
  int keylen = 16;

  if (!kdf->tested) {
    if (IS_CMAC == kdf->mode) {
      keylen = EVP_CIPHER_key_length(kdf->handle);
    }
    memcpy(tmp, KAD, keylen);
    kdf->tested = 1;
    for (i = 0; i < 10; i++) {
      (*(kdf->kdf))(kdf->handle, tmp, keylen, (unsigned char *)KAD, 5,
                    (unsigned char *)KAD, 17, tmpo, keylen);
      memcpy(tmp, tmpo, keylen);
    }

    /** \induced 501: KDF HMAC CTR mode force known answer failure
        @note this really only checks that we can hit the error paths
        and handle them properly.
    */
    if (501 == icc_failure) {
      tmpo[3] = ~tmpo[3];
    }
    if (memcmp(tmpo, kdf->kadata, sizeof(kdf->kadata)) != 0) {
#if defined(DEBUG_KDF)
      printf("KDF KA error %s: ", kdf->name);
      for (i = 0; i < 16; i++) {
        printf("%02x", tmpo[i]);
      }
      printf("\n");
#endif
      kdf->tested = -1;
      /* Set the flag even if we aren't in FIPS mode
         otherwise a failure won't be picked up if a non-FIPS
         context tries this first
      */
      if (kdf->fips) {
        SetFatalError("KDF known answer test failed", __FILE__, __LINE__);
      }
    }
  }
  return kdf->tested;
}
/*! @brief
   We need a scratch buffer for the induced failure tests,
   Attempting to modify static const data causes a segv so we need
   to move some data to a modifiable data area
   Since these tests result in ICC termination, 
   there's no need for this to be thread safe
   @note Since NIST haven't yet provided test vectors OR
   a requirement for known answer tests we don't both with
   startup known answer tests.
   They SHOULD be here, but until NIST gets it's act together
   we'll skip that.
*/
#if 0
static unsigned char kabuf[1024];
#endif


/*! @brief 
  HMAC CTR KDF
  @param x the HMAC message digest
  @param Ki the input key
  @param Kilen the length of the input key
  @param Label nonce data, usually protocol dependent
  @param Llen length of the Label data
  @param Context Nonce Shared information between two parties
  @param Clen length of the Context data
  @param K0 a buffer to hold the generated key
  @param L the length of the generated key => in bits <=
  @return 1 on success, 0 on failure, -1 "something bad happened"
  (invalid input, invalid md_ctx etc)
*/
int KDF_CTR_HMAC( void *x,
		  unsigned char *Ki,unsigned int Kilen,
		  unsigned char *Label, unsigned int Llen,
		  unsigned char *Context, unsigned int Clen,
		  unsigned char *K0,unsigned int L
		 )
{
  const EVP_MD *md = (const EVP_MD *)x;
  int rv = 1;
  long n = 0;
  unsigned long i = 1;
  unsigned long j = 0; 
  HMAC_CTX *hctx = NULL;
  unsigned char IA[4];
  unsigned char LA[4];
  unsigned int len = 0;
  unsigned int md_size = 0;
  unsigned char tmp[64]; /* the largest possible MAC using SHA512 */
  unsigned int bytes = L;

  md_size = EVP_MD_size(md); 
  n = L/md_size;

  uint2BS(L*8,LA);  
  if(n <= 0xffffffffL && md != NULL) {
    hctx = HMAC_CTX_new();
    if(NULL != hctx) {
      uint2BS(i,IA);
      while( bytes > 0 ) {      
	my_HMAC_Init(hctx,Ki,Kilen,md);
	HMAC_Update(hctx,IA,4);
	HMAC_Update(hctx,Label,Llen);
	HMAC_Update(hctx,C00,1);
	HMAC_Update(hctx,Context,Clen);
	HMAC_Update(hctx,LA,4);
	HMAC_Final(hctx,tmp,&len);
	HMAC_CTX_cleanup(hctx);
	j = (bytes > md_size) ? md_size : bytes;
	memcpy(K0,tmp,j);
	K0 += md_size;
	bytes -= j;  
	i++;
      }
      HMAC_CTX_free(hctx);
    } else {
      rv = -1;
    }
  } else {
    rv = -1;
  }

  return rv;
}
/*! @brief 
  HMAC Feedback KDF
  @param x the HMAC nmessage digest
  @param Ki the input key
  @param Kilen the length of the input key
  @param Label nonce data, usually protocol dependent
  @param Llen length of the Label data
  @param Context Nonce Shared information between two parties
  @param Clen length of the Context data
  @param K0 a buffer to hold the generated key
  @param L the length of the generated key => in bits <=
  @return 1 on success, 0 on failure, -1 "something bad happened"
  (invalid input, invalid md etc)
*/
int KDF_FB_HMAC(void *x,
		unsigned char *Ki,unsigned int Kilen,
		unsigned char *Label, unsigned int Llen,
		unsigned char *Context, unsigned int Clen,
		unsigned char *K0,unsigned int L
		)
{
  const EVP_MD *md = (const EVP_MD *)x;
  int rv = 1;
  long n;
  unsigned long i = 1;
  unsigned long j = 0; 
  HMAC_CTX *hctx = NULL;
  unsigned char IA[4];
  unsigned char LA[4];
  unsigned int len = 0;
  unsigned int md_size = 0;
  unsigned char tmp[64]; /* the largest possible MAC using SHA512 */
  unsigned int bytes = L;
  
  md_size = EVP_MD_size(md);
  n = L/md_size;

  uint2BS(L*8,LA);
  memset(tmp,0,sizeof(tmp));
  if(n <= 0xffffffffL && md != NULL) {
    hctx = HMAC_CTX_new();
    if(NULL != hctx) {
      while(bytes > 0) {      
	uint2BS(i,IA);
	my_HMAC_Init(hctx,Ki,Kilen,md);
	HMAC_Update(hctx,tmp,md_size);
	HMAC_Update(hctx,IA,4);
	HMAC_Update(hctx,Label,Llen);
	HMAC_Update(hctx,C00,1);
	HMAC_Update(hctx,Context,Clen);
	HMAC_Update(hctx,LA,4);
	HMAC_Final(hctx,tmp,&len);
	HMAC_CTX_cleanup(hctx);
	j = (bytes > md_size) ? md_size : bytes;
	memcpy(K0,tmp,j);
	K0 += md_size;
	bytes -= j;
	i++;
      }
      HMAC_CTX_free(hctx);
    } else {
      rv = -1;
    }
  } else {
    rv = -1;
  }

  return rv;
}
/*! @brief 
  HMAC Dual Pipeline KDF
  @param x the HMAC message digest
  @param Ki the input key
  @param Kilen the length of the input key
  @param Label nonce data, usually protocol dependent
  @param Llen length of the Label data
  @param Context Nonce Shared information between two parties
  @param Clen length of the Context data
  @param K0 a buffer to hold the generated key
  @param L the length of the generated key => in bits <=
  @return 1 on success, 0 on failure, -1 "something bad happened"
  (invalid input, invalid md_ctx etc)
*/
int KDF_DP_HMAC(void *x,
		unsigned char *Ki,unsigned int Kilen,
		unsigned char *Label, unsigned int Llen,
		unsigned char *Context, unsigned int Clen,
		unsigned char *K0,unsigned int L
		)
{
  const EVP_MD *md = (const EVP_MD*)x;
  int rv = 1;
  long n;
  unsigned long i = 1;
  unsigned long j = 0; 
  HMAC_CTX *hctx = NULL;
  unsigned char IA[4];
  unsigned char LA[4];
  unsigned int len = 0;
  unsigned int md_size = 0;
  unsigned char tmp[64]; /* the largest possible MAC using SHA512 */
  unsigned char tmpA[64];
  unsigned int bytes = L;

  md_size = EVP_MD_size(md);
  n = L/md_size;

  uint2BS(L*8,LA);
  memset(tmp,0,sizeof(tmp));
  memset(tmpA,0,sizeof(tmpA));
  if(n <= 0xffffffffL && md != NULL) {
    hctx = HMAC_CTX_new();
    if(NULL != hctx) {
      while( bytes > 0 ) {  
	uint2BS(i,IA); 
	my_HMAC_Init(hctx,Ki,Kilen,md);
	if(i == 1) { /* A(0) = Label || 0x00 || Context || [L] */
	  HMAC_Update(hctx,Label,Llen);
	  HMAC_Update(hctx,C00,1);
	  HMAC_Update(hctx,Context,Clen);
	  HMAC_Update(hctx,LA,4);
	} else { /* A(i) = PRF(Ki,A(i-1) */
	  HMAC_Update(hctx,tmpA,md_size);
	}
	HMAC_Final(hctx,tmpA,&len);
	HMAC_CTX_cleanup(hctx);
	/* K(i) = PRF(K,A(i) || [i] || Label || 0x00 || Context || [L] */
	my_HMAC_Init(hctx,Ki,Kilen,md);
	HMAC_Update(hctx,tmpA,md_size);
	HMAC_Update(hctx,IA,4);
	HMAC_Update(hctx,Label,Llen);
	HMAC_Update(hctx,C00,1);
	HMAC_Update(hctx,Context,Clen);
	HMAC_Update(hctx,LA,4);
	HMAC_Final(hctx,tmp,&len);
	HMAC_CTX_cleanup(hctx);

	j = (bytes > md_size) ? md_size : bytes;
	memcpy(K0,tmp,j);
	K0 += md_size;
	bytes -= j;    
	i++;
      }
      HMAC_CTX_free(hctx);
    }
  } else {
    rv = -1;
  }

  return rv;
}

/*! @brief 
  CMAC CTR KDF
  @param x the CMAC cipher
  @param Ki the input key
  @param Kilen the length of the input key
  @param Label nonce data, usually protocol dependent
  @param Llen length of the Label data
  @param Context Nonce Shared information between two parties
  @param Clen length of the Context data
  @param K0 a buffer to hold the generated key
  @param L the length of the generated key => in bits <=
  @return 1 on success, 0 on failure, -1 "something bad happened"
  (invalid input, invalid cipher etc)
*/
int KDF_CTR_CMAC(void *x,
		 unsigned char *Ki,unsigned int Kilen,
		 unsigned char *Label, unsigned int Llen,
		 unsigned char *Context, unsigned int Clen,
		 unsigned char *K0,unsigned int L
		 )
{
  const EVP_CIPHER *cipher = (const EVP_CIPHER *)x;
  int rv = -11;
  long n;
  unsigned long i = 1;
  unsigned long j = 0;
  CMAC_CTX *cctx = NULL;
  unsigned char IA[4];
  unsigned char LA[4];
  unsigned int cipher_size = 0;
  unsigned char tmp[16]; /* the largest possible MAC using any supported alg */
  unsigned int bytes = L;
  unsigned int keylen = 0;

  keylen = EVP_CIPHER_key_length(cipher);
  if(Kilen == keylen) {
    cipher_size = EVP_CIPHER_block_size(cipher);
    n = L/cipher_size;
    uint2BS(L*8,LA);  
    if(n <= 0xffffffffL && cipher != NULL) {
      cctx = CMAC_CTX_new();
      if(NULL != cctx) {
	uint2BS(i,IA);
	while( bytes > 0 ) {      
	  my_CMAC_Init(cctx,cipher,Ki,Kilen);
	  CMAC_Update(cctx,IA,4);
	  CMAC_Update(cctx,Label,Llen);
	  CMAC_Update(cctx,(unsigned char *)C00,1);
	  CMAC_Update(cctx,Context,Clen);
	  CMAC_Update(cctx,LA,4);
	  my_CMAC_Final(cctx,tmp,cipher_size);
	  j = (bytes > cipher_size) ? cipher_size : bytes;
	  memcpy(K0,tmp,j);
	  K0 += cipher_size;
	  bytes -= j;  
	  i++;
	}
	rv = 1;
	CMAC_CTX_free(cctx);
      }
    }
  }

  return rv;
}
/*! @brief 
  CMAC Feedback KDF
  @param x the CMAC cipher
  @param Ki the input key
  @param Kilen the length of the input key
  @param Label nonce data, usually protocol dependent
  @param Llen length of the Label data
  @param Context Nonce Shared information between two parties
  @param Clen length of the Context data
  @param K0 a buffer to hold the generated key
  @param L the length of the generated key => in bits <=
  @return 1 on success, 0 on failure, -1 "something bad happened"
  (invalid input, invalid cipher etc)
*/
int KDF_FB_CMAC(void *x,
		unsigned char *Ki,unsigned int Kilen,
		unsigned char *Label, unsigned int Llen,
		unsigned char *Context, unsigned int Clen,
		unsigned char *K0,unsigned int L
		)
{
  const EVP_CIPHER *cipher = (const EVP_CIPHER *)x;
  int rv = 1;
  long n;
  unsigned long i = 1;
  unsigned long j = 0;
  CMAC_CTX *cctx = NULL;
  unsigned char IA[4];
  unsigned char LA[4];
  unsigned int cipher_size = 0;
  unsigned char tmp[16]; /* the largest possible MAC using any supported alg */
  unsigned int bytes = L;
  unsigned int keylen = 0;

  keylen = EVP_CIPHER_key_length(cipher);
  if(Kilen == keylen) {
    cipher_size = EVP_CIPHER_block_size(cipher);
    n = L/cipher_size;
    uint2BS(L*8,LA);
    memset(tmp,0,sizeof(tmp));
    if(n <= 0xffffffffL && cipher != NULL) {
      cctx = CMAC_CTX_new();
      if(NULL != cctx) {
	while(bytes > 0) {      
	  uint2BS(i,IA);
	  my_CMAC_Init(cctx,cipher,Ki,Kilen);
	  CMAC_Update(cctx,tmp,cipher_size);
	  CMAC_Update(cctx,IA,4);
	  CMAC_Update(cctx,Label,Llen);
	  CMAC_Update(cctx,(unsigned char *)C00,1);
	  CMAC_Update(cctx,Context,Clen);
	  CMAC_Update(cctx,LA,4);
	  my_CMAC_Final(cctx,tmp,cipher_size);
	  j = (bytes > cipher_size) ? cipher_size : bytes;
	  memcpy(K0,tmp,j);
	  K0 += cipher_size;
	  bytes -= j;
	  i++;
	}
	rv = 1;
	CMAC_CTX_free(cctx);
      }
    } 
  }
  return rv;
}
/*! @brief 
  CMAC Dual Pipeline KDF
  @param x the CMAC cipher
  @param Ki the input key
  @param Kilen the length of the input key
  @param Label nonce data, usually protocol dependent
  @param Llen length of the Label data
  @param Context Nonce Shared information between two parties
  @param Clen length of the Context data
  @param K0 a buffer to hold the generated key
  @param L the length of the generated key => in bits <=
  @return 1 on success, 0 on failure, -1 "something bad happened"
  (invalid input, invalid cipher etc)
*/
int KDF_DP_CMAC(void *x,
		unsigned char *Ki,unsigned int Kilen,
		unsigned char *Label, unsigned int Llen,
		unsigned char *Context, unsigned int Clen,
		unsigned char *K0,unsigned int L
		)
{
  const EVP_CIPHER *cipher = (const EVP_CIPHER *)x;
  int rv = -1;
  long n;
  unsigned long i = 1;
  unsigned long j = 0;
  CMAC_CTX *cctx = NULL;
  unsigned char IA[4];
  unsigned char LA[4];
  unsigned int cipher_size = 0;
  unsigned char tmp[16]; /* the largest possible MAC using any supported alg */
  unsigned char tmpA[16];
  unsigned int bytes = L;
  unsigned int keylen = 0;

  keylen = EVP_CIPHER_key_length(cipher);
  if(Kilen == keylen) {
    cipher_size = EVP_CIPHER_block_size(cipher);
    n = L/cipher_size;
    uint2BS(L*8,LA);
    memset(tmp,0,sizeof(tmp));
    memset(tmpA,0,sizeof(tmpA));
    if(n <= 0xffffffffL && cipher != NULL) {
      cctx = CMAC_CTX_new();
      if(NULL != cctx) {
	while( bytes > 0 ) {  
	  uint2BS(i,IA); 
	  my_CMAC_Init(cctx,cipher,Ki,Kilen);
	  if(i == 1) { /* A(0) = Label || 0x00 || Context || [L] */
	    CMAC_Update(cctx,Label,Llen);
	    CMAC_Update(cctx,(unsigned char *)C00,1);
	    CMAC_Update(cctx,Context,Clen);
	    CMAC_Update(cctx,LA,4);
	  } else { /* A(i) = PRF(Ki,A(i-1) */
	    CMAC_Update(cctx,tmpA,cipher_size);
	  }
	  my_CMAC_Final(cctx,tmpA,cipher_size);
	  /* K(i) = PRF(K,A(i) || [i] || Label || 0x00 || Context || [L] */
	  my_CMAC_Init(cctx,cipher,Ki,Kilen);
	  CMAC_Update(cctx,tmpA,cipher_size);
	  CMAC_Update(cctx,IA,4);
	  CMAC_Update(cctx,Label,Llen);
	  CMAC_Update(cctx,(unsigned char *)C00,1);
	  CMAC_Update(cctx,Context,Clen);
	  CMAC_Update(cctx,LA,4);
	  my_CMAC_Final(cctx,tmp,cipher_size);

	  j = (bytes > cipher_size) ? cipher_size : bytes;
	  memcpy(K0,tmp,j);
	  K0 += cipher_size;
	  bytes -= j;    
	  i++;
	}
	rv = 1;
	CMAC_CTX_free(cctx);
      } 
    }
  }
  return rv;
}
/*! 
  \known Data: KDF data objects contain known answer 
  from 10 rounds of the KDF function. We only check the
  first 16 bytes of output in all modes.
  @brief
  Table defining: 
  - The public KDF name
  - The underlying algorithm name
  - FIPS compliant
  - HMAC or CMAC mode
  - The underlying hash or cipher object the algorithm uses
  - Self test status, 0 untested, 1 passed, -1 failed
  - Known answer data
  @note part of this is initialized on the first call to
  SP800_108_get_KDFbyname()
*/
static KDF_data_t KDFS[] = {
  {"SHA1-CTR","SHA1",0,IS_HMAC,KDF_CTR_HMAC,NULL,0,
   {0xeb,0x41,0xd7,0x5e,0xc3,0x51,0x8b,0x30,
    0xbe,0x28,0xf8,0xc3,0x22,0xb3,0x2f,0x96}},
  {"SHA224-CTR","SHA224",0,IS_HMAC,KDF_CTR_HMAC,NULL,0,
   {0x6a,0x1d,0x18,0xbc,0xa4,0x13,0xee,0x18,
    0xa2,0xca,0xb5,0x92,0xad,0x2c,0x08,0x86}},
  {"SHA256-CTR","SHA256",0,IS_HMAC,KDF_CTR_HMAC,NULL,0,
   {0x8a,0x19,0x8e,0x8f,0xe6,0xd2,0xea,0xb9,
    0xd1,0xf8,0x4a,0x2c,0xaf,0x57,0xff,0x80}},
  {"SHA384-CTR","SHA384",0,IS_HMAC,KDF_CTR_HMAC,NULL,0,
   {0x3d,0xcd,0xb6,0xdc,0x58,0xf0,0x7e,0x46,
    0xdf,0xde,0xd4,0x74,0x03,0x78,0x46,0xab}},
  {"SHA512-CTR","SHA512",0,IS_HMAC,KDF_CTR_HMAC,NULL,0,
   {0xe0,0x59,0xbb,0x22,0x6c,0x5d,0xf2,0x5b,
    0xb6,0xf9,0x05,0xee,0xa4,0xed,0x28,0xb1}},

  {"SHA1-FB","SHA1",0,IS_HMAC,KDF_FB_HMAC,NULL,0,
   {0x13,0x5b,0x77,0xbc,0xef,0x04,0x3c,0x26,
    0xd8,0x57,0xfe,0x6e,0xab,0x77,0x94,0x32}},
  {"SHA224-FB","SHA224",0,IS_HMAC,KDF_FB_HMAC,NULL,0,
   {0x93,0xd1,0x42,0x1f,0x20,0xbf,0x43,0xce,
    0x57,0xcc,0xb9,0x2b,0x23,0xb9,0x0e,0xb6}},
  {"SHA256-FB","SHA256",0,IS_HMAC,KDF_FB_HMAC,NULL,0,
   {0xae,0x47,0xd2,0x43,0x6e,0xed,0x35,0x1d,
    0xa4,0xcf,0xe9,0x07,0xed,0xf4,0xd1,0x2c}},
  {"SHA384-FB","SHA384",0,IS_HMAC,KDF_FB_HMAC,NULL,0,
   {0xc6,0xe3,0x16,0x26,0xd7,0x7e,0x18,0xe6,
    0x0f,0xee,0x93,0x8f,0xea,0x17,0x44,0x4e}},
  {"SHA512-FB","SHA512",0,IS_HMAC,KDF_FB_HMAC,NULL,0,
   {0xf8,0x9e,0x69,0x2e,0x7b,0xea,0x64,0x06,
    0x88,0x30,0x37,0x81,0xe6,0xd3,0x94,0xb8}},

  {"SHA1-DP","SHA1",0,IS_HMAC,KDF_DP_HMAC,NULL,0,
   {0xbf,0x2f,0x2d,0xd6,0xa2,0xc1,0x7a,0x80,
    0xfa,0x95,0xc6,0x5a,0x97,0x99,0x46,0x4c}},
  {"SHA224-DP","SHA224",0,IS_HMAC,KDF_DP_HMAC,NULL,0,
   {0x5f,0x08,0xdd,0x90,0xca,0xdd,0x78,0x90,
    0xa6,0x80,0xc0,0xe7,0x47,0x47,0x0d,0xe5}},
  {"SHA256-DP","SHA256",0,IS_HMAC,KDF_DP_HMAC,NULL,0,
   {0xea,0xe9,0xab,0x75,0xab,0x85,0xb9,0x8c,
    0x95,0xb5,0xd1,0xc9,0xd1,0x2c,0x0d,0x6b}},
  {"SHA384-DP","SHA384",0,IS_HMAC,KDF_DP_HMAC,NULL,0,
   {0xef,0xe8,0x2e,0x45,0x0d,0xa3,0x28,0xb8,
    0x0c,0x47,0x46,0xae,0x55,0x6d,0xc3,0xd0}},
  {"SHA512-DP","SHA512",0,IS_HMAC,KDF_DP_HMAC,NULL,0,
   {0x57,0xa5,0xaa,0x34,0x1f,0x38,0xe4,0xd5,
    0xc9,0x28,0xdc,0xd7,0xe0,0x2c,0x66,0xae}},

  {"AES-128-CTR","AES-128-CBC",0,IS_CMAC,KDF_CTR_CMAC,NULL,0,
   {0x28,0x78,0xd5,0x20,0x36,0x06,0x43,0x1c,
    0xc9,0x99,0x8a,0x3a,0x8f,0xf1,0x6e,0x98}},
  {"AES-192-CTR","AES-192-CBC",0,IS_CMAC,KDF_CTR_CMAC,NULL,0,
   {0x7f,0xb8,0xaa,0x2a,0x58,0x0c,0x78,0xec,
    0x24,0x5b,0x77,0x26,0x64,0x37,0x62,0x06}},
  {"AES-256-CTR","AES-256-CBC",0,IS_CMAC,KDF_CTR_CMAC,NULL,0,
   {0xe9,0x29,0x94,0x29,0x31,0x59,0x2d,0x5d,
    0xa5,0xfb,0x71,0x14,0xb7,0xe8,0x17,0xda}},

  {"AES-128-FB","AES-128-CBC",0,IS_CMAC,KDF_FB_CMAC,NULL,0,
   {0x77,0x78,0xcf,0xc5,0x73,0x8d,0x2d,0x88,
    0x4d,0x84,0x47,0x94,0x11,0x42,0xcf,0x3f}},
  {"AES-192-FB","AES-192-CBC",0,IS_CMAC,KDF_FB_CMAC,NULL,0,
   {0x2a,0x03,0xad,0x18,0xe0,0x88,0x6e,0x57,
    0x72,0xc5,0xa3,0x1f,0x27,0x1c,0xd9,0x70}},
  {"AES-256-FB","AES-256-CBC",0,IS_CMAC,KDF_FB_CMAC,NULL,0,
   {0xbf,0x8e,0x8d,0x56,0xc6,0xc5,0x59,0xa4,
    0xd9,0x32,0xeb,0x1d,0x53,0x7d,0xa1,0x6b}},

  {"AES-128-DP","AES-128-CBC",0,IS_CMAC,KDF_DP_CMAC,NULL,0,
   {0xef,0xd2,0xee,0x9d,0x48,0x0a,0x97,0xaa,
    0xf3,0x8e,0x08,0x73,0xb8,0xe4,0x22,0x9c}},
  {"AES-192-DP","AES-192-CBC",0,IS_CMAC,KDF_DP_CMAC,NULL,0,
   {0xb5,0x79,0xcb,0x08,0x3a,0x1e,0xdd,0x34,
    0x18,0x76,0xaa,0x64,0x54,0x58,0x4f,0xdc}},
  {"AES-256-DP","AES-256-CBC",0,IS_CMAC,KDF_DP_CMAC,NULL,0,
   {0x3c,0xcc,0xef,0x26,0x54,0xb5,0xe4,0x5f,
    0x11,0xae,0xb4,0xb9,0x30,0xed,0x15,0x64}},

  {"CAMELLIA-128-CTR","CAMELLIA-128-CBC",0,IS_CMAC,KDF_CTR_CMAC,NULL,0,
   {0xb5,0x4a,0x68,0x73,0x3e,0xd6,0x9d,0xb1,
    0x8b,0x4d,0xc8,0x52,0x09,0x60,0xec,0xe9}},
  {"CAMELLIA-192-CTR","CAMELLIA-192-CBC",0,IS_CMAC,KDF_CTR_CMAC,NULL,0,
   {0x78,0x5b,0xaf,0x44,0x6c,0x02,0xb6,0x55,
    0x33,0xc9,0x82,0x8a,0xb2,0x13,0xe4,0x55}},
  {"CAMELLIA-256-CTR","CAMELLIA-256-CBC",0,IS_CMAC,KDF_CTR_CMAC,NULL,0,
   {0x25,0x33,0x4a,0xc9,0x84,0x66,0xfe,0x0a,
    0xb2,0xdb,0xfd,0xcf,0x63,0x69,0x7e,0xbf}},

  {"CAMELLIA-128-FB","CAMELLIA-128-CBC",0,IS_CMAC,KDF_FB_CMAC,NULL,0,
   {0x21,0xe1,0x6f,0x28,0xbf,0xc6,0x1e,0x92,
    0x46,0x45,0x36,0x64,0x49,0x78,0x25,0xcb}},
  {"CAMELLIA-192-FB","CAMELLIA-192-CBC",0,IS_CMAC,KDF_FB_CMAC,NULL,0,
   {0xe8,0x67,0x30,0xca,0x2e,0xaf,0x00,0x76,
    0xac,0x53,0xc9,0xf1,0x75,0xb5,0xa3,0x08}},
  {"CAMELLIA-256-FB","CAMELLIA-256-CBC",0,IS_CMAC,KDF_FB_CMAC,NULL,0,
   {0x6a,0x53,0xc6,0x63,0xe0,0xe9,0x18,0x32,
    0x1d,0xea,0x76,0x20,0x97,0x60,0x6b,0x86}},

  {"CAMELLIA-128-DP","CAMELLIA-128-CBC",0,IS_CMAC,KDF_DP_CMAC,NULL,0,
   {0x9f,0xc5,0x55,0xf9,0x40,0x1b,0xed,0xa4,
    0x3a,0x8a,0xbd,0x3c,0xfd,0x50,0x57,0x91}},
  {"CAMELLIA-192-DP","CAMELLIA-192-CBC",0,IS_CMAC,KDF_DP_CMAC,NULL,0,
   {0xda,0x62,0xf3,0xe3,0xf8,0xf1,0xa9,0xc1,
    0xb4,0x07,0xd8,0x4b,0xcf,0xdb,0xc7,0xc5}},
  {"CAMELLIA-256-DP","CAMELLIA-256-CBC",0,IS_CMAC,KDF_DP_CMAC,NULL,0,
   {0xf7,0x92,0xa4,0x89,0xed,0x01,0xf1,0x38,
    0x80,0x42,0x2d,0xde,0x2b,0xf5,0xd7,0x89}},
  {NULL,NULL,0,0,NULL,NULL,0,
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}
};


/*!
 @brief Return a key derivation context for the specified mode
 Implemented modes are:
 [SHA1..SHA512]-[CTR|FB|DP]
 [[AES|CAMELLIA]-[128|192|256]-[CTR|FB|DP]
 i.e. "SHA384-CTR" "AES-192-DP"
 Non-FIPS allowed modes are not returned in FIPS mode.
 @param pcb Internal ICC Context
 @param kdfname The name of the function to use
 @return A KDF_CTX pointer, or NULL
 @note the KDF API is experimental, and may not be stable between ICC releases
*/
const KDF *SP800_108_get_KDFbyname(ICClib *pcb,char *kdfname)
{
  int fips = 0;
  int is_fips = 0; /* See commented block below WRT analyser warnings */
  int nid = 0;
  int i = 0;
  KDF_data_t *kdf = NULL;
  
  if(NULL != pcb) {
    fips = pcb->flags & ICC_FIPS_FLAG;
  }

  if(!(fips && getErrorState()) ) {
    for(i = 0 ;NULL != KDFS[i].name ; i++) {
      if(strcasecmp(KDFS[i].name,kdfname) == 0) {
        if(-1 != KDFS[i].tested) {
          if( !fips || KDFS[i].fips ) { /* Restrict algs in FIPS mode */
            is_fips = 1;
            switch(KDFS[i].mode) {
            case IS_HMAC:
              /* Initialize now if it wasn't previously done */
              if(NULL == KDFS[i].handle) {
                KDFS[i].handle = (void *)EVP_get_digestbyname(KDFS[i].algname);
              }
              /* And no, this can't be an "else" we don't know that 
                 META_EVP_get_digestbyname() above won't fail
              */
              if(NULL != KDFS[i].handle) {
                kdf = &KDFS[i];
                nid = EVP_MD_type(KDFS[i].handle);
              }
              break;
            case IS_CMAC:
              if(NULL == KDFS[i].handle) {
                KDFS[i].handle = (void *)EVP_get_cipherbyname(KDFS[i].algname);
              }
              if(NULL != KDFS[i].handle) {
                kdf = &KDFS[i];
                nid = EVP_CIPHER_type(KDFS[i].handle);
              }
              break;
            default:
              break;
            }
          }
          /* Break from the for(;;) loop, we matched the KDF name */
          break;
        }
      }
    }
  }
  
  if(NULL != kdf) {
    if(0 == kdf->tested) {
      KDF_KA(kdf);
    }
    if(-1 == kdf->tested) { 
      /* Failed self test at some point, unusable */
      kdf = NULL;
    }
  }
  if((NULL != kdf) && (NULL != pcb) && (NULL != pcb->callback)) {
    (*pcb->callback)("SP800_108_get_KDFbyname",nid,0 /*is_fips*/); /* Until we actually pass */
  }
  return (const KDF *)kdf;
}


/*!
 @brief Perform a Key Derivation function based on one of the 
 modes described in NIST SP800-108 
 @param xctx a KDF 
        Note that the KDF_CTX retains no sensitive data between calls
 @param Ki The derivation key
 @param Kilen The length of the derivation key
 @param Label Protocol specific nonce data
 @param Llen The length of Label 
 @param Context Instance specific nonce data
 @param Clen length of Context
 @param K0 The buffer in which to store the derived key
 @param L The length in BYTES of the derived key 
 @return 1 on success, 0 on failure, -1 on "something bad happened" 
 @note the KDF API is experimental, and may not be stable between ICC releases
*/
int SP800_108_KDF(const KDF *xctx,
		  unsigned char *Ki,unsigned int Kilen,
		  unsigned char *Label, unsigned int Llen,
		  unsigned char *Context, unsigned int Clen,
		  unsigned char *K0,unsigned int L)
{ 
  KDF_data_t *kctx = (KDF_data_t *)xctx;
  int rv = 0;
  if((NULL != kctx) && (NULL != kctx->handle) && (NULL != kctx->kdf)) {
    rv = (*(kctx->kdf))(kctx->handle,Ki,Kilen,
			Label,Llen,
			Context,Clen,
			K0,L);
  } 
  return rv;
}
/*!
  @brief get the list of FIPS compliant 
  SP800-108 modes so we can iterate through them
  (running the self tests) during POST
  @return the list of FIPS complaint KDF modes
*/
const char **get_SP800_108FIPS(void)
{
  static int initialized = 0;
  static char *Fips_list[sizeof(KDFS)/sizeof(KDF_data_t)];
  int i = 0;
  int j = 0;
  if(!initialized) {
    for(i = j = 0; NULL != KDFS[i].name; i++) {
      if(KDFS[i].fips) {
	Fips_list[j] = (char *)KDFS[i].name;
	j++;
      }
    }
    initialized = 1;
  }
  return (const char **)Fips_list;
}
/*!
  @brief clear the "tested" field in the structure if
  it wasn't a fail so that the SelfTest code can
  retest the functions
*/

void SP800_108_clear_tested(void)
{
  int i = 0;
  for(i = 0; NULL != KDFS[i].name; i++) {
    if(1 == KDFS[i].tested) {
      KDFS[i].tested = 0;
    }
  }
} 

   
