/*
 * prottbl_parse.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.14 $
 * $Date: 1993/02/01 23:57:13 $
 */

/*
 *  Parser for the protocol table file.
 *
 *  Syntax (nonterminals capitalized):
 *
 * 	Entry -> Name  Id   OptionalList
 * 	OptionalList -> { L } | null
 * 	List -> Name Id List | null
 * 	Name -> string
 * 	Id -> integer
 *
 *  A '#' marks a comment until end of line
 */


#include <stdio.h>
#include "platform.h"
#include "xk_debug.h"
#include "prottbl_i.h"
#include "x_stdio.h"
#include <ctype.h>

#define BUFSIZE 80


#ifdef __STDC__

static void	entry( void );
static void	error( ErrorCode );
static void	get( void );
static long	id( void );
static int	list( char * );
static char *	name( char * );
static void	optionalList( char * );

#else

static void	entry();
static void	error();
static void	get();
static long	id();
static int	list();
static char *	name();
static void	optionalList();

#endif __STDC__

static int	line = 1;
static int	position = 0;
static ErrorCode	errorCode;
static int	nextChar;
static FILE	*inputFile;
static char	*fileName;

/* 
 * Top-level parsing routine.
 */
ErrorCode
protTblParse(f)
    char *f;
{
    fileName = f;
    errorCode = NO_ERROR;
    inputFile = fopen(fileName, "r");
    if ( inputFile == 0 ) {
	error(ERR_FILE);
    } else {
	get();
	while ( nextChar != EOF && ! errorCode ) {
	    entry();
	}
	fclose(inputFile);
    }
    return errorCode;
}



static void
get()
{
    do {
	nextChar = getc(inputFile);
	position++;
	if (nextChar == '#') {
	    while (nextChar != '\n' && nextChar != EOF) {
		nextChar = getc(inputFile);
		position++;
	    }
	}
	if (nextChar == '\n') {
	    line++;
	    position = 1;
	}
    } while ( nextChar == '#' || nextChar == '\n' );
}


static void
skipWhite()
{
    while ( isspace(nextChar) ) {
	get();
    }
}


/* 
 * consumes c from the input stream, consuming white space on either side.
 * Returns non-zero if c is not the next character.
 */
static int
forceChar(c)
    char c;
{
    skipWhite();
    if ( nextChar != c ) {
	return 1;
    }
    get();
    skipWhite();
    return 0;
}
  

static void
error(code)
    ErrorCode code;
{
    char 	*errMsg = "UNKNOWN";
    char	buf[200];

    switch ( code ) {
      case ERR_FILE:
	sprintf(buf, "could not open protocol table file %s", fileName);
	xError(errBuf);
	return;

      case ERR_NAME:
	errMsg = "expected protocol name";
	break;
    
      case ERR_ID:
	errMsg = "expected id (integer)";
	break;

      case ERR_NAME_TOO_LARGE:
	errMsg = "string exceeded buffer size";
	break;

      case NO_ERROR:
	return;
    }
    sprintf(buf, "prot ID parse error: %s at line %d, pos %d\n",
		   errMsg, line, position);
    xError(errBuf);
    errorCode = code;
}


/* 
 * Entry -> Name  Id   OptionalList
 */
static void
entry()
{
    char	protName[BUFSIZE];
    long	protId;

    xTrace0(ptbl, TR_DETAILED, "ptbl parse: entry called");
    if ( name(protName) == 0 ) {
	error(ERR_NAME);
	return;
    }
    if ( (protId = id()) == -1 ) {
	error(ERR_ID);
	return;
    }
    protTblAddProt(protName, protId);
    optionalList(protName);
    xTrace2(ptbl, TR_FUNCTIONAL_TRACE, "ptbl parse: entry == { name == %s, id == %d }",
	    protName, protId);
}


static void
optionalList(n)
    char *n;
{
    xTrace0(ptbl, TR_DETAILED, "ptbl parse: optionalList called");
    if ( nextChar == '{' ) {
	/* 
	 * XXX -- create maps
	 */
	if ( forceChar('{') ) {
	    return;
	}
	if ( list(n) ) {
	    return;
	}
	forceChar('}');
    }
}
  
  
/* 
 * List -> Name Id List | null
 *
 * Returns zero on successful parse, non-zero on error
 */
static int
list(llpName)
    char *llpName;
{
    char	hlpName[BUFSIZE];
    long	hlpId;
    
    xTrace0(ptbl, TR_DETAILED, "ptbl parse: list called");
    if ( name(hlpName) == 0 ) {
	/* 
	 * null list
	 */
	return 0;
    }
    if ( (hlpId = id()) == -1 ) {
	error(ERR_ID);
	return 1;
    }
    protTblAddBinding(llpName, hlpName, hlpId);
    xTrace2(ptbl, TR_FUNCTIONAL_TRACE, "ptbl parse: list element %s -> %d", hlpName, hlpId);
    return list(llpName);
}


/* 
 * returns 0 if a name is not matched
 */
static char *
name(buf)
    char *buf;
{
    int		i;

    if ( ! isalpha(nextChar) ) {
	return 0;
    }
    i = 0;
    while ( nextChar && ! isspace(nextChar) ) {
	if ( i >= BUFSIZE ) {
	    error(ERR_NAME_TOO_LARGE);
	    return 0;
	}
	buf[i++] = nextChar;
	get();
    }
    skipWhite();
    buf[i] = 0;
    return buf;
}


/* 
 * Returns -1 (not a valid id) if an id is not matched
 */
static long
id()
{
    char	buf[80];
    int		i;
    long	n;
    int		res;

    if ( ! isdigit(nextChar) && nextChar != 'x' ) {
	return -1;
    }
    i = 0;
    /* 
     * Check for hex format
     */
    if ( nextChar == 'x' ) {
	buf[i++] = nextChar;
	get();
    }
    while ( nextChar && isdigit(nextChar) ) {
	if ( i >= sizeof(buf) ) {
	    error(ERR_NAME_TOO_LARGE);
	    return 0;
	}
	buf[i++] = nextChar;
	get();
    }
    skipWhite();
    buf[i] = 0;
    if ( buf[0] == 'x') {
	res = sscanf(buf + 1, "%x", &n);
    } else {
	res = sscanf(buf, "%d", &n);
    }
    if ( res != 1 ) {
	/* 
	 * The scan was botched.  We know this is an error, so we
	 * don't have to worry about pushing characters back.
	 */
	return -1;
    }
    return n;
}


