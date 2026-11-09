/*-----------------------------------------------------------------------------
// Copyright IBM Corp. 2023
// Licensed under the Apache License 2.0 (the "License").  You may not use
// this file except in compliance with the License.  You can obtain a copy
// in the file LICENSE in the source distribution.
//
// Description: minibuffer helper class for the TRNG code.
//                                                                           
*******************************************************************************/


#include <stdio.h>
#include <string.h>
#include "platform.h"
#include "TRNG/minibuf.h"


MINIBUF * minib_init(MINIBUF *b,MINIB_FILL_F *g,void *xdata)
{
  if(NULL == b) {
    b = ICC_Calloc(1,sizeof(MINIBUF),__FILE__,__LINE__);
    if(NULL != b) {
      b->allocated = 1;
    }
  } else {
    ICC_securezero(b->buffer,sizeof(b->buffer));
  }
  if(NULL != b) {
    b->g = g;
    b->xdata = xdata;
    b->index = -1;
  }
  return b;
}

unsigned char minib_get(MINIBUF *b)
{
  unsigned char u = 0;

  if(NULL != b) {
    if(NULL != b->g) {
      if((b->index < 0) || (b->index >= MINIBUF_SIZE)) {
	      (b->g)(b->xdata,b->buffer,MINIBUF_SIZE); /* This is the read data/fill buffer call */
	      b->index = (MINIBUF_SIZE -1);
      } 
      u = b->buffer[b->index--];
    }
  }
  return u;
}
/* Note that minibuf is normally burst filled, however we needed
  and extra buffer to carry timing attack defence data around 
  the entropy gathering so this gets stuffed with potentially 
  faster moving bits rather than mixing them in immediately.

  Ultimately we merge them as data appended at the HMAC compression phase
  We don't care if we overfilled, partly filled or partly old data, 
  this isn't entropy data, just hardening against timing attacks
  Note: minib_merge() doesn't require minib_init() be called first
*/
void minib_merge(MINIBUF *b, unsigned char v)
{
  if(NULL != b) {
    if(NULL != b->g) {
      if((b->index >= MINIBUF_SIZE) || (b->index < 0)) {
        b->index = 0;
      } 
      b->buffer[b->index++] ^= v;
    }
  }
}
void minib_free(MINIBUF *b)
{ 
  if(NULL != b) {
    if(b->allocated) {
      ICC_securezero(b,sizeof(MINIBUF));
      ICC_Free(b);
    } else {
      ICC_securezero(b->buffer,MINIBUF_SIZE);
      b->index = -1;
    }
  }
}
