/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: SP800-38F AES Key Wrap
//
*************************************************************************/


#include <openssl/evp.h>
#include <string.h>
#include "icc.h"
#include "fips-prng/utils.h"

extern void * CRYPTO_calloc(int nmemb,int size,const char *file, int line);

/** @brief
    Data structure for a semi-block 
*/
typedef struct {
  unsigned char F[8];
} KWX;

static unsigned char BE_1 = 1; /*!< Constant 1 */
static unsigned char BE_6 = 6; /*!< Constant 6 */
static unsigned char minus_1[8] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff}; /*!< Constant -1 */

static const KWX A0 = {{0xA6,0xA6,0xA6,0xA6,0xA6,0xA6,0xA6,0xA6}}; /*!< Check code for unpadded wrap */
static const KWX AP = {{0xA6,0x59,0x59,0xA6,0x00,0x00,0x00,0x00}}; /*!< Check code for padded wrap */

/*! 
  @brief Convert the key length into an AES cipher descriptor 
  @param kl key length, may be specified in bits or bytes as they can be disambiguated
  @return a cipher or NULL on failure
 */
static const EVP_CIPHER * GetCipher(int kl)
{
  const EVP_CIPHER *cip = NULL;
  switch(kl) {
  case 16:
  case 128:
    cip = EVP_get_cipherbyname("AES-128-ECB");
    break;
  case 24:
  case 192:
    cip = EVP_get_cipherbyname("AES-192-ECB");
    break;
  case 32:
  case 256:
    cip = EVP_get_cipherbyname("AES-256-ECB");
    break;
  default:
    break;
  }                  
  return cip;
}
/** @brief Initialize an encrypt operation
    @param cctx cipher context
    @param key  a pointer to a buffer holding an AES key
    @param kl the key length (in bits)
    @return 1 O.K.
*/
static SP800_38F_ERR EncryptInit(EVP_CIPHER_CTX *cctx,unsigned char *key, int kl)
{
  SP800_38F_ERR rv = SP800_38F_PARAM;
  const EVP_CIPHER *cip = GetCipher(kl);
  if(NULL != cip) {
    if( 1 ==  EVP_EncryptInit(cctx,cip,key,NULL) ) {
      rv = SP800_38F_OK;
    }
    EVP_CIPHER_CTX_set_padding(cctx,0);
  }
  return rv;
}

/** @brief  Basic indexed encryption transform 
    @param cctx cipher context
    @param in a pointer to an 8 byte input block
    @param out a pointer to an 8 byte output block
    @return 1 O.K.
*/
static int Encrypt(EVP_CIPHER_CTX *cctx,KWX * in,KWX *out)
{
  int outl = 0;
  return EVP_EncryptUpdate(cctx,(unsigned char *)out,&outl,(unsigned char *)in,16);
}

#if 0
/** @brief Cleanup from encrypt
    @param cctx cipher context
    @return 1 O.K.
*/

static int EncryptFinal(EVP_CIPHER_CTX *cctx) 
{
  int rv = 0;
  int outl = 0;
  char buffer[16];
  rv = EVP_EncryptFinal(cctx,buffer,&outl);

  
  return rv;
}
#endif

/** @brief Initialize a decrypt operation
    @param cctx cipher context
    @param key  a pointer to a buffer holding an AES key
    @param kl the key length (in bits)
    @return 1 O.K.
*/
static SP800_38F_ERR DecryptInit(EVP_CIPHER_CTX *cctx,unsigned char *key,int kl)
{
  SP800_38F_ERR rv = SP800_38F_PARAM;
  const EVP_CIPHER *cip = GetCipher(kl);
  if(NULL != cip) {
    if(1  ==  EVP_DecryptInit(cctx,cip,key,NULL) ) {
      rv = SP800_38F_OK;
    }
    EVP_CIPHER_CTX_set_padding(cctx,0);
  }
  return rv;
}
/** @brief  Basic indexed decryption transform 
    @param cctx cipher context
    @param in a pointer to an 8 byte input block
    @param out a pointer to an 8 byte output block
    @return 1 O.K.
*/
static int Decrypt(EVP_CIPHER_CTX *cctx,KWX *in,KWX *out)
{
  int outl = 0;
  return EVP_DecryptUpdate(cctx,(unsigned char *)out,&outl,(unsigned char *)in,16);
}

#if 0
/** @brief Cleanup from decrypt
    @param cctx cipher context
    @return 1 O.K.
*/

