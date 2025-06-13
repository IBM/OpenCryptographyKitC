/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*
// Description: Source file for the icc static library
*/

/* IMPORTANT NOTE
   This file generates the ICC static stub - a static library. 
   As there's no way to hide intermodule calls in a static library
   we make the private functions really private by creating the 
   library from only one object file.
   That means that this file #includes all the other C source files
   necessary to create libicc.a rather than using the librarian
   to combine separate objects.
*/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#define ICCSTATIC static
#include "icc_cdefs.h"
#include "icc.h"
#include "iccversion.h"
#include "platform.h"
#define ICC
#include "icc_common.h"
#undef ICC
#include "platform.c"

#if (NON_FIPS_ICC & 1)
#   define ICC_SCCSInfo ICCN_SCCSInfo
#else
#   define ICC_SCCSInfo ICCC_SCCSInfo
#endif


const char ICC_SCCSInfo[] =
  {
    "@(#)CompanyName:      IBM Corporation\n"
    "@(#)LegalTrademarks:  IBM\n"
    "@(#)FileDescription:  " ICC_DESCRIPTION1 ICC_DESCRIPTION2 "\n"
    "@(#)FileVersion:      " ICC_PRODUCT_VERSION "\n"
    "@(#)LegalCopyright:   Licensed Materials - Property of IBM\n"
    "@(#)                  ICC\n"
    "@(#)                  (C) Copyright IBM Corp. 2002,2018\n"
    "@(#)                  All Rights Reserved. US Government Users\n"
    "@(#)                  Restricted Rights - Use, duplication or disclosure\n"
    "@(#)                  restricted by GSA ADP Schedule Contract with IBM Corp.\n"
    "@(#)ProductName:      " ICC_PRODUCT_NAME "\n"
    "@(#)ProductVersion:   " ICC_PRODUCT_VERSION "\n"
  };


/*#define DEBUG_VERBOSE
 */
static char **parse_path (char *path);
static void parse_path_cleanup (char **paths);

#if defined(_WIN32)
#define wcsdup _wcsdup
#define strdup _strdup
static wchar_t **parse_pathW (wchar_t * path);
#endif


static void SetStatusPrivate (ICC_STATUS * stat, int majRC, int minRC,
			      char *mess);
static void SetStatusPrivateOK (ICC_STATUS * stat);
static int ICC_initialize_functions(ICC_CTX *pcb,ICC_STATUS *status,char *path,int iswchar);

  
static char icc_tmp; /*!< And this is only here to muzzle the compilers which report unused xyz */





/*! @brief This structure is anonymous as far as ICC users are
  concerned. It holds per-context information plus in funcs a link to 
  more detailed internal context
*/
struct ICC_t
{
  FUNC **funcs;  /*!< Pointer to the call table */
  int dummy2;      /*!< Dummy to wake up the debugger */       
};

static void SetStatusPrivateLn2(ICC_STATUS *stat,int majRC,int minRC,const char *m1, const char *m2);

#define ICC 1
#include "icc_a.c"
#undef ICC


ICCSTATIC void *ICC_Malloc(size_t sz, const char *file, int line)
{
  void *ptr = NULL;

  ptr = malloc(sz);

  return ptr;
}


ICCSTATIC void *ICC_Calloc(size_t n, size_t sz,const char *file, int line)
{
  void *ptr = NULL;

  ptr = calloc(n,sz);

  return ptr;
}
#if defined(USE_REALLOC)
ICCSTATIC void *ICC_Realloc(void *ptr,size_t sz,const char *file, int line)
{
  void *iptr = NULL;

  iptr = realloc(ptr,sz);

  return iptr;
}
#endif
ICCSTATIC void ICC_Free(void *ptr)
{

  free(ptr);

}
/*! 
  @brief Replacement for strdup: 
  use ICC_Malloc() since the storage is freed by ICC_Free()
  @param str the string to duplicate
  @return the duplicated string
*/

