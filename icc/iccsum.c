/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: Source file for the icc static library
//
*************************************************************************/

/*
  Program to checksum ICC sources and binaries.
  The source checksumming is a bit tricky - if we use
  CMVC to do extracts, macro's in the sources get expanded
  at extract - changing the checksum - 

  We need to be able to skip the CMVC headers in the source
  CONSISTANTLY or we'll always get checksum failures.

  That's one reason we don't use OS native programs,
  the other is that we'd like to use SHA(n) rather than the weaker MD5
  and that's available only on a few platforms.
*/
#include <stdio.h>

#include <ctype.h>
#include <string.h>
#include <assert.h>

#include <stdlib.h>

#if defined(__linux__)
#include <sys/types.h>
#include <regex.h>
#endif

#if defined(_solaris_) || defined(_aix_)
#include <re_comp.h>
#endif
#include "openssl/crypto.h"
#include "openssl/evp.h"


char **explode(char *explodeme, char *explodeon);
char *implode(char **exploded,char implodec);
void explodefree(char **exploded);
int explodesize(char **exploded);
void explode_dump(char **exp); /* debug print */

/* Template for our parser
   char c ,  next char to process
   int *istate , place to save internal state
   int *fixup , character offset to actual start/end of comment from where we are
*/

typedef int (*FHP)(char c,int *instate,int *fixup);
typedef int (*PFI)();

int C_parser(char c,int *instate,int *fixup);
int CPP_parser(char c,int *instate,int *fixup);
int Shell_parser(char c,int *instate,int *fixup);
int Asm_parser(char c,int *instate,int *fixup);
static void fail(char *test, char *input,char *expected, char *got);


typedef struct {
  char *lang;
  FHP handler;
} COMMENTTYPES;

typedef struct {
  char *lang;
  char *template;
  char *parsers;
  char **parserlist;   /* generated at startup */
  char **templatelist; /* generated at startup */
} LANGTYPES;


typedef struct {
  char *fname;      /*!< Filename, if non-NULL parsers must match , if NULL  parsers are used directly */ 
  char *parsers;    /*!< list of parsers we should test with */		      
  const char *reftext;    /*!< The reference text */
  long sot;          /*!< Which byte we should see the start of comment at */
  long eot;          /*!< Which byte we should see the end of comment at */
  FHP refhandler;   /*!< Which parser SHOULD see the hit */
  int pass_fail;    /*!< Whether this text should pass or fail */
} TESTDATA;         /*!< Parser test data structure */

typedef struct {
  char *fname;      /*!< The input filename */
  char *expected;   /*!< The file type we expect to see */
} NAMETESTDATA;
  

COMMENTTYPES commenttypes[] = {
  {"C",C_parser},
  {"C++",CPP_parser},
  {"Shell",Shell_parser},
  {"Asm",Asm_parser},
  {NULL,NULL}
};

typedef struct {
  char *ptype;   /*!< type of parser we use */
  FHP handler;   /*!< Pointer to parser function */
  long soc;       /*!< Place to save position of start of comment */
  long eoc;       /*!< Place to save position of end of comment */
  int istate;    /*!< Private state of parser function */
  int state;     /*!< Public state of parser function */
} PARSEINFO;

LANGTYPES langtypes[] = {
  {"Java","[.][jJ][aA][vV][aA]$","C++ C"},
  {"C++","[.][CH]$ [.][cChH][pP][pP]$ [.][cC][cC]$ [.][hH][hH]$","C++ C"},
  {"C","[.][ch]$","C"},
  {"Make","[Mm]akefile gnumakefile [.][mM][kK]$ [.]mak$","Shell"},
  {"Shell","[.]sh$ [.]ksh$ [.]bash$","Shell"},
  {"Perl","[.][pP][lL]$ [.][pP][eE][rR][lL]$","Shell"},
  {"Asm","[.][sS]$ [.]asm$","Asm"},
  {NULL,NULL,NULL}
};

