/*     
 * $RCSfile: ptbl_static.c,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.3 $
 * $Date: 1993/02/02 00:01:04 $
 */


#include "prottbl.h"

typedef struct {
    char	*name;
    long	relNum;
} MapEntry;


typedef struct {
    char 	*name;
    long	id;
    MapEntry	*map;
} Entry;


#include "ptblData.c"


int
protTblParse( s )
    char *s;
{
    static int	done = 0;
    Entry 	*e = entries;
    MapEntry	*mapEnt;

    printf("protTblParse called\n");
    if ( ! done ) {
	done = 1;
	for ( e = entries; e->name; e++ ) {
	    protTblAddProt(e->name, e->id);
	    if ( e->map ) {
		for ( mapEnt = e->map; mapEnt->name; mapEnt ++ ) {
		    protTblAddBinding(e->name, mapEnt->name, mapEnt->relNum);
		}
	    }
	}
    }
    return 0;
}
