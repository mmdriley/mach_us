/*
 * prottbl_i.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.5 $
 * $Date: 1993/02/01 23:57:59 $
 */

/* 
 * Declarations internal to the protocol id table utility
 */

#ifndef prottbl_i_h
#define prottbl_i_h

#include "idmap.h"

typedef enum {
    NO_ERROR = 0,
    ERR_FILE,
    ERR_NAME,
    ERR_ID,
    ERR_NAME_TOO_LARGE
} ErrorCode;


extern	Map	ptblNameMap;

typedef struct {
    char	*name;
    long	id;
    Map		idMap;		/* longs (id's) to relative protocol numbers */
    Map		tmpMap;		/* strings to relative protocol numbers */
    Map		revMap;		/* relative protocol numbers -> 0 */
} PtblEntry;




#ifdef __STDC__

/* 
 * protTblAddProt -- Create an entry in the protocol map binding 'name'
 * to the given protocol id
 */
extern void	protTblAddProt( char *name, long id );

/* 
 * protTblAddBinding -- Indicate in the map that 'hlp' uses 'relNum' as a
 * protocol number relative to 'llp'.
 */
extern void	protTblAddBinding( char *llp, char *hlp, long relNum );

/* 
 * protTblParse -- read in 'filename' and add the appropriate entries
 * to the protocol map via protTblAddProt and protTblAddBinding.
 * Returns 0 on a successful parse, non-zero if not.
 */
extern ErrorCode	protTblParse( char *filename );

#endif __STDC__


#endif !prottbl_i_h