void usage(char *whoami,char *where)
{
  int i;
  static const char sformat[] = "%-9s %-60s %s\n";
  if(where) printf("Error: %s\n",where);
  printf("Usage: %s [-s] [-t] filename\n",whoami);
  printf("       If \"-s\" is specified, the file being checked is assumed\n");
  printf("       to be version controlled source with version tags in the\n");
  printf("       first header block.\n");
  printf("       We attempt to determine the type of comment to look for from\n");
  printf("       the file type. We recognize the following\n");
  printf(sformat,"Language","File template","Parsed as");
  printf(sformat,"========","=============","=========");
  for(i = 0; langtypes[i].lang != NULL; i++) {
    printf(sformat,
	   langtypes[i].lang,
	   langtypes[i].template,
	   langtypes[i].parsers
	   );
  }
  printf("If you have problems with file recognition, try renaming the file.\n");
  printf("       Specifying \"-t\" will run internal self tests on the\n");
  printf("       filetype recognition, parsing and hash code\n");  
  
}

int TEST_names(void);
int TEST_Self(void);

void mklists(void);
void clnlists(void);
long getComment(char *fname, PFI cb,PARSEINFO **handler);

FILE * in = NULL;
int mygetc() {
  if(in == NULL) return -1;
  return(fgetc(in));
}    
int main(int argc, char *argv[])
{
  char *fname = NULL;
  int i;
  int is_source = 0;
  int rv = 0;
  long eoc;
  char buffer[4096];
  char md_buf[256];
  int len;
  EVP_MD_CTX *md_ctx;
  EVP_MD *md;

  mklists();
  TEST_names();

  if(argc < 2) {
    usage(argv[0],"Input file name required.\n");
    exit(1);
  }
 
  for(i = 1; i < argc; i++) {
    if( strcmp("-s",argv[i]) == 0) {
      is_source = 1;
    } else if( strcmp("-t",argv[i]) == 0) {
      if( TEST_Self() != 0) {
	fprintf(stderr,"Internal self tests failed.\n");
      } else {
	fprintf(stderr,"Internal self tests passed.\n");
      }
    } else {      
      fname = argv[i];
      in = fopen(fname,"r");
      if(in == NULL) {
	usage(argv[0],"Could not open file\n");
	exit(1);
      }
      eoc = getComment(fname,mygetc,NULL);
      fseek(in,eoc,SEEK_SET);
      OpenSSL_add_all_algorithms();
      md = EVP_get_digestbyname("SHA1");
      md_ctx = EVP_MD_CTX_create();
      if( md && md_ctx ) {

	EVP_DigestInit(md_ctx,md);
	while( (len = fread(buffer,1,4096,in)) > 0) {
	  EVP_DigestUpdate(md_ctx,buffer,len);
	}
	len = 0;
	EVP_DigestFinal(md_ctx,md_buf,&len);
 
	for(i = 0; i < len; i++) {
	  printf("%02x",md_buf[i] & 0xff);
	}
	
	printf("  %s\n",fname);
      } else {
      }
      fclose(in);
      in = NULL;
    }
  }
  clnlists();
  return rv;
}
/*!
  @brief Guess the file type from a file name 
  @param fname input file name
  @return the file type "Java" "C" "C++" "Shell" "Asm", or NULL for unclassified - a valid return.
*/
char *Ftype(char *fname)
{
  char *rv = NULL;
  int i,j;
  for(i = 0;langtypes[i].lang != NULL ; i++ ) { 
    for(j = 0; langtypes[i].templatelist[j] != NULL ; j++) {
      /*      printf("%s with [%s]\n",fname,langtypes[i].templatelist[j]); */
      re_comp(langtypes[i].templatelist[j]);
      if(re_exec(fname) == 1) {
	rv = langtypes[i].lang;
	break;
      }
    }
  }
  return rv;
}
/*!
  @brief select the parser to use for a given file type
  @param ftype the filetype - as returned from "Ftype()"
  @return a pointer to a LANGTYPES structure or NULL if no match
*/
LANGTYPES * LangType(char *ftype) 
{
  LANGTYPES *rv = NULL;
  int i;
  for(i = 0; langtypes[i].lang != NULL; i++) {
    if(strcmp(ftype,langtypes[i].lang) == 0) {
      rv = &langtypes[i];
      break;
    }
  }
  return rv;
}
/*!
  @brief returns a pointer to a parse function given a parser name
  @param parsername "C","C++","Shell","Asm"
  @return a pointer to a parser function or NULL
*/
FHP ParseFunction(char * parsername)
{
  FHP rv = NULL;
  int i;
  for(i = 0; commenttypes[i].lang != NULL; i++) {
    if(strcmp(parsername,commenttypes[i].lang) == 0) {
      rv = commenttypes[i].handler;
      break;
    }
  }
  return rv;
}
/*!
  @brief given the filename and a callback which gets one character
  finds the start and end position of the first comment in a file. 
  @param fname Filename
  @param cb pointer to a function returning a int, either a character or -1 (EOF)
  @param handler a list of parsers to select from
  @return end position
*/
long getComment(char *fname, PFI cb,PARSEINFO **handler)
{
  char *ftype = NULL;
  char ** plist;
  LANGTYPES *langtype = NULL;
  static PARSEINFO handlers[10];
  int active = -1; 
  long pos,spos;
  int done;
  int fixup;
  char ref[2] = {0,0};
  int i,j;
  long en = 0;

  int rv = 0;

  int c;

  /* do da business */
  ftype = Ftype(fname);
  if(ftype == NULL) { /* can't type it, treat it as binary */
    spos = 0;
    
  } else {
    langtype = LangType(ftype);
    if(langtype == NULL) {
      fail("PARSER.select_parser",fname,
	   "Unknown type","NULL");
      spos = 0;
    } else {
      
      plist = langtype->parserlist;
      for(j = 0; plist[j] != NULL && j < 9 ; j++) {
	handlers[j].handler = ParseFunction(plist[j]);
	handlers[j].ptype = plist[j];
	handlers[j].soc = 
	  handlers[j].eoc = 
	  handlers[j].istate = 
	  handlers[j].state = 0;      
	/* printf("Parser type %s found\n",handlers[j].ptype); */
	if(handlers[j].handler == NULL) {
	  fail("PARSER.get_handler",fname,plist[j],"NULL handler");	      
	}
      }
      handlers[j].handler = NULL;
      /* Now go see which parser finds a comment first ! */
      if(handlers[0].handler == NULL) {
	fail("PARSER.get_handler",fname,plist[j],"NULL handler");
	rv++;
      }
     
     
      pos = 0;
      while( ((c = (*cb)() ) >=0 ) &&  done == 0 ) {
	if(active < 0) { /* Until one of the possible comment parsers gets a hit ... */
	  for(j = 0; handlers[j].handler != NULL && done == 0; j++) {
	    handlers[j].state = (*handlers[j].handler)(c,&handlers[j].istate,&fixup);
	    if (handlers[j].state == 1)  {/* Found a comment start */
	      active = j;
	      handlers[j].soc = pos + fixup;
	      /* printf("Parser type %s hit\n",handlers[j].ptype); */	      
	      break;
	    }
	  }     
	} else { /* One handler got a hit, state should never be 0 or 1 again ... */
	  handlers[active].state = 
	    (*handlers[active].handler)(c,&handlers[active].istate,&fixup);
	  switch(handlers[active].state) {
	  case 0: /* Looking for comment start */
	  case 1: /* Got comment start */
	    ref[0] = handlers[active].state+'0';
	    fail("PARSER.parse",fname,"State 2 or 3",ref);
	    rv++;
	    break;
	  case 2: /* in comment */
	    break;
	  case 3: /* end of comment */
	    en = handlers[active].eoc = pos + fixup + 1;
	    done = 1;
	    break;
	  }
	}
	pos++;
      }	  
    }
  }

  if(handler != NULL) {
    if(active >= 0) {
      *handler = &handlers[active];
    } else {
      *handler = NULL;
    }
  }
  return en;
}
/*! 
  @brief Explode the readable strings of components into "lists" that are easier to iterate
*/ 
void mklists()
{
  int i;
  for(i = 0; langtypes[i].lang != NULL; i++) {
    langtypes[i].parserlist = explode(langtypes[i].parsers," ");
    /* explode_dump(langtypes[i].parserlist); */
    langtypes[i].templatelist = explode(langtypes[i].template," ");
    /*explode_dump(langtypes[i].templatelist);*/
  }
}
/*!
  @brief Cleanup the lists generated by the explode operation we performed at startup
*/
void clnlists()
{
  int i;
  for(i = 0; langtypes[i].lang != NULL; i++) {
    explodefree(langtypes[i].parserlist);
    explodefree(langtypes[i].templatelist);
  }
}
/* The parsers:

   What the parser does is scan the input character by character
   and return an operational state as a return value.
   We may wish to run these suckers in parallel remember - we could
   have C OR C++ comments in the same file - first hit wins the race.   

   We save internal state via a user supplied integer pointer so
   it'd be possible to make it threaded later. That's different from
   the return value BTW.

   States: 0 - I havn't found anything I'm interested in yet
   1 - I've found the start of a comment
   2 - I'm inside a comment block
   3 - I've found the end of a comment
   0 - I'm back to looking for comments again.

   Transitions: 0->1->2->3->0 are the only allowed transitions

*/   

