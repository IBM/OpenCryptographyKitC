/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: Get file size. 
//              I know, but using ls isn't as portable.
//
*************************************************************************/

 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


#if defined(_WIN32)
static int FileSize(char *path)
{
  int rv = 0;
  struct _stat mystat;
  if( 0 == _stat(path,&mystat) ) {
    rv = mystat.st_size;
  } 
  return rv;
}
static int FileSizeW(wchar_t *path)
{
  int rv = 0;
  struct _stat mystat;
    if( 0 == _wstat(path,&mystat) ) {
    rv = mystat.st_size;
  } 
  return rv;  
}

#else
static off_t FileSize(char *path)
{
  off_t rv = 0;
  struct stat mystat;
  if( 0 == stat(path,&mystat) ) {
    rv = mystat.st_size;
  }
  return rv;
}

#endif

#if defined(STAND_ALONE)
int main(int argc, char *argv[])
{
  int size = -1;
  if(argc < 2) {
    printf("filesize filename\n");
    exit(1);
  } else {
    size = FileSize(argv[1]);
    printf("%d\n",size);
  }
  return 0;
}
#endif
