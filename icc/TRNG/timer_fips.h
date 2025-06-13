/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

#if !defined(TIMER_FIPS_H)
#define TIMER_FIPS_H
#include "TRNG/nist_algs.h"
#include "TRNG/noise_to_entropy.h"

int FIPS_getbytes(E_SOURCE *E, unsigned char *buffer, int len);

void T_FILTER_Init(T_FILTER *TF);

unsigned int fips_loops();

#endif
