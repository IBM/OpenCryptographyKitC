/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: High resolution timing code, test case
//
*************************************************************************/

#include <stdio.h>
#include <DELTA/delta_t.h>   

int main(int argc, char *argv[])
{
  unsigned long delta = 0L;
  unsigned long r,et,mt,mn;
  unsigned long i;
  int k;
  volatile unsigned long j;
  double c2t = 1.0;
  /* Call the calibrartion routine 
   We use c2t in the print below to convert run counts to run time 
  */
  c2t = Delta2Time(0);

  Delta_T(1,&et); /* Initialize the counter for overall time */
  for(i = 0l; i < 28; i++) {
    Delta_T(1,&delta); /* Initialize the counter for this run */
    for(j = 0; j < (1<<i); j++);
    r = Delta_T(0,&delta); /* Get the test run delta counts */
    /* Print the output */
    fprintf(stderr,"i = %lu, r = %lu t = %e nS\n",i,r,c2t * r );
  }
  et = Delta_T(0,&et); /* Get the overall run counts */ 
  mn = 999999L;
  for(k = 0; k < 1024; k++) {   
    Delta_T(1,&mt); /* Minimum resolvable count */
    mt = Delta_T(0,&mt);
    if(mt < mn) mn = mt;
  }

  /* Print out the timer calibration data */
  fprintf(stderr,"\nCalibration:\nConversion factor = %g nS/count\nResolution = %lu count\nSpan = %lu counts, %g nS. Minimum resolvable counts %lu\n\n",c2t,Delta_res(),Delta_spanC(),Delta_spanT(),mn);

  /* Print out the elapsed counts/time */
  fprintf(stderr,"Elapsed counts = %lu, Elapsed time = %g Seconds\n",et, (et *c2t)*1e-9);
  return 0;

}
