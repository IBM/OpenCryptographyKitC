/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*
// Description: Manually created source for the ICCPKG wrapper for GSkit
*/

#include "icc.h" /* Only so trace source-of tags work */
#include "loaded.h"
#include "tracer.h"


#if !defined(LIBNAME)
#define LIBNAME ICCDLL_NAME
#endif

#if defined(__hpux)
#  if !defined(INSTDIR)
#    if defined(_ILP32)
#      define INSTDIR  "/opt/ibm/gsk8/lib"
#    else
#      define INSTDIR "/opt/ibm/gsk8_64/lib64"
#    endif
#  endif
#endif
#if defined(__MVS__)
#  include <iconv.h>
#endif


/*!
  @brief
  Return the best guess we have as to where ICC libraries
  are located
  @param returned_path The best guess
  @param path_len The maximum allowed path (sizeof(returned_len) -1)
  @return The path length, -1 if invalid input, or 0 on fail. 
*/
int FUNCTION_NAME(MYNAME,_path)(char *returned_path,int path_len)
{

  char  *dirName = NULL;   /*this library's directory name */
  char *tmpPtr = NULL;
  char  *runtimeName;
  char  *ptr; 
  int rv = 0;
  /*
   * verify args
   */
  IN();
  /* First - make sure returned_path[] is crash-proof if we can 
   */
  if( NULL != returned_path && path_len > 0) {
    returned_path[0] = '\0';
  }
  if( (returned_path == NULL) || (path_len < 0)) {
    return -1;
  }
  dirName = (char *)calloc(MAX_PATH,1);
  if(NULL != dirName) {
    runtimeName = FUNCTION_NAME(MYNAME,_loaded_from)();
    MARK("runtimeName",(NULL != runtimeName) ? runtimeName:"NULL");
    if( runtimeName != NULL) {       
      strncpy(dirName, runtimeName,MAX_PATH);
      dirName[MAX_PATH-1] = 0;
      tmpPtr = dirName;
      free( runtimeName);
    } 
    if(tmpPtr != NULL) {
    /*
     * on an installed system tmpPtr now contains something like: ......./lib[64]/LIBRARY_NAME
     * during BVT  tmpPtr now contains something like: ......./osnamerelease/LIBRARY_NAME
     */
      ptr  = strrchr(tmpPtr, PATH_DELIMITER);
      if (ptr) {
        *ptr = '\0';       
        rv = sprintf(returned_path, "%.*s", path_len -1, tmpPtr);
      }
      /*
      * else no delimiters - fall through
      */
    }
    free(dirName);
  }
  MARK("path",(NULL != returned_path) ? returned_path:"NULL");
  OUTRC(rv);
  /*
   * else gskiccs8_loaded_from or realpath failed -fall through
   */
  return rv;
}
#if defined(_WIN32)


int FUNCTION_NAME(MYNAME,_pathW)(wchar_t *returned_path,int path_len)
{

  wchar_t *dirName = NULL;   //this library's directory name
  wchar_t *tmpPtr = NULL;
  wchar_t  *runtimeName;
  wchar_t  *ptr; 
  int rv = 0;
  /*
   * verify args
   */
  IN();
  if( (returned_path == NULL) || (path_len < 0)) {
    return -1;
  }
  dirName = (wchar_t *)calloc(MAX_PATH,sizeof(wchar_t));
  runtimeName = FUNCTION_NAME(MYNAME,_loaded_fromW)();
  if(NULL != dirName) {
    if( runtimeName != NULL) {       
      wcsncpy(dirName, runtimeName,MAX_PATH-1);
      tmpPtr = dirName;
      free( runtimeName);
    } 
    if(tmpPtr != NULL) {
      ptr  = wcsrchr(tmpPtr,L'\\');
      if (ptr) {
        *ptr = L'\0';    
        wcsncpy(returned_path,tmpPtr,path_len-1);
        rv = (int)wcslen(returned_path);
      }
      /*
      * else no delimiters - fall through
      */
    }
    free(dirName);
  }
  
  /*
   * else gskiccs8_loaded_from or realpath failed -fall through
   */
  OUTRC(rv);
  return rv;
}


/*!
 * @brief
 * Find the path to this shared library - Windows specific variant
 * NOTE: ALLOCATES MEMORY THAT THE CALLER MUST FREE!!!!
 * @note this requires an ICC_Init() entry point in this library to function
 */
