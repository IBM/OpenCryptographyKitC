/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
//
// Description: Definitions for minibuffer helper class for the TRNG code.
//       
***************************************************************************/
/* Structure for handling a minibuffer
   these are used in a few places to improve performance
   i.e. reading /dev/random
   reading the timer registers on ARM
   The buffer is deliberately small to minimize security exposures
   on the other hand, where it's used an ~64x gain in performance is 
   a win
*/

#if !defined(_MINIBUF_H)
#define _MINIBUF_H

#if defined(STANDALONE)
/* Defines for standalone testing of the entropy core
   independant of ICC itself \sa GenRndData
*/   
#define ICC_Calloc(a,b,c,d) calloc(a,b)
#define ICC_Free(a) free(a)

#endif


#define MINIBUF_SIZE 64


typedef void (MINIB_FILL_F)(void *xdata,unsigned char *buffer,int n); /* read for example */

/*! @brief
    Small buffer structure to improve performance and 
    reduce lock contention throughout the RBG code.
    Size is fixed to 64 bytes. 
    @note Generally the buffer is used at  quite a low level, 
    and later compression/mixing reduces any
    impacts from these buffers being in memory.
    - In general, less than 64 bytes of RBG OUTPUT data would
    be exposed by residual information.
    - The data in these buffers is usually mixed with data from 
    other sources before compression.
    - The buffer is deliberately small to reduce the risks to long
    lived keys.
*/ 
typedef struct _minibuf {
  unsigned char buffer[MINIBUF_SIZE]; /* Minibuffer */
  int index;      /* Index into the buffer */
  MINIB_FILL_F *g; /*!< Get byte of data , must not be NULL */
  void *xdata;    /*!< Pointer to external data, may be NULL */
  int allocated; /* If this is embedded in another structure set to 0 */
} MINIBUF;

/*!@brief
  Initialize a minibuffer, note that this will usually be embedded
  inside another struct - and b will be passed in.
  @param b a pointer to a MINIBUF, or NULL,  in which case one will be allocated
  @param g callback to fill a buffer.
  @param xdata a pointer to a blob passed to g(), may for example
  be a pointer to a long lived fd etc
  @return a pointer to a minibuf, or NULL on failure
*/
MINIBUF * minib_init(MINIBUF *b,MINIB_FILL_F *g,void *xdata);

/*! @brief
  Call a minibuffer to get data.
  - if the buffer is empty g() will be called to fill the buffer
  - if there is data in the buffer, that will be returned and the buffer size
    decreased
  @param b a pointer to a minibuffer
  @return a byte of data, or if things are broken 0. 
   - note that 0 is a valid return value, but a stream of 0's SHOULD
     be detected by downstream code.
*/
unsigned char  minib_get(MINIBUF *b);

/*! @brief
  IF we allocated a minibuffer, clear and free it
  IF we didn't allocate the minibuffer clear it only
  @param b a pointer to a minibuffer

*/
void  minib_free(MINIBUF *b);

/* @brief
  Push one byte into a minibuf. Wraps on overflow/
  We needed a managed buffer to carry timing attack resistance
  data around the entropy gathering loops.
  We reused the minibuf data structure for this
  This routine is otherwise normally unused and
  note minib_merge is tolerant of b == NULL.
*/

void minib_merge(MINIBUF *b, unsigned char v);

#endif
