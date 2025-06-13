/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: High resolution timing code
//             Leverages the event counter code we use for RNG
//             seeding.
//
*************************************************************************/


#if defined(_WIN32)
#   include <windows.h>
#else
#   include <stdio.h>
#   include <sys/time.h>
#   include <string.h>
#   include <stdlib.h>
#endif
#include "TRNG/timer_entropy.h"
#include "DELTA/delta_t.h"

extern ICC_UINT64 RdCTR_raw();
extern int Shift();


#if defined(_WIN32)

struct timezone {
  long tz;
};
/*! @brief Approximate gettimeofday for windows 
  All this does is grab time and populate values
  to generate one-off data
  it's NOT accurate
  \Platf Windows only
  @param tv Pointer to a Unixy "struct timeval"
  @param tz Pointer to "Unixy" "struct timezone" (Unused)
  @return 0
*/
static int gettimeofday(struct timeval *tv, struct timezone *tz)
{
   /* Contains a 64-bit value representing the number of 100-nanosecond intervals since January 1, 1601 (UTC).*/
  FILETIME ft;
  unsigned long long ull;

  if( NULL != tv) {
    GetSystemTimeAsFileTime(&ft);
    ull = ft.dwHighDateTime;
    ull <<= 32;
    ull |= ft.dwLowDateTime;
    ull /= 10; /* usec */
    tv->tv_usec = ull % 1000000;
    tv->tv_sec = ull / 1000000;
  }
  return 0;
}

#endif




static double span = 0.0;
static int done = 0;


/*! @brief return the counter span in counts,
  The limit before the counter overflows
  @return  The counter span in counts
*/
unsigned long Delta_spanC()
{
  unsigned long rv = (unsigned int)(-1);
  /* !FIXME !, work out what this is on various OS's */
  return rv;
	    
}

   
unsigned long Delta_T(int mode, unsigned long *d)
{
  unsigned long rv = 0;
  unsigned long t;
  if(mode == 1) {
    *d = RdCTR_raw();
  } else {
    t = RdCTR_raw();
    if(t > *d) {
      rv = t - *d;
    } else {
      /* We need to know when this rolls over #!#! as this is HW/word size 
	      dependent 
	      This is good for x86_64, fixups are in Delta_spanC()
      */
      rv = (Delta_spanC() - (*d)) + t;
    }
  }
  return rv;
}

/*! @brief get the estimate for the base event counter resolution
  @return the estimated counter resolution (in counts)
  @note quite often the lower bits are stuck-at and are unusable
 */
unsigned long Delta_res()
{
  unsigned long j;
  j = (unsigned long)Shift();
  return (unsigned long) (1 << j);
}

/*! @brief return the APPOXIMATE time span of the counter in nS
  @return The approximate time span of the counter
  @note This will run Delta2Time() if it hasn't already been run
*/
double Delta_spanT()
{
  if(!done) {
    Delta2Time(0);
  }
  return span;
}
/*! @brief
   Calculates delta times from struct timeval's,
   @param x the after timeval
   @param y the before timeval
   @return Time difference in nS
   @note struct timeval only has uS resolution
   
*/
static double tv_sub ( struct timeval *x, struct timeval *y)
{

  long ds,dus,d;
  double result = 0.0;


  ds = x->tv_sec - y->tv_sec;
  dus = x->tv_usec - y->tv_usec;
  /* Normalize to uS */
  d = ds * 1000000 + dus;
  /* Convert to nS */
  result = d *1000.0;

  return result;
}


double Delta2Time(int recalc)
{

  static double rv = 1.0;
  struct timeval tv_now, tv_then;
  unsigned long c_now = 0L;
  unsigned long delta = 0L;
  memset(&tv_then,0,sizeof(struct timeval));

  memset(&tv_now,0,sizeof(struct timeval));
  
  if((!done) | recalc) {
    Delta_T(1,&c_now);
    gettimeofday(&tv_then,NULL);
    do {
      delta = Delta_T(0,&c_now);
    } while(delta < (1<<28));
 
   gettimeofday(&tv_now,NULL);

   /* Now do the calcs, we have something near the max count available, and the elapsed time 
    */ 
   rv = tv_sub(&tv_now,&tv_then);
   /* Calculate the longest usable run time (span)
      Needs an OS/wordlength specific switch here 
   */
   span = rv * ((double)((unsigned int)(-1))/delta);

   rv = (rv/(double)delta);

   done = 1;
  }
  return rv;
}
