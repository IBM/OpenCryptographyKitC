/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: Fragmented buffer handling
//
*************************************************************************/

/*
  Simple data chaining routines to simplfy the SP800-90 functions
*/

#if !defined(DS_HEADER)
#define DS_HEADER

/*! 
  @brief Number of fragments we can handle
 */

#define NDS 12

/*! \IMPLEMENT Data chain API, ds.h, ds.c
  Data structures to allow us to cope with the NIST chained data inputs
  without excessive alloc/copy/free work
  This API only tracks blocks of data, it never creates/destroys them
  or takes copies.
  This structure is limited length, simply because the aim
  isn't to have a general purpose routine, just something that'll track
  pre-allocated data and provide a consistant API so the code is less ugly
*/

/*! @brief
  Internal part of the Data Chain structure, 
  Implementation detail hidden from the public API
*/
typedef struct DSPrivate_t {
  unsigned int n;          /*!< how many bytes are available */
  const unsigned char *data; /*!< pointer -> data */
} DSPrivate;

/*! @brief
  Public part of the Data Chain structure
*/
typedef struct DS_t {
  unsigned int index;       /*!< Which block we are in */
  unsigned int count;       /*!< How many bytes we've pulled from this block */
  unsigned int total;       /*!< How many bytes left in total */
  DSPrivate dsp[NDS+1]; /*!< 0,NULL terminated array of data blocks */
} DS;


/*! 
  @brief copy bytes from a data chain structure into the output 
  @param dsc a pointer to a data chain structure
  @param buffer the output buffer
  @param n the number of bytes to copy
  @return the number of bytes (excluding any 0 fill) extracted
  @note this exists because the NIST spec synthesizes inputs from concatenated
  strings. That uses large amounts of allocated storage if we just blindly allocate 
  and copy - by using chaining buffers we reduce the alloc/copy overhead
  @note If insufficient bytes exist, the request is filled with 0 bytes
  
*/
int DS_Copy(DS *dsc,unsigned char *buffer,int n);

/*!
  @brief extract entries one at a time from a 
  data chain structure
  effectively the opposite of DS_Append()'s operation
  @param dsc a pointer to a data chain structure
  @param n a pointer to somewhere to store the number 
  of bytes left in the returned buffer pointer
  @param bptr a pointer to a place to store a copy of the data 
  pointer or NULL in which case the number of entries remaining
  is returned.
  @return The number of data items left in the chain
  @note you can mix and match DS_Copy and DS_Extract
  but note that DS_Extract always "empties" the next
  available buffer
*/
int DS_Extract(DS *dsc,unsigned int *n, unsigned char **bptr);

/*!
  @brief Initialize a chained data structure 
  no need to free anything, it's only referenced
  @param dsc a pointer to a data chain structure
*/
void DS_Init(DS *dsc);

/*! 
   @brief Reset the counters in a data chain structure
   leaving the data intact.
   @param dsc a pointer to a data chain structure
*/
void DS_Reset(DS *dsc);

/*!
  @brief add a new item to the end of chain list 
  @param  dsc a pointer to a data chain structure
  @param n the number of bytes of data
  @param data a pointer to the data
  @return 1 on sucess, 0 otherwise - usually "out of space"
*/
int DS_Append(DS *dsc,unsigned int n, const unsigned char *data);

/*!
  @brief insert a new item to the start of the chain list 
  @param  dsc a pointer to a data chain structure
  @param n the number of bytes of data
  @param data a pointer to the data
  @return 1 on sucess, 0 otherwise - usually "out of space"
*/
int DS_Insert(DS *dsc,unsigned int n, const unsigned char *data);

/*! @brief Return the number of bytes available in a DS struct
  @param dsc a pointer to a data chain structure
  @return the number of bytes accesible in data linked
*/
unsigned int DS_Size(DS *dsc);

#endif
