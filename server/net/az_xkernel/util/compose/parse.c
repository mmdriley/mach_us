/* 
 * parse.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.15 $
 * $Date: 1993/02/01 23:59:52 $
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#include "global.h"

#define BUF_SIZE 80

#define strsame(A,B) (!(strcmp((A),(B))))

static int cur;
static int done = 0;
static int debug = 0;
int fileLine = 1;
int filePosition = 0;
static enum {
    DEV_STATE, PROT_STATE, TRACE_STATE
} state;

static char *	DIR(void);
static void	EDGE(PROTOCOL *);
static void	FILELIST(PROTOCOL *);
static char *	FILENAME(void);
static void	FILES(PROTOCOL *);
#if 0
static int	NUMBER(void);
#endif
static ProtName	PNAME(void);
static void	PROTOCOLLIST(PROTOCOL *);
static ProtName	PROTOCOLNAME(void);
static char *	PROTOCOLSTRING( void );
static void	PROTS(PROTOCOL *);
static void 	S(void);
static char *	STRING(void);
static void	TBL( void );
static void	TRACE( PROTOCOL * );
static void	force_char(char);
static void	force_string(char *);
static void	get(void);
static void	skip_blanks(void);
static void	skip_blanks_force( void );


static void
changeState()
{
    switch( state ) {

      case DEV_STATE:
	lastDriver();
	state = PROT_STATE;
	break;

      case PROT_STATE:
	state = TRACE_STATE;
	break;
	
      case TRACE_STATE:
	errorTooManyStates();
	break;
    }
}


void
parse()
{
  if (debug)
    printf("\n>parse\n");
  get();
  S();
}


/*
 * S -> ( EDGE | TBL | AMPERSAND ) , S | EPSILON
 */
static void
S()
{
    PROTOCOL	p;
    
    if (debug)
      printf("\n>S\n");
    skip_blanks();
    while (!done) {
	if (cur == '@') {
	    force_char('@');
	    skip_blanks();
	    force_char(';');
	    changeState();
	} else if (cur == 'n') {
	    /* 
	     * Must be an EDGE
	     */
	    bzero((char *)&p, sizeof(PROTOCOL));
	    EDGE(&p);
	    if ( state == DEV_STATE || state == PROT_STATE ) {
		/* 
		 * These are the only states where the edge really
		 * represents a protocol.  In the TRACE_VAR state, the
		 * edge is just a trace variable
		 */
		addInstance(&p);
	    }
	    /* 
	     * Add to the trace variable list for both protocols and
	     * edges which represent only trace variables
	     */
	    {
		char	traceVar[80];
		
		if ( p.trace ) {
		    assert(strlen(p.n.name) < 74);
		    sprintf(traceVar, "trace%s", p.n.name);
		    if ( state == DEV_STATE || state == PROT_STATE ) {
			strcat(traceVar, "p");
		    }
		    addTraceVar(xerox(traceVar), p.trace);
		}
	    }
	} else if (cur == 'p') {
	    /* 
	     * Must be a table entry
	     */
	    TBL();
	} else {
	    syntaxErrorString("'name' or 'prottbl' or '@'");
	}
	skip_blanks();
    }
}


static void
TBL()
{
    char *	fileName;

    force_string("prottbl");
    skip_blanks();
    force_string("=");
    skip_blanks();
    fileName = FILENAME();
    if (*fileName == 0) {
	syntaxErrorString("filename");
    }
    addProtTbl(fileName);
    skip_blanks();
    force_char(';');
    skip_blanks();
}    




#if 0

/*
 * NUMBER 		-> [0..9]*
 */
static int
NUMBER()
{
  int i = 0;

  if (debug)
    printf("\n>NUMBER\n");

  while (isdigit(cur)) {
    i = i * 10 + cur - '0';
    get();
  }
  return i;
}

#endif

/*
 * EDGE ->
 *   PNAME " " (DIR " ") (FILES " ") (PROTS " ") ";"
 */
static void
EDGE(PROTOCOL *p)
{
    if (debug)
      printf("\n>EDGE\n");
    p->n = PNAME();
    while (1) {
	if ( cur != ';' ) {
	    skip_blanks_force();
	}
	switch ( cur ) {
	    
	  case ';':
	    force_char(';');
	    return;
	    
	  case 'd':
	    p->path = DIR();
	    break;
	    
	  case 'f':
	    FILES(p);
	    break;
	    
	  case 'p':
	    PROTS(p);
	    break;
	    
	  case 't':
	    TRACE(p);
	    break;
	    
	    default:
	    syntaxErrorString("'dir', 'files', 'protocols', 'trace', or ';'");
	}
    }
}


static void
TRACE( PROTOCOL *p )
{
    force_string("trace");
    skip_blanks();
    force_char('=');
    skip_blanks();
    p->trace = STRING();
}