static char *mystrdup(const char *str)
{
  char *dupstr = ICC_Malloc(strlen(str)+1,__FILE__,__LINE__);
  
  if (NULL != dupstr) {
    strncpy(dupstr, str,strlen(str)+1);
  }
  return dupstr;
}
#if defined(_WIN32)
/*! 
  @brief Replacement for wcsdup: 
  use ICC_Malloc() since the storage is freed by ICC_Free()
  @param str the string to duplicate
  @return the duplicated string
*/
static wchar_t * mywcsdup(const wchar_t *str) {
  wchar_t *dupstr = ICC_Calloc(wcslen(str)+1,sizeof(wchar_t),__FILE__,__LINE__);
  if (NULL != dupstr) {
    wcsncpy(dupstr, str,wcslen(str) + 1);
  }
  return dupstr;
}
#endif


/*!
  @brief a version of strncat that's useful.
  @param base a pointer to a fixed length buffer
  @param append the string to append
  @param maxlen the maximum size of base
*/
static void ICC_strlcat(char *base,const char *append,unsigned int maxlen)
{
  int l;
  l = (int)maxlen - (int)strlen(base);
  if( l > 1 ) {
    strncat(base,append,l);
  }
}

static ICC_STATUS default_status = {0,0,"O.K.",0};

/*!
  @brief Sets ICC_STATUS return values
  because string copies are involved, encapsulating this minimizes the areas where buffer overruns need to be trapped
  @param stat pointer to ICC_STATUS
  @param majRC major return code, ICC_OK, ICC_WARNING, ICC_ERROR
  @param minRC minor return code
  @param mess Text description
*/

static void
SetStatusPrivate (ICC_STATUS * stat, int majRC, int minRC, char *mess)
{
  stat->majRC = majRC;
  stat->minRC = minRC;
  strncpy (stat->desc, mess, ICC_DESCLENGTH - 1);
  stat->desc[ICC_DESCLENGTH - 1] = '\0';
}


/*!
  @brief Generate an error with two text strings
  @param stat a preallocated ICC_STATUS structure
  @param majRC the major return code
  @param minRC the minor return code
  @param m1 first - main - message
  @param m2 second message (detail)
*/

static void SetStatusPrivateLn2(ICC_STATUS *stat,int majRC,int minRC,const char *m1, const char *m2)
{
  stat->majRC = majRC;
  stat->minRC = minRC;

  strncpy(stat->desc,m1,ICC_VALUESIZE);
  ICC_strlcat(stat->desc,"(",ICC_VALUESIZE);
  ICC_strlcat(stat->desc,m2,ICC_VALUESIZE);
  ICC_strlcat(stat->desc,") ",ICC_VALUESIZE);
}


/*!
  @brief Calls SetStatusPrivate with ICC_OK,ICC_OK,"OK"
  @param stat pointer to ICC_STATUS
*/
static void SetStatusPrivateOK (ICC_STATUS * stat)
{
  SetStatusPrivate (stat, ICC_OK, ICC_OK, (char *)"OK");
}

/*!
  @brief Sets status to the default status
  @param stat pointer to ICC_STATUS
  @note if ICC_Attach() has failed the default status
  will have changed from O.K. to the last failed status
*/
static void SetStatusPrivateDef (ICC_STATUS * stat)
{
  memcpy(stat,&default_status,sizeof(ICC_STATUS));
}
/*!
  @brief Calls with the default status
  @param stat pointer to ICC_STATUS
  @note This is done in ICC_Attach() if a fatal error is detected
  will have changed from O.K. to the last failed status
*/
static void SetDefaultStatus(ICC_STATUS * stat)
{
  memcpy(&default_status,stat,sizeof(ICC_STATUS));
}



/*!
  @brief
  Initialize the connection to the ICC shared library
  @param status return status
  @param iccpath Unicode root path to ICC libraries, actual libs are (hardwired) relative to this. 
*/



