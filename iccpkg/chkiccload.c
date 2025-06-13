/* Copyright IBM Corp. 2023

 Licensed under the Apache License 2.0 (the "License"). You may not use
 this file except in compliance with the License. You can obtain a copy
 in the file LICENSE in the source distribution.
 */
/*
 Description: Check that FIPS/non-FIPS module is present and loadable
*/

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

#   if defined(ICCPKG)
#      define PATH NULL
#   else
#      define PATH "../package"
#   endif
#endif

float version = 0.0; /* Used to compare ICC version numbers */




int check_status( ICC_STATUS *status,char *tag, const char *file, int line )                
{                              
  const char *sev = "UNKNOWN ERROR TYPE";
  int rv = ICC_OK;


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
  case ICC_OK:
    sev = "OK";
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
  case ICC_OK:
    printf("%s: Status Check (%s): majRC: %d minRC: %d\n",tag,status->desc, status->majRC,status->minRC);
    break;
  default:
    printf("%s: Status Check (%s): majRC: %d minRC: %d  \"Something bad happened\"\n",tag,sev,status->majRC,status->minRC);
    break;
  }

  rv = status->majRC;

  return rv;
}


int doTest(int fips)
{
  int rv = ICC_OK;
  ICC_STATUS * status = NULL;
  ICC_CTX *ICC_ctx = NULL;
  int retcode = 0;
  char* value = NULL;

  status = (ICC_STATUS*)calloc(1,sizeof(ICC_STATUS));
  value = (char*)malloc(ICC_VALUESIZE);

  ICC_ctx = ICC_Init(status,PATH);

  rv = check_status(status, "Init", __FILE__, __LINE__);

  if(NULL == ICC_ctx) {
    printf("ICC_Init failed - NULL returned - ICC shared library missing or not loadable ?\n");
    rv = ICC_ERROR;
  } 
  else {
    retcode = ICC_SetValue(ICC_ctx,status,ICC_FIPS_APPROVED_MODE, fips?"on":"off");
    rv = check_status(status,"SetValue ICC_FIPS_APPROVED_MODE",__FILE__,__LINE__);
    
    if(retcode != ICC_OK) {
       printf("Couldn't enter %sFIPS mode.\n", fips?"":"NON-");
       rv = ICC_ERROR;
    }
    else {
       retcode = ICC_Attach(ICC_ctx, status);
       rv = check_status(status, "Attach", __FILE__, __LINE__);
       if (retcode != ICC_OSSL_SUCCESS) {
          printf("attach failed\n");
          rv = ICC_ERROR;
       }
       else {
          retcode = ICC_GetValue(ICC_ctx, status, ICC_FIPS_APPROVED_MODE, value, ICC_VALUESIZE);
          rv = check_status(status, "ICC_GetValue ICC_FIPS_APPROVED_MODE", __FILE__, __LINE__);
          if (retcode != ICC_OK) {
             printf("ICC_GetValue failed\n");
             rv = ICC_ERROR;
          }
          else if ((0 != strcmp(value, fips ? "on" : "off")))
          {
             printf("%s mode failed\n", fips ? "FIPS" : "NON-FIPS");
             rv = ICC_ERROR;
          }
          else {
             retcode = ICC_GetValue(ICC_ctx, status, ICC_VERSION, value, ICC_VALUESIZE);
             rv = check_status(status, "ICC_GetValue", __FILE__, __LINE__);
             if (retcode != ICC_OK) {
                printf("ICC_GetValue ICC_VERSION failed\n");
                rv = ICC_ERROR;
             }
             else {
                printf("ICC %s\n", value);
             }
          }
       }
    }

    ICC_Cleanup(ICC_ctx,status);
  }
  free(status);
  free(value);

  return rv;
}



static void usage(char *prgname,char *text)
{
  printf("Usage: %s -f -n",prgname);
  printf("       %s Checks if ICC is loadable, use -f to check for FIPS, -n for non-FIPS. Default set on compile\n",prgname);
  if(text) {
    printf("\n%s\n",text);
  }
}

#define DO_EXPAND(VAL)  VAL ## 1
#define EXPAND(VAL)     DO_EXPAND(VAL)
  
int main(int argc, char *argv[])
{
  int fips = 0;
  int argi = 1;
  int rc = 0;

  /* the Makefile knows the build settings so can tell us if FIPS should be present
  // ref: muppet.mk
  */
#if (defined(IS_FIPS) && (EXPAND(IS_FIPS) != 1))
  fips = IS_FIPS;
#endif
  if (!fips) {
     /* check for legacy FIPS module present in build
     // LEGACY is the name of the OLD_ICC FIPS module "" if not present (no-FIPS module)
     */
     const char* legacy = LEGACY;
     fips = strlen(legacy);
  }

  /* command line override for FIPS module check */
  while(argc > argi  ) {
     if (strcmp("-f", argv[argi]) == 0) {
        fips = 1;
     }
     else if (strcmp("-n", argv[argi]) == 0) {
        fips = 0;
     } else {
      usage(argv[0],NULL);
      exit(1);
    }
    argi++;
  }

   printf("FIPS %s\n", fips?"required":"not required");
   if (ICC_OK != doTest(fips)) {
      printf("Module check failed\n");
      rc = 1;
   }
   else {
      printf("Module check passed\n");
   }
  
  return rc;
}
