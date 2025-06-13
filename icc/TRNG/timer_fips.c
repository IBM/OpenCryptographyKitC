/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

#if !defined(_WIN32)
#   include <stdlib.h>
#endif

#include "platform.h"
#include <string.h>
#include "TRNG/nist_algs.h"
#include "TRNG/timer_fips.h"
#include "induced.h"

/*
#define DEBUG_FIPS
#define DEBUG_FIPS1
*/

#if defined(DO_STATS)
#include "TRNG/stats.h"
#endif 
/* Options for logging data, problem is we'll disturb the timing some no matter what,
   so allocate a very large buffer, write data to that, flush (and stall) at that point
   4M Samples.
   - better than nothing 
*/   
#if defined(INSTRUMENTED)
#define LOGN 4194304
static ICC_INT64 logbuf[LOGN];
static ICC_INT64 samples = 0;
#define NITERS 100
static int iters = 0;
static FILE *Flogfile = NULL;
#endif
#include "ext_filter.c"

void SetFatalError(const char *msg, const char *file,int line);
TRNG_ERRORS SetRNGError(const char *msg, const char *file,int line);

static const char * TFtag = "T_FILTER";

#if defined(DEBUG_FIPS)
#include <stdio.h>
static void printbin( char *s,int l)
{
  int i;
  fprintf(stderr,"len = %d :",l);
  for(i = 0; i < l ; i++) {
    fprintf(stderr,"%02x",((unsigned)s[i] & 0xff));
  }
  fprintf(stderr,"\n");
}
#endif

/* Timer based entropy gathering for FIPS mode
   this has more easily defensible noise harvesting
   than the default code.

   Grab samples
   Calculate the differences between samples
   Sort the differences (which is basically the delay between samples)
   Only use the outliers in the distribution. i.e.
   The one or two most common frequencies are fundamentals - remove those
   Anything that occurs too often isn't noise either - remove those
   Harvest what's left as entropy.
*/
extern int shift_done;           /* Have we run CalcShift() */
extern int ex_loops;    /* loops set from config file or environment */
static unsigned int loops;       /* loops that we picked, set from here (different in FIPS/non-FIPS modes */

#define PTE 11
#if (defined(__linux__) && defined(__s390__)) || defined(__MVS__)
// Z/Achitecture requires higher values because the counter is slower
// Future proof incase it is running on an extremely fast machine (assuming noise is roughly the same)
// Spending more time than needed between samples is much better than not being able to spend enough time between samples.
// If we can't spend enough time we never get a good sample, and we hang.
// For the 1 million entropy tool on z15 it fixes on one of the middle indexes, 4093 or 8191
static const int ptable[PTE] = {1021,1531,2017,3067,4093,6143,8191,12281,16381,32717,65521};
#else
static const int ptable[PTE] = {3,7,17,23,31,43,61,83,127,251,509}; /*!< Loop delay table */
#endif

unsigned int fips_loops()
{
    return loops;
}

void T_FILTER_Init(T_FILTER *TF)
{
    if(NULL != TF) {
        memset(TF,0,sizeof(T_FILTER));
        TF->lindex = 0;
        TF->id = TFtag;
    }
}


/* Populate the histogram 
   @param diff the difference between the last sample and this
   @param value this sample value
   @return a flag to indicate that some samples were so closely spaced we couldn't get a full byte of random data
   changing between samples 
*/
static void sorter_in(ICC_INT64 diff, ICC_UINT64 value,T_FILTER *tf)
{
    int i,freq;
    DIST *dist;

    dist=tf->dist;
    for (i = 0; i < TE_BUFLEN; i++)
    {
         /* Only accept samples where the MSBit of the LSByte has to have rolled over between samples 
            Anything else the top half of the sample may have low entropy
         */
         if (diff > 0x7f)
         {
#if defined(DO_STATS)
    StatD(diff);
    StatV(value);
#endif           
        if (diff == dist[i].v)
        {
                 freq = (dist[i].freq++);
                 if (freq < TE_MAXB)
                 {
                     dist[i].values[freq] = value;
                 }
                 break;
             } else if ((0 == dist[i].v) && (0 == dist[i].freq))
             {
                 dist[i].v = diff;
                 dist[i].freq++;
                 dist[i].values[0] = value;
                 break;
             }
         } 
    }
}
/* Sort function used to find the median of a set with a large range*/
static int sf(const void *a, const void *b)
{
    DIST *a1 = (DIST *)a;
    DIST *b1 = (DIST *)b;
    return (b1->freq - a1->freq);
}
/* Sort array by frequency */
static int sortit(DIST dist[TE_BUFLEN])
{
    int i = 0;
    for (i = 1; i < TE_BUFLEN; i++)
    {
        if (0 == dist[i].freq) {
            break;
        }
    }
    qsort(dist, i, sizeof(DIST), sf);
    return i; /* Number of buckets used */
}
  
