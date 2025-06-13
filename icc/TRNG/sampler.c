/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include "TRNG/timer_entropy.h"
#include "TRNG/stats.h"

#define DO_STATS
/* 
 gcc -fno-strict-aliasing  -m64  -g3 -O0 -D_REENTRANT -fno-strict-aliasing -fno-exceptions -fPIC -Wall  -DVTAG=085  -I ./ sampler.c  nist_algs.o timer_fips.o timer_entropy.o noise_to_entropy.o -lpthread -ldl -o sampler

*/



/* Gathering entropy from noise in software 
   In principle, we sample a counter when the processor has been dragged away to do other things and that should be random as seen from userspace.
   How do we determine that ?
   IF the processor were not disturbed we'd simply get repeating values for the difference between 
   samples counter values. 
   This software generates data covering that scenario. There are occasional drops in entropy at certain sampling rates but essentially this would mostly
   pass all the NIST tests. (Not a good thing (TM))

   The next scenario covered is sampling from a real PC event counter.


   This is test case code to cover most of the good AND bad scenarios because there are risks in sampling from a counter.

   So we guarantee we are only sampling the counter after there's been a burst of noise by
   histogram sorting the timing delta's and storing the first few actual timer values in each group (TE_MAXB)
   Throw away the sets with the largest buckets (>= TE_MAXB in the that bucket) and what we have left is sets of counter values from after a burst of noise.
   - The high pass filter doesn't guarantee we have only one bucket without noise as we could hop between values. 
   - Note also that we don't assume any particular burst of reads will contain noise, we simply keep trying until we have enough samples to assemble a byte. (See dynamic tuning)
   We mix the low bytes of 8 of these samples to produce a single byte of output. (rotated xor)

   The reason for mixing is that we can't guarantee that the LSBit of the timer was advancing by 1 count and we don't know the counter stride but we do know we
   sampled with noise injected and any entropy from that gets spread across the sampled data.
   By mixing we remove the need to know the stride. This being easier and faster than doing that by other means.

   There's a dynamic tuning phase here to try and keep us sampling in the region where we will pick up noise
   it's fairly crude but it doesn't have to be wonderfully accurate to work. That's needed because processor
   clocks aren't constant.

   We also pre-calculate which is the lowest moving bit and down shift so that's the LSBit, 
   having stuck or sometimes special purpose bits at the low end of these counters is common.

*/

#include <stdio.h>
#include "TRNG/timer_fips.h"

#include "DELTA/delta_t.h"
#include "TRNG/nist_algs.h"

#include "TRNG/stats.c"
#include "TRNG/timer_fips.c"

static const char *PMAXL_Tag = "PMAX_L";

extern unsigned int ex_loops;    /* loops set from config file or environment */
typedef struct {
  unsigned int nsam;           /*!< Number of samples */
  unsigned int syms[256];      /*!< Counts of symbols, on 32 bit range ~4G, we can't go lager without ilog2 being fixed as well*/
  const char *id;              /*!< String for debug */
} E_ESTL;
/*
  Ref:http://graphics.stanford.edu/~seander/bithacks.html
  Public domain.
  IBM legal approval obtained to use the algorithm
*/	


/*! 
  @brief integer log2 of a 32 bit int
  @param v value to find the log of
  @return the log of v
*/
static unsigned int ilog2(unsigned int v)
{

  static const unsigned int b[] = {0x2, 0xC, 0xF0, 0xFF00, 0xFFFF0000};
  static const unsigned int S[] = {1, 2, 4, 8, 16};
  int i;
  
  unsigned int r = 0; 
  for (i = 4; i >= 0; i--) {
    
    if (v & b[i]) {
      v >>= S[i];
      r |= S[i];
    } 
  }
  return r;
}
/*!
  @brief Entropy estimate based on Appendix C.3 SP800-90
  Returns entropic bits out/200 bits in
  This variant works on bytes
  @param data buffer to scan
  @param len length of buffer 
  @note Up to the caller to ensure enough bytes have been passed in (>= 512 recommended)
  @note You need to call pmaxLGetEnt to read out the entropy
*/
unsigned int pmaxLGetEnt(unsigned char *data, int len)
{
#if defined(TEST_DOUBLE)
  double p = 0.0, log2p = 0.0;
  double hmin = 0.0;
#endif
  int i = 0;
  int k = 0;
  int ip = 0;
  int ilog2p = 0;
  unsigned int syms[256];
  unsigned int est = 0;


  if (len >= 512)
  {
    memset(syms, 0, sizeof(syms));

    for (i = 0; i < len; i++)
    {
      syms[data[i]]++;
    }
    k = 0;
    for (i = 0; i < 256; i++)
    {
      if (syms[i] > k)
      {
        k = syms[i];
      }
    }

    /* 
       Convert the most common value to a probability
       Integer version we work in inverse space to keep the numbers
       > 1
  */
    ip = len / k;
    ilog2p = ilog2(ip);
    /*
      Our estimate is for 8 bits
      Convert to how many bits we need to guarantee 100 bits of entropy
      Note we have 512 samples
      The maximum samples we could have/bucket is 512 (no entropy)
      minimum and evenly distributed is 2 (max entropy)
      Table lookup here because it's simpler.
     */
    est = etabB[ilog2p];

#if defined(TEST_DOUBLE)
    printf("k = %d ", k);
    p = (double)k / (double)(e->nsam);
    log2p = -log2(p);
    printf("float %f int %d\n", log2p, est);
#endif
  }
  return est;
}


