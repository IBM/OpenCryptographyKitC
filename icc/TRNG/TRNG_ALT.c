/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License").  You may not use
// this file except in compliance with the License.  You can obtain a copy
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

#include <stdio.h>

#if defined(_WIN32)
#define WIN32_NO_STATUS /* Macros will be redefined by ntstatus so we don't want them from windows.h */
#endif

#include "platform.h"

#if defined(_WIN32)
#undef WIN32_NO_STATUS
#endif

#include "TRNG/nist_algs.h"
#include "TRNG/TRNG_ALT.h"
#include "induced.h"
/* Additional to what we get from platform.h, we only need it here */
#if defined(_WIN32)
#include <bcrypt.h>
#include "ntstatus.h"
#endif


static int fd_alt = -1;
#if defined(_WIN32)
static BCRYPT_ALG_HANDLE hProvider = NULL;
#endif
/*! Pre-init function for TRNG_ALT

*/

void ALT_preinit(int reinit)
{
  
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

    (void)ALT_Init(NULL, tmp,sizeof(tmp));
  }
  /*printf("ALT_Avail %d (-1 is off)\n", fd_alt);*/
  return (fd_alt != -1);
}


/*! @brief
  Fill our I/O buffer 
  @param buffer the scratch buffer to fill
  @param n the size of the buffer
 */
static int alt_read(unsigned char *buffer,int n)
{
  TRNG_ERRORS rv = TRNG_OK;
#if !defined(_WIN32)
  int i = 0,k = n;
#endif

  memset(buffer,0,n); /* If all else fails, return 0's */
  switch(fd_alt) {
  case -1: /* No PRNG source was found originally */
    break;
  case -3:
#if defined(_WIN32)
  {
    NTSTATUS status = 0;
    status = BCryptGenRandom(hProvider, (PUCHAR)buffer, n, 0);
    if(!BCRYPT_SUCCESS(status)) {
      rv = TRNG_REQ_SIZE; /* One of the parameters was likely not correct, or bad provider */
    }
    }
#endif
    break;
  default:
#if !defined(_WIN32)
  /* Some OS's limit the size of reads from /dev/(u)random ==> HPUX */
   while(k > 0) {
    i = read(fd_alt,buffer,k);
    k -= i;
    if((i > 0) && (k != 0) ) {
      buffer += i;
      continue;
    } else {
      rv = TRNG_REQ_SIZE;
      break;
    }
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
  {
    #define SIZE 8
    /* ON Windows ..... */
    /* If no HW RNG, OS RNG source */
    NTSTATUS status = 0;
    status = BCryptOpenAlgorithmProvider(&hProvider, BCRYPT_RNG_ALGORITHM, NULL, 0);
    if(BCRYPT_SUCCESS(status)) {
      fd_alt = -3;
    } else {
      rv = TRNG_INIT; /*error*/
    }
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
 @brief get a byte of random data.
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

  rv = alt_read(buffer,len); /* Read from the primary source */

  /*! \induced 221. TRNG_ALT. Fake the failure condition 
	  from the OS RNG source
  */
  if (221 == icc_failure)
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
   BCryptCloseAlgorithmProvider(hProvider, 0);
   hProvider = 0;
 }
#else 
  if(fd_alt >= 0) {
    close(fd_alt);
    fd_alt = -1;
  }
#endif
}
