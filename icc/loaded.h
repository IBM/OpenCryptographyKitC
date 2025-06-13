/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: Manually created source for the ICCPKG wrapper for GSkit
// 
*************************************************************************/

#if !defined(LOADED_H)

#define LOADED_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#if defined(__ARMEL__) || defined(__ARMEB__)
#  include <linux/limits.h>
#endif

#define PATH_DELIMITER '/'

#if defined(_WIN32)

#  include <share.h>
#  include <windows.h>


#  undef  PATH_DELIMITER
#  define PATH_DELIMITER '\\'
#  define PATH_DELIMITER_W L'\\'

#  define strcasecmp(a,b) _stricmp(a,b)

#elif defined(_AIX)
#  include <errno.h>
#  include <sys/ldr.h>
#  include <sys/utsname.h>
#  define MAX_PATH PATH_MAX
#elif defined(__MVS__) || defined(AS400)
# define MAX_PATH 256
#else
#if !defined(PATH_MAX)
# define PATH_MAX 1024
#endif
# define MAX_PATH PATH_MAX
#endif

#include <errno.h>

#if defined(__linux)
#  ifndef __USE_GNU
#    define __USE_GNU
#  endif
#endif

#if !defined(_WIN32)
#include <dlfcn.h>
#endif
#include "../icc/iccversion.h"


#define MAKE_FN_NAME1(x,y) x ## y
#define MAKE_FN_NAME2(x,y) MAKE_FN_NAME1(x,y)
#define MAKE_FN_NAME(x,y) MAKE_FN_NAME2(MAKE_FN_NAME2(MAKE_FN_NAME2(MAKE_FN_NAME2(MAKE_FN_NAME2(MAKE_FN_NAME2(x,y),ICC_VERSION_VER),_),ICC_VERSION_REL),_),ICC_VERSION_MOD)
#define FUNCTION_NAME(x,y) MAKE_FN_NAME(x,y)

int FUNCTION_NAME(MYNAME,_path)(char *returned_path,int path_len);

static char *FUNCTION_NAME(MYNAME,_loaded_from)();

#if defined(_WIN32)
int FUNCTION_NAME(MYNAME,_pathW)(wchar_t *returned_path,int path_len);

static wchar_t *FUNCTION_NAME(MYNAME,_loaded_fromW)();
#endif /* _WIN32 */

#endif /* LOADED_H */