/*
  Look for C style comments
*/
int C_parser(char c, int *istate,int *fixup) 
{
  /* Internal states
     0 = looking
     1 = found '/'
     2 = inside comment
     3 = inside comment, found '*'
  */   
  int rv = 0;
  *fixup = 0;
  switch(*istate) {
  default:
  case 0: /* looking for start of comment */
    if( c == '/' ) {
      *istate = 1;
    }
    break;
  case 1: /* Found '/' */
    if( c == '*' ) {
      *istate = 2;
      *fixup = -1;
      rv = 1;
    } else {
      *istate = 0;
    }
    break;
  case 2: /* inside comment */
    rv = 2;
    if( c == '*') {
      *istate = 3;
    }
    break; 
  case 3: /* inside comment, last char was '*' */
    if( c == '/') {
      *istate = 0;
      rv = 3;
    } else {
      *istate = 2;
      rv = 2;
    }
    break;
  }
  return rv;
}

/*
  Look for C++ style comments
*/

int CPP_parser(char c, int *istate, int *fixup) 
{
  /* Internal states
     0 = looking
     1 = found '/'
     2 = inside comment
     3 = new line, waiting for new '/'
     4 = new line, waiting for '//'
  */   
  int rv = 0;
  *fixup = 0;
  switch(*istate) {
  default:
  case 0: /* looking for start of comment */
    *istate = 0;
    if( c == '/' ) {
      *istate = 1;
    }
    break;
  case 1: /* Found '/' */
    if( c == '/' ) {
      *istate = 2;
      *fixup = -1;
      rv = 1;
    } else {
      *istate = 0;
    }
    break;
  case 2: /* inside comment */
    rv = 2;
    if( c == '\n') {
      *istate = 3;
    }
    break; 
  case 3: /* We finished the last line, do we have another comment maybe ? */
    if(isspace(c)) {
      rv = 2;
    } else if( c == '/') {
      *istate = 4;
      rv = 2;
    } else {
      rv = 3;
      *istate = 0;
    }
    break;
  case 4: /* We finished the last line, '/' */
    if( c == '/') {
      rv = 2;
      *istate = 2;
    } else {
      rv =3;
      *fixup = -1;
      *istate = 0;
    }
    break;
  }
  return rv;
}
/*
  Look for Asm style comments
*/

