/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: Use hardware RNG's directly.
//
*************************************************************************/


/*!
  \FIPS TRNG_ALT
   This entropy source relies on the OS RBG to provide entropy
*/

#include "platform.h"
#include "TRNG/nist_algs.h"
#include "TRNG/timer_nonFips.h"
#include "TRNG/TRNG_ALT.h"
#include "induced.h"



static int fd_alt = -1;

/*! Pre-init function for TRNG_ALT

*/

void ALT_preinit(int reinit)
{
#if !((defined(__MVS__)) || (defined(__linux__) && defined(__s390__)))
  Timer_NOISE_preinit(reinit);
#endif
}




/*! @brief
  Determine whether this noise source will is available
  @return 0 is not available , !0 if available
*/
int ALT_Avail()
{

  unsigned char tmp[16];

  if(fd_alt == -1) { 
    /* It either failed last time or has never been set up */

    ALT_preinit(0);
    (void)ALT_Init(NULL, tmp,sizeof(tmp));
  }
  return (fd_alt != -1);
}
#if defined(_WIN32)
static HCRYPTPROV hProvider = 0;
int get_trng_alt_init()
{  
  return (0 != hProvider);
}
#else

int get_trng_alt_init()
{  
  return (-1 != fd_alt);
}
#endif

/*! @brief
  Fill our I/O buffer 
  @param xdata in out case a pointer to the global fd_alt
  @param buffer the scratch buffer to fill
  @param n the size of the buffer
 */
static int alt_read(unsigned char *buffer,int n)
{
  TRNG_ERRORS rv = TRNG_OK;
  int i = 0,k = n;

  memset(buffer,0,n); /* If all else fails, return 0's */
  switch(fd_alt) {
  case -1: /* No PRNG source was found originally */
    break;
  case -3:
#if defined(_WIN32)  
    if( 0 != hProvider) {
      i = (int)CryptGenRandom(hProvider,n,buffer);
      if(0 == i) rv = TRNG_REQ_SIZE;
    } else {
      rv = TRNG_INIT;
    }
#endif    
    break;
  default:
#if !defined(_WIN32)
  /* Some OS's limit the size of reads from /dev/(u)random ==> HPUX */
    while (k > 0) {
        i = read(fd_alt, buffer, k);
        if (i <= 0) {
            /* read error or unexpected EOF */
      rv = TRNG_REQ_SIZE;
      break;
    }
        buffer += i;
        k -= i;
  }
#endif
    break;
  }
  return rv;
}

TRNG_ERRORS ALT_Init(E_SOURCE *E, unsigned char *pers, int perl)
{

  TRNG_ERRORS rv = TRNG_OK;

  /* Else probe for something else */
  if(-1 == fd_alt) {
#if defined(_WIN32)
    /* ON Windows ..... */
    /* If no HW RNG, OS RNG source */
    if(0 == hProvider) {
      CryptAcquireContext(&hProvider, NULL, NULL, PROV_RSA_FULL,
			  CRYPT_VERIFYCONTEXT);
    }
    if( 0 != hProvider) {
      fd_alt = -3;
    } else {
      rv = TRNG_INIT;
    }
#else
    /* On Unix .... */
    fd_alt = open("/dev/urandom",O_RDONLY);
    if(-1 == fd_alt) {
      fd_alt = open("/dev/random",O_RDONLY);
    }
#endif
  }
  /* If there's no /dev/ source, we'll return an error */
  if(-1 == fd_alt) {
    rv = TRNG_INIT;
  }

  /*! \induced 203. TRNG_ALT external entropy source not available
   */
  if(203 == icc_failure) {
    rv = TRNG_INIT;
  }


  return rv;
}





/*!
 @brief get a byte of "random" data.
 backup RNG for virtualized systems where the timer based source fails
 horribly.
 We get data from the OS RNG/HW RNG, but mix in the fastest moving timebase 
 counter bits we can trust. 
 We don't trust the OS RNG or the timebase samples to be 100% to be 
 unguessable, but it does mean that  both the OS rng has to be 
 vulnerable AND our process is subject to timing attacks
 before it fails totally.
 @param E Entropy filter (optional)
 @param buffer buffer for data
 @param len size of buffer
 @return status 
 @note
 - we do know reading one byte at a time is inefficient, but it gives 
 us more variation in the timebase counter we mix in for extra security.
 - if the external source is unavailable, we return 0's, this is detected
   at a higher level
*/
TRNG_ERRORS ALT_getbytes(E_SOURCE *E,unsigned char *buffer, int len)
{
  TRNG_ERRORS rv = TRNG_OK;
  int i = 0;

  rv = alt_read(buffer,len); /* Read from the primary source */
  if(TRNG_OK == rv) {
    i = getbytes(buffer,len); /* getbytes xor's data over the buffer */
    if(i != len) {
      rv = TRNG_REQ_SIZE;
    }
  }
  /*! \induced 221. TRNG_ALT. Fake the failure condition 
	  from the OS RNG source
  */
  if (205 == icc_failure)
  {
    memset(buffer,0,len);
  }

  return rv;
}

/*! @brief Cleanup any residual information in this entropy source 
  @param E The entropy source data structure
  @return TRNG_OK
 */
  
TRNG_ERRORS ALT_Cleanup(E_SOURCE *E)
{

  TRNG_ERRORS rv = TRNG_OK;

  return rv;
}


void ALT_Final()
{
#if defined(_WIN32)
 if((-3 == fd_alt) && (0 != hProvider)) {
   CryptReleaseContext(hProvider,0);
   hProvider = 0;
 } 
#else 
 if(fd_alt >= 0) {
    close(fd_alt);
    fd_alt = -1;
 }
#endif 
}
