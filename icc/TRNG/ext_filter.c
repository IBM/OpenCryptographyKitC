/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/


#include <stdio.h>
#include "iccdef.h"
/* Debug aid while we are developing the code 
   Note we also use the noop code when collecting raw data for later analysis (INSTRUMENTED)
   as the reason we collect raw data is to be able to test different filter options
*/
#if !defined(PROC_DEBIAS) || defined(INSTRUMENTED)
void proc_mem(T_FILTER *TF,unsigned char c)
{
}
int ChkMem(T_FILTER *TF,unsigned char c)
{
    return 1;
} 
#endif



#if defined(PROC_DEBIAS) && !defined(INSTRUMENTED)
/* Try to squeeze the distribution of incoming data

   Note there is a problem here, not that the filter works or does bad things

   Catch is: Lets say we run at 10kbytes/second, that's 4k or 400mS delay if we simply prefill that using incoming data, you 
   COULD do that but the result is unusable for performance reasons.

   On the other hand we really do need to fill that 4k FIFO before passing data through.
   Note: Unlike most FIFO's we don't take data from the FIFO output, it's simply there so we can create a stable sliding window.
   Note: With higher incoming data rates (say hardware) prefill() could be discarded and the FIFO actually used as the output FIFO
         we had to get clever to get sufficient performance.

   So. In the software case where data rates are low prefill the filter with well distributed data. 
   That doesn't impact the randomness of what's getting through, all it does is ensure it's well distributed as well.

   Note: ChkMem() is called BEFORE proc_mem(), again, remember we don't use data from here, we simply check what's incoming 
   for pushing the distribution of the sliding window too far off balance.

   This could be simplified down to one API call, but we tried other things before working this one out and that's how the API fell out.
*/

/* @brief Internal call.
   Needed because at startup we don't want to wait 400mS for the
   filter to initialise
   @param tf pointer to a T_FILTER struct
   @param v the next value to be used
*/
static void proc_mem_nocheck( T_FILTER *tf,unsigned char v)
{  
   unsigned char ov; /* old value */
   int idx = tf->idx;
   /* Update frequencies */
   ov = tf->fifo[idx];
   if(tf->totl >= HISTSZ) { /* FIFO full */
      if (tf->arry[ov] > 0) /* Paranoia */
      {
         tf->arry[ov]--; /* Old value from FIFO -- */
      }
   } else {  /* Caps at HISTSZ so we can't overflow */
      tf->totl++;
   }
   tf->arry[v]++; /* New value ++ */
   /* Replace oldest FIFO entry with new value */
   tf->fifo[idx] = v;
   idx++;
   tf->idx = idx;
}

/*
   The purpose of this is to avoid the need to initialize the distribution squeeze function with real data
   We aren't taking data from the FIFO and all this phase does is keep the distribution within the bounds expected by NIST
   without an intolerable peformance hit. 
 
   We prefill the FIFO with data that is evently distrubuted within each 256 byte block i.e. very flat.

   There is a very slight bias in the early data that gets through which is related to the initial data sequence so 
   we scramble that as well so it's different per instance.
   Final residual issue is that if you start with a perfectly flat distribution the possible values are overconstrained and the 
   distribution is too squeezed for a while. i.e. even with a cap at 18, you will usually get values less than 14 in some cells.
   
   You could get complicated, used saved data, try to synthesize it, but simply leaving 
   FREEDOM blocks at the end unfilled allows nearly the right amount of slack to get the same distribution short term as we'll
   get long term.

   Remember - if we use prefill we aren't pulling data from the end of the FIFO, this is simply a method of constraining distribution

   Note: Likely a simple sequence would be adequate as the input data (after we've done this) is still random and only the output constrained, 
   but having neither the time or ability to prove that we make it as variable per instantiation as we can within the required constraints
*/
static void prefill(T_FILTER *tf)
{
   int i,j;
   ICC_UINT64 buffer[(HISTSZ/256)*2];
   unsigned char start;
   unsigned char mask;


   /* Grab the nearest thing we have to random data. As above, doing this is just paranoia */
   RdCtrBurst(buffer,BASELINE*2,fips_loops());
   /* Create evenly distributed data in each block, just a counter with a random starting point
      and xor'd with a random mask for each block
   */
   for(i = 0; i < BASELINE - FREEDOM;i++) {
      mask = (buffer[i*2] & 0xff);
      start = (buffer[(i*2)+1] & 0xff);
      for(j = 0; j <256; j++) {
         proc_mem_nocheck(tf,(start+j)^mask);
      }
   }
   /* Final sanity check, all values in array should be the same (BASELINE-FREEDOM) at this point */
#if 0
   /* Used during development only, the alg is simple enough that it only needs debug once */
   for(i = 0; i < 256; i++) {
      if(tf->arry[i] != BASELINE-FREEDOM) {
         fprintf(stderr,"Something bad happened initializing the TRNG. We have non-uniform values index %d, value %d %s:%d, aborting\n",i,tf->arry[i],__FILE__,__LINE__);

      }
   }
#endif   
}

/*! @brief Check whether our entry occurs often enough in memory to be discarded as frequent long term
   @param tf T_FILTER structure pointer
   @param data to be considered for use truncted to int
   @return 1 use the data, 0 don't use the data
   @note somewhat overloaded as this is used differently in different implementations
   @note when running the setup phase to try and maximize entropy this is unhelpful
*/
int ChkMem(T_FILTER *tf,unsigned char c)
{
   int rv = 1;
   if(!tf->fifo_init) {
      prefill(tf);
      tf->fifo_init = 1;
   }
   if(1 == tf->done) {
      if(tf->arry[c] >= TOO_HIGH) {
         rv = 0;
      }

   }
   return rv;
}




/*! @brief Take the most frequent entries from in, update mem 
   @param tf pointer to a T_FILTER struct
   @param v the next value to be used
   @note 
   A real TRNG behaves like a PRNG with infinite internal state and would inevitably create sequences which 
   now and then be rejected by the test tool. This function simply limits how far from "well distributed" we
   can get by rejecting values which have occurred too frequently in a given sample window.
   @note
   This is quite difficult, not just adding the data, but stopping it climbing
   uncontrollably. So, what we use is a circular list (FIFO) and remove an entry from the list when
   we add new data.
   Find enough space for the next entry, if necessary remove/subtract old data to make space. Update distribution.
      prefill();
   We still check the final results with the same sort of error bound but event accumulation levels are
   set (fairly arbitrarily) at 1/4 the normal level.

   You can't make this much tighter or much loser without problems
   16 and below is "do not use" because this would block < 16 and at 16 it simply replays the initial 
   distribution.
   18 is as far as we can determine sufficient to still leave enough room for the output sequence to be 
   almost completely random, even with prefill.

*/
void proc_mem(T_FILTER *tf,unsigned char v)
{


   if ((tf->idx >= HISTSZ) || (tf->idx < 0))
   {
      tf->idx = 0;
   }
   proc_mem_nocheck(tf,v);
}

#endif