#if defined(_WIN32)
ICC_CTX *ICC_InitW(ICC_STATUS *status, const wchar_t *iccpath) {
  int i, x;
  wchar_t *path = NULL;
  wchar_t *wpath = NULL;
  ICC_CTX *pcb = NULL;
  wchar_t **paths = NULL;
  fptr_lib_init *tmpf = NULL;

  if (!ICCGlobal.mutexInit) { /* Race, but unavoidable */
    if (0 == ICC_CreateMutex(&(ICCGlobal.mtx))) {
      ICCGlobal.mutexInit = 1;
    } else if (NULL != status) {
      SetStatusPrivate(status, ICC_OS_ERROR, ICC_NOT_ENOUGH_MEMORY,
                       (char *)"Failed to create Mutex, out of handles ?");
      return NULL;
    }
  }
  if (status == NULL) {
    return NULL;
  }
  /* status may be filled with garbage when we get it first */
  SetStatusPrivateDef(status);
  if (ICC_OK != status->majRC) {
    return NULL;
  }
  if(NULL == iccpath) {
    iccpath = "";
  }
  x = (int)wcslen(iccpath);

  if (x >= MAX_PATH) {
    status->mode = -1;
    SetStatusPrivate(status, ICC_ERROR, ICC_INVALID_PARAMETER,
                     (char *)"Parameter is too large");
    return NULL;
  }
  path = ICC_Calloc(MAX_PATH, sizeof(wchar_t), __FILE__, __LINE__);
  wpath = ICC_Calloc(MAX_PATH, sizeof(wchar_t), __FILE__, __LINE__);
  pcb = (ICC_CTX *)ICC_Calloc(1, sizeof(ICC_CTX), __FILE__, __LINE__);
  if ((NULL == path) || (NULL == pcb) || (NULL == wpath)) {
    SetStatusPrivate(status, ICC_OS_ERROR, ICC_NOT_ENOUGH_MEMORY,
                     (char *)"Malloc failed");
    return NULL;
  }

  if (NULL == ICCGlobal.hICCLib) {
    paths = parse_pathW((wchar_t *)iccpath);

    for (i = 0; paths != NULL && paths[i] != NULL; i++) {
      /*memset the context to 0's as a default */
      memset(pcb, 0, sizeof(ICC_CTX));
      x = (int)wcslen(paths[i]);
      if (x > (MAX_PATH - (int)strlen(ICC_LIB_LOC) - 1))
        continue;

      wcsncpy(path, paths[i],MAX_PATH);
      MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, ICC_LIB_LOC, -1, wpath,
                          ICC_VALUESIZE - 1);

      wcsncat(path, wpath,MAX_PATH);
      path[MAX_PATH-1] = L'\0';
      ICCGlobal.hICCLib = ICC_LoadLibraryW(path);
      if (NULL == ICCGlobal.hICCLib) {
        /*Could not load DLL */
        ICC_GetLibraryError(status->desc, sizeof(status->desc));
        status->majRC = ICC_OS_ERROR;
        status->minRC = errno;
      } else {
        /* We got a shared library handle, but is it OUR shared library ??? */
        if (ICC_FAILURE == ICC_initialize_functions(pcb, status, paths[i], 1)) {
          /* Nope, so throw this one away ... */
          ICC_FreeLibrary(ICCGlobal.hICCLib);
          ICCGlobal.hICCLib = NULL;
          if (ICC_OK != status->majRC) {
            memcpy(&(ICCGlobal.status), status, sizeof(ICC_STATUS));
          }
        } else {
          /* Need to reset the status to OK if we find a match */
          SetStatusPrivateOK(status);
          ICCGlobal.initialized = 1;
          break; /* Found a library that passes the signature checks */
        }
      }
    }

    /* Clean up the search list if there is one */
    if (paths) {
      parse_path_cleanup((char **)paths);
    }
  } else if (ICCGlobal.initialized) {
    /* wchar flag is cleared as we aren't resetting the path */
    ICC_initialize_functions(pcb, status, NULL, 0);
    if (NULL == pcb->funcs) {
      ICC_Free(pcb);
      pcb = NULL;
    }
  } else { /* It's dead Jim */
    if (ICC_OK != ICCGlobal.status.majRC) {
      memcpy(status, &(ICCGlobal.status), sizeof(ICC_STATUS));
    }
    ICC_Free(pcb);
    pcb = NULL;
  }

  if (NULL != path) {
    ICC_Free(path);
  }
  if (NULL != wpath) {
    ICC_Free(wpath);
  }
  if (ICC_OK != status->majRC) {
    ICC_Free(pcb);
    pcb = NULL;
  }
  if (NULL != pcb) {
    ICC_LockMutex(&(ICCGlobal.mtx));
    ICCGlobal.refCount++;
    ICC_UnlockMutex(&(ICCGlobal.mtx));
  }
  return pcb;
}
#endif