static int DecryptFinal(EVP_CIPHER_CTX *cctx)
{
  int rv = 0;
  int outl;
  char buffer[16];
  rv = EVP_DecryptFinal(cctx,buffer,&outl);
  return rv;
}
#endif

/**  
     @brief Key Wrap function 
     @param in input buffer
     @param inl length of input buffer
     @param out output buffer (length of input +16)
     @param outl place to store the output length
     @param key the AES key
     @param kl Size of the AES key (bits)
     @param isEnc 1 Encrypt is used as the wrap function, 0 decrypt is used as the wrap function
     @param pad 1 if padding is enabled
     @return 1 O.K., length of output in *outl, 3 range error in input, 2 Unwrap mac mismatch
 */
static int KW(unsigned char *in, int inl, unsigned char *out, int *outl, unsigned char *key, int kl, int isEnc,int pad)
{
  SP800_38F_ERR rv = SP800_38F_OK;
  int i = 0;
  int j = 0;
  int n = 0; 
  int k = 0;
  KWX *R = NULL; /* Temporary output buffers */
  KWX *I = NULL; /* Overlay for the inpt buffer */ 
  KWX *C = NULL; /* Overlay for the output buffer */
  KWX t;
  KWX B[2];
  KWX T[2];
  KWX A;
  int padlen = 0;

  EVP_CIPHER_CTX *cctx = NULL;
  cctx = EVP_CIPHER_CTX_new();
  if(NULL == cctx) {
    rv = SP800_38F_MEM;
  }
  if(SP800_38F_OK == rv) {
    if(isEnc) {
      rv = EncryptInit(cctx,key,kl);
    } else {
      rv = DecryptInit(cctx,key,kl);
    }
  }
  if(SP800_38F_OK == rv) {
    if(! pad) { /* unpadded */
      /* Check that the length criteria for the input are met 
	 With padding off, it must be complete semi-blocks
      */
      n = inl/8;
      if(((n*8) != inl) || (n < 2)) {
	*outl = 0;
	rv = SP800_38F_PARAM;
      }
      /* 2 to (2^54)-1 semiblocks */
      if((sizeof(n) > 4) && (sizeof(long) > 4)) {
#if defined(_WIN64)
	long long l  = 0x40000000000000 - 1;
#else
	long l = 0x40000000000000 - 1;
#endif
	if(n > l) {
	  rv = SP800_38F_PARAM;
	}
      }
      /* Copy the tag to the working area */
      memcpy(&A,&A0,sizeof(KWX));
      k = n;
    } else { /* Padded */
      n = (inl+7)/8;
      k = inl/8; /* Number of complete blocks */
      /* Copy the different tag to the working area */
      memcpy(&A,&AP,sizeof(KWX));
      padlen = inl;
      for(i = 7; i > 3; i--) { /* Insert pad length, BE, bytes */
	A.F[i] = padlen & 0xff;
	padlen >>= 8;
      }
      /* 1 to (2^32)-1 octets */
      if(inl > 32767) {
	rv = SP800_38F_PARAM;
      }
    }
  }
  if(SP800_38F_OK == rv) {
  
    memset(&t,0,sizeof(t));
    /* Allocate working buffers */
    R = CRYPTO_calloc(n,sizeof(KWX),__FILE__,__LINE__);
    if(NULL == R) {
      rv = SP800_38F_MEM;
    }
  }
  if(SP800_38F_OK == rv) {
    I = (KWX *)in;
    /* Copy the whole semiblocks */
    for( i = 0; i < k; i++) { 
      memcpy(&R[i],&I[i],sizeof(KWX));
    }
    /* If there's a partial semiblock (padded mode)
       copy that and zero pad the end of the block
    */
    if(inl & 7) {
      for(i = 0; i < (inl & 7); i++) {
	R[k].F[i] = in[(k*8)+i];
      }
      for( ; i < 8; i++) {
	R[k].F[i] = 0;
      }
    }
    /* 
       If the length is <= 1 semi-block
       just encrypt the tag and data (One AES block)
       as the extra winding around doesn't add anything
       useful. And to save time, just stuf fthat directly 
       into the output buffer 
    */
    if(pad && (inl <= 8)) {
      memcpy(&T[0],&A,sizeof(KWX));
      memcpy(&T[1],&R[0],sizeof(KWX));
      if(isEnc) {
	Encrypt(cctx,&T[0],(KWX *)out);
      } else {
	Decrypt(cctx,&T[0],(KWX *)out);
      }
      *outl = 16;
    } else  {
      /* Else do the full rotate thing */
      for(j = 0; j < 6; j++) {    
	for( i = 0; i < n; i++) {
	  Add_BE((unsigned char *)&t,(unsigned char *)&t,8,&BE_1,1); 
	  memcpy(&T[0],&A,sizeof(KWX));
	  memcpy(&T[1],&R[i],sizeof(KWX));
	  if(isEnc) {
	    Encrypt(cctx,&T[0],&B[0]);
	  } else {
	    Decrypt(cctx,&T[0],&B[0]);
	  }
	  xor((unsigned char *)&A,(unsigned char *)&B[0],(unsigned char *)&t,sizeof(KWX));
	
	  memcpy(&R[i],&B[1],sizeof(KWX));
	}
      }
  
      C = (KWX *)out;
      memcpy(&C[0],&A,sizeof(KWX));
      *outl = 8;
      for(i = 0; i < n; i++) {
	memcpy(&C[i+1],&R[i],sizeof(KWX));
	*outl += 8;
      }
    }
    CRYPTO_free(R,__FILE__,__LINE__);
  }
  if( NULL != cctx ) {
    EVP_CIPHER_CTX_cleanup(cctx);
    EVP_CIPHER_CTX_free(cctx);
  }
  return rv;
}



