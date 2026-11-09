#if defined (__APPLE__)
#define __STDC_WANT_LIB_EXT1__ 1
#include <string.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#include "platform.h"

/* scraped from GSKit's gsk_securezero */
void ICC_securezero(void* s, size_t n)
{
#if defined(_WIN32)
   SecureZeroMemory(s, n);
#elif defined(LINUX) || defined(LINUX64) || defined(HPUX64) || defined(HPUX64_IA64) || defined(HPUX)
   explicit_bzero(s, n);
#elif defined(AIX4) || defined(_IA64) || defined(AIX64)
   {
      volatile unsigned char* p = (volatile unsigned char*)s;
      while (n--) {
         *p++ = 0;
      }
   }
#elif defined(OSX)
   /* see https://ibm-security.slack.com/archives/G6RL29LN7/p1772582197533479?thread_ts=1772579877.006559&cid=G6RL29LN7 */
   memset_s(s, n, 0, n);
#else
   (memset)(s, 0, n);
#endif
}

