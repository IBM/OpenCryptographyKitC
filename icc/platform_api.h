/*-----------------------------------------------------------------------------
// Copyright IBM Corp. 2023
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
//
// Description: Function instantiations for abstraction/indirection for platform
//              specific code.
//
//---------------------------------------------------------------------------*/

#ifndef INCLUDED_PLATFORM_API
#define INCLUDED_PLATFORM_API

#ifdef  __cplusplus
extern "C" {
#endif

/*! 
  @brief Returns the current process ID.
  @return the current process ID.
*/
ICCSTATIC DWORD ICC_GetProcessId(void);

/*! 
  @brief Returns the current thread ID.
  @return the current process ID.
*/
ICCSTATIC DWORD ICC_GetThreadId(void);

/*!
  @brief  Returns a handle to the shared library specified in
         "libName", or NULL if the library could not be loaded.
	 ~ dlopen() on UNIX.
  @param libName name of library to load
  @return library handle or NULL
*/

ICCSTATIC void* ICC_LoadLibrary(const char * libName);
#if defined(_WIN32)

/*!
  @brief  Returns a handle to the shared library specified in
         "libName", or NULL if the library could not be loaded.
	 ~ dlopen() on UNIX.
  @param libName Unicode name of library to load
  @return library handle or NULL
*/

ICCSTATIC void* ICC_LoadLibraryW(const wchar_t* libName);
#endif

/*! 
   @brief Returns the address of the functionName for the library
          represented by "handle", or NULL if the symbol could not be
          found.
	  ~ dlsym() on UNIX.
   @param handle the (non-NULL) handle returned by ICC_LoadLibrary
   @param functionName the string name of the function to get the address of
   @return a function pointer
*/
ICCSTATIC void* ICC_GetProcAddress(void* handle, const char* functionName);

/*!
  @brief Unloads the shared library represented by "handle" from
         memory.  Returns 0 upon success, the non-zero error code otherwise.
         ~ dlclose() on UNIX.
  @param handle the (non-NULL) handle returned by ICC_LoadLibrary
  @return 0 on sucess, non 0 on failure
*/
ICCSTATIC DWORD ICC_FreeLibrary(void* handle);

/*!
  @brief  Returns the last error that occurred while attempting to
          load/unload/access a shared library.
	  - Note this returns whatever the OS returns.
  @param errorStr - The place to put an ASCII error string
                    describing the error
  @param errorStrLen The maximum number of bytes "errorStr" can hold.
  @return NULL or a pointer to the error text
*/
ICCSTATIC char* ICC_GetLibraryError(char* errorStr, size_t errorStrLen);

/*! 
  @brief Creates and initializes a mutex structure. 
  @param mutexPtrRef a pointer to a location to return the new mutex pointer
  @return 0 on sucess, non-zero on failure
*/
ICCSTATIC int   ICC_CreateMutex(ICC_Mutex* mutexPtrRef);

/*!
  @brief aquires a lock on the given mutex.
  @param mutexPtr a pointer to the mutex to lock
  @return  0 on sucess, non-zero on failure
*/
ICCSTATIC int   ICC_LockMutex(ICC_Mutex* mutexPtr);

/*!
  @brief releases a lock on the given mutex.
  @param mutexPtr a pointer to the mutex to unlock
  @return  0 on sucess, non-zero on failure
*/
ICCSTATIC int   ICC_UnlockMutex(ICC_Mutex* mutexPtr);

/*!
  @brief destroys the given mutex.
  No thread should have the mutex in a lock state 
  (even the calling thread). 
  @param mutexPtr a pointer to the mutex to destroy
  @return  0 on sucess, non-zero on failure
*/
ICCSTATIC int   ICC_DestroyMutex(ICC_Mutex* mutexPtr);

#ifdef OS400
void	* GetSrvpgmSymbol(unsigned long long * handle, char * symbolname);
unsigned long long * OpenSrvpgm(const char * srvpgmName);
void     get_pgm_ptr(_SYSPTR * ptr_to_pgmptr, char *path, char *library, char *pgmname, char *pgmtype);



void	CloseSrvpgm (unsigned long long * handle);
int	init_trans(void);
void	term_trans(void);
int	__toebcdic(const char *inasciistr, char *outebcdicstr, int outsize);

#endif



#ifdef  __cplusplus
}
#endif


#endif  /* INCLUDED_PLATFORM */
