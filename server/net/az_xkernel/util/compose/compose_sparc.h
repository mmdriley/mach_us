/*
 * compose_sparc.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.5 $
 * $Date: 1993/02/01 23:59:39 $
 */

#include <string.h>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/dir.h>
#define DIRENT direct
#include <sys/stat.h>

int 	lstat( char *path, struct stat *buf );
char	toupper( char );
int	getopt( int, char **, char *);