/*! 
  @brief Key unwrap function 
  @param in input buffer
  @param inl length of input buffer
  @param out output buffer (length of input +16)
  @param outl place to store the output length
  @param key the AES key
  @param kl Size of the AES key (bits)
  @param isEnc 1 Encrypt is used as the wrap function, 0 decrypt is used as the wrap function
  @param isPad 1 if padding is enabled
  @return 1 O.K., length of output in *outl, 3 range error in input, 2 Unwrap mac mismatch
*/
static int KU(unsigned char *in, int inl, unsigned char *out, int *outl, unsigned char *key, int kl,int isEnc,int isPad)
{
  SP800_38F_ERR rv = SP800_38F_OK;
  int i = 0;
  int j = 0;
  int n = 0;
  KWX *C = NULL; /* Working space, which may be quite considerable */
  KWX *R = NULL; /* Temporary output buffers */
  KWX *P = NULL; /* Overlay for the output buffer */
  KWX t;
  KWX B[2];
  KWX T[2];
  KWX A;
  int bytes = 0;
#if defined(_WIN64)
  long long l = 0;
#else
  long l = 0;
#endif
  EVP_CIPHER_CTX *cctx = NULL;

  cctx = EVP_CIPHER_CTX_new();
  if(NULL == cctx) {
    rv = SP800_38F_MEM;
  }
  if(SP800_38F_OK == rv) {
    if(isEnc) {  /* These are unwrap paths, Encrypt is used on the forward path, decrypt going back */
      rv = DecryptInit(cctx,key,kl);
    } else {
      rv = EncryptInit(cctx,key,kl);
    }
  }
  if(SP800_38F_OK == rv) {
    n = inl/8;
 
    /* Check that the length criteria for the input are met 
       UnWrap input will always be a integer multiple of semiblocks
       Padded mode, 2 semiblocks is the minimum
       Unpadded 3
    */
    if((n*8) != inl) {
      rv = SP800_38F_DATA;
    }
  }
  *outl = 0;
  if(SP800_38F_OK == rv) {
    if(isPad) { /* Padded */
      if(n < 2) {
	rv = SP800_38F_DATA;
      }
      l = 0x20000000;
      if(n > l) {
	rv = SP800_38F_DATA;
      } 
    } else { /* Unpadded */
      if(n < 3) {
	rv = SP800_38F_DATA;
      }      
      if(sizeof(n) > 4) {
	l = 0x40000000000000;
	if(n > l) {	  
	  rv = SP800_38F_DATA;
	}
      }
    }
  }
  if(SP800_38F_OK == rv) {
    memset(&t,0,sizeof(t));
    /* Allocate working buffers */
    R = CRYPTO_calloc(n,sizeof(KWX),__FILE__,__LINE__);
    if(NULL == R) {
      rv = SP800_38F_MEM;
    }
  }

  C = (KWX *)in;

  if(SP800_38F_OK == rv) {
    if(isPad && n == 2) {
      memcpy(&T[0],&C[0],2*sizeof(KWX));
      if(isEnc) {
	Decrypt(cctx,&T[0],&B[0]);
      } else {
	Encrypt(cctx,&T[0],&B[0]);
      }
      memcpy(&A,&B[0],sizeof(KWX));
      memcpy(&R[0],&B[1],sizeof(KWX));
    } else {
      memcpy(&A,&C[0],sizeof(KWX));
      for( i = 0; i < (n-1); i++) {
	memcpy(&R[i],&C[i+1],sizeof(KWX));
      }
      n = n -1;
      for( i = 0; i < n; i++) {
	Add_BE((unsigned char *)&t,(unsigned char *)&t,8,&BE_6,1);
      }

      for(j = 0; j < 6 ; j++) {    
	for( i = n; i > 0; i--) {
	  xor((unsigned char *)&A,(unsigned char *)&A,(unsigned char *)&t,sizeof(KWX));
	  memcpy(&T[0],&A,sizeof(KWX));
	  memcpy(&T[1],&R[i-1],sizeof(KWX));
	  if(isEnc) {
	    Decrypt(cctx,&T[0],&B[0]);
	  } else {
	    Encrypt(cctx,&T[0],&B[0]); /* Yes, 0, param not needed now */
	  }
	  memcpy(&A,&B[0],sizeof(KWX));
	  memcpy(&R[i-1],&B[1],sizeof(KWX));
	  Add_BE((unsigned char *)&t,(unsigned char *)&t,8,minus_1,8);
	}
      }
    }

    P = (KWX *)out;
    *outl = 0;
    if(isPad) {
      /* This is ugly, because once we extract the unpadded length
	 we have to recalc the number of complete and partial
	 blocks to copy to the output. Unlike unpadded mode
	 it's not implicit in the input size
      */
      if(memcmp(&A,&AP,4) == 0 ) {
	bytes = 0;
	for(i = 4; i < 8; i++) {
	  bytes <<= 8;
	  bytes += A.F[i];
	}
	n = (bytes/8);  /* Number of complete semi-blocks */
	if(SP800_38F_OK == rv) {
	  for(i = 0; i < n; i++) {
	    memcpy(&P[i],&R[i],sizeof(KWX));
	    (*outl) += 8;
	  }
	  j = i; /* The remaining bytes */
	  for(i = 0; i < (bytes &7); i++) {
	    P[j].F[i] = R[j].F[i];
	    (*outl) ++;
	  } 
	  for(  ; i < 8; i++) { /* And check that the padding WAS 0's */
	    if(R[j].F[i] != 0) {
	      memset(out,0,*outl); /* On a padding error Scrub what was decrypted so far */
	      (*outl) = 0;
	      rv = SP800_38F_MAC; /* Padding error in final block */
	    }
	  }
	}
      } else {
	rv = SP800_38F_MAC;
      }      
    } else {
      if(memcmp(&A,&A0,sizeof(KWX)) == 0) { 
	for(i = 0; i < n; i++) {
	  memcpy(&P[i],&R[i],sizeof(KWX));
	  *outl += 8;
	}
      } else {
	rv = SP800_38F_MAC;
      }
    }
    CRYPTO_free(R,__FILE__,__LINE__);
  }
  if(NULL != cctx) {
    EVP_CIPHER_CTX_cleanup(cctx);
    EVP_CIPHER_CTX_free(cctx);
  }
  return rv;
}


