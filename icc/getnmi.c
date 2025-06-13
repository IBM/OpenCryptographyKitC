/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description:                                                                
//        i5/OS unique functions to read a library ie. service program.       
//
*************************************************************************/

#if defined(OS400)
#ifdef DEBUG
#define DEBUG_NMI
#endif

/* NOTE: This module must be compiled with EBCDIC literals and without QADRT */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <mih/rslvsp.h>
#include <mih/matsobj.h>
#include <spawn.h>			/* spawn() */
#include <sys/wait.h>		/* wait() */
#include <qp0z1170.h>		/* Qp0zInitEnv() */
#include <except.h>			/* exception handling */
#include <errno.h>			/* errno and values */
#include "platform.h"
#include "getnmi.h"

#ifdef ICCREAD	/* iccread does not have these apis */
#define ICC_Malloc(sz,file,line) malloc(sz)
#define ICC_Free(ptr) free(ptr)
#endif

/*! @brief
  \Platf OS400 only
*/

/*!
  \Platf OS400 only
*/
/* Common materialization data header
 * Receiver header below is followed by an array of these structures
 */
typedef struct _MBPG_Data_T {     /*!< Materialization Entry         */
                                        /*!< Common Entry Header:      */
     unsigned int     Off_To_Next;      /*!< Offset to next entry      */
     unsigned int     Pgm_Mat_Id;       /*!< Bound program mat. id     */
     unsigned int     Mod_Mat_Id;       /*!< Bound module mat. id      */
     unsigned int     Mod_Num;          /*!< Bound module number       */
     int              Present     : 1;  /*!< Data present for entry    */
     int              Partial     : 1;  /*!< Partial data              */
     int              Valid       : 1;  /*!< Data requested is valid   */
                                        /*!< for program type.         */
     int              reserved1   :29;
     char             reserved2[12];
     char             Entry;            /*!< Materialization Entry     */
     /* The individual Materialization Entries must be manually      */
     /* mapped to this location. The entry(s) will correspond to one */
     /* the following:                                               */
     /*     _MBPG_General_T             General bound program info   */
     /*     _MBPG_Copyright_T           Copyright strings            */
     /*     _MBPG_Srvc_Pgms_T           Bound service program info   */
     /*     _MBPG_Modules_T             Bound modules info           */
     /*     _MBPG_String_T              String directory component   */
     /*     _MBPG_Limits_T              Bound program limits         */
     /*     _MBPG_Bnd_Pgm_Info_T        Specific bound program info  */
     /*     _MBPG_Signatures_T          Signatures info              */
     /*     _MBPG_Export_Proc_T         Exported procedure info      */
     /*     _MBPG_Export_Data_T         Exported data info           */
     /*     _MBPG_AGP_Data_Imp_T        Act Group Data Imports   @02C*/
     /*     _MBPG_AGP_Data_Exp_T        Act Group Data Exports   @02C*/
  } _MBPG_Data_T;


/* Data returned by MATBPGM consists of this receiver headers followed by
 * one or more of the data headers above
 */

  typedef struct _MBPG_Receiver_T
  { /* Receiver Template - Must be aligned on a 16-byte boundary     */
     unsigned int     Template_Size;    /*!< Number of bytes provided  */
     unsigned int     Bytes_Used;       /*!< Number of bytes used      */
     char             reserved[8];
     _MBPG_Data_T     Data;             /*!< Materialized data         */
     /* For each bit on in _MBPG_Request_T (materialization          */
     /* options), this will contain the associated data. The         */
     /* individual materialization data templates must be manually   */
     /* mapped to this data space.                                   */
  } _MBPG_Receiver_T;

/* obtain pointer to an entry, which will be one of the types defined below */
#define GetEntry(TYPE, DATA)	((TYPE *)&(DATA->Entry))

/* Following are structures for the entry types that we request */