ICC_CTX *ICC_Init(ICC_STATUS *status, const char *iccpath) {
  int i = 0, x = 0;
  char *path = NULL;
  char **paths = NULL;
  ICC_CTX *pcb = NULL;

  /*
   * This is here only to keep agressive linkers from optimizing out our
   *    copyright/SCCS info.
   */
  const char *bogusVariable = ICC_SCCSInfo;

  if (!ICCGlobal.mutexInit) { /* Race, but unavoidable */
    if (0 == ICC_CreateMutex(&(ICCGlobal.mtx))) {
      ICCGlobal.mutexInit = 1;
    } else if (NULL != status) {
#if defined(__WIN32)
      SetStatusPrivate(status, ICC_OS_ERROR, ICC_NOT_ENOUGH_MEMORY,
                       (char *)"Failed to create Mutex, out of handles ?");
#else
      SetStatusPrivate(status, ICC_OS_ERROR, ICC_NOT_ENOUGH_MEMORY,
                       (char *)"Failed to create Mutex");
#endif
      return NULL;
    }
  }

  icc_tmp = bogusVariable[0] | bogusVariable[1];
  icc_tmp |= ICC_GetProcessId();
  icc_tmp |= ICC_GetThreadId();

  if (status == NULL) {
    return NULL;
  }

  /* status may be filled with garbage when we get it first */
  SetStatusPrivateDef(status);
  if (ICC_OK != status->majRC) {
    return NULL;
  }
  if(NULL == iccpath) {
    iccpath = "";
  }
  x = (int)strlen(iccpath);

  if (x >= MAX_PATH) {
    status->mode = -1;
    SetStatusPrivate(status, ICC_ERROR, ICC_INVALID_PARAMETER,
                     (char *)"Parameter is too large");
    return NULL;
  }
  path = ICC_Calloc(MAX_PATH, sizeof(char), __FILE__, __LINE__);
  pcb = (ICC_CTX *)ICC_Calloc(1, sizeof(ICC_CTX), __FILE__, __LINE__);
  if ((NULL == path) || (NULL == pcb)) {
    SetStatusPrivate(status, ICC_OS_ERROR, ICC_NOT_ENOUGH_MEMORY,
                     (char *)"Malloc failed");
    return NULL;
  }
  /* No sucessful initializations so far ? */
  if (NULL == ICCGlobal.hICCLib) {
    /* Fallback code, some ICC consumers move ICC as part of
       installing copies of the software in non-default locations.
       For this to work, LD_LIBRARY_PATH or the equivalent must be set
       so we traverse this to build a fallback search path for the ICC
       shared components
    */
    paths = parse_path((char *)iccpath);

    for (i = 0; paths != NULL && paths[i] != NULL; i++) {
      /*memset the context to 0's as a default */
      memset(pcb, 0, sizeof(ICC_CTX));
      x = (int)strlen(paths[i]);
      if (x > (MAX_PATH - (int)strlen(ICC_LIB_LOC) - 1))
        continue;

      strncpy(path, paths[i],MAX_PATH-1);
      strncat(path, ICC_LIB_LOC,MAX_PATH-1);
      path[MAX_PATH-1] = 0;
      ICCGlobal.hICCLib = ICC_LoadLibrary(path);
      if (NULL == ICCGlobal.hICCLib) {
        /*Could not load DLL */
        ICC_GetLibraryError(status->desc, sizeof(status->desc));
        status->majRC = ICC_OS_ERROR;
        status->minRC = errno;
      } else {
        /* We got a shared library handle, but is it OUR shared library ??? */
        if (ICC_FAILURE == ICC_initialize_functions(pcb, status, paths[i], 0)) {
          /* Nope, so throw this one away ... */
          ICC_FreeLibrary(ICCGlobal.hICCLib);
          ICCGlobal.hICCLib = NULL;
          if (ICC_OK != status->majRC) {
            memcpy(&(ICCGlobal.status), status, sizeof(ICC_STATUS));
          }
        } else {
          /* Need to reset the status to OK if we find a match */
          SetStatusPrivateOK(status);
          ICCGlobal.initialized = 1;
          break; /* Found a library that passes the signature checks */
        }
      }
    }
    /* Clean up the search list if there is one */
    if (paths) {
      parse_path_cleanup(paths);
    }
  } else if (ICCGlobal.initialized) {
    ICC_initialize_functions(pcb, status, NULL, 0);
    if (NULL == pcb->funcs) {
      ICC_Free(pcb);
      pcb = NULL;
    }
  } else { /* It's dead Jim */
    if (ICC_OK != ICCGlobal.status.majRC) {
      memcpy(status, &(ICCGlobal.status), sizeof(ICC_STATUS));
    }
    ICC_Free(pcb);
    pcb = NULL;
  }

  if (NULL != path) {
    ICC_Free(path);
  }
  if (ICC_OK != status->majRC) {
    ICC_Free(pcb);
    pcb = NULL;
  }
  if (NULL != pcb) {
    ICC_LockMutex(&(ICCGlobal.mtx));
    ICCGlobal.refCount++;
    ICC_UnlockMutex(&(ICCGlobal.mtx));
  }
  return pcb;
}
typedef int (*fptr_Attach)(void *pcb,ICC_STATUS* status);
/*!
 *  @brief Attach to the crypto. libraries. Called after ICC_Init.
 *  @param pcb ICC context pointer returned by a sucessful call to ICC_Init
 *  @param status pointer to a pre-allocated ICC_STATUS variable in which status will be returned
 *  @return
 *  ICC_FAILURE if a FIPS mode error occured, 1 O.K. 0 Fail (library validation/Algorithm Self test fails)
 *  -1 if an error occurred which prevented this from suceeding i.e. failed memory allocation etc
 *  @note This call should return ICC style status, but for historical reasons
 *  i.e. most of the failures came up from OpenSSL it returns OpenSSL style status.
 *  It's too late to fix this now.
 *  We recommend checking the major return code in ICC_STATUS instead. See \ref ICC_STATUS_t
 *  for the basic errors, and status->mode \ref ICC_FLAGS_ENUM for FIPS related status
 *
 * <b>Indirect call to:</b> \ref lib_attach()
 *  @see ICC_RC_ENUM
 */
