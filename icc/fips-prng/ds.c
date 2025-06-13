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


#include <stdio.h>
#include <string.h>
#include "ds.h"

#if defined(STANDALONE)

static int DS_Debug(DS *ds)
{
  int i;
  for(i = 0 ; ds->dsp[i].data != NULL && (i < NDS) ; i++) {
    printf("|%*s|",ds->dsp[i].n,ds->dsp[i].data); 
  }
  printf("\n");
}
/*!
  @brief test function for the data chain methods
  @return 1 on success, 0 otherwise
*/
static int DS_Test()
{
  int rv = 0;
  int i = 0;
  const unsigned char *testdata[10] = {"The"," quick"," brown"," fox",
				       " jumps"," over"," the"," lazy",
				       " dog",NULL};
  static unsigned char *ref = "The quick brown fox jumps over the lazy dog";
  unsigned char **tmp;
  unsigned char testbuffer[100];
  unsigned char *t1 = NULL;
  unsigned int n = 0;
  DS dsc;
  DS ds2;
  

  DS_Init(&dsc);
  for(i = 0;NULL != testdata[i]; i++) {
    DS_Append(&dsc,strlen(testdata[i]),testdata[i]);
  }
  if( DS_Size(&dsc) != strlen(ref)) {
    printf("FAILED - DS contains %d bytes, should be %d\n",
	   DS_Size(&dsc),strlen(ref));
    rv = 1;
  }
  DS_Copy(&dsc,testbuffer,99);
  if(0 != strncmp(ref,testbuffer,strlen(ref))) {
      printf("FAILED [%s]\n",testbuffer);
      DS_Debug(&dsc);
      rv = 1;
  }
  DS_Reset(&dsc);
  DS_Copy(&dsc,testbuffer,99);
  if(0 != strncmp(ref,testbuffer,strlen(ref))) {
      printf("FAILED [%s]\n",testbuffer);
      rv = 1;
  }  
  
  DS_Reset(&dsc);
  DS_Init(&ds2);
  i = 1;
  while (i) {
    i = DS_Extract(&dsc,&n,&t1);
    DS_Append(&ds2,n,t1);
  }
  DS_Copy(&ds2,testbuffer,99);
  if(0 != strncmp(ref,testbuffer,strlen(ref))) {
      printf("FAILED [%s]\n",testbuffer);
      rv = 1;
  }  
  /* Check that DS_Copy() will 0 pad */
  memset(testbuffer,0xff,100);
  DS_Init(&dsc);
  DS_Copy(&dsc,testbuffer,100);
  for(i = 0; i < 100; i++) {
    if( testbuffer[i] != 0) {
      printf("FAILED DS_Copy() did not zero pad\n");
    }
  }

  /* check DS_Insert() */
  DS_Init(&dsc);
  for(i = 8;i >= 0; i--) {
    DS_Insert(&dsc,strlen(testdata[i]),testdata[i]);
  }
  if( DS_Size(&dsc) != strlen(ref)) {
    printf("FAILED DS_Insert - DS contains %d bytes, should be %d\n",
	   DS_Size(&dsc),strlen(ref));
    rv = 1;
  }
  DS_Copy(&dsc,testbuffer,99);
  if(0 != strncmp(ref,testbuffer,strlen(ref))) {
      printf("FAILED [%s]\n",testbuffer);
      DS_Debug(&dsc);
      rv = 1;
  }
  DS_Reset(&dsc);
  DS_Copy(&dsc,testbuffer,99);
  if(0 != strncmp(ref,testbuffer,strlen(ref))) {
      printf("FAILED [%s]\n",testbuffer);
      rv = 1;
  }  
  
     
  return rv;
}

int main(int argc, char *argv[])
{
  return DS_Test();

}

#endif

/*!
  @note DS_ routines.
  These simply MANAGE chains of pre-existing data,
  they DO NOT take copies of the data passed in,
  or delete the data they manage. 
  Hence the "const unsigned char *" for their managed
  data type.
*/

int DS_Copy(DS *dsc,unsigned char *buffer,int n)
{
  int i = 0;
  while(n) {
    if(NULL == dsc->dsp[dsc->index].data) {
      memset(buffer+i,0,n);
      /* Don't update the count, return the bytes actually extracted ! */
      n = 0;
    } else {
      if(dsc->count < dsc->dsp[dsc->index].n) {
	buffer[i++] = dsc->dsp[dsc->index].data[dsc->count++];
	n--;
	dsc->total --;
      } else {
	dsc->count = 0;
	dsc->index ++;
      }
    }
  }
  return i;
}

void DS_Init(DS *dsc)
{
  memset(dsc,0,sizeof(DS));
}

void DS_Reset(DS *dsc)
{
  int i;
  dsc->index = 0;
  dsc->count = 0;
  dsc->total = 0;
  for(i = 0; i < NDS; i++) {
    if(NULL == dsc->dsp[i].data) break;
    dsc->total += dsc->dsp[i].n;
  }
}


int DS_Append(DS *dsc,unsigned int n, const unsigned char *data)
{
  int rv = 0;
  int i;
  if((n > 0) && (NULL != data) ) { /* Don't need to store it */
    for(i = 0; i < NDS ;i++) {
      if(NULL == dsc->dsp[i].data) {   
	dsc->dsp[i].data = data;
	dsc->dsp[i].n = n;
	dsc->total += n;
	rv = 1;
	break;
      }
    }
  } else { /* Not an error to try to Append no data */
    rv = 1;
  }
  return rv;
}

int DS_Insert(DS *dsc,unsigned int n, const unsigned char *data)
{
  int rv = 1;
  int i;
  if((n > 0) && (NULL != data)) { 
    /* check that there's room at the top */
    if(NULL == dsc->dsp[NDS-1].data) {
      /* 
	 Move the old entries up - we checked above that 
	 we had space 
      */
      for(i = NDS-1; i > 0 ; i--) {      
	dsc->dsp[i].data = dsc->dsp[i-1].data;
	dsc->dsp[i].n    = dsc->dsp[i-1].n;
      }
      /*
	Push the new entry into the front 
      */
      dsc->dsp[0].data = data;
      dsc->dsp[0].n = n;
      dsc->total += n;
    } else {
      rv = 0;
    }
  } /* It's not an error to try and push no data */
  return rv;
}

int DS_Extract(DS *dsc,unsigned int *n,unsigned char **data)
{
  int i = 0;
  unsigned int j = 0;
  
  if(NULL != data ) {
    *data = NULL;
    *n = 0;
    if (dsc->index < NDS ) {
      *data = (unsigned char *)dsc->dsp[dsc->index].data;
      *n = dsc->dsp[dsc->index].n;
      dsc->total = dsc->total - (*n);    
      dsc->index++;
    }
  }
  for(i = 0, j = dsc->index; j < NDS ; j++) {
    if(NULL == dsc->dsp[j].data) break;
    i++;
  }
  return i;
}

unsigned int DS_Size(DS *dsc)
{
  return dsc->total;
}