typedef struct _MBPG_Modules_T  { /*!< Bound Modules Information     */
     unsigned int     Size;             /*!< Length of materialization */
     unsigned int     Num_Mods;         /*!< # of modules bound in pgm */
     char             reserved1[8];
     struct { char    qualifier[30];    /*!< Module qualifier          */
              char    Name[30];         /*!< Module name               */
              char    reserved2[20];
     } Mod_Record[1];
     /* Use malloc to allocate records for each module bound into    */
     /* the bound program (given by Num_Mods).                       */
  } _MBPG_Modules_T;

/*! @brief
  \Platf OS400 only
*/
/* This is used as the structure for all of the module materialization options
 * as we are only concerned with the 'Size', common to all of the structures,
 * so we know how many bytes to return back to the signature generator.
 */
typedef struct _MMOD_Dict_T  {  /*!< Dictionary Component            */
     unsigned int     Size;             /*!< Length of component       */
     unsigned int     Version;          /*!< component version         */
     unsigned int     Num_Entries;      /*!< Dictionary definition     */
                                        /*!< table size.               */

     int              Unused;           /*!< Space available at end of */
                                        /*!< dict. definition table.   */
     char             reserved[4];
     char             Data;             /*!< First byte of dictionary  */
                                        /*!< definition table.         */
     /* Use malloc to allocate enough space to hold the dictionary   */
     /* definition table (ie. Size - Length of header is # bytes).   */
  } _MMOD_Dict_T;


/*! brief
  \Platf OS400 only
*/
  typedef struct _MBPG_Request_T {  /*!< Materialization Request Entry */
     _SPCPTR      Receiver;             /*!< Pointer to receiver       */
     int          General         : 1;  /*!< General bound program info*/
     int          Creat_Template  : 1;  /*!< Creation template         */
     int          Copyright       : 1;  /*!< Copyright strings         */
     int          Srvc_Pgms       : 1;  /*!< Bound service program info*/
     int          Modules         : 1;  /*!< Bound modules info        */
     int          String          : 1;  /*!< Program string directory  */
     int          Limits          : 1;  /*!< Program limits            */
     int          ProcOrd_RefExt  : 1;  /*!< Proc ord list ref ext @03C*/
     int          Obj_RefExt      : 1;  /*!< Obj list referential ext  */
     int          Sym_RefExt      : 1;  /*!< Sym list referential ext  */
     int          PrgExp_RefExt   : 1;  /*!< Prog export list ref ext  */
     int          SecAssoc_RefExt : 1;  /*!< Sec assoc spc lst ref ext */
     int          AG_Data_Imp     : 1;  /*!< Act. group data imports   */
     int          AG_Data_Exp     : 1;  /*!< Act. group data exports   */
     int          reserved3       : 2;
     /* Specific Bound Program Materialization Option:               */
     int          Bnd_Pgm_Info    : 1;  /*!< Materialize if program is */
                                        /*!< a NOT a service program.  */
     int          reserved4       : 7;
     /* Specific Bound Service Program Materialization Opts - Start  */
     int       reserved5          : 1;
     int       Signatures         : 1;  /*!< Signatures information    */
     int       Export_Proc        : 1;  /*!< Exported procedure info   */
     int       Export_Data        : 1;  /*!< Exported data information */
     int       ExpPgmProc_AliasSym: 1;  /*!< Exp prog proc alias symb  */
     int       ExpPgmData_AliasSym: 1;  /*!< Exp prog proc alias symb  */
     int       reserved6          : 2;
     /* Specific Bound Service Program Materialization Opts - End    */
     /* Bound Modules Materialization Options - Start                */
     int          Mod_General     : 1;  /*!< General module info       */
     int          Bind_Spec_Comp  : 1;  /*!< Binding spec component    */
     int          Mod_String      : 1;  /*!< Module string directory   */
     int          Dict_Comp       : 1;  /*!< Dictionary component      */
     int          Instr_Comp      : 1;  /*!< Instructions component    */
     int          Init_Comp       : 1;  /*!< Initialization component  */
     int          Alias_Comp      : 1;  /*!< Alias component           */
     int          Type_Inf_Comp   : 1;  /*!< Type info component       */
     int          Lit_Pool_Comp   : 1;  /*!< Literal pool component    */
     int          Mod_Creat_Tpl   : 1;  /*!< Module creation template  */
     int          Mod_Def_Info    : 1;  /*!< Module proc def'ns info   */
     int          reserved9a      : 7;
     int          Mod_Cpyw_Str    : 1;  /*!< Module copyright strings  */
     int          reserved9b      : 1;  /*!<                       @05C*/
     int          Mod_Creat_Tpl_Ext:1;  /*!< Module creation       @05C*/
                                        /*!< template extension    @05C*/
     int          Basic_Block_Comp: 1;  /*!< Basic block component @05C*/
     int          Fcn_Prot_Comp   : 1;  /*!< Function prototype    @06A*/
     int          Licopt_Comp     : 1;  /*!< LICOPT component      @0DA*/
     int          reserved9c      : 8;  /*!<                  @0DC @06C*/
     /* Bound Modules Materialization Options - End                  */
     unsigned int Mod_Num;              /*!< Module materialization #  */
     char         reserved10[4];
  } _MBPG_Request_T;

