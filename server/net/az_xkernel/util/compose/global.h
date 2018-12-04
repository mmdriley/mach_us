/*
 * global.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.11 $
 * $Date: 1993/02/01 23:59:22 $
 */

#define MAX_FILES 20
#define MAX_PROTOCOL 50

typedef struct {
    char *	name;
    char *	instance;
    int		index;		/* index into instance table */
} ProtName;

typedef enum {
    INIT_NOT_DONE,
    INIT_WORKING,
    INIT_DONE
} InitState;

typedef struct protocol {
    ProtName	n;
    char *	path;
    char *	trace;	  /* String rep. of initial value of trace variable */
    int		numfiles;
    char * 	files[MAX_FILES + 1];
    int 	numdown;
    ProtName	down[MAX_PROTOCOL + 1];
    InitState	initState;
} PROTOCOL;

extern int	fileLine;
extern int	filePosition;

void	addInstance( PROTOCOL * );
void	addProtTbl( char *name );
void	addTraceVar( char *name, char * value );
void	errorCycle( void );
void	errorCycleName( char * );
void	errorFile( char *s );
void	errorLaterInstanceFiles( PROTOCOL * );
void	errorProtlUndefined( char *p1, char *p2 );
void	errorTooManyStates( void );
void	finishErrorCycle( void );
char *	join( char *, char * );
void	lastDriver( void );
void	parse( void );
void	syntaxErrorChar( char expected, char got );
void	syntaxErrorString( char *expected );
char *	xerox( char * );
void 	warnCouldNotAccess( char * );
void	warnProtNotFound( char * );

/* 
 * System prototypes
 */
int	printf();
int	bzero( char *, int );
int	_flsbuf();
int	_filbuf();
void	exit( int );
int	fclose( FILE * );
int	fprintf( FILE *, char *, ... );