int ICC_LINKAGE ICC_Attach(ICC_CTX *pcb, ICC_STATUS *status) {
  int temp = (int)ICC_FAILURE;
  fptr_lib_attach tmpf = NULL;
  if (NULL != pcb) {
    if (NULL != pcb->funcs) {
      tmpf = (fptr_lib_attach)(*(pcb->funcs))[indexOf_lib_attach].func;
      if (NULL != tmpf) {
        temp = (tmpf)((void *)pcb->funcs, status);
        switch (temp) {
        case ICC_OK:
        case ICC_WARNING:
          break;
        default: /* Attach failed for some reason, make it sticky */
          SetDefaultStatus(status);
          break;
        }
      } else {
        SetStatusPrivate(status, ICC_ERROR, ICC_NULL_PARAMETER,
                         "ICC_Attach called with unititialized ICC context");
      }
    } else {
      SetStatusPrivate(status, ICC_ERROR, ICC_NULL_PARAMETER,
                       "ICC_Attach called with NULL ICC context");
    }
  }
  return temp;
}
/*!
  @brief
  Cleanup (destroy) and ICC context, the ICC context is not usable after this
  call.
  @param pcb ICC context to cleanup
  @param status erorr return
  @return ICC_OK or ICC_FAILURE
*/
int ICC_Cleanup (ICC_CTX * pcb, ICC_STATUS * status)
{
  int retcode;

#ifdef DEBUG_VERBOSE
  printf ("ICC_Cleanup()\n");
#endif

  if (status == NULL)
    return ICC_FAILURE;
  SetStatusPrivateOK (status);
  if (pcb == NULL) {
    status->mode = -1;
    SetStatusPrivate (status, ICC_ERROR, ICC_NOT_INITIALIZED,
		      (char *)"ICC has not been initialized");
    return ICC_ERROR;
  }
  if (ICCGlobal.hICCLib != NULL) {
    /*Free the cryptoDLL, note that the deref of pcb happens in this call */
    retcode = ICC_lib_cleanup (pcb, status);
    if (retcode != ICC_OSSL_SUCCESS) {
#ifdef DEBUG_VERBOSE
      printf ("Error in freeing crypto library\n");
#endif
    }
  }
  memset(pcb,0,sizeof(ICC_CTX));

  if(NULL != pcb) {
    ICC_LockMutex(&(ICCGlobal.mtx));
    ICCGlobal.refCount--;
    ICC_UnlockMutex(&(ICCGlobal.mtx));
  }
  ICC_Free (pcb);
  ICC_LockMutex(&(ICCGlobal.mtx));
  if((0 >= ICCGlobal.refCount) && (NULL != ICCGlobal.hICCLib) ) {
    ICC_FreeLibrary(ICCGlobal.hICCLib);
    ICCGlobal.hICCLib = NULL;
    ICCGlobal.initialized = 0;
    ICCGlobal.refCount = 0; 
    ICC_UnlockMutex(&(ICCGlobal.mtx));
    /* Race or memory leak, pick your poison 
       to avoid this keep a context alive for the life of the process
    */
    ICC_DestroyMutex(&(ICCGlobal.mtx));
    ICCGlobal.mutexInit = 0;
  } else {
    ICC_UnlockMutex(&(ICCGlobal.mtx));   
  }
  if (status->majRC != ICC_OK) {
    return ICC_FAILURE;
  }
  return ICC_OSSL_SUCCESS;
}

