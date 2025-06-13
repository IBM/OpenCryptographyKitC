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
//           This variant generates data via the SP800-90 API
//           allowing us to test output of the TRNG and the various
//           PRNG's ICC supports
//
*************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32) || defined(_OS2__)
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#endif


#include "icc.h"
  static const char *alglist[] = 
    {
      "DSS_3_1_SHA","DSS_3_2_SHA","DSS_3_1_SHArev","DSS_3_2_SHArev",
      "DSS_3_1_SHAgp","DSS_3_1_SHArevgp",
      "AES-128-ECB","AES-192-ECB","AES-256-ECB",
      "AES-128-ECB-NODF","AES-192-ECB-NODF","AES-256-ECB-NODF",
      "SHA1","SHA224","SHA256","SHA384","SHA512",
      "HMAC-SHA1","HMAC-SHA224","HMAC-SHA256","HMAC-SHA384","HMAC-SHA512",
      "TRNG_OS","TRNG_HW","TRNG_FIPS",
      "ETAP_OS","ETAP_HW","ETAP_FIPS",
      "NOISE_OS","NOISE_HW","NOISE_FIPS",	
      NULL
    };
#if defined(__hpux) || defined(__sun) || defined(_WIN32)

static int setenv(const char *name,const char *value,int overwrite)
{
  char *tmp = NULL;
  int l;
  if(!overwrite) {
    if(NULL != getenv(name) ) return 0;
  }
  l = strlen(name) + strlen(value) + 1 + 1;
  tmp = malloc(l);
  strncpy(tmp,name,l);
  strncat(tmp,"=",l);
  strncat(tmp,value,l);
  putenv(tmp);
  free(tmp);
  return 0;
}
#endif

void usage(char *me, char *why) {
  int i;
  if(why != NULL) {
    fprintf(stderr,"%s failed, reason: %s\n",me,why);
  }
#if !defined(ICCPKG)
  fprintf(stderr,"Usage %s pathToICC mode N >outfile\n",me);
  fprintf(stderr,"OR:   %s pathToICC mode N -o outfile\n",me);
  fprintf(stderr,"      pathToICC is the path to the ICC libraries passed to ICC_Init()\n"); 
#else 
  fprintf(stderr,"Usage %s mode N >outfile\n",me);
  fprintf(stderr,"OR:   %s mode N -o outfile\n",me);
#endif  
  fprintf(stderr,"      mode is one of:\n");
  for(i = 0; NULL != alglist[i]; i++) {
    fprintf(stderr,"         %s\n",(char *)alglist[i]);
  }
  fprintf(stderr,"      N is the number of bytes of data to generate, N should be > 20,000, 250,000 is recommended\n");
  fprintf(stderr,"      Output will be rounded up to a 256 byte boundary\n");
  fprintf(stderr,"      %s is intended to generate raw random from ICC's internal TRNG for offile statistical testing.\n",me);
  fprintf(stderr,"\n      NOTE: ETAP_* and NOISE_* are tap points for FIPS testing. DO NOT USE AS RNG's.\n");
}
/*!
  @brief Generate data needed for offline statistical testing of ICC's RNG sources
  @param argc number of command line arguments
  @param argv pointer to an array of strings containing command line arguments
  @return 0 = sucess, !0 = fail
*/
#define BUFSZ 1024
unsigned char buffer[BUFSZ];
unsigned char lastbuffer[BUFSZ];

