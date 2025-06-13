/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description:   
// This module implements platform shared library initializations. 
//
*************************************************************************/

#ifdef  __cplusplus
extern "C" {
#endif

#include "platfsl.h"
#include "platform.h"
#if defined(_WIN32)

BOOL APIENTRY DllMain(HANDLE hModule, DWORD reason, LPVOID lpReserved)
{
   int rc = 0;
/* printf(" ENTERING DllMain "); */
/* __asm { int 3}     */
   switch(reason)
   {
       case DLL_PROCESS_ATTACH:
          rc=ICCLoad();
          break;

       case DLL_PROCESS_DETACH:
          rc=ICCUnload();
          break;

       case DLL_THREAD_ATTACH:
       case DLL_THREAD_DETACH:
      //Don't do anything here!!!!
       break;
   }
   return rc==0;
}
#elif defined(__linux)   /* Actually GCC */

void __attribute__ ((__constructor__)) iccSLInit(void) { ICCLoad(); }
void __attribute__ ((__destructor__))  iccSLFini(void) { ICCUnload(); }

#elif defined(__APPLE__)   /* Actually GCC */
/* None of these actually work, passing -init ICCLoad to the linker works.
   Documented in the developer tools errata
*/
/*
extern void iccSLInit(void) { ICCLoad(); }
extern void iccSLFini(void) { ICCUnload(); }

#pragma CALL_ON_LOAD ICCLoad
#pragma CALL_ON_UNLOAD ICCUnload

Work on the later OS/X's
*/
void __attribute__ ((__constructor__)) iccSLInit(void) { ICCLoad(); }
void __attribute__ ((__destructor__))  iccSLFini(void) { ICCUnload(); }


#elif defined(_AIX) || defined(__sun) || defined(OS400)

/* works in conjunction with the linker flag -binitfini:iccSLInit:iccSLFini:0 */
/* in the makefile for AIX                                                    */
/* the pragma is sufficient for SUN.                                          */
extern void iccSLInit(void) { ICCLoad(); }
extern void iccSLFini(void) { ICCUnload(); }

#if defined(__sun )

#pragma init (iccSLInit)
#pragma fini (iccSLFini)

#endif /* __sun */
#elif defined(__hpux)


#if defined(__GNUC__)

void __attribute__ ((__constructor__)) iccSLInit(void) { ICCLoad();   }
void __attribute__ ((__destructor__))  iccSLFini(void) { ICCUnload(); }

#else

/*works in conjunction with the flag +I iccSLInitializer in the Makefile*/

void iccSLInitializer( shl_t handle, int loading)
{
     if (loading) ICCLoad();
     else         ICCUnload();
}
#endif  /* __GNUC__ */


/*#pragma INIT "icc_init" void iccSLInit() { ICCLoad(); }  */
/*#pragma FINI "icc_fini" void iccSLFini() { ICCUnload(); }*/

#elif (__OS2__)
/* Yes, no library load entry point exists on OS2 
  we rely on the (slightly racey) fallback code 
*/
#elif (__MVS__)
extern void iccSLInit(void) { ICCLoad(); }
extern void iccSLFini(void) { ICCUnload(); }
  
#else

# error Please provide platform specific code here.


# endif   /* __linux __AIX __sun __hpux */

#ifdef  __cplusplus
}
#endif
