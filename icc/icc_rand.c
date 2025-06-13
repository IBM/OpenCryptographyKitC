/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
//           This module provides a random number generator seed function for OpenSSL
//           It's used to build IBM's own "openssl" executable which we use to sign
//           the shared libraries. openssl is also a useful tool to have in the SDK.
// 
*************************************************************************/

#if defined(_WIN32) || defined(_OS2__)
#include "windows.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#endif

#if defined(_WIN32) || defined(_OS2__)
typedef HANDLE LMutex;
#elif defined(__linux) || defined(_AIX) || defined(__sun) || defined(__hpux) || defined(__MVS__) || defined(__APPLE__)
#  if defined(__MVS__)
#     ifndef pthread_included
#        define pthread_included
#        define _OPEN_THREADS 1
#     endif
#  endif
typedef pthread_mutex_t LMutex;
#elif defined(__OS2__)
typedef HMTX LMutex;
#else
#  error Please provide platform specific code here.
#endif



#include "openssl/evp.h"
#include "openssl/rand.h"
#include "openssl/err.h"
#include "openssl/hmac.h"

#include "iccerr.h"
#include "fips-prng/fips-prng-err.h"
#include "iccerr.c"
#include "fips-prng/fips-prng-err.c"

#include "noise_to_entropy.h"

#include "TRNG_FIPS.h"

#define efOPENSSL_HW_rand OPENSSL_HW_rand
int OPENSSL_HW_rand(unsigned char *buf);

/* Yes, it's included rather than linked as a separate object 
   The same source is used by multiple projects.
*/
int icc_failure = 0;

void ICC_Free(void *ptr) 
{
  CRYPTO_free(ptr);
}


void *ICC_Calloc(
		 size_t n, 
		 size_t sz, 
		 const char *file, 
		 int line)
{
  void *ptr = NULL;
  ptr = CRYPTO_malloc((n*sz),file,line);
  if(NULL != ptr) {
    memset(ptr,0,(n*sz));
  }
  return ptr;
}




ENTROPY_IMPL MY_TRNG = {
    "TRNG_FIPS",       /*!< Common name */
    TRNG_FIPS,          /*!< Enum used internally */  
    4, /*!< Number of bits needed to produce nominally one bit of entropy after compression */           
    TRNG_FIPS_getbytes, /*!< Callback to a buffer of entropy data */
    TRNG_FIPS_Init,    /*!< Callback for TRNG Initialization */
    TRNG_FIPS_Cleanup, /*!< Callback for TRNG Cleanup */
    TRNG_FIPS_preinit, /*!< Callback for (global) setup for this TYPE of entropy source */
    TRNG_FIPS_Avail,   /*!< availability */
    NULL,
    1
};

ENTROPY_HT MY_ht;


void GenerateRandomSeed (int num,unsigned char * buf);
void my_trng_raw(ENTROPY_IMPL *E,ENTROPY_HT *myht,unsigned char pers[TRNG_RD],
		 unsigned char* data, 
		 unsigned int len);



/** 
    @brief callback function for OpenSSL RandCleanup
*/
static void RandCleanup(void)
{
}
/**
   @brief callback function for OpenSSL RandSeed
   @param buff seeding data
   @param num the number of seeding bytes
*/
static void RandSeed(const void *buff,int num)
{

}
/**
   @brief callback function for OpenSSL RandAdd
   @param buf extra seeding data
   @param num number of bytes of extra seeding data
   @param entropy estimate of the number of entropy bits in the provided data
*/
static void RandAdd(const void *buf, int num, double entropy)
{

}
/* @brief callback function for OpenSSL RandPseudoBytes
   @param buf the output buffer
   @param num the number of random bytes tpo return
   @note Hack alert, the ICC TRNG is fairly quick, so to avoid
   having to code a PRNG here we use the TRNG
   @return 0 on failure, 1 on sucess
*/
static int RandPseudoBytes(unsigned char *buf, int num)
{
  GenerateRandomSeed (num,(void * )buf);
  return 1;
}
/* @brief callback function for OpenSSL RandBytes
   @param buf the output buffer
   @param num the number of random bytes tpo return
   @return 0 on failure, 1 on sucess
*/

static int RandBytes(unsigned char *buf, int num)
{
  GenerateRandomSeed (num,(void * )buf);
  return 1;
}

/** @brief callback function for OpenSSL RandStatus
    @return 1 if all is O.K., 0 otherwise
*/
static int RandStatus(void)
{
  return 1;
}

static const RAND_METHOD my_rand_meth = {
  RandSeed,
  RandBytes,
  RandCleanup,
  RandAdd,
  RandPseudoBytes,
  RandStatus
};