int Asm_parser(char c, int *istate,int *fixup) 
{
  /* Internal states
     0 = looking
     1 = found ';'
     2 = inside comment
     3 = new line, waiting for new ';'
  */   
  int rv = 0;
  *fixup = 0;
  switch(*istate) {
  default:
  case 0: /* looking for start of comment */
    *istate = 0;
    if( c == ';' ) {
      *istate = 1;
      rv = 1;
    }
    break;
  case 1: /* Found '/' */
    if( c == '\n' ) {
      *istate = 3;
      rv = 2;
    } else {
      *istate = 2;
      rv = 2;
    }
    break;
  case 2: /* inside comment */
    rv = 2;
    if( c == '\n') {
      *istate = 3;
    }
    break; 
  case 3: /* We finished the last line, do we have another comment maybe ? */
    if(isspace(c)) {
      rv = 2;
    } else if( c == ';') {
      *istate = 2;
      rv = 2;
    } else {
      rv = 3;
      *fixup = -1;
      *istate = 0;
    }
    break;
  }
  return rv;
}
/*
  Look for Shell style comments
*/

int Shell_parser(char c, int *istate,int *fixup) 
{
  /* Internal states
     0 = looking
     1 = found '#'
     2 = inside comment
     3 = new line, waiting for new '#'
  */   
  int rv = 0;
  *fixup = 0;
  switch(*istate) {
  default:
  case 0: /* looking for start of comment */
    *istate = 0;
    if( c == '#' ) {
      *istate = 1;
      rv = 1; /* Comment marker */
    }
    break;
  case 1: /* Found '/' */
    rv  = 2;
    if( c == '\n' ) {
      *istate = 3;
    } else {
      *istate = 2;
    }
    break;
  case 2: /* inside comment */
    rv = 2;
    if( c == '\n') {
      *istate = 3;
    }
    break; 
  case 3: /* We finished the last line, do we have another comment maybe ? */
    if(isspace(c)) {
      rv = 2;
    } else if( c == '#') {
      *istate = 2;
      rv = 2;
    } else {
      *fixup = -1;
      rv = 3;
      *istate = 0;
    }
    break;
  }
  return rv;
}
static const char t1[] = "/*\\ndskkhdshdskhds*dsdjkdj/*\n\n*\n\\*/int main() { \n/* *=/\nreturn 0;\n}\n";
static const char t2[] = "//*\\ndskkhdshdskhds*dsdjkdj/*\n\n*\nint main() { \n/* */\nreturn 0;\n}\n";
static const char t3[] = "//*************/\n//   xyz\n   // xyz */\nint main { \n/* */\nreturn 0;\n}\n";
static const char t4[] = "#!/bin/sh\n\n\n# Date: Time: #\n    # comment\nset x = `uname`\n# We have the OS\n\n";
static const char t5[] = "#!/bin/perl\n  \n\n# Date: Time: #\n    # comment\nset x = `uname`\n# We have the OS\n\n";
static const char t6[] = "\n\n;\n; Date: Time :; Date and time stamps\n;\n\tmulx\n\taddi ax,bx ; also a comment\n\n";

