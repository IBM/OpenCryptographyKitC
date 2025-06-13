/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

#if !defined(ENTROPY_ESTIMATOR_H)
#define ENTROPY_ESTIMATOR_H
#include "noise_to_entropy.h"

int GetDesignEntropy(TRNG *T);
int GetEntropy(TRNG *T);
int EntropyOK(TRNG *T);
int EntropyEstimator(TRNG *T,unsigned char *data,int n);
TRNG_ERRORS InitEntropyEstimator(TRNG *T);
void CleanupEntropyEstimator(TRNG *T);

#endif
