/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: Minimal unit test for ICC
//              Used for checking handling of ICC_INSTALL_PATH/
//              wide character environments
//
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "icc.h"


#if defined(_WIN32)
#   define strcasecmp(a,b) _stricmp(a,b)
#   if defined(ICCPKG)
#      define PATH NULL
#   else
#      define PATH L"../package"
#   endif
#else
ICC_CTX * ICC_LINKAGE ICC_InitW(ICC_STATUS* status,const char* iccpath)
{
  return ICC_Init(status,iccpath);
}
#   if defined(ICCPKG)
#      define PATH NULL
#   else
#      define PATH "../package"
#   endif
#endif

float version = 0.0; /* Used to compare ICC version numbers */
int alt_trng = 0; /* Set if ICC_TRNG=ALT is set in the environment */
int entropy1 = 0;
int entropy2 = 0;

int is_unicode = 0; /*<! contains unicode init state */

/* Assume we never use > 50k of stack */
#define CHECK_RANGE 50000
#define MARKER 0xa5


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
float VString2(char *vin);

static void printbin( char *s,int l)
{
  int i;
  fprintf(stderr,"len = %d :",l);
  for(i = 0; i < l ; i++) {
    fprintf(stderr,"%02x",((unsigned)s[i] & 0xff));
  }
  fprintf(stderr,"\n");
}

#if defined(_WIN32)
static void PrtNfo(ICC_CTX *ctx,ICC_STATUS *status,int unicode)
{
  char *value = NULL;
  wchar_t *valueW = NULL;
  value = (char *)calloc(1,2*ICC_VALUESIZE);  
  valueW = (wchar_t *)calloc(sizeof(wchar_t),2*ICC_VALUESIZE);
  memset(value,0xa5,2*ICC_VALUESIZE);
  memset(valueW,0xa5,2*ICC_VALUESIZE*sizeof(wchar_t));

  /* Note calling these BEFORE a sucessful ICC_Attach() is pointless */
  ICC_GetValue(ctx,status,ICC_VERSION,value,ICC_VALUESIZE);
  vers = VString2(value);
  printf("ICC version %s\n",value);
  if(unicode) {
    ICC_GetValue(ctx,status,ICC_INSTALL_PATH,valueW,ICC_VALUESIZE);
    wprintf(L"Wide install path %s\n",valueW);
    printbin((char *)valueW,2*ICC_VALUESIZE*sizeof(wchar_t));
  } else {
    ICC_GetValue(ctx,status,ICC_INSTALL_PATH,value,ICC_VALUESIZE);
    printf("Install path %s\n",value);
    printbin(value,2*ICC_VALUESIZE);
  }
  free(value);
  free(valueW);
}

#else

static void PrtNfo(ICC_CTX *ctx,ICC_STATUS *status,int unicode)
{
  char *value = NULL;
  value = (char *)calloc(1,2*ICC_VALUESIZE);  

  memset(value,0xa5,2*ICC_VALUESIZE);
  /* Note calling these BEFORE a sucessful ICC_Attach() is pointless */
  ICC_GetValue(ctx,status,ICC_VERSION,value,ICC_VALUESIZE);
  vers = VString2(value);
  printf("ICC version %s\n",value);

  ICC_GetValue(ctx,status,ICC_INSTALL_PATH,value,ICC_VALUESIZE);
  printbin(value,2*ICC_VALUESIZE);
  printf("Install path %s\n",value);
  printbin(value,2*ICC_VALUESIZE);
  free(value);

}