TESTDATA testdata[] = {
  {"test0.c","C",t1,0,35,C_parser,0},
  {"test1.C","C++ C",t1,0,35,C_parser,0},
  {"test2.CPP","C++ C",t1,0,35,C_parser,0},
  {"test3.C","C++ C",t2,0,32,CPP_parser,0},
  {"Cpp5.cpp","C++ C",t3,0,40,CPP_parser,0},
  {"a.sh","Shell",t4,0,42,Shell_parser,0},
  {"b.pl","Shell",t5,0,46,Shell_parser,0},
  {"a.s","Asm",t6,2,44,Asm_parser,0},
  {NULL,NULL,NULL,0,0,NULL,0},
};

NAMETESTDATA nametests [] = {
  {"a.java","Java"},
  {"a.Java","Java"},
  {"b.C","C++"},
  {"abCdef.c","C"},
  {"abraCaDabra.asm","Asm"},
  {"abracadabra.S","Asm"},
  {"abasjjkdsd_.s","Asm"},
  {"ashahsgd.HPP","C++"},
  {"jhddffhd.hpp","C++"},
  {"dhse2399.h","C"},
  {"Makefile","Make"},
  {"Make_file",NULL},
  {"xyz.txt",NULL},
  {"xzy.mk","Make"},
  {"abCPP.mak","Make"},
  {"CPP.sh","Shell"},
  {"sh.pl","Perl"},
  {"xyz.pli",NULL},
  {"C.asm","Asm"},
  {"C.sh","Shell"},
  {"Crap.shit",NULL},
  {"CPP.ksh","Shell"},
  {"xyz.bash","Shell"},
  {"Textfile",NULL},
  {"log",NULL},
  {"a.out",NULL},
  {"cpp.obj",NULL},
  {"thing.exe",NULL},
  {"ThingCPP.lib",NULL},
  {"Thing.dll",NULL},
  {"thing_.o",NULL},
  {NULL,NULL},
};