/* 
   Modifications to make ICC more friendly to scraped applications, 
   i.e. apps that were installed and have had the components copied and 
   placed in a tar file for installation in non-default directories.
*/
/*! 
  \EnvVar PATH
  - parse_pathW() is Windows/Unicode only
  - Usage: export PATH=< path to application libraries>
  - Platforms: Windows (PATH) only in Unicode mode (via ICC_InitW())
  - Uses: Fallback to searching relative to system paths for our
  shared library components if they cannot be located from the path 
  passed in. 
  - Why ?: ICC can be deeply embedded in ICC consumers, i.e.
  within code that's a relocatable component of another
  application - so the component that directly included ICC
  may have it's install paths changed by a higher level app.
  - The actual path used can be determined by the application
  via ICC_GetValue(...,ICC_INSTALL_PATH...)
  -
  - FIPS: Allowed in FIPS mode
  - Reason: As we signature check the libraries this is "safe" at the 
  ICC/FIPS level.
*/

#if defined(_WIN32)
/*!
 * @brief returns an array of paths to search, with an option user-supplied
 * path as the first option
 * @param path user supplied path, may be NULL
 * @return a pointer to an array of path components, caller free's
 * see parse_path_cleanup()
 */
static wchar_t **parse_pathW (wchar_t * path)
{
  wchar_t **paths = NULL;
  wchar_t *env = NULL;
  wchar_t *myenv = NULL;
  wchar_t *tmp = NULL, *ptr = NULL;
  int nelem = 0;
  int i = 0;
   

  /* First find out how many paths we have to search */
  if (path != NULL) {
    /* One at the front, and a copy at the end - least surprise 
       That way if they all fail the app. gets the path/error they
       expect.
    */
    nelem+=2;
  }

  env = _wgetenv (SYSLIBPATH_W);
  ptr = env;

  while ((ptr != NULL) && (*ptr != L'\0')) {
    ptr = wcschr (ptr, LIB_PATH_SPLIT_W);
    if (ptr != NULL) {
      ptr++;
    }
    nelem += 2;               /* PATH and PATH/../ (effectively) */
  }
  nelem++;  /* Plus a NULL string at the end */
  /* Allocate the array */
  paths = (wchar_t **)ICC_Calloc (nelem, sizeof (wchar_t *),__FILE__,__LINE__);
  i = 0;
  if (path != NULL) {
    paths[i++] = mywcsdup (path);
  }

  if (env != NULL) {
    myenv = mywcsdup (env);
    ptr = myenv;
    while ((ptr != NULL) && (*ptr != L'\0')) {
      tmp = wcschr (ptr, LIB_PATH_SPLIT_W);
      if (tmp != NULL) {
	      *tmp++ = L'\0';
      }
      if(wcslen(ptr) > 1) {
	      paths[i++] = mywcsdup (ptr);
      }
      ptr = tmp;
      /* 
	      See if there's a path split field in here, if 
	      so also try down one level from the LD_LIBRARY_PATH
	      components 
      */
      if((i > 0) && (NULL != paths[i-1])) {
        tmp = wcsrchr (paths[i - 1], PATH_SPLIT_W);
        if (tmp != NULL) {
	        /* There was so copy it ... */
	        paths[i] = mywcsdup (paths[i - 1]);
	        /* Find the path separator again */
	        tmp = wcsrchr (paths[i], PATH_SPLIT_W);
	        /* And truncate the path at that point */
	        if(NULL != tmp) { 
            *tmp = L'\0';
          }
	        i++;
        }	 
      }
    }
    /* 
       This tries the app supplied component again, if we fall through
       everything this leaves the application with the expected path/error
       not a path they know nothing about
    */
    if(path != NULL) {
      paths[i++] = mywcsdup(path);
    }
    ICC_Free (myenv);
    myenv = NULL; /* make debug easier */
  }

  return paths;
}

