/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: Header for TRNG_ALT
//
*************************************************************************/

#if !defined(TRNG_ALT_H)
#define TRNG_ALT_H

#include "noise_to_entropy.h"



void ALT_preinit(int reinit);

int ALT_Avail(void);

TRNG_ERRORS ALT_Init(E_SOURCE *E, unsigned char *pers, int perl);

TRNG_ERRORS ALT_getbytes(E_SOURCE *E,unsigned char *buffer,int len );

TRNG_ERRORS ALT_Cleanup(E_SOURCE *E);


#endif
