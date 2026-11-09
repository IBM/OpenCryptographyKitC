/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: Noise source setup for the FIPS ICC TRNG ("TRNG_FIPS")
//              This file exists to keep the code layering consistent.
//              Most of the code resides in timer_entropy.c and timer_fips.c
//
*************************************************************************/

#include "platform.h"
#include "TRNG/TRNG_FIPS.h"
#include "TRNG/timer_entropy.h"
#include "induced.h"
int getbytes(unsigned char *buffer,int n);

int TRNG_FIPS_Avail()
{
  return 1;
}

/*! @brief Perform setup for the default TRNG type 
 */
void TRNG_FIPS_preinit(int reinit)
{

}

/*! @brief Initialize the default TRNG instance
  @param T a pointer to the TRNG_t object
  @param pers a pointer to personalization data - if needed
  @param perl length of personalization data provided
  @note a No-op in this case
*/
TRNG_ERRORS TRNG_FIPS_Init(E_SOURCE *E, unsigned char *pers, int perl)
{
  TRNG_ERRORS rv = TRNG_OK;
  rv = T_MEMORY_Init(&(E->tf));

  return rv;
}

/*! 
  @brief Get  a buffer from the TRNG instance
  @param E pointer to an E_SOURCE stuct
  @param buffer
  @param number of bytes to place in buffer
  @return TRNG_OK or an error
  @note The FIPS TRNG has to perform health tests on it's input so this 
  makes sure it has a large enough buffer and:
  Either fills and discards excess if the request is smaller than the local buffer
  Or repeatedly calls FIPS_getbytes until the request is fullfilled
*/
TRNG_ERRORS TRNG_FIPS_getbytes(E_SOURCE *E,unsigned char *buffer,int len)
{
  TRNG_ERRORS rv = TRNG_OK;
  int tlen = 0;
  if(NULL != E) {
    while(len > 0) {
     if(E_ESTB_BUFLEN != FIPS_getbytes(E,E->nbuf,E_ESTB_BUFLEN, NULL)) {
        rv = TRNG_REQ_SIZE;
        break;
      }       
      tlen = len;
      if(tlen > E_ESTB_BUFLEN) {
        tlen = E_ESTB_BUFLEN;
      }
      memcpy(buffer,E->nbuf,tlen);
      buffer += tlen;
      len -= tlen;
    }
  } else {
    rv = TRNG_INIT;
  }
  return rv;
}


/*! @brief cleanup routine fo rthe default TRNG
  @param T pointer to TRNG data
  @return Error status - always TRNG_OK in this case
*/
TRNG_ERRORS TRNG_FIPS_Cleanup(E_SOURCE *E)
{
  if(NULL != E ) {
    T_MEMORY_Clean(&(E->tf));
  }
   return TRNG_OK;
}

