/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "icc.h"


#if defined(_WIN32)
#define strcasecmp(a,b) _stricmp(a,b)

#endif

float version = 0.0; /* Used to compare ICC version numbers */
int alt_trng = 0; /* Set if ICC_TRNG=ALT is set in the environment */
int entropy1 = 0;
int entropy2 = 0;

int is_unicode = 0; /*<! contains unicode init state */

/* Assume we never use > 50k of stack */
#define CHECK_RANGE 50000
#define MARKER 0xa5

/*! 
  @brief Change the sizes of some keys used in testing 
  We need to test that small keys do work in non-FIPS mode
  for interop.
*/
static int fips_mode = 0;

/*!
  @brief If set ICC_INDUCED_FAILUE is set after ICC_ATTACH
  - proves these code paths work
  - allows us to check some failure points for post-startup
  failures (add 1000 to the normal failure value)
*/  
static unsigned int allow_at_runtime = 0;

/*! 
  @brief The induced failure to trip if set
*/
static unsigned int icc_failure = 0;
/*! @brief prototype for a pointer to OpenSSL style malloc() 
  The last two parameters are expected to be __FILE__, __LINE__
*/
typedef void *(*MallocFunc) (int, const char *,int);

/*! @brief prototype for a pointer to realloc() used by OpenSSL*/
typedef void *(*ReallocFunc) (void *,int, const char *, int);

/*! @brief prototype for a pointer to free() used by OpenSSL */
typedef void (*FreeFunc) (void *);

/*! Numeric version of "Version" used for compares
  Note that 1.4.5 => 1.0405
*/
static float vers = 0.0;
/* 
   ICC isn't threaded when we use these, so we create these scratch buffers
   once as statics
*/
/*! @brief
  Initialization for the ICC1.9+ pre-initialization 
  setup modes. Memory callbacks, RNG's, induced failure tests
  @note this is done unconditionally and returns no errors
  we can't get the ICC version this early ....
  so this WILL fail against ICCPKG
*/

int doPreInit() {
  ICC_STATUS stat, *status = &stat;
  int rv = ICC_OK;
  char *env = NULL;

  env = getenv("ICC_INDUCED_FAILURE");
  if (NULL != env) {
    icc_failure = (unsigned int)atoi(env);
    printf("Testing ICC failure paths [%d]\n",
           ((icc_failure < 1000) ? icc_failure : icc_failure - 1000));

    /* For the test case code,setting values  with 1000 added with
       set the failure flag post-startup
    */
    if (icc_failure > 1000) {
      allow_at_runtime = 1;
    }
  }
  return 1;
}

 /*! @brief print the state of a context, FIPS, errors, 
    TRNG, PRNG used
    @param ctx an initialized ICC context
    @param prefix padding so the print can be offset, printed every line
*/
static void print_cfg(ICC_CTX *ctx, char *prefix)
{
  static char buf[ICC_VALUESIZE]; 
  ICC_STATUS status, *stat = &status;

  buf[0] = 0;
  if(NULL == prefix) prefix = "";

  ICC_GetValue(ctx,stat,ICC_SEED_GENERATOR,buf,20);
  printf("%sFIPS %s\n",prefix,(stat->mode & ICC_FIPS_FLAG) ? "Yes":"No");
  printf("%sERR  %s\n",prefix,(stat->mode & ICC_ERROR_FLAG) ? "Yes":"No");

  printf("%sTRNG %s\n",prefix,buf);
  ICC_GetValue(ctx,stat,ICC_RANDOM_GENERATOR,buf,20);
  printf("%sPRNG %s\n",prefix,buf); 
}
   
int check_status( ICC_STATUS *status, const char *file, int line )                
{                              
  const char *sev = "UNKNOWN ERROR TYPE";
  int rv = ICC_OK;

  if((status->majRC) != ICC_OK) {
    switch(status->majRC) {
    case ICC_ERROR:
      sev = "ICC_ERROR";
      break;
    case ICC_WARNING:
      sev = "ICC_WARNING";  
      break;
    case ICC_FAILURE:
      sev = "ICC_FAILURE";  
      break;
    case ICC_OPENSSL_ERROR:
      sev = "ICC_OPENSSL_ERROR";
      break;
    case ICC_OS_ERROR:
      sev = "ICC_OS_ERROR";
      break;
    default:
      rv = ICC_ERROR;
      break;
    }
    switch(status->majRC) {
    case ICC_ERROR:
    case ICC_FAILURE:
    case ICC_WARNING:
    case ICC_OPENSSL_ERROR:
    case ICC_OS_ERROR:  
      printf("Line %d: Status Check (%s): majRC: %d minRC: %d Error string: %s\n",line, sev,status->majRC, status->minRC , status->desc);
      rv = status->majRC;
      break;
    default:
      printf("Line %d: Status Check (%s): majRC: %d minRC: %d  \"Something bad happened\"\n",line,sev,status->majRC,status->minRC);
      break;
    }
  }                      
  return rv;
}



/*!
  @brief convert the string ICC version to a number
  that can be used in comparisons
  @param vin The input string
  @return a float version of the number i.e. 1.405 1.0902030
  @note
  We allow a 100 range between digits so "1.4.5.0" -> 1.0405000
  NOT 1.450
*/
float VString2(char *vin)
{
  double rv = 0.0;
  double mult = 0.01;
  rv = (double)atoi(vin); /* major - left of '.' */
  for( ; *vin ; vin++) {
    while(*vin && *vin != '.') vin++;
    if(*vin) vin++;
    rv = rv + (atoi(vin) * mult);
    mult *= 0.01;
  }
  return (float)rv;
}




