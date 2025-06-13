/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: Stub ICC calls
//
*************************************************************************/

#if defined(_WIN32)
#   include <windows.h>
#else
#   include <stdio.h>
#   include <stdlib.h>
#endif


unsigned int icc_failure = 0;

void *ICC_Calloc(size_t n, size_t sz,const char *file, int line)
{
  return calloc(n,sz);
}

void ICC_Free(void *ptr)
{
  free(ptr);
}

/* Disable this facility in GenRndData for now 
 */
int efOPENSSL_HW_rand(unsigned char *buf)
{
  return 0;
}
/* Not on ARM */
#if defined(__ARMEL__) || defined(__ARMEB__)
long efOPENSSL_rdtscX()
{
  fprintf(stderr,"No direct asm for TSC support on ARM\n");
  exit(1);
}
#endif
