/*     
 * $RCSfile: dirname.c,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/01 23:59:17 $
 */


#include <x_stdio.h>

extern	char *	rindex(
#ifdef __STDC__
		       char *, char
#endif		       
		       );

/* 
 * Not all platforms support the 'dirname' utility, so we provide this one. 
 */


void
main( argc, argv )
    int		argc;
    char	**argv;
{
    char	*str;
    char	*lastSlash;

    str = (argc > 1) ? argv[1] : "";
    
    lastSlash = rindex(str, '/');
    if ( lastSlash == 0 ) {
	/* 
	 * No '/' in the pathname.
	 */
	str = ".";
    } else if ( lastSlash == str ) {
	/* 
	 * File in root directory '/'
	 */
	str = "/";
    } else {
	/* 
	 * Print everything up to the last slash
	 */
	*lastSlash = 0;
    }
    printf("%s\n", str);
}
  
