/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description:                                                               
//           This module implements platform dependent operations.
//
*************************************************************************/

/* Note: DO NOT #include icclib.h. 
   It includes macros which resolve to function references on older compilers
   and that in turn makes libicc.a directly dependent on openssl
*/
#include "platform.h"

#if defined(__OS2__)
    char LoadError[256];
#endif

#if defined(_WIN32)

ICCSTATIC DWORD ICC_GetProcessId(void)
{
    return ((DWORD)GetCurrentProcessId());
}

ICCSTATIC DWORD ICC_GetThreadId(void )
{
    return ((DWORD)GetCurrentThreadId());
}
/* Ugly Windows hacks for Unicode environment */
ICCSTATIC void* ICC_LoadLibrary(const char*  path)
{
    void* lib;
    lib =  ((void*)LoadLibraryA(path));
    return lib;
}
ICCSTATIC void *ICC_LoadLibraryW(const wchar_t * path)
{
  void *lib;
  lib = ((void*)LoadLibraryW(path));
  return lib;
}

ICCSTATIC void* ICC_GetProcAddress(void* handle, const char* name)
{
    return ((void*)GetProcAddress(handle, name));
}
ICCSTATIC DWORD ICC_FreeLibrary(void* handle)
{
    int rc=0;
    rc = FreeLibrary(handle);
    rc = (rc ? 0 : GetLastError());
    return rc;
}
ICCSTATIC char* ICC_GetLibraryError(char* errStr, size_t errStrSize)
{
    int lastError;
    WORD  langID;
    DWORD formMsg;
    lastError = GetLastError();

    langID =  MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
    formMsg = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                            NULL, lastError, langID,
                           (LPTSTR)errStr, errStrSize, NULL);
    if (formMsg == 0) {
      memset(errStr, 0, errStrSize);
      strncpy(errStr, "Unknown error", errStrSize-1);
    }
    return  errStr;
}

ICCSTATIC int ICC_CreateMutex(ICC_Mutex* mutexPtr)
{
    int rc = 0;
    *mutexPtr = CreateMutex(NULL, FALSE, NULL);
    rc = ((*mutexPtr == NULL) ? GetLastError() : 0);
    return rc;
}
ICCSTATIC int ICC_LockMutex(ICC_Mutex* mutexPtr)
{
    int rc = 0;
    rc = WaitForSingleObject(*mutexPtr, INFINITE);
    rc = ((rc == WAIT_OBJECT_0) ? 0 : GetLastError());
    return rc;
}
ICCSTATIC int ICC_UnlockMutex(ICC_Mutex* mutexPtr)
{
    return (ReleaseMutex(*mutexPtr) ? 0 : GetLastError());
}
ICCSTATIC int ICC_DestroyMutex(ICC_Mutex* mutexPtr)
{
    return (CloseHandle(*mutexPtr) ? 0 : GetLastError());
}

#elif defined(__linux) || defined(_AIX) || defined(__sun) || defined(__hpux) || defined(__APPLE__) || defined(__MVS__)

ICCSTATIC DWORD ICC_GetProcessId(void)
{
    return ((DWORD)getpid());
}
ICCSTATIC DWORD ICC_GetThreadId(void)
{
#if defined(__MVS__)
/**
    \BUG Descrription: on I5/OS and z/OS a TID is 8 bytes but OpenSSL
    assumes 4 bytes. Provided TIDS are allocated sequentially
    this should seldom hit a product. Symptoms: Wierd hard to reproduce locking failures. Severity: Low, at least 2^32 threads need to be created
    and some low PID's still need to be in flight. Recommended workround: - use thread pools rather than creating and destroying threads. 

*/

	pthread_t t = pthread_self();
	unsigned long *p = (unsigned long *)&t;
	return ((DWORD) (p[0] ^ p[1]) );
#else
    return            ((DWORD)pthread_self());
#endif
}

ICCSTATIC int ICC_CreateMutex(ICC_Mutex* mutexPtr)
{
    int rc = 0;
    rc = pthread_mutex_init(mutexPtr, NULL);
    return rc;
}
ICCSTATIC int ICC_LockMutex(ICC_Mutex* mutexPtr)
{
    return pthread_mutex_lock(mutexPtr);
}
ICCSTATIC int ICC_UnlockMutex(ICC_Mutex* mutexPtr)
{
    return pthread_mutex_unlock(mutexPtr);
}
ICCSTATIC int ICC_DestroyMutex(ICC_Mutex* mutexPtr)
{
    int rc = 0;
    rc = pthread_mutex_destroy(mutexPtr);
    return rc;
}

