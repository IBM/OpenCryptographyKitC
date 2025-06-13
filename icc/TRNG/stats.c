/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/**  Collect stats from a T_FILTER
 *    Distribution of Delta's, distribution of data
 */
#include <stdio.h>
#include <string.h>

#define MAX_D 1024
typedef struct
{  
    long delta;
    int freq;             /* Frequency with which that difference occured */
} DIST_S;

typedef struct {
   unsigned int f[256]; /* Frequency with which each LSB occurred in samples */
   DIST_S d[MAX_D];        /* Frequency with which each delta occurred */
} STATS;

static STATS S;

void StatsClear()
{
   memset(&S,0,sizeof(S));
}

static int sorter(const void *ai, const void *bi)
{
   DIST_S *a = (DIST_S *)ai;
   DIST_S *b = (DIST_S *)bi;
   return a->delta - b->delta;

}
/* Accumulate stats of deltas */
void StatD(unsigned long d)
{
   int i;
   for(i = 0; i < (MAX_D-1); i++) {
      if(0 == S.d[i].delta) {
          S.d[i].delta = d;
      }
      if(S.d[i].delta == d) {
         S.d[i].freq++;
         break;
      }
   }
   if(i == MAX_D-1) {
         S.d[MAX_D-1].freq++;
   }
}
/* Accumulate stats on values of LSB's */
void StatV(unsigned long v)
{
   S.f[v &0xff]++;
}

void dump_stats(const char *prefix)
{
   FILE *fout = NULL;
   char filename[256]; /* Not worried about overflows, this is internal test code */
   int i, nd;
   /* Delta data needs some sorting to make sense */
   for(nd = 0; nd < MAX_D -1; nd++)
   {
      if(0 == S.d[nd].freq) break;
   }

   sprintf(filename,"%sValues.dat",prefix);
   fout = fopen(filename,"w");
   for( i = 0; i < 255; i++) {
      fprintf(fout,"%d %d\n",i,S.f[i]);
   }
   fclose(fout);
      qsort(&(S.d),nd,sizeof(DIST_S),sorter);
   sprintf(filename,"%sDeltas.dat",prefix);               
   fout = fopen(filename,"w");
   for( i = 0; i < nd; i++) {
      fprintf(fout,"%ld %d\n",S.d[i].delta,S.d[i].freq);
   }   
   /* fprintf(fout,"---,%d\n",S.f[MAX_D-1]);*/
   fclose(fout);
}


