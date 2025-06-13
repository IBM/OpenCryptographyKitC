/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/
/*
// Description: Error message routines.Only usable in an icclib context.
*/

#include "icc.h"
#include "icclib.h"
#include "status.h"
#include "tracer.h"

#include "iccversion.h"

#define ICC_VTAG " (ICC"\
 MAKESTRING(ICC_VERSION_VER) "." \
 MAKESTRING(ICC_VERSION_REL) "." \
 MAKESTRING(ICC_VERSION_MOD) "." \
 MAKESTRING(ICC_VERSION_FIX) ")" 

/* Destination for the first fatal error message */
extern FILE * errorfile;

static ICC_STATUS last_status; /* Cached last fatal error status */
static int error_state = 0; /*!< Internal global error state */
extern void DisableAPI(); /*!< In icclib.c, selectively disable the interface */
extern void SetFlags(ICClib *pcb, ICC_STATUS *status); /*!< icclib.c echo flags from opaque ICC context to status */
/*! @brief
  One one trip to stopped
  Set the flag to say we have a hard error
  and disable all functions allocating new objects,
  generating keys, or initializing operations.
*/
void setErrorState(void) {
  error_state = 1;
  if (NULL != errorfile) {
    fprintf(errorfile, "\n\n%s\n\n", ('\0' != last_status.desc[0]) ? last_status.desc: "Unknown error");
  }
  MARK("FATAL ERROR",('\0' != last_status.desc[0]) ? last_status.desc: "Unknown error");
  DisableAPI();
} 
int getErrorState(void) { 
  return error_state; 
}

/*!
  @brief a version of strncat that's useful.
  @param base a pointer to a fixed length buffer
  @param append the string to append
  @param maxlen the maximum size of base
*/
ICCSTATIC void ICC_strlcat(char *base, const char *append,
                           unsigned int maxlen) {
  int l;
  l = maxlen - strlen(base);
  if (l > 1) {
    strncat(base, append, l);
  }
}

static void StatCat(char *dest,const char *msg)
{
  ICC_strlcat(dest,msg,ICC_DESCLENGTH);
  dest[ICC_DESCLENGTH-1] = '\0';
}    
/*! 
  @brief Called from code way down there to give detail on the bad that happened
  @param msg The error message 
  @param file The file the message came from
  @param line the line number
  @note The majRC/minRC are embedded in the saved desc
*/     
ICCSTATIC void SetFatalError(const char *msg,const char *file,int line)
{
  char linetxt[10];

  /* Only commit evil if evil wasn't already commited */
  if (0 == getErrorState()) {
    last_status.desc[0] = 0;
    StatCat(last_status.desc, msg);
    if(NULL != file) {
      StatCat(last_status.desc, ": ");
      StatCat(last_status.desc, file);
      StatCat(last_status.desc, ",");
      ICC_utoa(linetxt, 10, line);
      StatCat(last_status.desc, linetxt);
    }
    last_status.minRC = ICC_DISABLED;
    last_status.majRC = ICC_ERROR;
    setErrorState();
  }
}

/*!
  @brief If we have a 'root cause' error try to prepend it to the new one
  @param stat ICC status
*/
static void OriginalError(ICC_STATUS *stat) 
{
  stat->desc[0] = 0;
  if(error_state && (0 != strlen(last_status.desc)) ) {
    StatCat(stat->desc, "Original error [");
    StatCat(stat->desc, last_status.desc); /* This is already formatted */
    StatCat(stat->desc, "] ");
  }
}