/* There's a problem with RTLD_LOCAL on Apple, probably with how we link - look at "bundle" etc 
   and see if it can be fixed.
   See below for the fix.
*/
#  if defined(__APPLE__)
ICCSTATIC void* ICC_LoadLibrary(const char* path)
{
  void *rv = NULL;
  /* Try this first, it works on other Unix's and on OS/X 10.4 
     and should give us a local copy of the library image.
     ( a good thing (tm))
  */
  rv =  ((void*)dlopen(path, RTLD_NOW | RTLD_LOCAL));
  /* If that failed, fall back to RTLD_GLOBAL - which prevents us
     loading with another lib of the same name on OS/X 10.4+
  */
  if(rv == NULL) rv = ((void*)dlopen(path, RTLD_NOW | RTLD_GLOBAL));
  return rv;
}
ICCSTATIC DWORD ICC_FreeLibrary(void* handle)
{
 if( dlclose(handle) != 0) {
    /* The RTLD_GLOBAL means we won't unload if we were loaded multiple times 
       But hopefully, somewhere, a reference count gets decremented. 
       This appears to be the most graceful fix.
    */
    dlerror(); 
  }
  return(0);	
}
#  else
ICCSTATIC void* ICC_LoadLibrary(const char* path)
{
    return        ((void*)dlopen(path, RTLD_NOW | RTLD_LOCAL));
}

ICCSTATIC DWORD ICC_FreeLibrary(void* handle)
{
    return   dlclose(handle);
}
#   endif 

ICCSTATIC void* ICC_GetProcAddress(void* handle, const char* name)
{
    return ((void*)dlsym(handle, name));
}
ICCSTATIC char* ICC_GetLibraryError(char* errStr, size_t errStrSize)
{
  char *tmp = NULL;
  tmp = dlerror();
  memset(errStr, 0, errStrSize);
  if( NULL != tmp) {
    strncpy(errStr,tmp, errStrSize);
  } else {
    strncpy(errStr,"Unknown failure during dlopen(), dlerror() reports no error", errStrSize);
  }
  dlerror(); /* Clear the error, flush the error string */
  return errStr;
}


#elif defined(__OS2__)

ICCSTATIC DWORD ICC_GetProcessId(void)
{
    return (getpid());
}

ICCSTATIC DWORD ICC_GetThreadId(void )
{
    PTIB ptib = NULL;
    PPIB ppib = NULL;
    int rc = 0;
    rc = DosGetInfoBlocks(&ptib, &ppib);
    return (ptib->tib_ordinal);
}
ICCSTATIC void* ICC_LoadLibrary(const char*  path)
{
    HMODULE ModuleHandle = NULLHANDLE;
    DosLoadModule(LoadError,
                     sizeof(LoadError),
                     path,
                     &ModuleHandle);
    return (void *)ModuleHandle;
}
ICCSTATIC void* ICC_GetProcAddress(void* handle, const char* name)
{
    int rc;
    PFN ModuleAddr = 0;
    rc = DosQueryProcAddr(handle,
                             0L,
                             name,
                             &ModuleAddr);

    return (void *)ModuleAddr;
}
ICCSTATIC DWORD ICC_FreeLibrary(void* handle)
{
    int rc=0;
    rc = DosFreeModule(handle);
    return rc;
}
ICCSTATIC char* ICC_GetLibraryError(char* errStr, size_t errStrSize)
{
  if ((NULL != errStr) && ( strlen(errStr) < 256)) {
        strncpy(errStr, LoadError,255);
  }
  return  errStr;
}

ICCSTATIC int ICC_CreateMutex(ICC_Mutex* mutexPtr)
{
    int rc = 0;
    rc = DosCreateMutexSem(NULL, mutexPtr, 0, FALSE);
    return rc;
}
ICCSTATIC int ICC_LockMutex(ICC_Mutex* mutexPtr)
{
    int rc = 0;
    rc = DosRequestMutexSem(*mutexPtr, (ULONG) SEM_INDEFINITE_WAIT);
    return rc;
}
ICCSTATIC int ICC_UnlockMutex(ICC_Mutex* mutexPtr)
{
   int rc = 0;
   rc = DosReleaseMutexSem(*mutexPtr);
   return rc;
}
ICCSTATIC int ICC_DestroyMutex(ICC_Mutex* mutexPtr)
{
int rc;
    rc = DosCloseMutexSem(*mutexPtr);
    return rc;
}
#elif defined(OS400)