#endif




/*!
 * @brief returns an array of paths to search, with an option user-supplied
 * path as the first option
 * @param path user supplied path, may be NULL
 * @return a pointer to an array of path components, caller free's
 * see parse_path_cleanup()
 */

static char **parse_path (char *path)
{
  char **paths = NULL;
  char *env = NULL;
  char *myenv = NULL;
  char *tmp = NULL, *ptr = NULL;
  int nelem = 0;
  int i = 0;

  /* First find out how many paths we have to search */
  if (path != NULL) {
    /* One at the front, and a copy at the end - least surprise 
       That way if they all fail the app. gets the path/error they
       expect.
    */
    nelem+=2;
  }
#ifdef OS400
  /* OS400 does not use LD_LIBRARY_PATH, but will add two hardcoded paths */
  nelem += 2;
#else
  /*! 
    \EnvVar PATH (Windows)
    \EnvVar LIBPATH (AIX)
    \EnvVar DYLD_LIBARRY_PATH (OS/X)
    \EnvVar LD_LIBRARY_PATH (Linux, Solaris, HP/UX, z/OS)
    - \ref parse_path() 
    - Usage: export PATH|LIBPATH|LD_LIBRARY_PATH|DYLD_LIBRARY_PATH=< path to application libraries>
    - Platforms: Windows (PATH) AIX (LIBPATH) OS/X ( DYLD_LIBARRY_PATH) Linux, Solaris , HP/UX , z/OS (LD_LIBRARY_PATH)
    - Uses: Fallback to searching relative to system paths for our
    shared library components if they cannot be located from the path 
    passed in. 
    - Why ?: ICC can be deeply embedded in ICC consumers, i.e.
    within code that's a relocatable component of another
    application - so the component that directly included ICC
    may have it's install paths changed by a higher level app.
    - The actual path used can be determined by the application
    via ICC_GetValue(...,ICC_INSTALL_PATH...)
    -
    - FIPS: Allowed in FIPS mode.
    - Reason: As we signature check the libraries this is "safe" at the 
    ICC/FIPS level.
  */
  env = getenv (SYSLIBPATH);
  ptr = env;

  while ((ptr != NULL) && (*ptr != '\0')) {
    ptr = strchr (ptr, LIB_PATH_SPLIT);
    if (ptr != NULL) {
      ptr++;
    }
    nelem += 2;		/* LD_LIBRARY_PATH and effectively LD_LIBRARY_PATH/../ */
  }
#endif
  nelem++; /* and a NULL string at the end */
  /* Allocate the array */
  paths = (char **)ICC_Calloc (nelem, sizeof (char *),__FILE__,__LINE__);
  i = 0;
  if (path != NULL) {
    paths[i++] = mystrdup (path);
  }
#ifdef OS400
  /* add the two hardcoded qsys paths */
  paths[i++] = mystrdup(ICC_LIB_PATH);
  paths[i++] = mystrdup(ICC_OSSL_PATH);
#else
  if (env != NULL) {
    myenv = mystrdup (env);
    ptr = myenv;
    while ((ptr != NULL) && (*ptr != '\0')) {
      tmp = strchr (ptr, LIB_PATH_SPLIT);
      if (tmp != NULL) {
	      *tmp++ = '\0';
      }
      if(strlen(ptr) > 1) {
	      paths[i++] = mystrdup (ptr);
      }
      ptr = tmp;
      /* 
	      See if there's a path split field in here, if 
	      so also try down one level from the LD_LIBRARY_PATH
	      components 
      */
      if((i > 0) && (NULL != paths[i-1])) {      
        tmp = strrchr (paths[i - 1], PATH_SPLIT);
        if (tmp != NULL) {
	        /* There was so copy it ... */
	        paths[i] = mystrdup (paths[i - 1]);
	        /* Find the path separator again */
          if(NULL != paths[i]) {
	          tmp = strrchr (paths[i], PATH_SPLIT);
          }
	        /* And truncate the path at that point */
          if(NULL != tmp) {
	          *tmp = '\0';
          }
          i++;
        }
      }
    }
    /* 
       This tries the app supplied component again, if we fall through
       everything this leaves the application with the expected path/error
       not a path they know nothing about
    */	
    if(path != NULL) {
      paths[i++] = mystrdup(path);
    }
    ICC_Free (myenv);
    myenv = NULL; /* make debug cleaner */
  }
#endif
  return paths;
}