static char *FUNCTION_NAME(MYNAME,_loaded_from)()
{       
  char  *dirName = NULL;  /*this library's initial directory name */                            
  char *result = NULL;    
  char *path = LIBNAME;
  HMODULE  libHandle;
  IN();
  dirName = (char *)calloc(MAX_PATH,1);
  libHandle = GetModuleHandleA(path);
  if (!libHandle) {
     libHandle = GetModuleHandleA(NULL);
  }
  if(NULL != dirName) {
    if (libHandle  &&
      GetModuleFileNameA(libHandle,dirName, MAX_PATH-1) < MAX_PATH) {
      MARK("dirName",dirName != NULL ? dirName : "NULL");
      result = (char *)calloc(strlen(dirName)+1,1);
      if (NULL != result) {
        strncpy(result, dirName,strlen(dirName));
      }
    }
    free(dirName);
  }
  MARK("path",(NULL != result) ? result:"NULL");
  OUT();
  return result;   
}

static wchar_t * FUNCTION_NAME(MYNAME,_loaded_fromW)()
{       
  wchar_t  *dirName = NULL;  /*this library's initial directory name */
  wchar_t *result = NULL;    
  wchar_t *path = NULL;
  HMODULE  libHandle;
  IN();
  path = (wchar_t *)calloc(MAX_PATH,sizeof(wchar_t));
  MultiByteToWideChar(CP_ACP,0,
                      LIBNAME,-1,
                      path,MAX_PATH-1);

  dirName = (wchar_t *)calloc(MAX_PATH,sizeof(wchar_t));
  libHandle = GetModuleHandleW(path);
  if(NULL != dirName) {
    if (libHandle  &&
      GetModuleFileNameW(libHandle,dirName, MAX_PATH-1) < MAX_PATH) {
    
      result = (wchar_t *)calloc(wcslen(dirName)+1,sizeof(wchar_t));
      if (NULL != result) {
        wcsncpy(result, dirName,wcslen(dirName));
      }
    }
    free(dirName);
  }
  if(NULL != path) {
    free(path);
  }
 
  OUT();
  return result;   
}


#elif defined(_AIX) /* End _WIN32 */

#define MAX_INFO 1024
#define MAX_INFO2 (MAX_INFO*10)

static char *gskiccs8_loaded_from_i(struct ld_info *dllinfo,int entries)
{
  char  *dirName = NULL;  /*this library's initial directory name 
			    (may be relative) */
  char  *fname = NULL;    /*this library's full path name */
  char *result = NULL;    
  int sts = 0;
  int foundit = 0; /* Set to 1 if we can actually locate a path */
  char *rprv = NULL;

  char *path = LIBNAME;
  struct ld_info *next = NULL;
  long k;
  int i, j;
  char tmp[20];
  IN();
  dirName = (char *)calloc(MAX_PATH,1);
  fname = (char *)calloc(MAX_PATH,1);
  if((NULL != dirName) && (NULL != fname)) {
    sts = loadquery(L_GETINFO,dllinfo,entries * sizeof(struct ld_info));
    snprintf(tmp,sizeof(tmp),"%d",sts);
    MARK("loadquery rv",tmp);
    if (sts >= 0) {
      next = dllinfo;
      i = 0;
      while ((next != NULL) && (next < &dllinfo[entries])) {
        i++;
        j = strlen(next->ldinfo_filename);
        if (strstr(next->ldinfo_filename, path)) {
          /* found it */
          strncpy(fname, next->ldinfo_filename, MAX_PATH - 1);
          MARK("foundit", fname);
          foundit = 1;
          rprv = realpath(fname, dirName);
          MARK("realpath rv", rprv != NULL ? rprv : "NULL");
          if (NULL != rprv) {
            MARK("realpath", dirName != NULL ? dirName : "NULL");
            break;
          }
        }
        k = (long)next;
        if (next->ldinfo_next) {
          k += next->ldinfo_next;
          next = (struct ld_info *)k;
        } else {
          next = NULL;
        }
      }
    }
  }
  if (foundit) {
    result = (char *)calloc(strlen(dirName)+1,1);
    if (result) {
      strncpy(result, dirName,strlen(dirName));
    }
  }
  if(NULL != dirName) {
    free(dirName);
  }
  if(NULL != fname) {
    free(fname);
  }
  MARK("path",(NULL != result) ? result:"NULL");
  OUT();
  return result;   
}

