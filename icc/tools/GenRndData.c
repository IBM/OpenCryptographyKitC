/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description:                                                               
//           This module generates N bytes of random data for offline         
//           statistical testing. (I.e. with the NIST test suite)
//
*************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32) || defined(_OS2__)
#include <io.h>
#include <fcntl.h>
#define strcasecmp(x,y) _stricmp(x,y)
#else
#include <unistd.h>
#endif

#include "TRNG/noise_to_entropy.h"
#include "TRNG/TRNG_FIPS.h"

#define MIN_SAMP 64


unsigned int icc_failure = 0;


void SetFatalError(const char *msg, const char *file, int line) {
  fprintf(stderr, "%s: %s,%d\n", msg, file, line);
  exit(1);
}
void SetRNGError(const char *msg, const char *file, int line) {
  SetFatalError(msg,file,line);
}

void *ICC_Calloc(size_t n, size_t sz, const char *file, int line) {
  return calloc(n, sz);
}

void ICC_Free(void *ptr) { free(ptr); }

/* Disable this facility in GenRndData for now
*/
int OPENSSL_HW_rand(unsigned char *buf) { return 0; }
/* Not on ARM */
#if defined(__ARMEL__) || defined(__ARMEB__) || defined(__aarch64__)
long efOPENSSL_rdtsc()
{
  fprintf(stderr,"No direct asm for TSC support on ARM\n");
  exit(1);
}
#endif

unsigned int Personalize(unsigned char *buffer)
{
  time_t t;
  if(NULL != buffer) {
    time(&t);
    strcpy((char *)buffer,ctime(&t));
  }
  return 80;
}

static E_SOURCE trng;

static ENTROPY_IMPL MYTRNGS [2] = {
  {
    "TRNG_FIPS",       /*!< Common name */
    TRNG_FIPS,          /*!< Enum used internally */  
    4, /*!< Number of bits needed to produce nominally one bit of entropy after compression */           
    TRNG_FIPS_getbytes, /*!< Callback to a buffer of entropy data */
    TRNG_FIPS_Init,    /*!< Callback for TRNG Initialization */
    TRNG_FIPS_Cleanup, /*!< Callback for TRNG Cleanup */
    TRNG_FIPS_preinit, /*!< Callback for (global) setup for this TYPE of entropy source */
    TRNG_FIPS_Avail,   /*!< availability */
    NULL,
    1
  }


};



void usage(char *me, char *why) {
  if(why != NULL) {
    fprintf(stderr,"%s failed, reason: %s\n",me,why);
  }
  fprintf(stderr,"Usage %s N >outfile\n",me);
  fprintf(stderr,"OR:   %s N -o outfile\n",me);
  fprintf(stderr,"      N is the number of bytes of data to generate, N should be > 20,000, 250,000 is recommended\n");
  fprintf(stderr,"      Output will be rounded up to a 256 byte boundary\n");
  fprintf(stderr,"      %s is intended to generate raw random from ICC's internal TRNG for offile statistical testing.\n",me);

  fprintf(stderr,"      Note: To test other variants, install GSKit-Crypto and use GenRndData2\n");

}
/*!
  @brief Generate data needed for offline statistical testing of ICC's "raw" TRNG source
  Note that this is independent of ICC/OpenSSL code, and simply contains the "raw" entropic
  byte source. Normally this data will be further whitened by hashing within ICC. However
  doing that will hide real defects.
  @param argc number of command line arguments
  @param argv pointer to an array of strings containing command line arguments
  @return 0 = sucess, !0 = fail
*/

int main(int argc, char *argv[])
{
  int n;
  unsigned char buffer[256];

  FILE *fp = NULL;
  char *outFile = NULL;
  char *env = NULL;



  if (argc < 2)
  {
    usage(argv[0], "Insufficient arguments");
    exit(0);
  }
  n = atoi(argv[1]);
  if (n <= 0)
  {
    usage(argv[0], "N must be a positive number");
    exit(1);
  }
  if (n < 256)
  {
    usage(argv[0], "N is unusably small, try 20000");
    exit(1);
  }
  if (argc > 3)
  {
    outFile = argv[3];
    if ((fp = fopen(outFile, "wb")) == NULL)
    {
      printf("Output file [%s] could not be opened \n", outFile);
      exit(1);
    }
  }
  else
  {
    fp = stdout;
  }


#if defined(_WIN32) || defined(_OS2__)
  fflush(stdout);
  _fmode = _O_BINARY;
  _setmode(_fileno(stdout), _fmode);
#endif

    memcpy(&trng.impl, &MYTRNGS[0], sizeof(trng.impl));
    ht_Init(&(trng.hti), 50);


  if (NULL != trng.impl.preinit)
  {
    trng.impl.preinit(0);
  }
  if (TRNG_OK != trng.impl.init(&trng, NULL, 0))
  {
    fprintf(stderr, "Could not initialize TRNG, exiting\n");
    exit(1);
  }
  trng.impl.gb(&trng, buffer, 1);
  do
  {
    memset(buffer, 0, sizeof(buffer));
    trng.impl.gb(&trng, buffer, 256);
    fwrite(buffer, 256, 1, fp);
    n -= 256;
  } while (n > 0);

  if (NULL != trng.impl.cleanup)
  {
    trng.impl.cleanup(&trng);
  }

  if ((NULL != fp) && (stdout != fp))
  {
    fclose(fp);
  }
  exit(0);
}