/**
   @brief hook the OpenSSL RNG functions
*/
void hook_rand(void)
{
  static int initialized = 0;
  ERR_STRING_DATA (*temp)[]=NULL;

  if(!initialized) {
    MY_TRNG.preinit();
    ht_Init(&MY_ht,50);
    initialized = 1;
  } 
  ERR_load_crypto_strings();

  get_ICC_str_libraries(&temp);
  ERR_load_strings(ICC_ERR_L_ICC, *temp);
  get_ICC_str_functions(&temp);
  ERR_load_strings(ICC_ERR_L_ICC, *temp);
  get_ICC_str_reasons(&temp);
  ERR_load_strings(ICC_ERR_L_ICC, *temp);
  getPrngErrFuncts(&temp);
  ERR_load_strings(ERR_LIB_RAND, *temp);
  getPrngErrReasons(&temp);
  ERR_load_strings(ERR_LIB_RAND, *temp);

  RAND_set_rand_method(&my_rand_meth);
  
}

void unhook_rand(void)
{
  RAND_set_rand_method(NULL);
  ht_Cleanup(&MY_ht);
}
void my_trng_raw(ENTROPY_IMPL *E,ENTROPY_HT *myht,unsigned char pers[TRNG_RD],
		 unsigned char* data, 
		 unsigned int len)
{
  int n = 0;
  int i;
  unsigned char  s;
  unsigned char tbuf[TRNG_RD];

  myht->htfail = 0;
  /* Get one buffer full of bytes nominally meeting our entropy guarantee 
     and not failing the health checks
  */
  
  while(n < len) {
    for(i = 0; i< TRNG_RD; ) {
      s = E->gb(&(E->econd));
      if(ht(myht,s)) continue;
      tbuf[i++] = s;
    }
    /* copy this buffer full to the output buffer 
       poisoning it by XOR'ing with our retained
       personalization data.
    */
    for(i = 0; (n < len) && (i < TRNG_RD) ; i++,n++) {
      data[n] = tbuf[i] ^ pers[i];
    }
  }
  /* And update the retained personalization data from the last
     bufferful of output.
  */
  for(i = 0; i < TRNG_RD ; i++) {
    pers[i] ^= tbuf[i] ;
  }
}

/*!
  @brief Run the entropy source through the conditioning function
  to produce a byte stream of nominally 1 bit/bit entropy
  @param E The entropy source
  @param outbuf output buffer
  @param n length of buffer
  @note we prefer the HMAC version simply because our CMAC code is slow
*/
static void conditioner(ENTROPY_IMPL *E, unsigned char* outbuf, unsigned len)
{
  unsigned char tbuf[32];
  static unsigned char rdata[32];
  static unsigned char ldata[32]; 
  static int initialized = 0;
  int n = 0;
  int mlen = 0;
  int i = 0;
  HMAC_CTX hctx ;
  const EVP_MD *md = NULL;
  static unsigned char hmac_key[32];
  if(!initialized) {
    md = EVP_get_digestbyname((char *)"SHA256");
    for(i = 0; i < sizeof(hmac_key); i++) {
      hmac_key[i] = E->gb(&(E->econd));
    }
    /* Initialize our personalization data */
    for(i = 0; i < sizeof(rdata); i++) {
      tbuf[i] = E->gb(&(E->econd));
    }
  }

  while( n < len) {
    HMAC_Init(&hctx,hmac_key,sizeof(hmac_key),md);
    HMAC_Update(&hctx,rdata,sizeof(rdata));
    for(i = 0; i < 2; i++) {
      my_trng_raw(E,&MY_ht,rdata,tbuf,sizeof(tbuf));
      HMAC_Update(&hctx,tbuf,sizeof(tbuf));
    }
    HMAC_Final(&hctx,tbuf,(unsigned int *)&mlen);
    /* Copy the compressed data to the output */
    for(i = 0; i < mlen && n < len; i++, n++) {
      outbuf[n] = tbuf[i];
    }
  }

  /* Check that we aren't generating duplicated data */
  if(0 == memcmp(tbuf,ldata,sizeof(tbuf))) {
    fprintf(stderr,"Cryptographic failure, duplicated seeds File %s line %d\n",__FILE__,__LINE__);
    abort();
  }
  memcpy(ldata,tbuf,sizeof(ldata));

}

/*
 This is debug code. It gets reinvented so often that I've decided
 that it may as well stay here. Note that by default it's not compiled
 It's simply left here to avoid typing it in again
*/
static void printbin( char *s,int l)
{
  int i;
  fprintf(stderr,"len = %d :",l);
  for(i = 0; i < l ; i++) {
    fprintf(stderr,"%02x",((unsigned)s[i] & 0xff));
  }
  fprintf(stderr,"\n");
}
/* Lock here or bad things happen under thread load
   thread.ksh will hang running ossl_thread
*/
void  GenerateRandomSeed (int num,unsigned char *buf)
{
  CRYPTO_w_lock(CRYPTO_LOCK_RAND);
  conditioner(&MY_TRNG,buf,num);
  CRYPTO_w_unlock(CRYPTO_LOCK_RAND);
}