/*!
 * @brief
 * Find the path to this shared library on AIX
 * NOTE: ALLOCATES MEMORY THAT THE CALLER MUST FREE!!!!
 */
static char *FUNCTION_NAME(MYNAME,_loaded_from)()
{       
  char *result = NULL;
  static char msg_string[] = "Greater than 10k library objects loaded, recursive library load suspected";
  struct ld_info *dllinfo = NULL;  

  IN();
  dllinfo = calloc(MAX_INFO,sizeof(struct ld_info));
  if((NULL != dllinfo) && 
     (NULL == (result = gskiccs8_loaded_from_i(dllinfo,MAX_INFO)))) {
    free(dllinfo);
    dllinfo = calloc(MAX_INFO2,sizeof(struct ld_info));
    if((NULL != dllinfo ) && 
       NULL ==  (result = gskiccs8_loaded_from_i(dllinfo,MAX_INFO2))) {
      free(dllinfo);
      dllinfo = NULL;
      result = malloc(sizeof(msg_string));
      strncpy(result, msg_string, sizeof(msg_string));
    }
  }
  if(NULL != dllinfo) {
    free(dllinfo);
  }
  MARK("path",(NULL != result) ? result:"NULL");
  OUT();
  return result;
}
#elif defined(__MVS__)
#define BYTE unsigned char

typedef struct {
  BYTE ceeFDCB_GLUE[8]; /* Glue code for direct branches to function pointer */
  void *ceeFDCB_FuncAddr;    /* Pointer to function */
  BYTE *ceeFDCB_DLL_CWSA;    /* Pointer to exporting DLL's C_WSA */
  void *ceeFDCB_MoreGlue;    /* Address used by glue code (above) */
  void *ceeFDCB_DLLE;        /* Address of DLL entry point */
  void *ceeFDCB_CEESTARTPtr; /* Pointer to CEESTART */
  BYTE *ceeFDCB_CWSA;        /* Pointer to this porgram object's C_WSA */
} FDCB_t;
int LOADPATH(void *addr, char pathName[1026]);
/*!
 * @brief
 * Find the path to this shared library on generic Unix platforms
 * NOTE: ALLOCATES MEMORY THAT THE CALLER MUST FREE!!!!
 * @note this requires an entry point *ONLY* in this library to function
 * some implementations are broken and pick off the first function name
 * that matches which could be "Undefined" i.e. in a calling lib!
 * So - make it a static, and give it a unique name
 */