/**
   @brief a routine for converting line numbers (unsigned) to ascii
   It's used in places where malloc has failed, so sprintf() isn't
   likely to work.
   @param buf a buffer for the output unsigned number long enough to take the result including the terminator
   @param base the number base - only 10 is tested
   @param n the line number 0 <= n <= 99999
   @return the number of bytes in the buffer, including the terminator.
   
*/
ICCSTATIC int ICC_utoa(char *buf,unsigned int base,unsigned int n)
{
  char ibuf[(sizeof(unsigned int)*8) +1]; /* Long enough for binary */
  int i,j = 0;
  if(base > 0) {
    if( n == 0) {
      buf[j++] = '0';
    } else {
      for(i = 0; n ; i++)  {
	      ibuf[i] = (char)(n - (n/base) * base);
	      n = (n - ibuf[i])/base;
	      if((base > 10) && (ibuf[i] > 9)) {
	        ibuf[i] += ('a' - 10);
	      } else {
	        ibuf[i] += '0';
	      }
      }   
      for(i-- ; i >= 0 ; i--) {
	      buf[j++] = ibuf[i];
      }
    }
  }
  buf[j++] = '\0';
  return j;
}
/**
   @brief a routine for converting unsigned long to ascii
   It's used in places where malloc has failed, so sprintf() isn't
   likely to work.
   @param buf a buffer for the output unsigned number long enough to take the result including the terminator
   @param base the number base - only 16
   @param n the input
   @param digits is the number of digits required. We zero fill.
   @return the number of bytes in the buffer, including the terminator.
   
*/
ICCSTATIC int ICC_ultoa(char *buf,unsigned int base,unsigned long n,unsigned int digits)
{
  char ibuf[(sizeof(unsigned long)*8) +1]; /* Long enough for binary */
  int i = 0,j = 0, offs = 0;
  unsigned int c = 0;
  if(base > 0) {
    if( n == 0) {
      buf[j++] = '0';
    } else {
      for(i = 0; n ; i++)  {
	      ibuf[i] =(char) (n - (n/base) * base);
	      n = (n - ibuf[i])/base;
	      if((base > 10) && (ibuf[i] > 9)) {
	        ibuf[i] += ('a' - 10);
	      } else {
	        ibuf[i] += '0';
	      }
      }
      c = i; /* Number of characters in scratch buffer */
      if(c > digits) { /* Truncate */
	      offs = c - digits;
	      i -= offs;
      } else if(c < digits) {
	      while ( c < digits) { /* zero pad */
	        buf[j++] = '0';
	        c++;
	      }
      }
      for(i-- ; i >= 0 ; i--) {
        buf[j++] = ibuf[i + offs];
      }
    }
  }
  buf[j++] = '\0';
  return j;
}
/*! 
  @brief returns the last error message set by ICC 
  @param stat a pointer to an ICC_STATUS structure
  @return stat->majRC
*/
ICCSTATIC int ICC_GetLastError(ICC_STATUS *stat)
{
  memcpy(stat,&last_status,sizeof(ICC_STATUS));
  return stat->majRC;
}
/*!
  @brief Generate an error from an OpenSSL error return
  @param pcb the library context
  @param stat a preallocated ICC_STATUS structure
  @param file the file name
  @param line the line number
  @return stat->majRC
*/

ICCSTATIC int OpenSSLError(ICClib *pcb,ICC_STATUS *stat,char *file,int line)
{
  int rv = ICC_OK;
  int evpRC = 0;
  char *tmp = NULL;
  tmp = ICC_Malloc(ICC_VALUESIZE,__FILE__,__LINE__);
  if( NULL == tmp) {
    rv = SetStatusMem(pcb,stat,file,line);
  } else {
    tmp[0] = '\0';
    evpRC = ERR_get_error();
    if (evpRC == 0) {
      rv = SetStatusLn(pcb,stat,
		                   ICC_OPENSSL_ERROR,evpRC,
		                  "Unknown error when performing OpenSSL operations",file,line);  
    } else {
      ERR_error_string_n(evpRC, tmp, ICC_VALUESIZE);
      ERR_clear_error();
      rv  = SetStatusLn(pcb,stat,ICC_OPENSSL_ERROR,evpRC,tmp,file,line);
    }
    ICC_Free(tmp);
  }
  SetFlags(pcb,stat);
  return rv;
}


/**
   @brief Specific handling of allocation failures - we can't allocate scratch buffers
   so scribble directly into the ICC_STATUS buffer
   @param pcb an ICClib structure pointer - it's undefined type in some places we call this
   @param stat a pre-allocated ICC_STATUS structure
   @param file the file name where the error occurred
   @param line the line number where the error ocurred.
   @return stat->majRC
   @note We assume we are in an out of memory situation already and that file and line are "sane"
*/
ICCSTATIC int SetStatusMem (ICClib *pcb,ICC_STATUS * stat, char *file, int line)
{
  return SetStatusLn(pcb,stat,ICC_ERROR | ICC_FATAL,ICC_NOT_ENOUGH_MEMORY,"Memory allocation failed",file,line);
}

