/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: Definitions for the time specific entropy source
//              Refactored from the original egather.c
//              - which just grew and grew and grew 
//
*************************************************************************/

#if !defined(TIMER_ENTROPY_H)
#define TIMER_ENTROPY_H
#include "nist_algs.h"
#include "iccdef.h"


ICC_UINT64 RdCTR();
ICC_UINT64 RdCTR_raw();
int Get_default_tuner();
int Set_default_tuner(int tuner);
int Set_rng_setup(int setup);
int timer_status();
int RdCtrBurst(ICC_UINT64 *buffer,unsigned int len,int loops);
ICC_UINT64 CalcShift(int min_loops);
#endif