static char *FUNCTION_NAME(MYNAME, _loaded_from)() {
  char *dirName = NULL; /*this library's initial directory name
                          (may be relative) */
  char *fname = NULL;   /*this library's full path name */
  int sts = 0;
  char *result = NULL;
  int foundit = 0; /* Set to 1 if we can actually locate a path */

  FDCB_t *fdcb;
  char *loadPath_fname = NULL;
  int fnameLen;
  char *rprv = NULL;
  char tmp[20];
  int  rc;
  iconv_t iconvH;
  char *cpIn, *cpOut;
  size_t inBytesLeft, outBytesLeft;

  IN();
  dirName = (char *)calloc(MAX_PATH, 1);
  fname = (char *)calloc(MAX_PATH, 1);
  
  loadPath_fname =
      (char *)__malloc31(1026); /* first two bytes are length of string */
  if ((NULL != fname) && (NULL != dirName) && (NULL != loadPath_fname)) {

    /* HP/UX needs a public symbol, linux gets confused and
       resolves a reference as the symbol
    */

    fdcb = (FDCB_t *)FUNCTION_NAME(MYNAME, _loaded_from);
#if defined(__64BIT__)
#define funcAddr fdcb->ceeFDCB_FuncAddr
#else
#define funcAddr fdcb->ceeFDCB_DLLE
#endif
    snprintf(tmp, sizeof(tmp), "0x%0lx", funcAddr);
    MARK("LOADPATH(funcAddr, )", tmp);
    sts = LOADPATH(funcAddr, loadPath_fname);
    snprintf(tmp, sizeof(tmp), "%0lx", sts);
    MARK("LOADPATH rv", tmp);
    if (sts == 0) {
      fnameLen = *(short *)loadPath_fname;
      loadPath_fname[fnameLen + 2] = '\0';
      /* Code below assumes non-zero is success, rather than 0 */
      sts = 1;
    }

    if (NULL != getenv("ICC_DISABLE_AUTOPATH")) {
      MARK("ICC_DISABLE_AUTOPATH", getenv("ICC_DISABLE_AUTOPATH"));
      sts = 0;
    }

    if (sts) { /* != 0 is success in this case */

      inBytesLeft = *((short*)loadPath_fname);
#if defined (CHARSET_EBCDIC)
      MARK("ZOS","CHARSET_EBCDIC");
      strncpy(fname, loadPath_fname + 2, fnameLen);
#else
      /* convert to ASCII from EBCDIC */
      MARK("ZOS","ASCII");
      iconvH = iconv_open("ISO8859-1", "IBM1047");
      if (iconvH == (iconv_t)(-1)) {
        MARK("iconv_open failed","");
      } else {
        cpIn = loadPath_fname+2;
        cpOut = fname;
        outBytesLeft = MAX_PATH - 1;
        rc = iconv(iconvH, &cpIn, &inBytesLeft, &cpOut, &outBytesLeft);
        iconv_close(iconvH);
        if (rc < 0) {
          MARK("iconv failed","");
        }
      }
#endif
      MARK("foundit", fname);
      foundit = 1;
      rprv = realpath(fname, dirName);
      MARK("realpath rv", rprv != NULL ? rprv : "NULL");
      if (NULL == rprv) {
        *dirName = 0;
        sts = 0; /* Force us to use the fallback path */
      } else {
        /* sprintf(fname, "%s", dllinfo.dli_fname); */
        MARK("realpath", dirName != NULL ? dirName : "NULL");
      }
    }

    /* And if all else fails, use the global install dir */
    if (0 == sts) {
#if defined(ICCPKG)
      /* INSTDIR+"/"+GSK_LIBNAME + \0 */
      strncpy(fname, INSTDIR, MAX_PATH);
      strncat(fname, "/", MAX_PATH);
      strncat(fname, GSK_LIBNAME, MAX_PATH); /* Stripped off by the caller */
      fname[MAX_PATH - 1] = 0;
#else
#if defined(INSTDIR)
      /* INSTDIR+"/N/icc/icclib/"+ICCDLLNAME + \0 */
      strncpy(fname, INSTDIR,
              MAX_PATH - (strlen("/N/icc/icclib/") + strlen(ICCDLL_NAME) + 1));
#endif
#if defined(NON_FIPS_ICC)
      strncat(fname, "/N/icc/icclib/", MAX_PATH);
#else
      strncat(fname, "/C/icc/icclib/", MAX_PATH);
#endif
      strncat(fname, ICCDLL_NAME, MAX_PATH); /* Stripped off by the caller */
#endif
      fname[MAX_PATH - 1] = 0;
      MARK("fallback to install dir", fname);
      rprv = realpath(fname, dirName);
      MARK("realpath rv", rprv != NULL ? rprv : "NULL");
      if (NULL != rprv) {
        MARK("realpath", dirName != NULL ? dirName : "NULL");
        foundit = 1;
      }
    }
    if (foundit) {
      result = (char *)calloc(strlen(dirName) + 1, 1);
      if (result) {
        strncpy(result, dirName, strlen(dirName));
      }
    }
  }
  if (foundit) {
    result = (char *)calloc(strlen(dirName) + 1, 1);
    if (result) {
      strncpy(result, dirName, strlen(dirName));
    }
  }
  if (NULL != dirName) {
    free(dirName);
  }
  if (NULL != fname) {
    free(fname);
  }

  if (NULL != loadPath_fname) {
    free(loadPath_fname);
  }


  MARK("path", (NULL != result) ? result : "NULL");
  /* printf("_loaded_from() %s\n",result); */
  OUT();
  return result;
}

#else

/* End z/OS, start Generic UNIX */ 

/*!
 * @brief
 * Find the path to this shared library on generic Unix platforms
 * NOTE: ALLOCATES MEMORY THAT THE CALLER MUST FREE!!!!
 * @note this requires an entry point *ONLY* in this library to function
 * some implementations are broken and pick off the first function name
 * that matches which could be "Undefined" i.e. in a calling lib!
 * So - make it a static, and give it a unique name
 */
