/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License").  You may not use
// this file except in compliance with the License.  You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: Higher level noise conditioning routines for ICC
// 
*************************************************************************/

#include <stdio.h>
#if !defined(STANDALONE)
#include "platform.h"
#endif

#include "noise_to_entropy.h"

extern unsigned int icc_failure;


/* Implements the higher level code to turns a noise source into an entropy
   source with a trustworthy entropy guarantee
*/




void printbin( unsigned char *s,int l)
{
  int i;
  printf("len = %d :",l);
  for(i = 0; i < l ; i++) {
    fprintf(stderr,"%02x",((unsigned)s[i] & 0xff));
  }
  printf("\n");
}

/*! 
  @brief
  - This routine implements common processing for noise
  sources ICC uses.
  - Checks the entropy
  - Applies the NIST Adapative Prediction and Repeat Count
  tests
  - turns a stream of bytes into a buffer full of bytes
  with the entopy guarantee met
  - DOES NOT perform further conditioning
  @param E  pointer to a structure to hold entropy source
  @param data buffer for returned value
  @param len length of returned value
  @return 0 if we got entropy, 1 otherwise
  @note 
  - Our entropy guarantee at this point is 0.5bits/bit and
  that's determined by the tables in the AP and RC tests.
  - There's a final sanity check on output data, a compression
  test, which doesn't appear in the NIST tests
*/
int trng_raw(E_SOURCE *E,
             unsigned char *data,
             unsigned int len)
{
  int rv = 0;
  int k = 0;
  int e = 0;
  int failcount = 0;

  /* Get one buffer full of bytes nominally meeting our entropy guarantee 
     and not failing the health checks
    
     Note that while we will block waiting for sufficient entropy we do eventually fail. 
     
  */

  while (len > 0)
  {
    /* Try to gather data with sufficient entropy, if we can't eventually 
      time out and die. We read a buffer at a time, optomistically copy
      data when it passes the health tests, refill buffer if we didn't 
      get enough 'good' data
    */

    k = len;
    if (k > E->cnt)
    {
      k = E->cnt;
    }
    /* 201 is a transient failure, 202 persistent */
    if((icc_failure == 201) || (icc_failure == 202)) {
      failcount = MAX_HT_FAIL +1;
      /* Pretend to clear the buffer so we goto error */
      k = 0;
      E->cnt = 0;
    }
    if(0 == k) {
      E->impl.gb(E,&(E->nbuf[0]), E_ESTB_BUFLEN);
      E->cnt = E_ESTB_BUFLEN; /* The FIPS RBG has done the health test */
      e = pmaxLGetEnt(E->nbuf, E_ESTB_BUFLEN); /* So just do an entropy check here for all modes, equivalent to AP anyway */
      if(e < (50*2) ) { /* The entropy estimator uses integer maths, number back is 2x real entropy so we can handle 12.5, 87.5 etc */
       failcount++;
        E->cnt = 0;

      }
      if(failcount > MAX_HT_FAIL) {
        rv = 1;
        goto error;
      }
      continue;
    }
    /* printbin(E->nbuf,RNG_BUFLEN); */
    /* Health tests passed generally, Entropy assessment buffer has enough data, entropy is O.K */
    memcpy(data,&(E->nbuf[0]) + (E_ESTB_BUFLEN - E->cnt),k);
    data += k;
    len -= k;
    E->cnt -= k;
    failcount = 0;
  }
error:
    /* Transient case */
    if(icc_failure == 201) {
      icc_failure = 0;
    }  
  return rv;
}
