/*
 * error.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.8 $
 * $Date: 1993/02/01 23:59:26 $
 */

#include <stdio.h>
#include "global.h"

static char	*cycleArray[MAX_PROTOCOL];
static int	cycleIndex = -1;

/* 
 * System prototypes
 */
void	perror( char * );

void
warnProtNotFound(char *d)
{
  fprintf(stderr, "compose warning: protocol \"%s\" found neither locally nor in libraries\n", d);
  fprintf(stderr, "                 (assuming in library)\n");
}


void
warnCouldNotAccess(char *f)
{
  char buf[200];
  sprintf(buf, "compose warning: could not access %s", f);
  perror(buf);
}


void
syntaxErrorChar(char expected, char got)
{
  fprintf(stderr,
	  "compose: syntax error: expecting \"%c\" got \"%c\" at line %d, pos %d\n",
	  expected, got, fileLine, filePosition);
  exit(1);
}


void
syntaxErrorString(char *s)
{
  fprintf(stderr,
	  "compose: syntax error: expecting \"%s\" at line %d, pos %d\n",
	  s, fileLine, filePosition);
  exit(1);
}


void
errorFile(char *s)
{
  fprintf(stderr, "compose error: can't open required file \"%s\"\n", s);
}


void
errorProtlUndefined(char *p1, char *p2)
{
  fprintf(stderr,
	  "compose error: undefined protocol \"%s\" referenced by \"%s\"\n",
	  p1, p2);
}


void
errorTooManyStates()
{
    fprintf(stderr, "compose error: too many state transition characters\n");
    exit(1);
}

void
errorCycle()
{
    fprintf(stderr, "Compose error -- cycle detected in dependency graph:\n");
}

void
errorCycleName( char *n )
{
    cycleArray[++cycleIndex] = n;
}

void
finishErrorCycle()
{
    fprintf(stderr, "\t");
    for ( ; cycleIndex > 0; cycleIndex-- ) {
	fprintf(stderr, "%s -> ", cycleArray[cycleIndex]);
    }
    fprintf(stderr, "%s\n", cycleArray[cycleIndex]);
    exit(1);
}


void
errorLaterInstanceFiles( PROTOCOL *p )
{
    fprintf(stderr, "Compose error at line %d, pos %d\n", fileLine,
	    filePosition);
    fprintf(stderr, "\tprotocol %s has a previous instance\n", p->n.name);
    fprintf(stderr,
	    "\tSubsequent instances may only define 'name' and 'protocols'\n");
    exit(1);
}
