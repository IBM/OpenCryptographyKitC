/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: Header for TRNG_ALT4
//
*************************************************************************/

#if !defined(TRNG_ALT4_H)
#define TRNG_ALT4_H

#include "noise_to_entropy.h"


void ALT4_preinit(int reinit);

int ALT4_Avail();

TRNG_ERRORS ALT4_Init(E_SOURCE *E, unsigned char *pers, int perl);

TRNG_ERRORS ALT4_getbytes(E_SOURCE *E,unsigned char *buf,int len );

TRNG_ERRORS ALT4_Cleanup(E_SOURCE *T);


#endif