static char *FUNCTION_NAME(MYNAME, _loaded_from)() {
  char *dirName = NULL; /*this library's initial directory name
                          (may be relative) */
  char *fname = NULL;   /*this library's full path name */
  int sts = 0;
  char *result = NULL;
  int foundit = 0; /* Set to 1 if we can actually locate a path */
  Dl_info dllinfo;
  char *rprv = NULL;
  char tmp[20];
  IN();
  dirName = (char *)calloc(MAX_PATH, 1);
  fname = (char *)calloc(MAX_PATH, 1);
  /* HP/UX needs a public symbol, linux gets confused and
     resolves a reference as the symbol
  */
  if ((NULL != fname) && (NULL != dirName)) {
#if defined(__hpux)
    sts = dladdr((void *)FUNCTION_NAME(MYNAME, _path), &dllinfo);
    snprintf(tmp, sizeof(tmp), "%0lx", sts);
    MARK("dladdr 1 rv", tmp);
    /* Except where it doesn't work, and we need to drop back 10 and punt */
    if (NULL == sts) {
      sts = dladdr((void *)FUNCTION_NAME(MYNAME, _loaded_from), &dllinfo);
      snprintf(tmp, sizeof(tmp), "%0lx", sts);
      MARK("dladdr 2 rv", tmp);
    }
#else
    sts = dladdr((void *)FUNCTION_NAME(MYNAME, _loaded_from), &dllinfo);
    snprintf(tmp, sizeof(tmp), "%0x", sts);
    MARK("dladdr rv", tmp);
#endif /* __hpux */
    if (NULL != getenv("ICC_DISABLE_AUTOPATH")) {
      MARK("ICC_DISABLE_AUTOPATH", getenv("ICC_DISABLE_AUTOPATH"));
      sts = 0;
    }
    if (sts) { /* != 0 is success in this case */
      strncpy(fname, dllinfo.dli_fname, MAX_PATH - 1);
      MARK("foundit", fname);
      foundit = 1;
      rprv = realpath(fname, dirName);
      MARK("realpath rv", rprv != NULL ? rprv : "NULL");
      if (NULL == rprv) {
        *dirName = 0;
        sts = 0; /* Force us to use the fallback path */
      } else {
        /* sprintf(fname, "%s", dllinfo.dli_fname); */
        MARK("realpath", dirName != NULL ? dirName : "NULL");
      }
    }
    /* And if all else fails, use the global install dir */
    if (0 == sts) {
#if defined(ICCPKG)
      /* INSTDIR+"/"+GSK_LIBNAME + \0 */
      strncpy(fname, INSTDIR, MAX_PATH);
      strncat(fname, "/",MAX_PATH);
      strncat(fname, GSK_LIBNAME,MAX_PATH); /* Stripped off by the caller */
      fname[MAX_PATH-1] = 0;
#else
#if defined(INSTDIR)
      /* INSTDIR+"/N/icc/icclib/"+ICCDLLNAME + \0 */
      strncpy(fname, INSTDIR,
              MAX_PATH - (strlen("/N/icc/icclib/") + strlen(ICCDLL_NAME) + 1));
#endif
#if defined(NON_FIPS_ICC)
      strncat(fname, "/N/icc/icclib/",MAX_PATH);
#else
      strncat(fname, "/C/icc/icclib/",MAX_PATH);
#endif
      strncat(fname, ICCDLL_NAME,MAX_PATH); /* Stripped off by the caller */
#endif
      fname[MAX_PATH-1] = 0;
      MARK("fallback to install dir", fname);
      rprv = realpath(fname, dirName);
      MARK("realpath rv", rprv != NULL ? rprv : "NULL");
      if (NULL != rprv) {
        MARK("realpath", dirName != NULL ? dirName : "NULL");
        foundit = 1;
      }
    }
    if (foundit) {
      result = (char *)calloc(strlen(dirName) + 1, 1);
      if (result) {
        strncpy(result, dirName, strlen(dirName));
      }
    }
  }
  if (NULL != dirName) {
    free(dirName);
  }
  if (NULL != fname) {
    free(fname);
  }
  MARK("path", (NULL != result) ? result : "NULL");
  /* printf("_loaded_from() %s\n",result); */
  OUT();
  return result;
}
#endif /* Generic Unix */