static void fail(char *test, char *input,char *expected, char *got)
{
  fprintf(stderr,"Test %s failure: Input [%s], Expected \"%s\", actual \"%s\"\n",
	  test,input,expected,(got != NULL) ? got: "NULL");
}
int TEST_names() 
{
  int rv = 0;
  int i;
  char *got;
  for(i = 0; nametests[i].fname != NULL; i++) {
    got = Ftype(nametests[i].fname);
    if( got == NULL && nametests[i].expected == NULL) continue;
    if( (got == NULL && nametests[i].expected != NULL) || 
	(nametests[i].expected == NULL && got != NULL) || 
	strcmp(got, nametests[i].expected) != 0 ) {
      fail("FileName",nametests[i].fname,nametests[i].expected,got);
      rv ++;
    }
  }
  return rv;
}
/* Not thread safe, that's cool, we aren't a threaded program ... */
static const char *tstring = NULL;

/*!
  @brief Callback function for parsing test data 
  @return end of comment offset
*/
int t_callback() {
  int rv;
  if(tstring == NULL || *tstring == '\0') {
    rv = -1;
  }
  else {
    rv = *tstring++;
  }
  return rv;
}

int TEST_parsing() 
{
  int rv = 0;
  int i,j;
  PARSEINFO *handler;
  
  for(i = 0; testdata[i].reftext != NULL; i++) {
    tstring = testdata[i].reftext;
    getComment(testdata[i].fname,t_callback,&handler);
    /* Now check our results */
    if(  handler != NULL && (testdata[i].pass_fail != 1) ) {
      fail("PARSER.parse",testdata[i].fname,"Expected to find comment","None found");
      rv++;
      continue;
    }
    if(handler != NULL && (testdata[i].pass_fail != 0) ) {
      fail("PARSER.parse",testdata[i].fname,"Expected to find comment","None found");
      rv++;
      continue;
    }
    if(handler != NULL && (handler->handler != testdata[i].refhandler)) {
      fail("PARSER.handler.select",testdata[i].fname,testdata[i].parsers,"Wrong handler");      
      rv++;
    }
    if(handler != NULL && (handler->soc != testdata[i].sot)) {
      char tmp[10];
      char tmp1[10];
      int l;
      l = strlen(testdata[i].reftext);
      if(handler->soc < l) {
	strncpy(tmp,testdata[i].reftext+handler->soc,9);
      } else strncpy(tmp,">EOT",9);
      strncpy(tmp1,testdata[i].reftext+testdata[i].sot,9);
      fail("PARSER.parse.start",testdata[i].fname,tmp1,tmp);
      rv++;
    }
    if(handler != NULL && (handler->eoc != testdata[i].eot)) {
      char tmp[10];
      char tmp1[10];
      int l;
      l = strlen(testdata[i].reftext);
      if(handler->eoc < l) {
	strncpy(tmp,testdata[i].reftext+handler->eoc,9);
      } else strncpy(tmp,">EOT",9);
      strncpy(tmp1,testdata[i].reftext+testdata[i].eot,9);
      fail("PARSER.parse.end",testdata[i].fname,tmp1,tmp );
      rv++;
    }
  }
  return rv;
}
int TEST_Self()
{
  return ( 
	  TEST_names() ||
	  TEST_parsing()
	  );
}
/* ===================== included Explode/Implode code ==================== */

