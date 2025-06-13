/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: read an i5/OS service program (library)
//
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(OS400)
#include "getnmi.h"
#endif

/* this program is used to read an i5/OS program or service pgm and write to file	*/
/* usage: ./iccread ../package/icc/icclib/libicclib.so icc.nmi		*/

int main(int argc, char* argv[])
{

#if !defined(OS400)
    return 0;
#else
    READ_PGM_T rPgm;
    READ_PGM_T *pPgm = NULL;
    int rc = 0;
    int i;
    unsigned char *buf = NULL;
    char *ifile, *ofile;
    FILE *fout;
    

    if (argc < 3) {
	printf("Invalid arguments\n");
	exit(1);
    }

    ifile = argv[1];
    ofile = argv[2];

    fout = fopen(ofile, "wb");
    if(fout == NULL)
    {
	printf("Error opening file %s\n", ofile);
	exit(1);
    }

    pPgm = &rPgm;
    rc = init_read_pgm(pPgm, ifile);
    if (rc) {		
      printf("Unable to process [%s]\n", ifile);
	exit(1);      
    }

    rc = read_pgm(pPgm, (void**) &buf, &i);
    while((0 == rc) && (i > 0)) {
      if(fwrite(buf, i, 1, fout) <= 0)
	{
	  printf("Error writing to file %s\n", ofile);
	  fclose(fout);
	  exit(-1);
	}

      rc = read_pgm(pPgm, (void**) &buf, &i);
    }

    if (pPgm) term_read_pgm(pPgm);
    fclose(fout);
    return 0;
#endif
}