/*! 
  @brief Key unwrap function, Public API
  @param in input buffer
  @param inl length of input buffer
  @param out output buffer (length of input +16)
  @param outl place to store the output length
  @param key the AES key
  @param kl Size of the AES key (bits)
  @param flags 
  - 1 Wrap 
  - 2 Forward decrypt
  - 4 Pad
  @return 1 O.K., length of output in *outl  
  - 0 Parameter error 
  - 2 Unwrap mac mismatch
  - 3 range error in input, 
  - 4 Memory error
*/
int SP800_38F_KW(unsigned char *in, int inl, unsigned char *out, int *outl, unsigned char *key, int kl,unsigned int flags)
{
  int rv = 1;
  switch(flags) {
    /* Wrap paths */
  case ICC_KW_WRAP:
    rv = KW(in,inl,out,outl,key,kl,1,0);
    break;
  case ICC_KW_WRAP | ICC_KW_FORWARD_DECRYPT:
    rv = KW(in,inl,out,outl,key,kl,0,0);
    break;
  case ICC_KW_WRAP  |  ICC_KW_PAD:
    rv = KW(in,inl,out,outl,key,kl,1,1);
    break;
  case ICC_KW_WRAP  | ICC_KW_FORWARD_DECRYPT | ICC_KW_PAD:
    rv = KW(in,inl,out,outl,key,kl,0,1);
    break;
    /* Unwrap paths */
  case 0:
    rv = KU(in,inl,out,outl,key,kl,1,0);
    break;
  case ICC_KW_FORWARD_DECRYPT:
    rv = KU(in,inl,out,outl,key,kl,0,0);
    break;
  case ICC_KW_PAD:
    rv = KU(in,inl,out,outl,key,kl,1,1);
    break;
  case  ICC_KW_FORWARD_DECRYPT | ICC_KW_PAD:
    rv = KU(in,inl,out,outl,key,kl,0,1);
    break;
  default:
    rv = 0;
    break;
  }
  return rv;
}