static char *strCcpy(char *str);


/*
 * string copy with Create space
 */
static char *strCcpy(char *str)
{
  char *s;
  int i;
  i = strlen(str) + 1;
  s = (char *)malloc(i);
  assert(s != NULL);    
  strcnpy(s,str,i);
  s[i] = 0
  return s;
}

/*
  diagnostic for explode(), print the split out strings
*/

void explode_dump(char **exp)
{
  int i;
  if(exp == NULL) printf("(null)\n");
  else {
    for(i = 0; i < explodesize(exp); i++) {
      printf("{%s} ",exp[i] ? exp[i] : "(null)");
    }
    printf("\n");
  }
}


int explodesize(char *exp[])
{
  int i;
  if(!exp) return 0;
  for(i = 0;exp[i] != NULL; i++);
  return i;
}
/* 
   C version of explode, note that it's up to the caller to free the 
   scratch buffer i.e.   explodefree(returnvalue);

   This ISN'T efficient, but it is effective. 
 
   Note that strings in the explode'd arrays are replaceable, BUT make
   sure the storage is malloc'd (strCcpy), and don't null them or we'll leak mem.
*/

char **explode(char *input,char *targ)
{
  char **rv,*ptr,*p1;
  int i = 0;
  rv = NULL;
  /* make sure we have input */
  if(input && strlen(input)) {
    ptr = strCcpy(input);      
    for(i = 0,p1 = strtok(ptr,targ); p1 ; i++, p1 = strtok(NULL,targ));

    /* allocate the pointers to substrings  and 1 for NULL at end */
    rv = (char **)calloc(sizeof(char *),i+1);
    assert(rv);
    free(ptr);
    /*
     * copy / duplicate the strings
     */
    ptr = strCcpy(input);      
    for(i = 0,p1 = strtok(ptr,targ); p1 ; i++, p1 = strtok(NULL,targ) ) {   
      rv[i] = strCcpy(p1);
    }   
    free(ptr);
    rv[i] = NULL;  
  }
  return rv;
}

/* Free storage created by explode */
void explodefree(char *freeme[])
{
  int i;
  if(freeme) {
    for(i = 0;freeme[i] != NULL; i++) {
      free(freeme[i]);
    }        
    free(freeme);
  }
} 

/* C version of implode. Again, it's up to the caller to free any scratch buffers */
char *implode(char **list,char targ)
{
  char **ptr;
  char *rv,*rvt;
  unsigned i,l;

  rv = NULL;

  if(list) {
    /* Find the length of substrings with the new separator */ 
    for( i=0,ptr = list; *ptr != NULL;ptr ++)       {
      i += (strlen(*ptr)+1);
    }
    /* if we have a string to return */
    if(i) {
      /* allocate the space for it */
      rv = (char  *)malloc(i);
      rvt = rv;
      assert(rv != NULL);
      /* now copy the string, plus separator */
      for(ptr = list;*ptr != NULL; ptr ++)  {
	l = strlen(*ptr);
	rvt = strncpy(rvt,*ptr,l) + l;
	*rvt++  = targ;                
      }
      /* last separator becomes '\0' */
      *(--rvt) = '\0';
    }
  }
  return rv;    
}
