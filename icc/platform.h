/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: Provides a layer of abstraction/indirection for platform
//              specific code.
//
*************************************************************************/

#ifndef INCLUDED_PLATFORM
#define INCLUDED_PLATFORM

#ifdef  __cplusplus
extern "C" {
#endif


#include <string.h>
  /* This gets defined as "static" by icc.c before it includes this
     header and platform.c 
     The C file include is ugly, but it's the only way to avoid exposing
     private function within a static library.
  */
#if !defined ICCSTATIC 
#define ICCSTATIC
#endif

#if !defined VTAG
#pragma message("VTAG not defined")
#define VTAG 0
#endif

  /* 
     multiple component path separators and environment variables
     used to determine the fallback path
  */
#if defined(_WIN32)
#define PATH_SPLIT '\\'
#define LIB_PATH_SPLIT ';'
#define SYSLIBPATH "PATH"
#define PATH_SPLIT_W L'\\'
#define LIB_PATH_SPLIT_W L';'
#define SYSLIBPATH_W L"PATH"
#else  /* Unix-like OS's */
#if defined(_AIX)
#define SYSLIBPATH "LIBPATH"
#elif defined(__APPLE__)
#define SYSLIBPATH "DYLD_LIBRARY_PATH"
#else
#define SYSLIBPATH "LD_LIBRARY_PATH"
#endif
#define PATH_SPLIT '/'
#define LIB_PATH_SPLIT ':'
#endif 


/* Picked up from a Makefile define */
#define VT MAKESTRING(VTAG)

#if defined(_WIN32)

  /* # define WIN32_LEAN_AND_MEAN */
# include <windows.h>

#  define strcasecmp(a,b) _stricmp(a,b)

typedef HANDLE ICC_Mutex;

  /*! \def Windows: location of ICC dll within the ICC directory */
#define ICC_LIB_NAME "icclib"VT".dll"
#define ICC_LIB_LOC "/icc/icclib/icclib"VT".dll"
  /*! \def Windows: location of OpenSSL dll within the ICC directory */
#define ICC_OSSL_LOC "/icc/osslib/libeay32IBM"VT".dll"

#define ICC_LIB_LOC_W L"/icc/icclib/icclib"L VT L".dll"
#define ICC_OSSL_LOC_W L"/icc/osslib/libeay32IBM"L VT L".dll"


#elif defined(__linux) || defined(_AIX) || defined(__sun) || defined(__hpux) 


# include <unistd.h>
# include <pthread.h>
# include <sys/types.h>
# include <sys/time.h> /* gettimeofday */
# include <limits.h> /* PATH_MAX */
# include <stdlib.h>
# include <fcntl.h>


# if defined(__linux)
  typedef u_int32_t DWORD;
# else
  typedef uint32_t DWORD;
# endif

  typedef pthread_mutex_t ICC_Mutex;


# if defined(__hpux)
#  include <dl.h>
#  include <errno.h>
# endif   /* __hpux */

#  include <dlfcn.h>

  /* location of DLLs within the ICC directory */

# if defined(__hpux)
#   if defined(__ia64) 
#     define  ICC_LIB_NAME  "libicclib"VT".so"
#     define ICC_OSSL_NAME  "libcryptoIBM"VT".so.1.0.1"
#   else
#     define  ICC_LIB_NAME  "libicclib"VT".sl"
#     define ICC_OSSL_NAME  "libcryptoIBM"VT".sl.1.0.1"
#   endif 
# else
#  define  ICC_LIB_NAME  "libicclib"VT".so"
#  if defined(__linux) || defined(__sun) || defined(_AIX)
#   define ICC_OSSL_NAME "libcryptoIBM"VT".so.1.0.1"
#  else
#   error Please provide openssl shared library name here.
#  endif  /* __linux || __sun */
# endif   /* __hpux  */

# define ICC_LIB_LOC  "/icc/icclib/" ICC_LIB_NAME
# define ICC_OSSL_LOC "/icc/osslib/" ICC_OSSL_NAME


#elif defined(__APPLE__)
  /* A bit lax, but we don't plan to support anything prior to OS X */

# include <unistd.h>
# include <pthread.h>
# include <sys/types.h>
# include <sys/time.h> /* gettimeofday */
# include <dlfcn.h>
# include <stdlib.h>
# include <fcntl.h>
# define ICC_LIB_NAME  "libicclib"VT".dylib"
# define ICC_OSSL_NAME  "libcryptoIBM"VT".1.0.1.dylib"

# define ICC_LIB_LOC  "/icc/icclib/" ICC_LIB_NAME
# define ICC_OSSL_LOC "/icc/osslib/" ICC_OSSL_NAME

  typedef pthread_mutex_t ICC_Mutex;

  typedef u_int32_t DWORD;

#elif defined(OS400)

#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/time.h>        /* gettimeofday */
#include <stdlib.h>	     /* malloc				     */
#include <unistd.h>	     /* readlink			     */
#include <ctype.h>           /* toupper()                            */
#include <except.h>          /* Direct monitors                      */
#include <qleawi.h>	     /* QleActBndPgm, QleGetExp              */
#include <miptrnam.h>	     /* rslvsp				     */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <iconv.h>

#include <fcntl.h>

  typedef unsigned long DWORD;
  typedef pthread_mutex_t ICC_Mutex;

/* 
   OS400 DLLs cannot have the VT version tag in their names, because they are limited to 10 characters.
   Instead, these are in version specific libraries (ICC_LIB_PATH and ICC_OSSL_LOC).
   We only needed the version tags because some OS variants won't load
   two shared libs of the same name even if they are on different paths
*/

#define ICC_LIB_NAME	"libicclib.srvpgm"
#define ICC_ICC400_NAME	"icc400.pgm"
#define ICC_OSSL_NAME	"libcrypto.srvpgm"

/* ICC_LIB_PATH and ICC_OSSL_PATH are the default qsys paths of the libraries (and may be the same)
   ICC_HOSTBIN and OSSL_HOSTBIN are defined on the compile command line */

# define ICC_LIB_PATH  "/qsys.lib/" ICC_HOSTBIN ".lib"
# define ICC_OSSL_PATH "/qsys.lib/" OSSL_HOSTBIN ".lib"

/* The *_LIB_LOC defines are the srvpgm names with a prepended '/', since OS400 qsys file system
   where srvpgms must go is non-hierarchical and has no icc/icclib or icc/osslib in the path */

#define ICC_LIB_LOC	"/" ICC_LIB_NAME
#define ICC_ICC400_LOC	"/" ICC_ICC400_NAME
#define ICC_OSSL_LOC	"/" ICC_OSSL_NAME

#elif defined(__MVS__)


#include <pthread.h>
typedef pthread_mutex_t ICC_Mutex;

#include <dlfcn.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>        /* gettimeofday */
#include <stdlib.h>          /* malloc                               */
#include <ctype.h>           /* toupper()                            */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <iconv.h>
#include <fcntl.h>

#define ICC_LIB_NAME  "libicclib"VT".so"
#define ICC_OSSL_NAME  "libcryptoIBM.1.0.1.dll"
#define ICC_LIB_LOC  "/icc/icclib/libicclib"VT".so"
#define ICC_OSSL_LOC "/icc/osslib/libcryptoIBM.1.0.1.dll"

  typedef unsigned long DWORD;

#else

# error Please provide platform specific code here.

#endif

/* Maximum path allowed by the OS */
#if defined(__MVS__) || defined(AS400)
# define MAX_PATH 256
#else
#if !defined(PATH_MAX)
# define PATH_MAX 1024
#endif
/* on win32/64 system headers define MAX_PATH so dont change it */
#if !defined(_WIN32)
#define MAX_PATH PATH_MAX
#endif
#endif

# if defined(__sun) || defined(__hpux)
# define iccInline
# else
# define iccInline __inline
#endif



  /*! @brief prototype for a pointer to OpenSSL style malloc() 
    The last two parameters are expected to be __FILE__, __LINE__
  */
  typedef void *  (*MallocFunc) (size_t, const char *,int);

  /*! @brief prototype for a pointer to realloc() used by OpenSSL*/
  typedef void *  (*ReallocFunc) (void *, size_t, const char *, int);

  /*! @brief prototype for a pointer to free() used by OpenSSL */
  typedef void  (*FreeFunc) (void *);




#if !defined(ICC_Malloc)

  /* 
     The allocators used by ICC, note this gets a bit messy
     because we need allocators working in two disconnected
     bits of code at some points. 
     i.e. in the ICC static stub, and in the ICC shared
     library.
  */
  /*!
    @brief ICC's malloc, wraps around the callback function
    @param sz the size of the allocation required
    @param file __FILE__
    @param line __LINE__
    @return NULL or a pointer to the allocated region
  */
  ICCSTATIC  void  *ICC_Malloc(size_t sz, const char *file, int line);

  /*!
    @brief ICC's calloc, wraps around the callback function
    @param n the number of blocks of sz requested
    @param sz the size of the allocation required
    @param file __FILE__
    @param line __LINE__
    @return NULL or a pointer to the allocated and zero'd region
  */

  ICCSTATIC void  *ICC_Calloc(size_t n, size_t sz, const char *file, int line);

#if defined(USE_REALLOC)
  /*!
    @brief ICC's realloc, wraps around the callback function
    @param ptr the old allocated block
    @param sz the size of the allocation required
    @param file __FILE__
    @param line __LINE__
    @return NULL or a pointer to the allocated and zero'd region
  */
  ICCSTATIC void *ICC_Realloc(void *ptr,size_t sz,const char *file, int line);
#endif
  /*!
    @brief ICC's free, wraps around the callback function
    @param ptr the old allocated block
  */

  ICCSTATIC void  ICC_Free(void *ptr);
#endif

#ifdef  __cplusplus
};
#endif
#endif  /* INCLUDED_PLATFORM */