int doUnitTest(int test,char *fips, int unicode)
{
  int rv = ICC_OK;
  int error = 0;
  ICC_STATUS * status = NULL;
  ICC_CTX *ICC_ctx = NULL;
  int retcode = 0;
  char* value = NULL;
  unsigned int tfailure = 0;
  static char *path = NULL;
  int rc = 0;

#if defined(_WIN32)
  static wchar_t *wpath = NULL;
#endif

#if !defined(ICCPKG)
  path = "../package";
#   if defined(_WIN32)
  wpath = L"../package";
#   endif
#endif
  fips_mode = 0;
  /* need to set env externally */
  /* setenv("ICC_ALLOW_INDUCED","1",1); */
  status = (ICC_STATUS*)calloc(1,sizeof(ICC_STATUS));
  value = (char *)malloc(ICC_VALUESIZE);

  if ((NULL != status) && (NULL != value))
  {
    printf("Loading and Initializing ICC with: [%s]\n", path ? path : "NULL (ICCPKG)");
    ICC_ctx = ICC_Init(status, path);

    rv |= check_status(status, __FILE__, __LINE__);

    if (NULL == ICC_ctx)
    {
      error = 1;
      printf("ICC_Init failed - NULL returned - ICC shared library missing or not loadable ?\n");
      rv = ICC_ERROR;
    }

    if (!error && (0 == strcmp(fips, "on")))
    {

      retcode = ICC_SetValue(ICC_ctx, status, ICC_FIPS_APPROVED_MODE, fips);
      /* counter-intuitive ICC_Init is a macro in non-FIPS versions 
       If we run through check_status() here we produce an error 
       which the build scanning scripts pick up when this is supposed
       to fail.
    */
      if (retcode != ICC_OK)
      {
        printf("Couldn't enter FIPS mode. Is this is a non-FIPS build ?\n");
      }
      fips_mode = 1;
    }
    if (!error)
    {
      retcode = ICC_GetStatus(ICC_ctx, status);
      printf("\nAttach()\n");
      retcode = ICC_Attach(ICC_ctx, status);
      rv = check_status(status, __FILE__, __LINE__);
      if (retcode != ICC_OSSL_SUCCESS)
      {
        error = 1;
        rv = ICC_ERROR;
        ICC_Cleanup(ICC_ctx, status);
        printf("\tCheck handling of errors in ICC_Attach()\n");
        ICC_ctx = ICC_Init(status, path);
        if (NULL != ICC_ctx)
        {
          printf("\t\tError. ICC_Attach() failure should be persistant (return code from ICC_Init()) \n");
        }
        if (ICC_OK == status->majRC)
        {
          printf("\t\tError. ICC_Attach() failure should be persistant (status)\n");
        }
        check_status(status, __FILE__, __LINE__);
      }
    }

    /* If we are doing extended - post startup testing, we've already
     set the flag to allow this post ICC_Attach()
     so set the appropriate flag late 
  */
    if (!error)
    {
      if (allow_at_runtime && (icc_failure > 1000))
      {
        tfailure = icc_failure - 1000;
        if (ICC_OK != ICC_SetValue(ICC_ctx, status, ICC_INDUCED_FAILURE, (const void *)&tfailure))
        {
          rv = ICC_OSSL_FAILURE;
        }
        check_status(status, __FILE__, __LINE__);
      }
    }
  }
  else
  {
    rv = ICC_FAILURE;
  }
  if(ICC_OK == rv) {
    printf("\nRun the integrity check\n");
    rc = ICC_IntegrityCheck(ICC_ctx,status);    
    printf("Integrity check returns %d\n",rc);
    check_status(status, __FILE__, __LINE__);
  }
  /* clean up leaks caused by errors being tripped during testing */
  if(NULL != ICC_ctx) {
    printf("Cleanup()\n");
    ICC_Cleanup(ICC_ctx,status);
    ICC_ctx = NULL;
  }


  if(NULL != status) {
    free(status);
  }
  if(NULL != value) {
    free(value);
  }
  if(0 == rv) {
    rv = ICC_OSSL_SUCCESS;
  }
  return rv;
}
static void usage(char *prgname,char *text)
{
  printf("Usage: %s [n]/[-u]/[-h]\n",prgname);
  printf("       %s runs the ICC BVT tests, this covers most of the API\n",prgname
	 );
  printf("           note that correct usage of the ICC API is not guaranteed\n");
  printf("       n = a single test number to run\n");
  printf("       -u<nicode> start ICC with a Unicode path (Windows only)\n");
  printf("       -h this text\n");
  if(NULL != text) {
    printf("\n%s\n",text);
  }
}
  
int main(int argc, char *argv[])
{
  int test = 0;
  int unicode = 0;
  int argi = 1;
  while(argc > argi  ) {
    if(strncmp("-h",argv[argi],2) == 0) {
      usage(argv[0],NULL);
      exit(0);
    } 
    else {
      test = atoi(argv[argi]);
    }
    argi++;
  }
  doPreInit();
  if (ICC_OSSL_SUCCESS != doUnitTest(test,"on",unicode)) {
    printf("ICC Unit Test Failed - FIPS mode!\n");
    return 1;
  }

  if (ICC_OSSL_SUCCESS != doUnitTest(test,"off",unicode)) {
    printf("ICC Unit Test Failed - non-FIPS mode!\n");
    return 1;
  }
  printf("ICC Unit Test Program Exiting Successfully !\n");

  return 0;
}