/*! @brief
  \Platf OS400 only
*/
  typedef struct _MBPG_Template_T
  { /* Materialization Request Template                              */
     unsigned int     Template_Size;    /*!< Number of bytes provided  */
     char             reserved1[4];
     unsigned int     Num_Requests;     /*!< Number of materialization */
                                        /*!< requests.                 */
     char             reserved2[4];
     _MBPG_Request_T  Request[1];       /*!< Materialization request(s)*/
     /* Varying length array of materialization requests. Use malloc */
     /* to allocate space for "Num_Requests" number of entries.      */
  } _MBPG_Template_T;


/*! @brief options used to request specific object data used in the signatures
   \Platf OS400 only
*/

/* Bound program materialization option that we will request from MATBPGM */
const unsigned int PgmOptions =

		0x08000000 ;		/* bound modules info */

/* Bound module materialization components that we will request from MATBPGM */
const unsigned int ModOptions =

		0x10000000 |		/* dictionary */
		0x08000000 |		/* instructions */
		0x04000000 |		/* initialization */
		0x02000000 |		/* alias */
		0x01000000 |		/* type info */
		0x00800000 |		/* literal pool */
		0x00000400 |		/* basic block */
		0x00000200 ;		/* function prototypes */


/* for requesting data on all modules */
const unsigned int AllModules = 0;


#ifdef __ILEC400__
  #pragma linkage (_MATBPGM, builtin)
#else
  extern "builtin"
#endif

  /*!
    \Platf OS400 only
  */
void _MATBPGM (_MBPG_Template_T *, _SYSPTR *bound_pgm);

/*! @brief
  \Platf OS400 only
*/
typedef struct _PROGRAM_T
{
    _SYSPTR sysptr;
    _MBPG_Receiver_T *receiver;  /* pointer to receiver data */
	_MBPG_Data_T *next_data;     /* next data header in receiver data */
    int NumMods;
    int TotalLength; 
#if defined(ICCREAD) || defined(DEBUG_NMI)
	char name[36];					/* name as library/name.type */
	#define MAXMODNUM	500
	unsigned char mods[MAXMODNUM+1];/* keep track of which modules were read */
#endif
} _PROGRAM_T;

_MBPG_Receiver_T* ProgramRequest(_PROGRAM_T* pProgram, unsigned int modnum, unsigned int modopts, unsigned int pgmopts);
_MBPG_Data_T* NextReceiverData(_PROGRAM_T *pgm);

#define ReceiverDataValid(DATA)		((DATA)->Present && !(DATA)->Partial)

#define GetEntry(TYPE, DATA)	((TYPE *)&(DATA->Entry))

/*
** NOTE: The following function runs the CHKOBJITG command which requires special *AUDIT
** authority.  Since the caller of ICC (e.g. QNOTES) might not have the special authority,
** this code spawns the ICC400 program to run the actual CHKOBJITG command.  ICC400 has
** QSYS as the owner and USRPRF(*OWNER).
*/ 
/*! 
  @brief Validate the crypto libraries on OS400 by calling ICC400 program
  @param library the ICC shared library name
  @param pgmname name of the program
  @param pgmtype type of the program (pgm or srvpgm)
  @return 0 if validated O.K, 1 validation failed, 2 bad args passed to pgm, 3 system call in pgm failed,
	 4 bad input args, 5 bad environ, 6 spawn failure, 7 wait error, 8 unknown child status, 9 unknown spawn error
  \Platf OS400 only
*/

