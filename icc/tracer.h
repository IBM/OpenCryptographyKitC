/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License").  You may not use
// this file except in compliance with the License.  You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

#if !defined(__TRACER_H)
#define __TRACER_H
#define TRACE 1

#if defined(_WIN32)
#define __func__ __FUNCTION__
#else
#include <unistd.h>
#endif

#if !defined(TRACE)
#define IN() 
#define OUT()
#define MARK(x,y)
#define TRACE_START(x,y,z)
#define TRACE_END(x)
#else 

extern int TRACE_indent;
extern FILE *logfile;
extern unsigned long trace_counter;
#if !defined(NON_FIPS_ICC)
#   define FIPS_TAG "S"
#else 
#   define FIPS_TAG (NON_FIPS_ICC ? "N":"C")
#endif

const char *TimeMark();
void TRACE_START(const char *source, const char *application, const char* fn);
void TRACE_END(const char* fn);

#if defined(_WIN32)
#include "platform.h"
    DWORD mypid();
#else
    pid_t mypid();
#endif

#if defined(IN)
   #undef IN
#endif
#if defined(OUT)
   #undef OUT
#endif 

#define TRACE_START_EX(source, application) TRACE_START(source, application, __FILE__)
#define TRACE_END_EX() TRACE_END(__FILE__)
#define IN() if(NULL != logfile) { fprintf(logfile,"%-16s:%-16s:%-8d:%-1s:%*s>%s\n",TimeMark(),__FILE__,mypid(),FIPS_TAG,(TRACE_indent < 40) ? TRACE_indent++:40,"",__func__); fflush(logfile);}
#define OUT() if (NULL != logfile) { fprintf(logfile,"%-16s:%-16s:%-8d:%-1s:%*s<%s\n",TimeMark(),__FILE__,mypid(),FIPS_TAG,((--TRACE_indent) < 40) ? TRACE_indent:40,"",__func__);fflush(logfile);}
/* Out with integer return code */
#define OUTRC(rc) if(NULL != logfile) { fprintf(logfile,"%-16s:%-16s:%-8d:%1s:%*s<%s (%d)\n" ,TimeMark(),__FILE__,mypid(),FIPS_TAG,((--TRACE_indent) < 40) ? TRACE_indent : 40, "", __func__,rc);fflush(logfile);}
#define MARK(x,y) if(NULL != logfile) { fprintf(logfile,"%-16s:%-16s:%-8d:%-1s:%*s!%s %s %s\n",TimeMark(),__FILE__,mypid(),FIPS_TAG,(TRACE_indent < 40) ? TRACE_indent:40,"",__func__,(x),(y));fflush(logfile);}
#define MARK2(a,b) {MARK(a,b); if (pcb->trace_callback) pcb->trace_callback((a), (b)); }
/* Include this ONLY in one file which will contain the active tracing code */

#if defined(TRACE_CODE)

FILE *logfile = NULL;

int TRACE_indent = 0;

unsigned long trace_counter = 0L;
/* 
   This is NOT thread safe, and AFAIK there is no way to make
   it thread safe
*/
const char *TimeMark()
{
  static char tmp[32];
  unsigned long r;
  clock_t ck;
  ck = clock();
  r = ck;
  /* Windows doesn't have snprintf so just make sure we use a large buffer */
  sprintf(tmp,"%16ld",r);
  return tmp;
}
/*!
  @brief return the Day of the week as a string given a  
  week number 0-6 Sun->Sat
  @param wday the weekday number
  @return the weekday text
*/
static const char *DoW(int wday)
{
  const char *wk = "---";
  static const char *DoWA[7] = { "Sun","Mon","Tue","Wed","Thu","Fri","Sat" };
  
  if((wday >= 0) && (wday < 7)) {
    wk = DoWA[wday];
  }
  return wk;
}
/*!
  @brief return the Month of the year as a string given
  the month number (0-11)
  @param month the month number
  @return the month text 
*/
static const char *MoY(int month)
{
  const char *mo = "---";
  static const char *MoYA[12] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
  if((month >= 0) && (month < 12)) {
    mo = MoYA[month];
  }
  return mo;
}


#if defined(_WIN32)
/*!
  @brief reproduce the Unix date/time function
  @param buffer a place to return the Timestamp text
  @return the timestamp text
*/
static char *TimeStamp(char *buffer)
{  
  SYSTEMTIME lt;
  GetSystemTime(&lt);

  sprintf(buffer,"%s %s %02d %02d:%02d:%2d %04d",
          DoW(lt.wDayOfWeek),
          MoY(lt.wMonth-1),
          lt.wDay,
          lt.wHour,
          lt.wMinute,
          lt.wSecond,
          lt.wYear
          );
  buffer[24] = ' ';
  buffer[25] = '\0';
  return buffer;
}