ICCSTATIC void CloseSrvpgm (unsigned long long* handle);
ICCSTATIC unsigned long long * OpenSrvpgm(const char * asrvpgmName);
ICCSTATIC void * GetSrvpgmSymbol(unsigned long long * handle, char * asymbolname);
ICCSTATIC void term_trans(void);

#define OS400_Malloc(sz) malloc(sz)



ICCSTATIC DWORD ICC_GetProcessId(void)
{
    return ((DWORD)getpid());
}
/**
    \BUG Descrription: on I5/OS and z/OS a TID is 8 bytes but OpenSSL
    assumes 4 bytes. Provided TIDS are allocated sequentially
    this should seldom hit a product. Symptoms: Wierd hard to reproduce locking failures. Severity: Low, at least 2^32 threads need to be created
    and some low PID's still need to be in flight. Recommended workround: - use thread pools rather than creating and destroying threads. 

*/


ICCSTATIC DWORD ICC_GetThreadId(void)
{

  pthread_id_np_t   tid;
  tid = pthread_getthreadid_np();
  return ((DWORD)tid.intId.lo);
}

ICCSTATIC int ICC_CreateMutex(ICC_Mutex* mutexPtr)
{
    int rc = 0;
    rc = pthread_mutex_init(mutexPtr, NULL);
    return rc;
}
ICCSTATIC int ICC_LockMutex(ICC_Mutex* mutexPtr)
{
    return pthread_mutex_lock(mutexPtr);
}
ICCSTATIC int ICC_UnlockMutex(ICC_Mutex* mutexPtr)
{
    return pthread_mutex_unlock(mutexPtr);
}
ICCSTATIC int ICC_DestroyMutex(ICC_Mutex* mutexPtr)
{
    int rc = 0;
    rc = pthread_mutex_destroy(mutexPtr);
    return rc;
}
ICCSTATIC void* ICC_LoadLibrary(const char* path)
{
   return ((void *)OpenSrvpgm((char *) path));
}

ICCSTATIC DWORD ICC_FreeLibrary(void* handle)
{
    CloseSrvpgm((unsigned long long *) handle);
    return 0; 
}

ICCSTATIC void* ICC_GetProcAddress(void* handle, const char* name)
{
    return ((void*)GetSrvpgmSymbol((unsigned long long *) handle, (char *) name));
}

ICCSTATIC char* ICC_GetLibraryError(char* errStr, size_t errStrSize)
{
    return errStr;
}


/*
  Extra OS400 specific messy stuff 

*/

static int	gTransInit = 0;
static iconv_t gAtoE;
#define OS400_MAX_NAMELEN 512                                   

/*****************************************************************************/
/*                                                                           */
/*   get_lib_and_pgm          - Get library and program from symlnk          */
/*                                                                           */
/*  Read a symlnk that points to a pgm/srvpgm object and return the library  */
/*  and pgm/srvpgm name.                                                     */
/*                                                                           */
/*  Input:  pointer to the name of the symlnk                                */
/*  Output:                                                                  */
/*                                                                           */
/*          Return ptr to library name                                       */
/*          Return ptr to program name                                       */
/*          Return ptr to program type (pgm or srvpgm)                       */
/*                                                                           */
/*****************************************************************************/
ICCSTATIC void get_lib_and_pgm(char * tgtpath, char * lib, char * pgm, char * type)
{

	int len;
	char * ptr;

	char * ptr2;
	char * libptr = NULL;
	char * pgmptr = NULL;
	char * typeptr = NULL;
	char path[OS400_MAX_NAMELEN];
	char linkpath[OS400_MAX_NAMELEN];

	memset(lib,0,11);
	memset(pgm,0,11);
	memset(type,0,11);

	strncpy(path, tgtpath, sizeof(path)-1);
	path[sizeof(path)-1] = '\0';

	/* resolve all symbolic links */
	while (1)
	{
		if((len = readlink(path, linkpath, sizeof(linkpath)-1)) <= 0)
			break;
		linkpath[len] = '\0';
		strncpy(path, linkpath, sizeof(path)-1);
		path[sizeof(path)-1] = '\0';
	}

	/* convert path to upper case (os400 is case insensitive) */

	for (ptr = path; *ptr != '\0'; ptr++) *ptr = toupper(*ptr);


	/* we should now have a path of the form: /QSYS.LIB/<library>.LIB/<objname>.<objtype>
	 * if not, it is time to punt */

	ptr = path;
	while(isspace(*ptr) || *ptr == '/') ptr++;
	if (strncmp(ptr, "QSYS.LIB/", 9) != 0) return;
	ptr += 9;
	while(*ptr == '/') ptr++;

	/* now grab the library name */

	libptr = ptr;
	ptr2 = strchr(ptr, '.');
	if (ptr2 == NULL) return;
	*ptr2++ = '\0';
	ptr = ptr2;

	/* verify it has a "LIB" suffix */


	if (strncmp(ptr, "LIB/", 4) != 0) return;
	ptr += 4;
	while(*ptr == '/') ptr++;

	/* now grab the program name */

	pgmptr = ptr;
	ptr2 = strchr(ptr, '.');
	if (ptr2 == NULL) return;
	*ptr2++ = '\0';
	ptr = ptr2;

	/* what's left is the program type */

	typeptr = ptr;

	strncpy(lib,libptr,10);
	strncpy(pgm,pgmptr,10);
	strncpy(type,typeptr,10);
	
	return;
}

