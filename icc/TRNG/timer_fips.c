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

char* g_memptr = NULL;
static int g_memptr_ctr = 0;
#if defined(_WIN32)
ICC_Mutex g_memptr_mutex = NULL;
#else
ICC_Mutex g_memptr_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

void SetFatalError(const char *msg, char *file,int line);
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
int ex_memsize = -1;
#define DEFAULT_MEMSIZE 4194304 /* 2^22 = (1<<22) */

unsigned int fips_loops()
{
    return loops;
}

int T_MEMORY_Init(T_FILTER *TF)
{
    if(NULL != TF) {
        memset(TF,0,sizeof(T_FILTER));
	    TF->loops = 3;
        TF->id = TFtag;
        if (ex_memsize == -1){
            TF->memsize = DEFAULT_MEMSIZE;
        } else {
            TF->memsize = ex_memsize;
        }
#if defined(_WIN32)
        if (g_memptr_ctr == 0 && g_memptr_mutex == NULL) {
            ICC_CreateMutex(&g_memptr_mutex);
        }
#endif
        ICC_LockMutex(&g_memptr_mutex);
        if (NULL == g_memptr) {
            TF->memptr = (char *)malloc(TF->memsize);
            if(NULL == TF->memptr) {
                return TRNG_MEM;
            }
            g_memptr = TF->memptr;
        } else {
            TF->memptr = g_memptr;
    }
        g_memptr_ctr++;
        ICC_UnlockMutex(&g_memptr_mutex);

    }
    return TRNG_OK;
}

int T_MEMORY_Clean(T_FILTER *TF)
{
    if(NULL != TF) {
        ICC_LockMutex(&g_memptr_mutex);
        if (NULL != g_memptr && g_memptr_ctr == 1) {
            free(g_memptr);
            g_memptr = NULL;
        }
        if (NULL != TF->memptr) {
            TF->memptr = NULL;
        }
        g_memptr_ctr--;
        ICC_UnlockMutex(&g_memptr_mutex);

        memset(TF,0,sizeof(T_FILTER));
        TF->loops = 3;
        TF->id = TFtag;
    }
    return TRNG_OK;
}