DWORD mypid(void) 
{
  static DWORD pid = (DWORD)-1;
  if (((DWORD)-1) == pid) {
    pid = (DWORD)GetCurrentProcessId();
  }
  return pid;
}
#else 
/*!
  @brief reproduce the Unix date/time function
  @param buffer a place to return the Timestamp text
  @return the timestamp text (== buffer)
  Note that it's simpler to do this by hand than it is to include the
  correct headers across every OS variant
*/
char *TimeStamp(char *buffer)
{
  time_t timep;
  struct tm tm;
  time(&timep);
  gmtime_r(&timep,&tm);
  sprintf(buffer,"%s %s %02d %02d:%02d:%02d %04d",
          DoW(tm.tm_wday),
          MoY(tm.tm_mon),
          tm.tm_mday,
          tm.tm_hour,
          tm.tm_min,
          tm.tm_sec,
          tm.tm_year +1900
          );
  buffer[24] = ' ';
  buffer[25] = '\0';
  return buffer;
}
pid_t mypid() {
  static pid_t pid = (pid_t)-1;
  if ((pid_t)-1 == pid) {
    pid = getpid();
  }
  return pid;
}
#endif


void TRACE_START(const char *source, const char *application, const char *fn)
{
  FILE *tmpfile = NULL;
  char *path = NULL;
  size_t alen = 0;
  char *platform = OPSYS;
  /* Yes, thread safe, this is called during library load */
  static char trc_buffer[1024]; /* Path construction */
  static char trc_abuf[256];    /* App name (optionally passed through) */  
#if defined (_WIN32)
  char *tmpishere = "C:\\Temp\\";
#else
  char *tmpishere = "/tmp/";
#endif

  if(NULL == source) {
    source = "unknown";
  }

  if(NULL == application || (strlen(application) >= (sizeof(trc_abuf)-5)) ) {
    application = "GSKIT_CRYPTO";
  }
  sprintf(trc_abuf,"%s.log",application);
 
  path = getenv("GSK_TRACE_PATH");
  if(NULL != path) {
    tmpishere = path;
  }
  /* Try the local dir first */
  strcpy(trc_buffer,trc_abuf);
  tmpfile = fopen(trc_buffer,"r");
  if(NULL != tmpfile) {
    fclose(tmpfile);
    logfile = fopen(trc_buffer,"a");
  }
  /* Nothing in the local dir, try 'tmpiswhere'/'application'.log */
  if(NULL == logfile) {
    alen = strlen(tmpishere)+strlen(trc_abuf)+2;
    if(alen < sizeof(trc_buffer)) {
      strcpy(trc_buffer,tmpishere);
      /* Fix possibly missing directory seperators from the env */
      if((trc_abuf[strlen(trc_buffer)-1] != '\\') &&  (trc_abuf[strlen(trc_buffer)-1] != '/')) {
        strcat(trc_buffer,"/"); /* Also works on Windows */
      }
      strcat(trc_buffer,trc_abuf);
      tmpfile = fopen(trc_buffer,"r");
      if(NULL != tmpfile) {
        fclose(tmpfile);
        logfile = fopen(trc_buffer,"a");
      }
    }
  }
  if(NULL != logfile) {
    setbuf(logfile,NULL);
    TimeStamp(trc_buffer);
    if(NULL != logfile) {
      fprintf(logfile,"%-16s:%-16s:%-8d:%1s:%s %s %s %s\n",TimeMark(),fn,mypid(),FIPS_TAG,application,source,platform,trc_buffer);
      fprintf(logfile,"%-16s:%-16s:%-8d:%1s:CLOCKS_PER_SEC=%ld\n",TimeMark(),fn,mypid(),FIPS_TAG,(long)CLOCKS_PER_SEC);
      fflush(logfile);
    }  
  }
}

void TRACE_END(const char* fn)
{
  char buffer[256];
  if(NULL != logfile) {
    TimeStamp(buffer);
    fprintf(logfile,"%-16s:%-16s:%-8d:%1s,%s\n",TimeMark(),fn,mypid(),FIPS_TAG,buffer);
    fprintf(logfile,"%-16s:%-16s:%-8d:%1s:<TRACE ENDS>\n\n",TimeMark(),fn,mypid(),FIPS_TAG);
    if(logfile != stderr) {
      fclose(logfile);
    }
    logfile = NULL;
  }
}

#endif /* __TRACE_CODE */
#endif /* __TRACED_H */




#endif