/*****************************************************************************/
/*                                                                           */
/*  get_pgm_ptr                                                              */ 
/*                                                                           */
/*  Input:  pointer to _SYSPTR for the *SRVPGM                               */
/*          pointer to the name of the library to search                     */
/*  Output:                                                                  */
/*                                                                           */
/*          Return ptr to pgm/srvpgm if succcessful                          */
/*          Optionally return library, pgm name and type (in ebcdic)         */
/*          Return NULL if unsuccessful                                      */
/*                                                                           */
/*****************************************************************************/


ICCSTATIC void     get_pgm_ptr(_SYSPTR * ptr_to_pgmptr, char * path, char *outlibrary, char *outpgmname, char *outpgmtype)
{
	int is_pgm;
	char lib[11];
	char pgmname[11];
	char pgmtype[11];
	char elib[11];
	char epgmname[11];
	char epgmtype[11];

	*ptr_to_pgmptr = NULL;

	if (outlibrary)
		*outlibrary = '\0';

	if (outpgmname)
		*outpgmname = '\0';

	if (outpgmtype)
		*outpgmtype = '\0';

	get_lib_and_pgm(path, lib, pgmname, pgmtype);

	if (__toebcdic(lib, elib, sizeof(elib)) == 0)  /* translate failed - returned length=0 */
	    return;

	if (__toebcdic(pgmname, epgmname, sizeof(epgmname)) == 0)  /* translate failed - returned length=0 */
	    return;
	
	if (__toebcdic(pgmtype, epgmtype, sizeof(epgmtype)) == 0)  /* translate failed - returned length=0 */
	    return;

	is_pgm = (strnicmp(pgmtype, "PGM", 3) == 0);
	
#pragma exception_handler(APIrslverr, 0, 0, _C2_MH_ESCAPE, _CTLA_HANDLE_NO_MSG)
	*ptr_to_pgmptr = rslvsp(is_pgm ? WLI_PGM : WLI_SRVPGM, epgmname, elib, _AUTH_ALL);
#pragma disable_handler

	if (outlibrary) {
	    strncpy(outlibrary, elib,OS400_MAX_NAMELEN);
    }
	if (outpgmname) {
	    strncpy(outpgmname, epgmname,OS400_MAX_NAMELEN);
    }
	if (outpgmtype) {
	    strncpy(outpgmtype, epgmtype,OS400_MAX_NAMELEN);
    }
	APIrslverr:
	  return;
}
	

ICCSTATIC void CloseSrvpgm (unsigned long long* handle)
{
  if(handle) {
    free (handle);
  }
  term_trans();
}

