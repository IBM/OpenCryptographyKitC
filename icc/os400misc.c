/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description:                                                               
//           This module implements platform dependent operations. 
//
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <qp0z1170.h>

#include <except.h>          /* exception handling */

/* This code must be compiled with EBCDIC literals and without QADRT */

/*
** NOTE: this program runs the CHKOBJITG command which requires a special *AUDIT
** authority. Since the caller of ICC (e.g. QNOTES) might not have the special authority,
** this program needs to adopt QSYS authority in order to run. Thus, any product that uses 
** ICC will need to change icc400.pgm and set QSYS as the owner and USRPRF(*OWNER).
*/ 
/*! 
  @brief Validate the crypto library on OS400 using CHKOBJITG
  @param argc argument count
  @param argv an array of argument values 
  - argv[1] library the ICC shared library name
  - argv[2] pgmname the program name
  - argv[3] type either pgm or srvpgm
  @return 0 if validated O.K, 1 if validation failed, 2 if bad args, 3 unknown error
  \Platf OS400 only
*/
int main (int argc, char *argv[])
{
    int rc = 1;
	char *library = argv[1];
	char *pgmname = argv[2];
	char *type = argv[3];
    char sys_call[256];

	if (library == NULL || pgmname == NULL || type == NULL)
	{
		return 2;	/* bad args */
	}

    sprintf(sys_call, "CHKOBJITG OBJ('/qsys.lib/%s.lib/%s.%s') OUTFILE(%s/ICC_OUTF) CHKDMN(*NO) CHKPGMMOD(*YES) CHKCMD(*NO) CHKSIG(*NONE) CHKLIB(*NO) SCANFS(*NO)", library, pgmname, type, library);

	/* run the CHKOBJITG command */

#pragma exception_handler(SystemErr, 0, 0, _C2_ALL, _CTLA_HANDLE_NO_MSG)
    rc = system(sys_call);
	if (rc == 0)
	{
		return 0;	/* validated okay */
	}
	return 1;		/* validation failed */
#pragma disable_handler

SystemErr:
    return 3;		/* unknown error */
}



