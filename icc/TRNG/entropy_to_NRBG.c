/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License").  You may not use
// this file except in compliance with the License.  You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: Takes our entropy sources, turns them into TRNG's
//
*************************************************************************/

#include "icclib.h"
#if 0
/* non header fix if including ICC_NRBG.h is a problem */
unsigned int TRNG_guarantee(TRNG* T);
#else
#include "TRNG/ICC_NRBG.h"
#endif
#include "TRNG/entropy_to_NRBGi.h"
extern void printbin( unsigned char *s,int l);
#if 0 
static void printbin( char *s,int l)
{
  int i;
  fprintf(stderr,"len = %d :",l);
  for(i = 0; i < l ; i++) {
    fprintf(stderr,"%02x",((unsigned)s[i] & 0xff));
  }
  fprintf(stderr,"\n");
}
#endif

void xcompress(TRNG *T,unsigned char outbuf[SHA_DIGEST_SIZE],unsigned char *in, int len)
{
  unsigned int mlen = SHA_DIGEST_SIZE;
  HMAC_Init(T->cond.hctx,T->cond.key,sizeof(T->cond.key),T->md);
  HMAC_Update(T->cond.hctx,outbuf,SHA_DIGEST_SIZE);   
  HMAC_Update(T->cond.hctx,in,len);
  HMAC_Final(T->cond.hctx,outbuf,&mlen);
  HMAC_CTX_cleanup(T->cond.hctx);
}
/*! @brief HMAC compression step 
  @param T TRNG parameters
  @param outbuf buffer for compressed data
  @param len amount of data required
 
*/

int conditioner(TRNG *T, unsigned char* outbuf, unsigned len)
{
 
  unsigned int n = 0;
  unsigned int i = 0;
  unsigned int j = 0; /* Loop counter for extra compression */
  unsigned int guarantee = 2;
  unsigned mlen = SHA_DIGEST_SIZE;
  int rv = 0;
  unsigned char tbuf[SHA_DIGEST_SIZE *2];
  memset(tbuf,0,SHA_DIGEST_SIZE *2);  
  guarantee = TRNG_guarantee(T);
  while( n < len) {
    HMAC_Init(T->cond.hctx,T->cond.key,sizeof(T->cond.key),T->md);
    /* personalization data */
    HMAC_Update(T->cond.hctx,T->cond.rdata,sizeof(T->cond.rdata));
    for(j = 0; j < guarantee; j++) { 
      if( 0 != trng_raw(&(T->econd),tbuf,SHA_DIGEST_SIZE) ) {
        rv = SetRNGError("Insufficient entropy",__FILE__,__LINE__);
        if(TRNG_OK != rv) {
          HMAC_CTX_cleanup(T->cond.hctx);
          return rv;
        }          
      }
      HMAC_Update(T->cond.hctx,tbuf,sizeof(tbuf));
    }

    HMAC_Final(T->cond.hctx,tbuf,&mlen);
    
    for(i = 0; (i < mlen) && (n < len); ) {
      outbuf[n++] = tbuf[i++];
    }
    
  }
  HMAC_CTX_cleanup(T->cond.hctx);
  /* Update our "IV"
  for(i = 0; i < sizeof(T->cond.rdata); i++) {
      T->cond.rdata[i] ^= tbuf[i];
  }
  */
  return rv;
}
/*
  Note we use a 4 bit entropy estimation on the OUTPUT data from the TRNG
  This isn't NIST mandated function, so we can do sensible here :)
  1) Performance. We don't have to gather 512 bytes and toss most of them away
  2) Security. Less vulnerable to cache line attacks
  3) Less vulnerable to allowing a counter through
*/

TRNG_ERRORS Entropy_to_TRNG(TRNG *T, unsigned char *data, unsigned int len)
{
  TRNG_ERRORS rv = TRNG_OK;
  int i = 0, j = 0,m = 0, k,l;
  int e = 0;
  unsigned int digestL = 0;
  unsigned char buffer[SHA_DIGEST_SIZE];

  memset(buffer,0,SHA_DIGEST_SIZE);
  while (i < len)
  {
    /* See egather.c for the implementaton of trng_raw()
       which is designed to ensure that short term batches of
       data are at least somewhat well distributed.
     */
    for (j = 0; j < (TRNG_RETRIES) && (i < len);j++ )
    {
      for (l = 0; l < len; l += SHA_DIGEST_SIZE)
      {
        rv = conditioner(T, buffer, SHA_DIGEST_SIZE);
        if (TRNG_OK != rv) {
          return rv;
        }
        e = pmax4(buffer,SHA_DIGEST_SIZE);
        if(e < 50) {
          break;
        }
        k = (len - i) < SHA_DIGEST_SIZE ? (len - i) : SHA_DIGEST_SIZE;
        if (k > 0)
        {
          memcpy(data + i, buffer, k);
          i += k;
        }
      }
    }

    if (j >= TRNG_RETRIES)
    {
      rv = SetRNGError("Unable to obtain sufficient entropy", __FILE__, __LINE__);
      if(TRNG_OK != rv) {
        return rv;
      }
    }
    /* Final sanity check, we got out, is our overall entropy good with a compression function 
      This isn't what NIST uses but all the NIST algs will pass sequence generators
    */
    EntropyEstimator(T, data, len);
    if (!EntropyOK(T))
    {
      rv = SetRNGError("Long term entropy is below acceptable limits", __FILE__, __LINE__);
      if (TRNG_OK != rv) {
        return rv;
      }
    }
    /*!
    \FIPS
    FIPS CRNG test for the TRNG, make sure we never return
    identical seeds back-to-back
    We store only the hash of the seed to minimize exposure
    of the generated seeds
  */
    EVP_DigestInit(T->md_ctx, T->md);
    EVP_DigestUpdate(T->md_ctx, data, len);
    EVP_DigestFinal(T->md_ctx, buffer, &digestL);

    if (memcmp(buffer, T->lastdigest, SHA_DIGEST_SIZE) != 0)
    {
      /* Pass, so save the last hash for comparison next time */
      memcpy(T->lastdigest, buffer, SHA_DIGEST_SIZE);
    } else { /* FAIL, round we go again after checking for repeat failures */
      i = 0;
      m++;
      if(m > 5) {
        rv = SetRNGError("Repeated duplicate seeds from TRNG", __FILE__, __LINE__);
        if (TRNG_OK != rv) {
          EVP_MD_CTX_reset(T->md_ctx);
          return rv;
        }
      }
      continue;
    }
  }
  /* Scrub the state here, but don't free it */
  EVP_MD_CTX_reset(T->md_ctx);
  return rv;
}