extern char **environ;		/* Pointer to environment variable array */

int ICC_Verify(char* library, char* pgmname, char *pgmtype)
{
	int pgm_rc = -1;
	pid_t child_pid;
	int wt_stat_loc;
	char *Argv[5];
	struct inheritance inherit={0};
	char pgm_path[256];

    if ((library == NULL) || (pgmname == NULL) || (pgmtype == NULL))
    {
		return 4;		/* invalid input parameters */
    }

	#pragma exception_handler(VerifyErr, 0, 0, _C2_ALL, _CTLA_HANDLE_NO_MSG)

	/* Set argument array for spawn(). */
	sprintf(pgm_path, "%s%s", ICC_LIB_PATH, ICC_ICC400_LOC);
	Argv[0] = pgm_path;
	Argv[1] = library;
	Argv[2] = pgmname;
	Argv[3] = pgmtype;
	Argv[4] = NULL;

	/* Initialize environ pointer if not already set */
	if (environ == NULL && Qp0zInitEnv() == -1)
		return 5;	/* bad env init */

	/* Spawn to the child program that will run the CHKOBJITG command */
	if ((child_pid=spawn(Argv[0], 0, NULL, &inherit, Argv, environ)) < 0)
	{
		return 6;	/* spawn failed */
	}

	/* Wait for the child process to end. */
	while(1)
	{
		if (waitpid(child_pid, &wt_stat_loc, 0) < 0)
		{
			/* For EINTR, we want to go back onto the waitpid() */
			if (errno == EINTR)
			{
				continue;
			}
			return 7;	/* wait failed */
		}
		/* If the child process is stopped, go back on the wait */
		if (WIFSTOPPED(wt_stat_loc))
		{
			continue;
		}
		/* successful wait, break out of loop */
		break;
	}

	/* Positive validation status if child exited and returned 0 */
	if (WIFEXITED(wt_stat_loc))
	{
		pgm_rc = WEXITSTATUS(wt_stat_loc);

		/*	0 = validation passed
			1 = validation failed
			2 = bad args received
			3 = system call failed
		 */

		return pgm_rc;	
	}
	return 8;	/* unknown child status */
	#pragma disable_handler

	VerifyErr:
	return 9;	/* unknown spawn errors */
}


/*! @brief return header for next materialized data entry
  \Platf OS400 only
*/
_MBPG_Data_T* NextReceiverData(_PROGRAM_T *pgm)
{
	_MBPG_Data_T *data;

	if (pgm == NULL || pgm->next_data == NULL)
		return NULL;

	data = pgm->next_data;

	if (data->Off_To_Next == 0)
		pgm->next_data = NULL;
	else
		pgm->next_data = (_MBPG_Data_T*)((char *)pgm->next_data + data->Off_To_Next);

	return data;
}
	