/** @brief Set the ICC status to no errors 
    @param pcb A pointer to an ICClib structure or NULL
    @param stat a pointer to an ICC_STATUS structure
    @return stat->majRC
*/

int SetStatusOK (ICClib *pcb,ICC_STATUS * stat)
{
  /* Bug 2124, needs to be cleared as it may be uninitialized */
  stat->mode = 0;
  strncpy(stat->desc,"O.K.",ICC_DESCLENGTH);
  stat->majRC = 0;
  stat->minRC = 0;
  SetFlags(pcb,stat);
  return stat->majRC;
}
/**
   @brief Set status with message, rc's and file/line number
   @param pcb an ICClib structure pointer - it's undefined type in some places we call this
   @param stat a pre-allocated ICC_STATUS structure
   @param majRC the major return code
   @param minRC the minor return code
   @param msg The main message to go in the status
   @param file the file name where the error occurred
   @param line the line number where the error ocurred.
   @return stat->majRC
   @note We assume we are in an out of memory situation already and that file and line are "sane"
*/
ICCSTATIC int SetStatusLn (ICClib *pcb,ICC_STATUS * stat,int majRC,int minRC,const  char *msg, const char *file, int line)
{
  char linetxt[10];
  stat->desc[0] = 0;
  OriginalError(stat);
  StatCat(stat->desc, msg);
  StatCat(stat->desc, ": ");
  StatCat(stat->desc, file);
  StatCat(stat->desc, ":");
  ICC_utoa(linetxt,10,line);
  StatCat(stat->desc, linetxt);
  StatCat(stat->desc,ICC_VTAG);
  stat->majRC = majRC & (~ICC_FATAL);
  stat->minRC = minRC;
  if(majRC & ICC_FATAL) {
    SetFatalError(stat->desc,NULL,0); /* Kill file,line as we've already done that */
  }
  if (getErrorState()) {
    if (NULL != pcb) {
      pcb->flags |= ICC_ERROR_FLAG;
    }
    stat->mode |= ICC_ERROR_FLAG;
  }  
  SetFlags(pcb,stat);
  return (stat->majRC);
}
/*!
  @brief Generate an error with two text strings
  @param pcb the library context
  @param stat a preallocated ICC_STATUS structure
  @param majRC the major return code
  @param minRC the minor return code
  @param m1 first - main - message
  @param m2 second message (detail)
  @param file the file name
  @param line the line number
  @return icc_stat->majRC
*/

ICCSTATIC int SetStatusLn2(ICClib *pcb,ICC_STATUS *stat,int majRC,int minRC,const char *m1, const char *m2,const char *file,int line)
{
  char *tmp = NULL;

  tmp = ICC_Malloc(ICC_DESCLENGTH,__FILE__,__LINE__);
  if( NULL == tmp) {
    SetStatusMem(pcb,stat,__FILE__,__LINE__);
  } else {
    tmp[0] = 0;
    StatCat(tmp,m1);
    StatCat(tmp,"(");
    StatCat(tmp,m2);
    StatCat(tmp,") ");
    SetStatusLn(pcb,stat,majRC,minRC,tmp,file,line);
    ICC_Free(tmp);
  }
  SetFlags(pcb,stat);
  return (stat->majRC);
}

 /*!
  @brief
  Development/debug code.
  Prints byte strings in hex suitable for cut and paste into source code.
  This code has no security impacts. Anything you can do with this, you can
  do with a debugger.
  @param bytes input buffer
  @param len length of input buffer
  \debug Code: iccPrintBytes: Always enabled
*/ 
ICCSTATIC void iccPrintBytes(unsigned char bytes[], int len)
{
    int blocksize = 16;
    int numChunks, remChunks, i, j;

    if (bytes != NULL && len > 0)
    {
        numChunks = len/blocksize;
        remChunks = len%blocksize;
        for (i=0;i<numChunks;i++)
        {
            for (j=0;j<blocksize;j++) printf("0x%02X,",(0xFF & bytes[i*blocksize+j]));
            printf("\n");
        }
        if (remChunks > 0)
        {
            for (j=0;j<remChunks-1;j++) printf("0x%02X,",(0xFF & bytes[i*blocksize+j]));
            printf("0x%02X",(0xFF & bytes[i*blocksize+j]));
            printf("\n");
        }
    }
    else
    {
        printf (" the buffer is empty\n");
    }
}