#if defined(STANDALONE)
void xor(unsigned char *dest, unsigned char *s1, unsigned char *s2, unsigned blen)
{
  unsigned int i;
  for(i = 0; i < blen; i++) {
    dest[i] = s1[i] ^ s2[i];
  }
}
void Add_BE(unsigned char *dest,
         unsigned char *src1,unsigned int s1, 
         unsigned char *src2,unsigned int s2)
{
  unsigned int t = 0,t1 = 0,t2 = 0;
  int i = 0;
  unsigned int cy = 0;
  if(0 == s2) s2 = s1;
  
  for(i = 1; i <= (int)s1 ; i++) {
    t1 = src1[s1-i];
    t2 = 0;
    if( ((int)s2 - i) >= 0) {
      t2 = src2[s2 - i];
    }
    t = t1 + t2 + cy;     
    cy = 0;
    if(t > 255 ) {
      cy = 1;
    }
    dest[s1-i] = t & 0xff;
  }
}

void *CRYPTO_calloc(int nmemb, int size,const char *file,int line)
{
  void *ptr = NULL;
  ptr = CRYPTO_malloc(nmemb*size,file,line);
  if(NULL != ptr) {
    memset(ptr,0,nmemb*size);
  }
  return ptr;
}
int main(int argc, char *argv[])
{
  int rv = 0;
  char test[32] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,19,30,31,32};
  unsigned char key[16] =  {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
  char out[48];
  char test1[32];
  int outl = 0;
  
  OPENSSL_init_crypto(OPENSSL_INIT_NO_LOAD_CONFIG |OPENSSL_INIT_LOAD_CRYPTO_STRINGS |OPENSSL_INIT_ADD_ALL_DIGESTS | OPENSSL_INIT_ADD_ALL_CIPHERS  ,NULL);

  if(1 != SP800_38F_KW(test,32,out,&outl,key,128,ICC_KW_WRAP)) {
    rv = 1;
  }
  if(1 != SP800_38F_KW(out,40,test1,&outl,key,128,0)) {
    rv = 1;
  }
  if(0 != memcmp(test,test1,sizeof(test)) ) {
    printf("KWE/KUE error\n");
    rv = 1;
  }
  if(1 != SP800_38F_KW(test,32,out,&outl,key,128,ICC_KW_WRAP | ICC_KW_FORWARD_DECRYPT)) {
    rv = 1;
  }
  if(1 != SP800_38F_KW(out,40,test1,&outl,key,128,ICC_KW_FORWARD_DECRYPT)) {
    rv = 1;
  }
  if(0 != memcmp(test,test1,sizeof(test)) ) {
    printf("KWD/KUD error\n");
    rv = 1;
  }


  if(1 != SP800_38F_KW(test,2,out,&outl,key,128,ICC_KW_WRAP | ICC_KW_PAD)) {
    rv = 1;
  }
  if(1 != SP800_38F_KW(out,outl,test1,&outl,key,128,ICC_KW_PAD)) {
    rv = 1;
  }
  if(0 != memcmp(test,test1,sizeof(test)) ) {
    printf("KWEP/KUEP error\n");
    rv = 1;
  }
  if(1 != SP800_38F_KW(test,5,out,&outl,key,128,ICC_KW_WRAP | ICC_KW_FORWARD_DECRYPT | ICC_KW_PAD)) {
    rv = 1;
  }
  if(1 != SP800_38F_KW(out,outl,test1,&outl,key,128,ICC_KW_FORWARD_DECRYPT | ICC_KW_PAD)) {
    rv = 1;
  }
  if(0 != memcmp(test,test1,sizeof(test)) ) {
    printf("KWDP/KUDP error\n");
    rv = 1;
  }
  OPENSSL_cleanup();
  return rv;
}
#endif