/*
 * PNAME -> "name=" PROTOCOLNAME
 */
static ProtName
PNAME()
{
    if (debug)
      printf("\n>PNAME\n");
    force_string("name");
    skip_blanks();
    force_char('=');
    skip_blanks();
    return PROTOCOLNAME();
}


/*
 * DIR -> "dir=" STRING
 */
static char *
DIR()
{
  if (debug)
    printf("\n>DIR\n");
  force_string("dir");
  skip_blanks();
  force_char('=');
  skip_blanks();
  return STRING();
}
  

/*
 * FILES -> "files=" FILELIST
 */
static void
FILES(PROTOCOL *p)
{
  if (debug)
    printf("\n>FILE\n");
  force_string("files");
  skip_blanks();
  force_char('=');
  skip_blanks();
  FILELIST(p);
}


/*
 * FILELIST -> FILENAME | FILENAME "," FILELIST
 */
static void
FILELIST(PROTOCOL *p)
{
  if (debug)
    printf("\n>FILELIST\n");
  p->numfiles = 0;
  p->files[0] = FILENAME();
  while (cur == ',') {
    get();
    p->numfiles++;
    p->files[p->numfiles] = FILENAME();
  }
  p->numfiles++;
}


static ProtName
PROTOCOLNAME()
{
    ProtName	p;

    p.name = PROTOCOLSTRING();
    if ( cur == '/' ) {
	get();
	p.instance = PROTOCOLSTRING();
    } else {
	p.instance = "";
    }
    return p;
}


/*
 * PROTOCOLSTRING -> [a-z_0..9]*
 */
static char *
PROTOCOLSTRING()
{
    int len = 0;
    char buf[BUF_SIZE];
    
    if (debug)
      printf("\n>PROTOCOLNAME\n");
    while (islower(cur) || isupper(cur) || isdigit(cur) || (cur == '_')) {
	buf[len++] = cur;
	get();
    }
    buf[len] = 0;
    return (xerox(buf));
}


/*
 * STRING -> [^ \t\n;]*
 */
static char *
STRING()
{
  int len = 0;
  char buf[BUF_SIZE];

  if (debug)
    printf("\n>STRING\n");
  while (cur != ' ' && cur != '\t' && cur != '\n' && cur != ';') {
    buf[len++] = cur;
    get();
  }
  buf[len] = 0;
  return (xerox(buf));
}


/*
 * PROTS --> "protocols=" PROTOCOLLIST
 */
static void
PROTS(PROTOCOL *p)
{
  if (debug)
    printf("\n>PROTS\n");
  force_string("protocols");
  skip_blanks();
  force_char('=');
  skip_blanks();
  PROTOCOLLIST(p);
}


/*
 * PROTOCOLIST -> PROTOCOLNAME "," PROTOCOLLIST
 */
static void
PROTOCOLLIST( PROTOCOL *p )
{
    if (debug)
      printf("\n>PROTOCOLLIST\n");
    p->numdown = 0;
    p->down[0] = PROTOCOLNAME();
    while (cur == ',') {
	get();
	skip_blanks();
	p->numdown++;
	p->down[p->numdown] = PROTOCOLNAME();
    }
    p->numdown++;
}


/*
 * FILENAME -> [./-A-Za-z_0..9]*
 */
static char *
FILENAME()
{
  int len = 0;
  char buf[BUF_SIZE];

  if (debug)
    printf("\n>FILENAME\n");
  while (islower(cur) || isupper(cur) || isdigit(cur) || (cur == '_') ||
	 (cur == '/') || (cur == '.') || (cur == '-')) {
    buf[len++] = cur;
    get();
  }
  buf[len] = 0;
  return (xerox(buf));
}


static void
force_char(char ch)
{
  if (cur == ch) {
    get();
  } else {
    syntaxErrorChar(ch, cur);
  }
}


static void
force_string(char *str)
{
  char *temp;

  temp = str;
  while (cur == *temp) {
    get();
    temp++;
  }
  if (*temp != 0) {
    syntaxErrorString(str);
  }
}


static void
skip_blanks()
{
    while ( (cur == ' ') || (cur == '\t') || (cur == '\n') ) {
	get();
    }
}


static void
skip_blanks_force()
{
    if ( (cur == ' ') || (cur == '\t') || (cur == '\n') ) {
	skip_blanks();
    } else {
	syntaxErrorString("white space");
    }
}


static void
get()
{
  if (done)
    return;
  cur = getchar();
  if ( cur == '#' ) {
      while ( cur != '\n' && cur != EOF ) {
	  cur = getchar();
      }
  }
  filePosition++;
  if (debug)
    putchar(cur);
  if (cur == EOF) {
    done = 1;
    cur = 0;
  }
  if (cur == '\n') {
    filePosition=0;
    fileLine++;
    cur = ' ';
    if (debug)
      putchar(cur);
  }
}




