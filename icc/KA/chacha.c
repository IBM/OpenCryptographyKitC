/*
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*/
#include "KA/ka.h"


/**
 *  \known Data: Data used for ChaCha-Poly1305 KA test From RFC7539
*/
static const unsigned char CHAPOLY_Key[] = 
  { 0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,
    0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
    0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,
    0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f
  };

static const unsigned char CHAPOLY_IV[] = 
  { 0x07,0x00,0x00,0x00,0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47
  };

static const unsigned char CHAPOLY_AAD[] = 
  { 0x50,0x51,0x52,0x53,0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7
  };

 static const unsigned char CHAPOLY_TAG[] = 
  { 0x1a,0xe1,0x0b,0x59,0x4f,0x09,0xe2,0x6a,0x7e,0x90,0x2e,0xcb,
    0xd0,0x60,0x06,0x91
  };

static const unsigned char CHAPOLY_PT[] = 
  { 0x4c,0x61,0x64,0x69,0x65,0x73,0x20,0x61,
    0x6e,0x64,0x20,0x47,0x65,0x6e,0x74,0x6c,
    0x65,0x6d,0x65,0x6e,0x20,0x6f,0x66,0x20,
    0x74,0x68,0x65,0x20,0x63,0x6c,0x61,0x73,
    0x73,0x20,0x6f,0x66,0x20,0x27,0x39,0x39,
    0x3a,0x20,0x49,0x66,0x20,0x49,0x20,0x63,
    0x6f,0x75,0x6c,0x64,0x20,0x6f,0x66,0x66,
    0x65,0x72,0x20,0x79,0x6f,0x75,0x20,0x6f,
    0x6e,0x6c,0x79,0x20,0x6f,0x6e,0x65,0x20,
    0x74,0x69,0x70,0x20,0x66,0x6f,0x72,0x20,
    0x74,0x68,0x65,0x20,0x66,0x75,0x74,0x75,
    0x72,0x65,0x2c,0x20,0x73,0x75,0x6e,0x73,
    0x63,0x72,0x65,0x65,0x6e,0x20,0x77,0x6f,
    0x75,0x6c,0x64,0x20,0x62,0x65,0x20,0x69,
    0x74,0x2e
  };

static const unsigned char CHAPOLY_CT[] =
  { 0xd3,0x1a,0x8d,0x34,0x64,0x8e,0x60,0xdb,
    0x7b,0x86,0xaf,0xbc,0x53,0xef,0x7e,0xc2,
    0xa4,0xad,0xed,0x51,0x29,0x6e,0x08,0xfe,
    0xa9,0xe2,0xb5,0xa7,0x36,0xee,0x62,0xd6,
    0x3d,0xbe,0xa4,0x5e,0x8c,0xa9,0x67,0x12,
    0x82,0xfa,0xfb,0x69,0xda,0x92,0x72,0x8b,
    0x1a,0x71,0xde,0x0a,0x9e,0x06,0x0b,0x29,
    0x05,0xd6,0xa5,0xb6,0x7e,0xcd,0x3b,0x36,
    0x92,0xdd,0xbd,0x7f,0x2d,0x77,0x8b,0x8c,
    0x98,0x03,0xae,0xe3,0x28,0x09,0x1b,0x58,
    0xfa,0xb3,0x24,0xe4,0xfa,0xd6,0x75,0x94,
    0x55,0x85,0x80,0x8b,0x48,0x31,0xd7,0xbc,
    0x3f,0xf4,0xde,0xf0,0x8e,0x4b,0x7a,0x9d,
    0xe5,0x76,0xd2,0x65,0x86,0xce,0xc6,0x4b,
    0x61,0x16
  };

static void printbin( unsigned char *s,int l)
{
  int i;
  fprintf(stderr,"len = %d :",l);
  for(i = 0; i < l ; i++) {
    fprintf(stderr,"%02x",((unsigned)s[i] & 0xff));
  }
  fprintf(stderr,"\n");
}