/* Sort function used t oput the noisy timer samples back into time order*/
static int sflong(const void *a, const void *b)
{
    ICC_INT64 *a1 = (ICC_INT64 *)a;
    ICC_INT64 *b1 = (ICC_INT64 *)b;
    return ((int)((ICC_INT64)*b1 - (ICC_INT64)*a1));
}
/* Sort array by value (time of arrival) */
static int sorttime(ICC_UINT64 *in,int samples)
{

    qsort(in, samples, sizeof(ICC_INT64), sflong);
    return samples; /* Number of noise samples in the array */
}

#if defined(DEBUG_FIPS1)
void dump_DIST(DIST *dist,int loops,int low)
{
    int i,j,tv = 0;
    printf("\nSamples. Loops = %d, low = %d\n",loops,low);
    for(i = 0; i < TE_BUFLEN; i++) {
        if(0 == dist[i].freq)  break;
        printf("\n%0ld [%d]:",dist[i].v,dist[i].freq);
        tv+=dist[i].freq ;
        
        for(j = 0; (j < dist[i].freq) && (j < TE_MAXB); j++) {
            printf("%0ld,",dist[i].values[j]);
        }
        
    }
    printf("\nTotal values %d\n",tv);
}
#endif
/* Data gathering, basic high pass filter followed by a
   histogram based HP filter.
   Used to capture entropy but also
   for tuning the entropy gathering process
   so it's in the entropy rich zone where we
   are sampling noise events but not averaging
   them out.
*/
static int dgl(T_FILTER *TF)
{
    int buckets = 2;
    int i = 0,j = 0;
    ICC_INT64 delta;

    if (NULL != TF)
    {

        memset(TF->samples, 0, sizeof(TF->samples));

        /* Clear the histogram data */
        memset(TF->dist, 0, sizeof(DIST) * TE_BUFLEN);

        /* Grab a load of samples */
        RdCtrBurst(TF->samples, TE_BUFLEN, ptable[TF->lindex]);
 #if defined(INSTRUMENTED)
        if(NULL == Flogfile) {
            Flogfile = fopen("sample.dmp","wb");
    	}
        if(samples > (LOGN - TE_BUFLEN)) 
        {
            /*
            for(i = 0; i < LOGN; i++) {
                fprintf(stdout,"%lx\n",logbuf[i]);
            }
            */
            fwrite(logbuf,sizeof(ICC_INT64),LOGN,Flogfile);
            samples = 0;
            iters++;
            if(iters > NITERS) {
                fclose(Flogfile);
                exit(0);
            }    
        }
        memcpy(logbuf+samples,TF->samples,(TE_BUFLEN * sizeof(ICC_INT64)));
        samples += TE_BUFLEN;
#endif
        /* High pass filter. Yes it's a very crude filter but the histogram sort isn't */
        delta = 0;
        for (i = 1; i < TE_BUFLEN; i++)
        {
            delta = TF->samples[i] - TF->samples[i - 1];
            /* printf("%ld %lu\t",delta, in[i]);*/
            /* NOTE: The extra shift is inserted HERE */
            sorter_in(delta, TF->samples[i], TF);
        }


#if defined(DEBUG_FIPS1)
        dump_DIST(TF->dist,TF->lindex,0);
#endif                       
        /* Frequency sort the samples */
        buckets = sortit(TF->dist);

        memset(TF->samples, 0, sizeof(TF->samples)); /* Clear the input buffer */
        /* Push the values we decided came after noise back into the input buffer */
        for (i = 2, TF->nnoise = 0; i < buckets; i++)
        { 
            /* Skip the first two buckets - which occur as a result of sampling */
            if (TF->dist[i].freq < (TE_MAXB - 1))
            { 
                /* Skip sets with too many entries to plausibly be noise */
                for (j = 0; j < TF->dist[i].freq; j++)
                { /* Copy the noise values for further processing */
                    TF->samples[TF->nnoise] = TF->dist[i].values[j];
                    TF->nnoise++;
                }
            }
        }
        /* Sort the samples we'll be using back into time order as they arrived 
            at this point we've removed the fundamental frequency and any significant harmonics
        */
        if (TF->nnoise > 1)
        {
            sorttime(TF->samples, TF->nnoise);
        } 
#if defined(DEBUG_FIPS)
     /*   dump_samples(TF); */
#endif
    }
    else
    {
        SetFatalError("Unexpected NULL data", __FILE__, __LINE__);
    }
    return buckets;
}

