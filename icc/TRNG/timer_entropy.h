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


unsigned long RdCTR();
unsigned long RdCTR_raw();
int Get_default_tuner();
int Set_default_tuner(int tuner);
int Set_rng_setup(int setup);
int timer_status();
int RdCtrBurst(unsigned long *buffer,unsigned int len, int localloops, char* memptr, unsigned int memsize);
unsigned long CalcShift(int min_loops, char* memptr, unsigned int memsize);
unsigned int capture(unsigned char in,int *counter,unsigned char *ov);
void setDelay(int d);
int getDelay(void);
#endif