/*! @brief needs documentation
   @param pProgram unknown
   @param modnum unknown
   @param modopts unknown
   @param pgmopts unknown
   @return unknown unknown
  \Platf OS400 only
*/
_MBPG_Receiver_T* ProgramRequest(_PROGRAM_T* pProgram, unsigned int modnum, unsigned int modopts, unsigned int pgmopts)
{
    _MBPG_Receiver_T temp;
    _MBPG_Receiver_T* receiver= NULL;
    _MBPG_Request_T* request = NULL;
    unsigned int* optptr = NULL;
    _MBPG_Receiver_T* result= NULL;

    /* Set up material request template */
    _MBPG_Template_T matReqTemplate;
    memset(&matReqTemplate, 0, sizeof(_MBPG_Template_T));
    matReqTemplate.Template_Size = sizeof(_MBPG_Template_T);
    matReqTemplate.Num_Requests = 1;

    /* Setup a temporary receiver so we can figure out the size of the request */
    receiver = &temp;
    memset((void*) receiver, 0, sizeof(_MBPG_Receiver_T));  
    receiver->Template_Size = 8;

    request = matReqTemplate.Request;
    request->Receiver = receiver;
    optptr = (unsigned int *)((char *)request + sizeof(receiver));
    *optptr++ = pgmopts;
    *optptr = modopts;
    request->Mod_Num = modnum;
#pragma exception_handler(MatErr, 0, 0, _C2_MH_ESCAPE, _CTLA_HANDLE_NO_MSG)
    _MATBPGM(&matReqTemplate, &pProgram->sysptr);
#pragma disable_handler

    /* We know the size so create a receiver to hold the results */
    result = (_MBPG_Receiver_T*) ICC_Malloc(receiver->Bytes_Used,__FILE__,__LINE__);
    memset(result, 0, receiver->Bytes_Used);
    result->Template_Size = receiver->Bytes_Used;
    matReqTemplate.Request[0].Receiver = result;
#pragma exception_handler(MatErr, 0, 0, _C2_MH_ESCAPE, _CTLA_HANDLE_NO_MSG)
    _MATBPGM(&matReqTemplate, &pProgram->sysptr);
#pragma disable_handler
    return result;

MatErr:
    if (result) ICC_Free(result);
    return(0);
}


/*! @brief read program data and prepare to make it available for signature validation
   @param pRead the data buffer
   @param pathfilename the path to the file to verify
   @return  0 -> successful
   \Platf OS400 only
 */
int init_read_pgm(READ_PGM_T * pRead, char * pathfilename)
{
	int rc = 0; 
	_SYSPTR pgm_ptr=NULL;
	_PROGRAM_T *pProgram;
	_MBPG_Data_T *data;
	_MBPG_Receiver_T* receiver = NULL;
	_MBPG_Modules_T* modules = NULL;
	char library[11];
	char pgmname[11];
	char pgmtype[11];
	char name[256];

	if (pRead == NULL)
		return -1;

	get_pgm_ptr(&pgm_ptr, pathfilename, library, pgmname, pgmtype);

	/* srvpgm (like library) can never be NULL */
	if ((pgm_ptr == NULL) || (*library == '\0') )
		return -1;

#if defined(ICCREAD) || defined(DEBUG_NMI)
	sprintf(name, "%s/%s.%s", library, pgmname, pgmtype);
	printf("init_read_pgm: beginning read of %s\n", name);
#endif

#ifdef ICCREAD
	/* skip this validation in iccread build utility */
#else
	/* use CHKOBJITG to see if the object has been tampered with */
	rc = ICC_Verify(library, pgmname, pgmtype);
	if (rc != 0) {
		/* value of rc will indicate the failure reason: see ICC_Verify */
		return rc;
	}
#endif

	pProgram = (_PROGRAM_T *) ICC_Malloc(sizeof(_PROGRAM_T),__FILE__,__LINE__);
	if (pProgram == NULL)
		goto InitReadError;

	memset(pProgram, 0, sizeof(_PROGRAM_T));
	pProgram->sysptr = pgm_ptr;	/* save the pgm ptr for later use */

#if defined(ICCREAD) || defined(DEBUG_NMI)
	strncpy(pProgram->name, name, sizeof(pProgram->name)-2);
#endif

	/* request all of the desired observability data on this object */
	receiver = (_MBPG_Receiver_T *) ProgramRequest(pProgram, AllModules, ModOptions, PgmOptions);
	if (receiver == NULL)
		goto InitReadError;

	pProgram->receiver = receiver;

	/* init the data pointer to the first materialization data entry */
	pProgram->next_data = &pProgram->receiver->Data;

	/* first entry should be the bound modules info, not used in the signature
	 * used only to record the number of modules, useful for debug
	 */
	data = NextReceiverData(pProgram);
	if (data == NULL || data->Pgm_Mat_Id != PgmOptions || !ReceiverDataValid(data))
		goto InitReadError;

	modules = GetEntry(_MBPG_Modules_T, data);
	pProgram->NumMods = modules->Num_Mods;

	pRead->pPgm = pProgram;

	return 0;

	InitReadError:
	if (receiver != NULL)
		ICC_Free(receiver);
	if (pProgram != NULL)
		ICC_Free(pProgram);

	return -1;
}