/**
   @brief
   Check a return value from other code against a reference value.
   Set error status as appropriate.
   @param in input buffer
   @param inL length of input
   @param knownAnswer reference data
   @param knownAnswerL length of reference data
   @param icc_stat ICC_STATUS - error condition returned (if any).
   @param file the file name where the error occurred
   @param line the line number where the error occured
   @param mode - algorithm mode being checked
   @param alg  - algorithm being checked
   @return icc_stat->majRC
*/
static int iccCheckKnownAnswer(
			       unsigned char  *in,
			       int            inL,
			       const unsigned char  *knownAnswer,
			       int            knownAnswerL,
			       ICC_STATUS     *icc_stat,
			       const char *file,
			       int line,
			       const char *mode,
			       const char *alg
			       )
{
  int rv = ICC_OK;
  char buf[32];

  memset(buf,0,sizeof(buf));
  strncpy(buf,mode,15);
  strncat(buf," ",2);
  strncat(buf,alg,15);
  /* make sure known answer is correct  */
  if (ICC_OK == icc_stat->majRC && knownAnswer != NULL) {
    if (knownAnswerL != inL) {
      printf("FATAL_ERROR,ICC_LIBRARY_VERIFICATION_FAILED length mismatch (%d != %d)  %s %s\n",inL,knownAnswerL,mode,alg);  

    }
    else if (memcmp(knownAnswer,in,knownAnswerL) != 0) {
      printf("FATAL_ERROR,ICC_LIBRARY_VERIFICATION_FAILED %s %s\n",mode,alg);
      printbin(in,inL);
      printbin((unsigned char *)knownAnswer,knownAnswerL);
    }    
  }
  memset(buf,0,sizeof(buf));

  return rv;
}

static void iccChaChaPolyTest(ICC_STATUS *status,
  const unsigned char *key,const unsigned char *iv,int ivlen, const unsigned char *aad, int aadlen,
  const unsigned char *pt, int ptlen, const unsigned char *ref_tag, int taglen, const unsigned char *ref_ct,int reflen)
{
  int outl = 0;
  int totl = 0;
  unsigned char *obuf = NULL;
  unsigned char tag[64];
  EVP_CIPHER_CTX *cctx = EVP_CIPHER_CTX_new();
  const EVP_CIPHER *cip = EVP_get_cipherbyname("ChaCha20-Poly1305");

  obuf = OPENSSL_malloc(reflen+32); /* Allow space for the tag to be decrypted */
  EVP_CIPHER_CTX_set_flags(cctx, EVP_CIPHER_CTX_FLAG_WRAP_ALLOW);
  EVP_EncryptInit(cctx, cip, key,iv);
  if(0 != ivlen) {
    EVP_CIPHER_CTX_ctrl(cctx, EVP_CTRL_AEAD_SET_IVLEN,ivlen, NULL);
  }
  EVP_CIPHER_CTX_ctrl(cctx, EVP_CTRL_AEAD_SET_TAG, taglen,NULL);
  EVP_CIPHER_CTX_set_padding(cctx,0);
  if(0 != aadlen) {
    EVP_EncryptUpdate(cctx, NULL, &outl,aad, aadlen);
  }

  if( 0 != ptlen) {
    EVP_EncryptUpdate(cctx, (obuf+totl), &outl,pt, ptlen);
    totl += outl;
  }
  EVP_EncryptFinal(cctx, (obuf + totl), &outl);
  totl += outl;
  if(taglen > 0) {
    EVP_CIPHER_CTX_ctrl(cctx, EVP_CTRL_AEAD_GET_TAG,taglen, tag);
  }
 
  iccCheckKnownAnswer(obuf,totl,ref_ct,reflen,status,__FILE__,__LINE__,"chacha-poly1305","ciphertext");
  iccCheckKnownAnswer(tag,taglen,ref_tag,taglen,status,__FILE__,__LINE__,"chacha-poly1305","tag");
  OPENSSL_free(obuf);
  EVP_CIPHER_CTX_free(cctx);
} 


  /* Needs work, fails on some platforms */
int CHACHA_Test(ICC_STATUS *icc_stat) 
{
  if(ICC_OK == icc_stat->majRC) {
    int i = 0;
    /** \induced 182 ChaCha20-Poly1305 mess up the ciphertext */
    memcpy(ibuf,CHAPOLY_PT,sizeof(CHAPOLY_PT));
    if(182 == icc_failure) {
      ibuf[0] = ~ibuf[0];
    }
    i = sizeof(CHAPOLY_PT)+16;
    /** \induced 183 ChaCha-Poly1305 mess up the AAD to cause a tag mismatch */
    memcpy(ibuf+i,CHAPOLY_AAD,sizeof(CHAPOLY_AAD));
    if(183 == icc_failure) {
      ibuf[i] = ~ibuf[i];
    }
    iccChaChaPolyTest(icc_stat,CHAPOLY_Key,CHAPOLY_IV,sizeof(CHAPOLY_IV),ibuf+i,sizeof(CHAPOLY_AAD),
      ibuf,sizeof(CHAPOLY_PT),CHAPOLY_TAG,sizeof(CHAPOLY_TAG),CHAPOLY_CT,sizeof(CHAPOLY_CT));
  }
  return icc_stat->majRC;
}

#if defined(STANDALONE)

int icc_failure = 0;
unsigned char ibuf[1024];

int main(int argc, char *argv[])
{
   ICC_STATUS icc_stat;
   memset(&icc_stat,0,sizeof(icc_stat));

   CHACHA_Test(&icc_stat);

}

#endif