/*! 
 * @brief free an array of path components
 * @param paths the array of paths to cleanup
 */
static void parse_path_cleanup (char **paths)
{
  int i = 0;

  if (paths != NULL) {
    for (i = 0; paths[i] != NULL; i++) {
      ICC_Free (paths[i]);
      paths[i] = NULL; /* Just makes debug easier */
    }
    ICC_Free (paths);
    paths = NULL;  /* debug */
  }
}



/*!
 *  @brief Set configuration data
 *  @param pcb ICC context pointer returned by a sucessful call to ICC_Init
 *  @param status pointer to a pre-allocated ICC_STATUS variable in which status will be returned
 *  @param valueID ID of parameter to set
 *  @param value pointer to configuration value data
 *  @return ICC_OK, ICC_WARNING, ICC_ERROR or ICC_FAILURE.
 */
int ICC_SetValue(ICC_CTX *pcb,ICC_STATUS* status,ICC_VALUE_IDS_ENUM valueID,const void* value)
{
  int temp =  (int)ICC_OK;
  fptr_SetValue tempf = NULL;

  if(NULL == status) return ICC_ERROR;
  SetStatusPrivateOK(status);
  /* Check to see if ICC_Attach() has been called sucessfully yet - i.e.
     has the function table been set up ?
  */
  if ((NULL != pcb) && (NULL != pcb->funcs) ) {
    tempf = (fptr_SetValue)(*pcb->funcs)[indexOf_SetValue].func;
    if(NULL != tempf) {
      temp = (tempf)((void*)pcb->funcs,status,valueID,value);
    }
  } else {
    SetStatusPrivate(status,ICC_ERROR,ICC_UNABLE_TO_SET,"Attempted to set an initialization value when ICC was in an uninitialized state");
    temp = ICC_ERROR;
  }

  return temp;
}



/*!
  @brief Library initialization.
  With the 2014 update most of the initialization is done internally by the single crypto. library
  @param pcb Unititialized ICC Context
  @param status pointer to an ICC_STATUS
  @param path now unused
  @param iswchar A flag to indicate initialization via a wide character path
  @note What is actually returned is a nominally opaque data structure with
  the list of names/ptrs to functions at the head of it.
*/


static int ICC_initialize_functions(ICC_CTX *pcb,ICC_STATUS *status, char *path,int iswchar)
{
  int rv = ICC_OSSL_SUCCESS;
  char *libname = "C_lib_init";
  fptr_lib_init efunc = NULL;

#if (NON_FIPS_ICC & 1)
  libname = "N_lib_init";
#endif    

  if(NULL != pcb && NULL != ICCGlobal.hICCLib) {
    efunc = ICC_GetProcAddress(ICCGlobal.hICCLib,libname);
    if(NULL == efunc) {
      SetStatusPrivateLn2 (status, ICC_ERROR, ICC_LIBRARY_NOT_FOUND,"Symbol not found in library",libname);
    } else {
      /* Indicate wchar_t path by passing "W" through in the now unused
	       icclibhash param */
      pcb->funcs = (FUNC **) (efunc)(NULL,status,path,iswchar ? "W":NULL,NULL);
    }  
    if(NULL == pcb->funcs) {
      rv = ICC_FAILURE;
      if(ICC_OK == status->majRC) {
	      SetStatusPrivateLn2 (status, ICC_ERROR, ICC_LIBRARY_NOT_FOUND,"ICC shared library not initialized",libname);
      }
    }
  }

  return rv;
}