/* Populate the histogram 
   @param diff the difference between the last sample and this
   @param value this sample value
   @return a flag to indicate that some samples were so closely spaced we couldn't get a full byte of random data
   changing between samples 
*/
static void sorter_in(long diff, unsigned long value,T_FILTER *tf)
{
    int i, freq;
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
        }
        else if ((0 == dist[i].v) && (0 == dist[i].freq))
             {
                 dist[i].v = diff;
                 dist[i].freq++;
                 dist[i].values[0] = value;
                 break;
             }
        }
        else
        {
            tf->low++; /* Count short deltas, too many and the sampling interval is too short */
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
    long *a1 = (long *)a;
    long *b1 = (long *)b;
    return ((int)((long)*b1 - (long)*a1));
}
/* Sort array by value (time of arrival) */
static int sorttime(unsigned long *in,int samples)
{

    qsort(in, samples, sizeof(long), sflong);
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
   Used for tuning the noise sampling so random events are measurable in the noise
*/
static int dgl(T_FILTER *TF)
{
    int buckets = 2;
    int i = 0,j = 0;
    long delta1[TE_BUFLEN];

    if (NULL != TF)
    {
        memset(delta1, 0, sizeof(delta1));
        TF->low = 0;
        /* High pass filter */
        delta1[0] = 0;
        for (i = 1; i < TE_BUFLEN; i++)
        {
            delta1[i] = TF->samples[i] - TF->samples[i - 1];
            /* printf("%ld %lu\t",delta1[i], in[i]);*/
            /* NOTE: The extra shift is inserted HERE */
            sorter_in(delta1[i], TF->samples[i], TF);
        }

#if defined(DEBUG_FIPS1)
        dump_DIST(TF->dist,TF->loops,TF->low);
#endif                       
        /* Frequency sort the samples */
        buckets = sortit(TF->dist);

        /* Don't touch the input buffer */
        /* memset(TF->samples, 0, sizeof(TF->samples)); */
        TF->nnoise = TE_BUFLEN; /* needs to match len in RdCtrBurst call */


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

/* Populate the output buffer with noise samples, return the number of bytes of noise we have in the buffer 
   We have a tuning phase to try and keep us in the region where we are picking up noise but not averaging it out
   We have a final check to see if the distribution of noise is plausible, if not we reject it and try again.
   
   Optional timestamp_buffer (NULL if not used) is used to return the raw timer samples that were used to generate the output,
   this is for testing purposes and is not required for normal operation.
   It should be at least len * sizeof(unsigned long) bytes long if used.
*/
int FIPS_getbytes(E_SOURCE *E, unsigned char *buffer, int len, unsigned long *timestamp_buffer)
{
    int i = 0;
    int buckets = 2;
    int count = 0;
    int ecount = 0;
    T_FILTER *TF = NULL;

    /* len must always be E_ESTB_BUFLEN */
    /* assert(E_ESTB_BUFLEN == 2*sizeof(TF->samples)) */
    if ((NULL == E) || (E_ESTB_BUFLEN != len))
    {
        SetFatalError("Corrupted RNG state detected", __FILE__, __LINE__);
    }
    else
    {
        TF = &(E->tf);
        if (!shift_done)  {
            CalcShift(0, TF->memptr, TF->memsize);
            shift_done = 1;
        }
        if (ex_loops > 0)
        {
            loops = ex_loops;
        }
        if (loops > 0)
        {
            TF->loops = loops; /* Already tried to set a decent number */
        }

        while (count < E_ESTB_BUFLEN)
        {
            do
            {
                memset(TF->samples, 0, sizeof(TF->samples));
                /* Clear the histogram data */
                memset(TF->dist, 0, sizeof(DIST) * TE_BUFLEN);

                /* Grab a load of samples
                Is there a reason we don't grab 512 and just ht that and output it? Precedence set of 256 for the filter?
                */
                RdCtrBurst(TF->samples, TE_BUFLEN, TF->loops, TF->memptr, TF->memsize);
                if (timestamp_buffer != NULL) {
                    /* For analysis we will return the actual samples too */
                    size_t len = sizeof(TF->samples);
                    if (count > E_ESTB_BUFLEN / 2) {
                        /* Don't overflow the timestamp buffer, copy only what we can fit */
						len = (E_ESTB_BUFLEN - count) * sizeof(unsigned long);
                    }
                    memcpy(timestamp_buffer + count, TF->samples, len);
                }

                /* Run our very sharp cutoff digital filter */
                buckets = dgl(TF);
                /*  - Check that we aren't getting a lot of short delta's
                    - And if we have very few buckets then we can be almost certain there's little noise
                    so increase the sampling delay by ~50%

					ex_loops == 3, set externally, is a special case for the FIPS test,
                    we don't want to change the loops in that case
                */
                if ((buckets < MIN_BUCKETS) && (!TF->done) && ex_loops != 3)
                {
                    TF->loops = ((TF->loops * 3) / 2) + 1; /* N-R isn't good on noise and even the noise distribution is noisy */
                }
                else
                {
                    TF->done = 1;
                }
            } while (!TF->done);

            /* If we are not getting any plausible noise, try re-tuning 
            The reason we halve our estimate of the loops required here is processor clock scaling
            MOST CPU's it's not that aggressive and the range is only 2:1 or so
            We drop the loop count and restart as the noise rate could have gone up or down
            and the code just above will lift it again if need be.

            ex_loops == 3, set externally, is a special case for the FIPS test,
            we don't want to change the loops in that case
            */
            if (buckets < MIN_BUCKETS && ex_loops != 3)
            {
                TF->deadcnt++;
                if (TF->deadcnt > 20)
                {
                    TF->done = 0; /* re-tune */
                    TF->deadcnt = 0;
                    TF->loops = (TF->loops / 2) + 1;
                }
                continue;
            }

            loops = TF->loops;
            /*  We plausibly had outliers in the samples collected, process those as noise 
            The first two sets of data were assumed to be non-noisy 
            Either the sampling was synced to the timer or it was hopping
            between two values. 
            Note: Even with a close sync there can be infrequent hops so the first two buckets are always unreliable.
            As is anything that filled it's buckets, so all we process as noise is outliers
            At this point our entropy guarantee is still very low, all we are assured of here is that this was gathered from noise events
            */
           /* 
            The logic for this loop was to copy from each iteration's TF->nnoise. Each iteration could have new data from 
            0...TF->nnoise amount in TF->samples. Since we are bypasing the filter, we will copy all samples.
            TF->nnoise will always be 256.
           */
           for (i = 0; (count < E_ESTB_BUFLEN) && (i < TF->nnoise); i++)
            {
                /* Try and construct a byte of data from what was captured */
                buffer[count] = (TF->samples[i]) & 0xff;
                count++; 
            }
            /*! \induced 222. TRNG_FIPS. Fake failure of TRNG source */
            if(222 == icc_failure) {
                memset(buffer, 'f', count);
            }
            if(count == E_ESTB_BUFLEN) {
                if(0 != ht(&(E->hti),buffer)) {
                    ecount++;
                    TF->done = 0;
                    count = 0;
                }
            }
            if(ecount > MAX_HT_FAIL) {
                SetRNGError("Repeated failure of low level entropy checks",__FILE__,__LINE__);
                count = 0;
                break;
            }
        }
    }
    return count;
}