#endif

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
    switch(status->minRC) {
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
   printf("Line %d: Status Check (%s): majRC: %d minRC: %d\n",line,status->desc, status->majRC,status->minRC);
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




int doTestNoDef(int unicode)
{
  int rv = ICC_OK;
  ICC_STATUS * status = NULL;
  ICC_CTX *ICC_ctx = NULL;
  ICC_CTX *ICC_ctx1 = NULL;
  int retcode = 0;
#if defined(_WIN32)
  static wchar_t *wpath = NULL;
#endif

  status = (ICC_STATUS*)calloc(1,sizeof(ICC_STATUS));
  ICC_ctx = ICC_InitW(status,PATH);
  rv = check_status(status,__FILE__,__LINE__);

  if(NULL == ICC_ctx) {
    printf("ICC_Init failed - NULL returned - ICC shared library missing or not loadable ?\n");
    rv = ICC_ERROR;
  }  
  retcode = ICC_Attach(ICC_ctx,status);
  rv = check_status(status,__FILE__,__LINE__);
  if (retcode != ICC_OSSL_SUCCESS)  {
      printf ("attach failed\n");
      rv = ICC_ERROR;
  }


  PrtNfo(ICC_ctx,status,1);
  

  /* second context */

  ICC_ctx1 = ICC_InitW(status,PATH);

  rv = check_status(status,__FILE__,__LINE__);

  if(NULL == ICC_ctx1) {
    printf("ICC_Init failed - NULL returned - ICC shared library missing or not loadable ?\n");
    rv = ICC_ERROR;
  }  
  retcode = ICC_Attach(ICC_ctx1,status);
  rv = check_status(status,__FILE__,__LINE__);
  if (retcode != ICC_OSSL_SUCCESS)  {
      printf ("attach failed\n");
      rv = ICC_ERROR;
  }
  PrtNfo(ICC_ctx1,status,1);    
  

  return rv;
}


int doTestNoFIPS(int unicode)
{
  int rv = ICC_OK;
  ICC_STATUS * status = NULL;
  ICC_CTX *ICC_ctx = NULL;
  ICC_CTX *ICC_ctx1 = NULL;
  int retcode = 0;
  char* value = NULL;

#if defined(_WIN32)
  static wchar_t *wpath = NULL;
#endif


  status = (ICC_STATUS*)calloc(1,sizeof(ICC_STATUS));
  value = (char *)malloc(ICC_VALUESIZE);  
 
  ICC_ctx = ICC_InitW(status,PATH);

  rv = check_status(status,__FILE__,__LINE__);

  if(NULL == ICC_ctx) {
    printf("ICC_Init failed - NULL returned - ICC shared library missing or not loadable ?\n");
    rv = ICC_ERROR;
  } 

  retcode = ICC_SetValue(ICC_ctx,status,ICC_FIPS_APPROVED_MODE,"off");
  rv = check_status(status,__FILE__,__LINE__);
    
  if(retcode != ICC_OK) {
     printf("Couldn't enter NON-FIPS mode. Is this is a non-FIPS build ?\n");
  }

  retcode = ICC_Attach(ICC_ctx,status);
  rv = check_status(status,__FILE__,__LINE__);
  if (retcode != ICC_OSSL_SUCCESS)  {
      printf ("attach failed\n");
      rv = ICC_ERROR;
    }
      
  /* Note calling these BEFORE a sucessful ICC_Attach() is pointless */
  value[0] = '\0';
  retcode = ICC_GetValue(ICC_ctx,status,ICC_VERSION,value,ICC_VALUESIZE);

  print_cfg(ICC_ctx,"    ");
  PrtNfo(ICC_ctx,status,1);

  /* second context */
  ICC_ctx1 = ICC_InitW(status,PATH);
  rv = check_status(status,__FILE__,__LINE__);

  if(NULL == ICC_ctx1) {
    printf("ICC_Init failed - NULL returned - ICC shared library missing or not loadable ?\n");
    rv = ICC_ERROR;
  }  

  retcode = ICC_SetValue(ICC_ctx1,status,ICC_FIPS_APPROVED_MODE,"off");
  rv = check_status(status,__FILE__,__LINE__);
    
  if(retcode != ICC_OK) {
     printf("Couldn't enter NON-FIPS mode. Is this is a non-FIPS build ?\n");
  }

  retcode = ICC_Attach(ICC_ctx1,status);
  rv = check_status(status,__FILE__,__LINE__);
  if (retcode != ICC_OSSL_SUCCESS)  {
      printf ("attach failed\n");
      rv = ICC_ERROR;
    }
      
  PrtNfo(ICC_ctx1,status,1);
  print_cfg(ICC_ctx1,"    ");

  return rv;
}


int doTestFIPS(int unicode)
{
  int rv = ICC_OK;
  ICC_STATUS * status = NULL;
  ICC_CTX *ICC_ctx = NULL;
  ICC_CTX *ICC_ctx1 = NULL;
  int retcode = 0;
#if defined(_WIN32)
  static wchar_t *wpath = NULL;
#endif

  status = (ICC_STATUS*)calloc(1,sizeof(ICC_STATUS));  
  ICC_ctx = ICC_InitW(status,PATH);
  rv = check_status(status,__FILE__,__LINE__);

  if(NULL == ICC_ctx) {
    printf("ICC_Init failed - NULL returned - ICC shared library missing or not loadable ?\n");
    rv = ICC_ERROR;
  } 

  retcode = ICC_SetValue(ICC_ctx,status,ICC_FIPS_APPROVED_MODE,"on");
  rv = check_status(status,__FILE__,__LINE__);
    
  if(retcode != ICC_OK) {
     printf("Couldn't enter FIPS mode. Is this is a non-FIPS build ?\n");
  }

  retcode = ICC_Attach(ICC_ctx,status);
  rv = check_status(status,__FILE__,__LINE__);
  if (retcode != ICC_OSSL_SUCCESS)  {
      printf ("attach failed\n");
      rv = ICC_ERROR;
    }
      
  PrtNfo(ICC_ctx,status,1);
  print_cfg(ICC_ctx,"    ");
  

  /* second context */
  ICC_ctx1 = ICC_InitW(status,PATH);
  rv = check_status(status,__FILE__,__LINE__);

  if(NULL == ICC_ctx1) {
    printf("ICC_Init failed - NULL returned - ICC shared library missing or not loadable ?\n");
    rv = ICC_ERROR;
  }  

  retcode = ICC_SetValue(ICC_ctx1,status,ICC_FIPS_APPROVED_MODE,"on");
  rv = check_status(status,__FILE__,__LINE__);
    
  if(retcode != ICC_OK) {
     printf("Couldn't enter NON-FIPS mode. Is this is a non-FIPS build ?\n");
  }

  retcode = ICC_Attach(ICC_ctx1,status);
  rv = check_status(status,__FILE__,__LINE__);
  if (retcode != ICC_OSSL_SUCCESS)  {
      printf ("attach failed\n");
      rv = ICC_ERROR;
    }
      
  PrtNfo(ICC_ctx1,status,1);
  print_cfg(ICC_ctx1,"    ");

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
  int unicode = 0;
  int argi = 1;
  while(argc > argi  ) {
    if(strncmp("-u",argv[argi],2) == 0) {
      unicode = 1;
    } else if(strncmp("-h",argv[argi],2) == 0) {
      usage(argv[0],NULL);
      exit(0);
    } 
    argi++;
  }

  if (ICC_OK != doTestNoDef(unicode)) {
    printf("ICC Unit Test Failed - No default mode!\n");
    return 1;
  }

  if (ICC_OK != doTestNoFIPS(unicode)) {
    printf("ICC Unit Test Failed - FIPS mode!\n");
    return 1;
  }

  if (ICC_OK != doTestFIPS(unicode)) {
    printf("ICC Unit Test Failed - No default mode!\n");
    return 1;
  }

  printf("ICC Unit Test Program Exiting Successfully !\n");

  return 0;
}
