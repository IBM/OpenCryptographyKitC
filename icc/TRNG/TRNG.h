/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: Collects all the bits and pieces and sets them up
//              for use within ICC
//
*************************************************************************/

#if !defined(TRNG_TRNG_H)
#define TRNG_TRNG_H
#include "timer_nonFips.h"
#include "noise_to_entropy.h"

/* Callbacks for the default timer based TRNG */

int TRNG_Avail(void);
void TRNG_preinit(int reinit);
TRNG_ERRORS TRNG_Init(E_SOURCE *E, unsigned char *pers, int perl);
TRNG_ERRORS TRNG_getbytes(E_SOURCE *E,unsigned char *buf,int len);
TRNG_ERRORS TRNG_Cleanup(E_SOURCE *E);


#endif
