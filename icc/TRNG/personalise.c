/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description:Shared personalization function
//
*************************************************************************/

#include "icclib.h"


extern unsigned long RdCTR();

#if defined(_WIN32)

struct timezone {
  long tz;
};
/*! @brief Approximate gettimeofday for windows 
  All this does is grab time and populate values
  to generate one-off data
  it's NOT accurate
  \Platf Windows only
  @param tv Pointer to a Unixy "struct timeval"
  @param tz Pointer to "Unixy" "struct timezone" (Unused)
  @return 0
*/
static int gettimeofday(struct timeval *tv, struct timezone *tz)
{
  FILETIME ft;

  if( NULL != tv) {
    GetSystemTimeAsFileTime(&ft);
    tv->tv_sec = ft.dwLowDateTime;
    tv->tv_usec = ft.dwHighDateTime;
  }
  return 0;
}
/*! @brief gethostname for windows
  \Platf Windows only
  @param buffer The buffer in which to return the machine name
  @param size  The number of bytes (max) of the name to return
  @return Status. (Ignore it - this is just a hack)
  @note This is in wsock32 but we want ICC to have minimal
  dependencies.
*/
static int eg_gethostname(char *buffer,int size) 
{
  DWORD lsize = size;
  return GetComputerName(buffer,&lsize);
}
#else 
#define eg_gethostname(x,y) gethostname(x,y)
#endif

#define efPSIZE (sizeof(struct timeval) +  (2 * sizeof(DWORD)) + sizeof(unsigned long) +  sizeof(name) )

unsigned int Personalize(unsigned char *buffer)
{
  int rv = 0;
  struct timeval tv;
  unsigned long ccount;
  DWORD pid;
  DWORD tid;
  int i;
  unsigned char *tmp = NULL;
  unsigned char s = 0;
  unsigned long l;

  /* As much of the machine name as will fit in 80 bytes 
     This is static, and thread safe'ish.
     i.e. the fixed data is the same, and random
     mashing of the random data across threads makes no large difference
     to security
   */
  static char name[80]; 
  if(NULL == buffer) {
    rv = efPSIZE;
  } else {
    if('\0' == name[0]) {
      eg_gethostname(name,79);
    }    
    gettimeofday(&tv,NULL);

    pid = ICC_GetProcessId();
    tid = ICC_GetThreadId();

    ccount = RdCTR();
  
    tmp = buffer;
    memcpy(tmp,&tv,sizeof(struct timeval));
    tmp += sizeof(struct timeval);

    memcpy(tmp,&ccount,sizeof(ccount));
    tmp += sizeof(ccount);

    memcpy(tmp,&pid,sizeof(pid));
    tmp += sizeof(pid);

    memcpy(tmp,&tid,sizeof(tid));
    tmp += sizeof(pid);

    strncpy((char *)tmp,name,sizeof(name)-1);

    /* 
       And if there's space left, i.e. the machine name was less than 80 bytes
       fill with bytes from the raw noise source 
     */
    for( tmp = tmp+strlen((char *)tmp); tmp < (buffer + efPSIZE) ; tmp ++) {
      l = RdCTR();
      for(i = 0; i < sizeof(unsigned long); i++) {
	      s ^= (l & 0xff);
	      l >>= 8;
      }
      *tmp = s;
    }
  }
  return rv;
}