ICCSTATIC unsigned long long * OpenSrvpgm(const char * asrvpgmName)
{
     _SYSPTR pgm_ptr=NULL;
     unsigned long long actmark=0;
     unsigned long long * actmarkptr=NULL;
     int rc=0;
     char srvpgmName[OS400_MAX_NAMELEN];


     if (init_trans() != 0)
	 return NULL;

     get_pgm_ptr(&pgm_ptr, (char*) asrvpgmName,NULL,NULL,NULL);
     if(pgm_ptr==NULL) return NULL;

#pragma exception_handler(OpenErr,0,0,_C2_MH_ESCAPE,_CTLA_HANDLE)
     actmark = QleActBndPgmLong(&pgm_ptr, NULL, NULL, NULL, NULL);
#pragma disable_handler

     actmarkptr = (unsigned long long *)OS400_Malloc(sizeof(unsigned long long));

     if(actmarkptr!=NULL)
     {
	 *actmarkptr = actmark;
     }

OpenErr:
     return actmarkptr;
}

ICCSTATIC void * GetSrvpgmSymbol(unsigned long long * handle, char * asymbolname)
{
    void	* routine=NULL;
    char	symNameEbc[128];
    unsigned long long *pActivationMark;		
    unsigned long long AllActivationsMark = 0;
    int		ExportType;
    int		ProcNameLen=0;
    Qus_EC_t	ErrInfo;		/* Error code template*/

     if (__toebcdic(asymbolname, symNameEbc, sizeof(symNameEbc)) == 0)  /* translate failed - returned length=0 */
	 return NULL;
     /*
      * If caller doesn't specify an activation mark, search all
      * activations in this program's activation group.
      */
     if (handle == (unsigned long long *)NULL)	
	 pActivationMark = &AllActivationsMark;	
     else
	 pActivationMark = (unsigned long long *)handle;

     ErrInfo.Bytes_Provided=0;
#pragma exception_handler(SymErr,0,0,_C2_MH_ESCAPE,_CTLA_HANDLE) 
    routine = QleGetExpLong((unsigned long long *)pActivationMark, 0, 0, (char*)symNameEbc,
			       &routine, &ExportType, (Qus_EC_t*)&ErrInfo );
#pragma disable_handler

     /* Make sure this was a exported procedure and not data */
     if (ExportType!=QLE_EX_PROC)
	 routine = NULL;

SymErr:
    return routine;
}

static char from_ascii[]="IBMCCSID008190000110\0\0\0\0\0\0\0\0\0\0\0\0";
static char to_ebcdic[]="IBMCCSID00000\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
static char to_ebcdic37[]="IBMCCSID00037\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";


/*
 * init_trans()
 * output: 0 - successfull
 */
ICCSTATIC int init_trans()
{

    if (! gTransInit)
    {
	gAtoE=iconv_open(to_ebcdic, from_ascii);
	if (gAtoE.return_value == -1)
	{
	    printf("iconv_open failed.\n");
	    return 1;	
	}
	gTransInit= 1;
    }

    return 0;
}

/*
 * init_trans()
 * output: length of translated data
 */
ICCSTATIC int __toebcdic(const char *ina, char *oute, int outsize)
{
    char	*inptr;
    char	*outptr;
    unsigned int inbytesleft=0;
    unsigned int out_size;
    int		rc;
    iconv_t	lociconv;

    outptr = oute;
    *outptr = '\0';
    inptr = (char*) ina;
    out_size = outsize;
    lociconv.cd[0] = 0;

    if (! gTransInit) {
      lociconv = iconv_open(to_ebcdic, from_ascii);
      if (lociconv.return_value == -1) {
	printf("iconv_open failed.\n");
	return 0;	
      }
      rc = iconv(lociconv, &inptr, &inbytesleft, &outptr, &out_size);	
    }
    else {
      rc = iconv(gAtoE, &inptr, &inbytesleft, &outptr, &out_size);
    }
    if (rc == -1)
    {
      if (errno == E2BIG || errno == EILSEQ) {
	printf("EBCDIC output string is too small, errno=%d \n", errno);
      }
      else {
	printf("Error from iconv() in __toebcdic, errno=%d \n", errno);
      }
      return 0;  /* iconv failed, set the return translated length to zero */
    }
    
    if (out_size > 0) { /* if there is still room left in the output buffer */
      *outptr = '\0';
    }
    if ((lociconv.cd[0]) != 0) {
	iconv_close(lociconv); /* close iconv */
    }
    
    return outptr-oute;

}

ICCSTATIC void term_trans()
{
	iconv_t lociconv;
	char * locstring;

	if ((gAtoE.cd[0]) != 0) {
		lociconv = gAtoE;
		gAtoE.cd[0] = 0;
		gTransInit = 0;
		iconv_close(lociconv); /* close iconv */
	}
}

/* end OS400 */

#else


# error Please provide platform specific code here.


#endif