int main(int argc, char *argv[])
{
  int n,i;
  const char *mode = NULL;
  FILE * fp = NULL;
  char * outFile;
  ICC_STATUS stat, *status = &stat;
  ICC_CTX *ICC_ctx = NULL;
  ICC_PRNG *rng = NULL;
  ICC_PRNG_CTX * ctx = NULL;
  SP800_90STATE state = SP800_90UNINIT;
  char *path = NULL;
  int test_old = 0;
  int m = 0;
#if !defined(ICCPKG)
#define MAXARGS 3
  if(argc < (MAXARGS+1)) {
    usage(argv[0],"Insufficient arguments\n");
    exit(0);
  }
  path = argv[1];

#else
#define MAXARGS 2
  if(argc < (MAXARGS+1)) {
    usage(argv[0],"Insufficient arguments\n");
    exit(0);
  }

#endif
  memset(lastbuffer,0,sizeof(lastbuffer));
    for(i = 0; alglist[i] != NULL; i++) {
    if(0 == strcmp(argv[MAXARGS-1],alglist[i]) ) {
      mode = alglist[i];
      break;
    }
  }
  if(NULL == mode) {
    usage(argv[0],"mode was not a valid ICC RNG mode");
    exit(1);
  }
  n = atoi(argv[MAXARGS]);
  if(n <= 0) {
    usage(argv[0],"N must be a positive number");
    exit(1);
  }

  if (argc > MAXARGS+2)
  {
      outFile = argv[MAXARGS+2];
      if ( (fp = fopen(outFile, "wb")) == NULL ) {
	  printf ("Output file [%s] could not be opened \n", outFile);
	  exit(1);
      }
  } else {
    fp = stdout;
#if defined(_WIN32) || defined(_OS2__)
    fflush(stdout);  
    _fmode = _O_BINARY;
    _setmode(_fileno(stdout), _fmode);
#endif
  }
  if(NULL == mode) {
    usage(argv[0],"No RNG mode selected, exiting\n");
    exit(1);
  }
  /* 
     If testing TRNG_ALT or TRNG_ALT2 - select them as the default TRNG
     so we don't hit the latency of the default
  */
  if(strstr(mode,"OS")) {
    setenv("ICC_TRNG","TRNG_OS",0);
  } else if(strstr(mode,"HW")) {
    setenv("ICC_TRNG","TRNG_HW",0);
  } else if(strstr(mode,"FIPS")) {
    setenv("ICC_TRNG","TRNG_FIPS",0);
  }

  memset(status,0,sizeof(ICC_STATUS));
  ICC_ctx = ICC_Init(status,path);
  if(NULL == ICC_ctx) {
    fprintf(stderr,"Could not initialize ICC [%s], exiting\n",status->desc);
    exit(1);
  }
  ICC_SetValue(ICC_ctx,status,ICC_FIPS_APPROVED_MODE,"off");
  if( ICC_ERROR == ICC_Attach(ICC_ctx,status) ) {
    fprintf(stderr,"Could not initialize ICC [%s], exiting\n",status->desc);
    ICC_Cleanup(ICC_ctx,status);
    exit(1);
  }
#if 0  
  /* If testing TRNG's make sure the mode was set as the default */
  if(strstr(mode,"TRNG")) {
     ICC_GetValue(ICC_ctx,status,ICC_SEED_GENERATOR,buffer,20);
     /* fprintf(stderr,"mode [%s] set [%s]\n",mode,buffer); */
     if(strstr(mode,"ALT4")) {		
    	if(0 == strstr((const char *)buffer,"ALT4")) {
      	   fprintf(stderr,"ICC TRNG was not set by default to the specified mode\n");
      	   fprintf(stderr,"Try \"export ICC_TRNG=ALT4\" before running %s\n",argv[0]);
           exit(1);
	}
     } else if(strstr(mode,"ALT3")) {		
    	if(0 == strstr((const char *)buffer,"ALT3")) {
      	   fprintf(stderr,"ICC TRNG was not set by default to the specified mode\n");
      	   fprintf(stderr,"Try \"export ICC_TRNG=ALT3\" before running %s\n",argv[0]);
           exit(1);
	}
     } else if(strstr(mode,"ALT2")) {		
    	if(0 == strstr((const char *)buffer,"ALT2")) {
      	   fprintf(stderr,"ICC TRNG was not set by default to the specified mode\n");
      	   fprintf(stderr,"Try \"export ICC_TRNG=ALT2\" before running %s\n",argv[0]);
           exit(1);
	}
     } else if(strstr(mode,"ALT")) {
	if(0 == strstr((const char *)buffer,"ALT")) {
      	   fprintf(stderr,"ICC TRNG was not set by default to the specified mode\n");
      	   fprintf(stderr,"Try \"export ICC_TRNG=ALT\" before running %s\n",argv[0]);
           exit(1);
	}     
     }  else if(0 != strcmp((const char *)buffer,"TRNG"))  {
      	fprintf(stderr,"ICC TRNG was not set by default to the specified mode\n");
      	fprintf(stderr,"Try \"export ICC_TRNG=\" before running %s\n",argv[0]);
        exit(1);
     }
  }
#endif
  ctx = ICC_RNG_CTX_new(ICC_ctx);

  if (NULL == ctx)
  {
    if (0 == strcmp(mode, "TRNG_ALT") || 0 == strcmp(mode, "TRNG"))
    {
      test_old = 1;
    }
    else
    {
      usage(argv[0], "Could not create the requested RNG context");
      fclose(fp);
      ICC_Cleanup(ICC_ctx,status);
      exit(1);
    }
  }
  ICC_RNG_CTX_free(ICC_ctx, ctx);

  if (0 == test_old)
  {
    ctx = ICC_RNG_CTX_new(ICC_ctx);
    rng = ICC_get_RNGbyname(ICC_ctx, mode);
    if (NULL == rng)
    {
      usage(argv[0], "Mode could not be selected as an RNG mode for this ICC context");
      fclose(fp);
      ICC_RNG_CTX_free(ICC_ctx,ctx);
      ICC_Cleanup(ICC_ctx,status);
      exit(1);
    }

    ICC_RNG_CTX_Init(ICC_ctx, ctx, rng, NULL, 0, 0, 0);

    do
    {
      memcpy(lastbuffer, buffer, sizeof(lastbuffer));
      memset(buffer, 0, sizeof(buffer));
      i = (n <= BUFSZ) ? n : BUFSZ;
      state = ICC_RNG_Generate(ICC_ctx, ctx, buffer, i, NULL, 0);
      switch (state)
      {
      case SP800_90RUN:
        break;
      case SP800_90RESEED:
        ICC_RNG_ReSeed(ICC_ctx, ctx, NULL, 0);
        break;
      default:
        fprintf(stderr, "Critical error, RNG state invalid (%d) , aborting\n", state);
        fclose(fp);
       ICC_RNG_CTX_free(ICC_ctx,ctx);
      ICC_Cleanup(ICC_ctx,status);
        exit(1);
        break;
      }
      fwrite(buffer, i, 1, fp);
      n -= i;
      m += i;

      if (0 == memcmp(lastbuffer, buffer, i))
      {
        fprintf(stderr, "Critical error, duplicated data %d - %d , aborting\n", m - i, m);
        exit(1);
      }
    } while (n > 0);

    ICC_RNG_CTX_free(ICC_ctx, ctx);
  }
  else
  { /* test TRNG/TRNG_ALT using old paths */
    do
    {
      memcpy(lastbuffer, buffer, sizeof(lastbuffer));
      memset(buffer, 0, sizeof(buffer));
      i = (n <= BUFSZ) ? n : BUFSZ;
      ICC_GenerateRandomSeed(ICC_ctx, status, i, buffer);
      if (status->majRC != ICC_OK)
      {
        fprintf(stderr, "Internal RNG failure [%s]\n", status->desc);
        fclose(fp);
        ICC_RNG_CTX_free(ICC_ctx,ctx);
        ICC_Cleanup(ICC_ctx,status);
        exit(1);
      }
      else
      {
        fwrite(buffer, i, 1, fp);
        n -= i;
        m += i;
      }
      if (0 == memcmp(lastbuffer, buffer, i))
      {
        fprintf(stderr, "Critical error, duplicated data %d - %d , aborting\n", m - i, m);
        fclose(fp);
        ICC_RNG_CTX_free(ICC_ctx,ctx);
        ICC_Cleanup(ICC_ctx,status);
        exit(1);
      }
    } while (n > 0);
  }
  ICC_Cleanup(ICC_ctx, status);
  ICC_ctx = NULL;
  if ((NULL != fp) && (stdout != fp))
  {
    fclose(fp);
  }

  exit(0);
}