/*! @brief Return block of data for the next module component in the materialize data
   @param pRead unknown
   @param ppData unknown
   @param pLen unknown
  \Platf OS400 only
*/
int read_pgm(READ_PGM_T * pRead, void ** ppData, int *pLen  )
{
	int		rc = 0;
	_PROGRAM_T	*pProgram;
	_MBPG_Data_T *data;
	_MMOD_Dict_T *mod_data;
	void	*rcv = NULL;
	int		outLen = 0;

	*ppData = NULL;

	if (pLen)
		*pLen = 0;

	if (pRead == NULL)
		return -1;

	pProgram = (_PROGRAM_T *) pRead->pPgm;

	/* get and return next block of module data */
	do {

		data = NextReceiverData(pProgram);

#if defined(ICCREAD) || defined(DEBUG_NMI)
		/* having no data for a module for some materialization types is normal,
		 * but not partial data or an invalid request */
		if (data != NULL && data->Partial)
			printf("read_pgm: WARNING partial data returned for module %d, id 0x%x\n", data->Mod_Num, data->Mod_Mat_Id);
		else if (data != NULL && !data->Valid)
			printf("read_pgm: WARNING data request not valid for module %d, id 0x%x\n", data->Mod_Num, data->Mod_Mat_Id);
#endif

	} while (data != NULL && !ReceiverDataValid(data));

	if (data == NULL)
		return 0;		/* no more data */

#if defined(ICCREAD) || defined(DEBUG_NMI)
	/* sanity checks and debug */
	if (data->Pgm_Mat_Id != 0 || data->Mod_Mat_Id == 0)
		printf("read_pgm: WARNING module %d, unexpected ids: pgm = 0x%x mod = 0x%x\n", data->Mod_Num, data->Pgm_Mat_Id, data->Mod_Mat_Id);

	if (data->Mod_Num > MAXMODNUM)
		printf("read_pgm: WARNING module %d exceeds MAXMODNUM %d\n", data->Mod_Num, MAXMODNUM);
	else if (pProgram->mods[data->Mod_Num] == 0xff)
		printf("read_pgm: WARNING module %d data received more than 256 times\n", data->Mod_Num);
	else
		pProgram->mods[data->Mod_Num]++;
#endif

	mod_data = GetEntry(_MMOD_Dict_T, data);

	*ppData = (void *)mod_data;
	if (pLen)
		*pLen = mod_data->Size;

	pProgram->TotalLength += mod_data->Size;

	return 0;
}

/*! @brief Free memory allocated by init_read_pgm
   @param pRead unknown
   \Platf OS400 only
*/
void term_read_pgm(READ_PGM_T * pRead)
{
	_PROGRAM_T	*pProgram;

	pProgram = (_PROGRAM_T *)pRead->pPgm;

	if (pProgram){

#if defined(ICCREAD) || defined(DEBUG_NMI)
		int i;
		unsigned int mods_read = 0;
		unsigned int data_entries = 0;

		printf("term_read_pgm: end read of %s\n", pProgram->name);
		printf("term_read_pgm: Number of modules = %d, Total Length = %d\n", pProgram->NumMods, pProgram->TotalLength);

		for (i = 1; i <= MAXMODNUM; i++)
		{
			if (pProgram->mods[i])
			{
				mods_read++;
				data_entries += pProgram->mods[i];
			}
			if (pProgram->mods[i] == 0 && i <= pProgram->NumMods)
				printf("term_read_pgm: WARNING no data for module %d\n", i);
			if (pProgram->mods[i] != 0 && i > pProgram->NumMods)
				printf("term_read_pgm: WARNING out of range module data %d\n", i);
		}

		printf("term_read_pgm: Modules read = %d, number of entries = %d\n", mods_read, data_entries);
#endif

		if (pProgram->receiver)
			ICC_Free(pProgram->receiver);

		ICC_Free(pProgram);
	}

}

#endif
