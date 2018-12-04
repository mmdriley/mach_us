/*
 * fperm.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.4 $
 * $Date: 1993/02/01 23:59:06 $
 */

/* 
 * usage:  fperm file mask
 *
 * fperm sets its exit code to indicate whether the binary AND of
 * 'mask' and the permission bits of 'file' was zero.  'mask' must
 * be given in octal.
 *
 * Exit status:
 * 	0: 	AND was zero
 * 	1:	AND was non-zero
 * 	2:	some sort of error
 */
 
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>

#define ERROR	2

main( argc, argv )
    int		argc;
    char **	argv;
{
    int		mask;
    struct stat	sb;
    char	errBuf[MAXPATHLEN + 80];
    
    if ( argc != 3 ) {
	fprintf(stderr, "usage: fperm file mask\n");
	exit(ERROR);
    }
    if ( stat(argv[1], &sb) ) {
	perror(sprintf(errBuf, "fperm could not access %s", argv[1]));
	exit(ERROR);
    }
    if ( sscanf(argv[2], "%o", &mask) == 0 ) {
	fprintf(stderr, "fperm: mask appears mangled\n");
	exit(ERROR);
    }
#if 0
    printf("mask: %o  mode: %o  AND: %o\n", mask, sb.st_mode,
	   mask & sb.st_mode);
#endif
    exit((mask & sb.st_mode) ? 1 : 0);
}