int FIPS_getbytes(E_SOURCE *E, unsigned char *buffer, int len)
{
    int i = 0;
    int buckets = 2;
    int count = 0;
    int ecount = 0;
    T_FILTER *TF = NULL;
    TRNG_ERRORS rv = TRNG_OK;
    
    if ((NULL == E) || (E_ESTB_BUFLEN != len))
    {
        SetFatalError("Corrupted RNG state detected", __FILE__, __LINE__);
    }
    else
    {
        TF = &(E->tf);
        if (!shift_done)  {
            CalcShift(0);
            shift_done = 1;
        }
        if (ex_loops > 0)
        {
            loops = ex_loops;
        }
        if (loops > 0)
        {
            for(i = 0; i < PTE; i++) {
               if(ptable[i] >= loops) {
                  TF->lindex = i;
                  break;
               }
            }
        }

        while (count < E_ESTB_BUFLEN)
        {
            do
            {
                /* Run our very sharp cutoff digital filter */
                buckets = dgl(TF);
                /*  - Check that we aren't getting a lot of short delta's
                    - And if we have very few buckets then we can be almost certain there's little noise
                    so increase the sampling delay by ~50%
                */
                if ((buckets < MIN_BUCKETS)  && (!TF->done))
                {
                  TF->lindex++;
                  if(TF->lindex >= PTE) {
                     TF->lindex = 0;
                  } 
                }
                else
                {
                    TF->done = 1;
                }
            } while (!TF->done);

            /*
            We drop the loop count and restart as the noise rate could have gone up or down
            and the code just above will lift it again if need be.
            */
            if (ex_loops <= 0) {
                if (buckets < MIN_BUCKETS)
                {
                    TF->deadcnt++;
                    if (TF->deadcnt > 20)
                    {
                        TF->done = 0; /* retune */
                        TF->deadcnt = 0;
                        TF->lindex = 0;
                    }
                    continue;
                }
            }
            loops = ptable[TF->lindex];

            /*  We plausibly had outliers in the samples collected, process those as noise 
            The first two sets of data were assumed to be non-noisy 
            Either the sampling was synced to the timer or it was hopping
            between two values. 
            Note: Even with a close sync there can be infrequent hops so the first two buckets are always unreliable.
            As is anything that filled it's buckets, so all we process as noise is outliers
            At this point our entropy guarantee is still very low, all we are assured of here is that this was gathered from noise events
            Note the slighly improved collection technique.
            */
           for (i = 0; (count < E_ESTB_BUFLEN) && (i < TF->nnoise); i++)
            {
                unsigned char c;
                c  = ((TF->samples[i]) & 0xff);
                if(1 == ChkMem(TF,c)) { /* Skip values that were frequent long term */
                    /* Try and construct a byte of data from what was captured */
                    buffer[count] = c;
                    count++;
                    proc_mem(TF,c); 
                }    
            }
            if(count == E_ESTB_BUFLEN) {
                /*! \induced 222. TRNG_FIPS. Fake failure of TRNG source */
                if(222 == icc_failure) {
                    memset(buffer, 'f', count);
                }
                if(0 != ht(&(E->hti),buffer) ) {
                    ecount++;
                    TF->done = 0;
                    count = 0;
                }
            }
            if(ecount > MAX_HT_FAIL) {
                rv = SetRNGError("Repeated failure of low level entropy checks",__FILE__,__LINE__);
                if(TRNG_OK == rv) {
                    continue;
                } else {
                    count = 0;
                    break;
                }    
            }
        }
    }
    return count;
}