unsigned long CalcShift(int mn);
unsigned int Shift();

unsigned int icc_failure = 0;

#define TARGET 1000
#define BLKSZ 256
/* Need thse because some of the objects we link need them and
    we are building without most of ICC
*/
void  *ICC_Calloc(size_t n, size_t sz, const char *file, int line)
{
    return calloc(n,sz);
}
void ICC_Free(void *x)
{
    free(x);
}
void SetFatalError(const char *msg, char *file,int line)
{

}
/* Where the bad things dwell */
int SampleCounter(int span)
{
    unsigned char s;
    int i;
    int est;
    char buffer[1000];



    for(i = 0; i < 1000; i++) {
        buffer[i] = i*span;
        /* printf("%02x ",s); */
    }
    est = pmaxLGetEnt(buffer,1000)/2;
    return est;
}


int SampleCounterReal(int span)
{
    unsigned char *buffer = NULL;
    int i;
    volatile int j,k;
    int est;
    E_ESTL e;
    int shift;
    
    buffer = calloc(1,10000);
    k = span;


    shift = Shift();

    /* Grab entropy estimates over 1000 samples */
    for (i = 0; i < 1000; i++) {
        for(j = 0; j < k; j++);
        buffer[i] = (unsigned char)RdCTR_raw() >> shift;
  

    est = pmaxLGetEnt(buffer,1000)/2;
    free(buffer);
    return est;
}

#if 1
int SampleFIPS(int xloops)
{
  T_FILTER *TF = NULL;
    unsigned char *buffer = NULL;
    int est,i;
    E_ESTL e; 

    TF = T_FILTER_new();

    if(xloops >= 1) {
      TF->done = 1;
      ex_loops = xloops;
    }
    buffer = calloc(1,1000000);


    FIPS_getbytes(TF,buffer,1000000);
  
    est = pmaxLGetEnt(buffer,1000000)/2;
  
    T_FILTER_free(TF);
    free(buffer);

    return est;
}
#endif
#if 1
/* Check the stats on what FIPS gets & processes 
   run once to get stats and the loops value
   then again with loops forced to be 2x and 0.5x
*/

int SampleFIPS_Stats()
{
  int myloops;
  int i;
  int est;
  char label[10];
  StatsClear();
  SampleFIPS(-1);
  myloops = fips_loops();

  for(i = 1; i < (myloops *4); i++) {  
    StatsClear();
    est = SampleFIPS(i);
    sprintf(label,"%d,%d\n",i,est);
    /* dump_stats((const char *)label); */
  }
  return 1;
}
#endif
int main(int argc, char *argv[])
{
    int est;
    char tbuf[80];
    int i;
    FILE *fp = NULL;
    int test = 0;
    int myloops;

    if(argc > 1) {
        test = atoi(argv[1]);
    }    
    
    CalcShift(0);
    
    switch(test) {
        case 0: /* FIPS code */
        fp = fopen("FIPS.dat","w");
        for(i = 0; i < 10 ; i++) {
            /* fprintf(stderr,"."); */
            est = SampleFIPS(-1); /* Let it find it's own value */
            fprintf(fp,"%5d %3d\n",i,est);
        }
        fclose(fp);
        break;
        case 1: /* Sampled counter, byte entropy assessment, step through sample interval */
        fp = fopen("fake8.dat","w");
        for(i = 0; i < 10000 ; i++) {
            est = SampleCounter(i);
            fprintf(fp,"%5d %3d\n",i,est);
        }     
        fclose(fp);       
        break;
        case 2: /* Demonstrate the impact of sampling interval */
            StatsClear();
            SampleFIPS(-1);
            myloops = fips_loops();
            sprintf(tbuf,"%d",myloops);
            dump_stats(tbuf);

            StatsClear();
            SampleFIPS(myloops/2);
            myloops = fips_loops();
            sprintf(tbuf,"%d",myloops/2);
            dump_stats(tbuf);

            StatsClear();
            SampleFIPS(myloops*2);
            myloops = fips_loops();
            sprintf(tbuf,"%d",myloops*2);
            dump_stats(tbuf);
        break;
        case 3: /* Real sampled counter */

        /* Not exactly repeatable, but similar run to run, and given the first results
            can you trust the output ?
            (No)
            And note that increasing the interval between samples does not automatically improve the entropy
            or provide more randomness
        */
        /* fprintf(stderr,"Shift = %d\n",Shift()); */
        fp = fopen("real_dubious.dat","w");
        for(i = 0; i < 10000 ; i++) {
            est = SampleCounterReal(i);
            fprintf(fp,"%5d %3d\n",i,est);
        }  
        fclose(fp);
        break;
        case 5: /* stats from FIPS code */
        fp = fopen("FIPS_stats.dat","w");
          StatsClear();
          SampleFIPS(-1);
          myloops = fips_loops();
          for(i = 1; i < (myloops * 4); i++) {  
            StatsClear();
            est = SampleFIPS(i);
            fprintf(fp,"%d %d\n",i,est);
          }
        fclose(fp);
        break;
        default:
        break;
    }

     
    return 0;
}


