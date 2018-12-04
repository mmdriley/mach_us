/*
 * prottbl.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.26 $
 * $Date: 1993/02/03 18:48:54 $
 */

/*
 * Management of the "protocol id / relative number" table
 */

#ifndef XKMACHKERNEL
#  include "x_stdio.h"
#  include "x_libc.h"
#endif ! XKMACHKERNEL
#include "prottbl.h"
#include "prottbl_i.h"
#include "idmap.h"
#include "xk_debug.h"
#include "assert.h"

typedef PtblEntry	Entry;

int	traceptbl;

#define PTBL_MAP_SIZE		101
#define PTBL_NAME_MAP_SIZE	101
#define PTBL_ID_MAP_SIZE	101

#ifdef __STDC__
char *	protIdToStr( long );
#endif __STDC__

Map	ptblNameMap = 0;	/* strings to Entry structures */
static Map	idMap = 0;
static char	errBuf[200];
static int	errorOccurred;

#define MAX_PROT_NAME	16
/* 
 * Error messages
 */
#define MULT_PROT_MSG   "prot table: multiple protocols with id %d declared"
#define NO_ID_MSG     "prot table: %s (declared as hlp to %s) has no id number"
#define NAME_REPEATED_MSG  "prot table: protocol %s is declared with different id's"
#define BIND_REPEATED_MSG  "prot table: binding for %s above %s defined multiple times"
#define NO_ENTRY_MSG "prot table: No entry exists for llp %s"

#define NUM_BOUND_MSG "prot table: relative number %d is already bound for llp %s"

static void
error(msg)
    char *msg;
{
    xError(msg);
    errorOccurred = 1;
}


static void
mkKey(key, name)
    char *key;
    char *name;
{
    bzero(key, MAX_PROT_NAME);
    strncpy(key, name, MAX_PROT_NAME);
}


/* 
 * This function is called for each entry in a lower protocol's temp
 * map (binding higher protocol name strings to relative numbers.)
 * We find the hlp's id number and bind it to the relative number,
 * signalling an error if we can't find the hlp's id number.
 */
static int
backPatchBinding(key, relNum, arg)
    VOID *key;
    int relNum;
    VOID *arg;
{
    Entry	*llpEntry = (Entry *)arg;
    Entry	*hlpEntry;
    
    if ( mapResolve(ptblNameMap, key, &hlpEntry) == XK_FAILURE ) {
	sprintf(errBuf, NO_ID_MSG, (char *)key, llpEntry->name);
	error(errBuf);
    } else {
	xTrace3(ptbl, TR_GROSS_EVENTS,
		"Backpatching binding of %s->%s using %d",
		hlpEntry->name, llpEntry->name, relNum);
	mapBind(llpEntry->idMap, &hlpEntry->id, relNum);
    }
    return MFE_CONTINUE;
}


/* 
 * This function is called for each entry (lower protocol) in the name
 * map.  If there are entries in the llp's tmpMap (higher protocols
 * which were bound with a string instead of an id), we backpatch the
 * id binding.
 */
static int
checkEntry(key, value, arg)
    VOID *key;
    int value;
    VOID *arg;
{
    Entry	*llpEntry = (Entry *)value;

    /* 
     * If there is anything in this entry's tmpMap, transfer it to the
     * idMap.  If we don't know the id number of the hlp in the
     * tmpMap, signal an error.
     */
    if ( llpEntry->tmpMap ) {
	mapForEach(llpEntry->tmpMap, backPatchBinding, llpEntry);
	mapClose(llpEntry->tmpMap);
	llpEntry->tmpMap = mapCreate(PTBL_MAP_SIZE, MAX_PROT_NAME);
    }
    return MFE_CONTINUE;
}


int
protTblBuild(filename)
    char *filename;
{
    xTrace1(ptbl, TR_GROSS_EVENTS, "protTblBuild( %s )", filename);
    if ( ptblNameMap == 0 ) {
	ptblNameMap = mapCreate(PTBL_NAME_MAP_SIZE, MAX_PROT_NAME);
	idMap = mapCreate(PTBL_ID_MAP_SIZE, sizeof(long));
    }
    if ( (protTblParse(filename)) != 0 || errorOccurred ) {
        xTrace0(ptbl, TR_ERRORS, "protTblBuild failed to parse input file(s)");
	return -1;
    }
    xTrace1(ptbl, TR_GROSS_EVENTS, "protTblBuild( %s ) checking map consistency", filename);
    /* 
     * Check the consistency of the protocol map.  
     */
    mapForEach(ptblNameMap, checkEntry, 0);
    xIfTrace(ptbl, TR_ERRORS) {
      if (errorOccurred) printf("prottbl: protTblBuild: Consistency error in file %s", filename);
    }
    return errorOccurred;
}


long
protTblGetId( protocolName )
    char *protocolName;
{
    Entry	*e;
    char	key[MAX_PROT_NAME];
    
    xAssert(ptblNameMap);
    mkKey(key, protocolName);
    if ( mapResolve(ptblNameMap, key, &e) == XK_FAILURE ) {
        xTrace0(ptbl, TR_ERRORS, "protTblGetId failed to find key");
	return -1;
    }
    return e->id;
}


char *
protIdToStr( id )
    long	id;
{
    Entry	*e;

    if ( mapResolve(idMap, &id, &e) == XK_FAILURE ) {
	return 0;
    }
    return e->name;
}


long
relProtNum(hlp, llp)
    XObj hlp;
    XObj llp;
{
    Entry	*llpEntry;
    long	res;
    
    xAssert(ptblNameMap);
    if ( ! ( xIsProtocol(hlp) && xIsProtocol(llp) ) ) {
        xIfTrace(ptbl, TR_ERRORS) {
	  if ( !(xIsProtocol(hlp))) printf("prottbl: relProtNum: hlp is not a protocol\n");
	  if ( !(xIsProtocol(llp))) printf("prottbl: relProtNum: llp is not a protocol\n");
	}
	return -1;
      }
    if ( mapResolve(idMap, &llp->id, &llpEntry) == XK_FAILURE ) {
      xTrace1(ptbl, TR_ERRORS, "prottbl: relProtNum: Cannot find llp id %d in protocol table", llp->id);
      return -1;
    }
    if ( llpEntry->idMap == 0 ) {
	/* 
	 * The lower protocol does not use relative naming.  Return the
	 * absolute id of the upper protocol.
	 */
      xTrace1(ptbl, TR_EVENTS, "prottbl: relProtNum: Returning absolute id of upper protocol, %d", hlp->id);
      return hlp->id;
    }
    if ( mapResolve(llpEntry->idMap, &hlp->id, &res) == XK_FAILURE ) {
	res = -1;
    }
    xTrace1(ptbl, TR_EVENTS, "prottbl: relProtNum: Returning %d", res);
    return res;
}


void
protTblAddProt(name, id)
    char *name;
    long int id;
{
    Entry	*e;
    char	key[MAX_PROT_NAME];
    
    xTrace2(ptbl, TR_MAJOR_EVENTS, "protTblAddProt adding %s(%d)", name, id);
    mkKey(key, name);
    if ( mapResolve(ptblNameMap, key, &e) == XK_FAILURE ) {
	/* 
	 * Entry does not exist for this name
	 */
	if ( mapResolve(idMap, (void *)&id, 0) == XK_SUCCESS ) {
	    sprintf(errBuf, MULT_PROT_MSG, id);
	    error(errBuf);
	    return;
	}
	e = (Entry *)xMalloc(sizeof(Entry));
	e->name = xMalloc(strlen(name) + 1);
	strcpy(e->name, name);
	e->id = id;
	e->idMap = e->tmpMap = 0;
	mapBind(ptblNameMap, key, e);
	mapBind(idMap, &e->id, e);
    } else {
	/* 
	 * Make sure that this declaration has the same id number as
	 * the previous declarations.
	 */
	if ( e->id != id ) {
	    sprintf(errBuf, NAME_REPEATED_MSG, name);
	    error(errBuf);
	}
    }
}



void
protTblAddBinding(llpName, hlpName, relNum)
    char *llpName;
    char *hlpName;
    long int relNum;
{
    Entry	*llpEntry, *hlpEntry;
    char	key[MAX_PROT_NAME];
    
    xTrace3(ptbl, TR_MAJOR_EVENTS, "protTblAddBinding adding %s->%s uses %d",
	    hlpName, llpName, relNum);
    mkKey(key, llpName);
    if ( mapResolve(ptblNameMap, key, &llpEntry) == XK_FAILURE ) {
	/* 
	 * No entry exists for the lower protocol
	 */
	sprintf(errBuf, NO_ENTRY_MSG, llpName);
	error(errBuf);
	return;
    }
    if ( llpEntry->idMap == 0 ) {
	llpEntry->idMap = mapCreate(101, sizeof(long));
	llpEntry->tmpMap = mapCreate(101, MAX_PROT_NAME);
	llpEntry->revMap = mapCreate(101, sizeof(long));
    }
    if ( mapResolve(llpEntry->revMap, &relNum, 0) == XK_SUCCESS ) {
	sprintf(errBuf, NUM_BOUND_MSG, relNum, llpName);
	error(errBuf);
	return;
    }
    mkKey(key, hlpName);
    if ( mapResolve(ptblNameMap, key, &hlpEntry) == XK_FAILURE ) {
	/* 
	 * Entry for hlp doesn't exist yet.  Add the binding from the
	 * hlp name to the number in tmpMap.  The binding will be
	 * tranferred to idMap after the hlp entry has been added.
	 */
	if ( mapResolve(llpEntry->tmpMap, key, 0) == XK_FAILURE ) {
	    mapBind(llpEntry->tmpMap, key, relNum);
	    mapBind(llpEntry->revMap, &relNum, 0);
	} else {
	    sprintf(errBuf, BIND_REPEATED_MSG, hlpName, llpName);
	    error(errBuf);
	}
    } else {
	/* 
	 * Add the entry directly to the idMap
	 */
	if ( mapResolve(llpEntry->idMap, &hlpEntry->id, 0) == XK_FAILURE ) {
	    mapBind(llpEntry->idMap, &hlpEntry->id, relNum);
	    mapBind(llpEntry->revMap, &relNum, 0);
	} else {
	    sprintf(errBuf, BIND_REPEATED_MSG, hlpName, llpName);
	    error(errBuf);
	}
    }
}


#ifdef XK_DEBUG

static int
dispHlp(key, relNum, arg)
    VOID *key;
    int relNum;
    VOID *arg;
{
    int	*hlpId = (int *)key;

    xTrace2(ptbl, TR_ALWAYS,
	    "      hlp id %d uses rel num %d", *hlpId, relNum);
    return MFE_CONTINUE;
}


static int
dispEntry(key, value, arg)
    VOID *key;
    int value;
    VOID *arg;
{
    Entry	*e = (Entry *)value;

    xTrace2(ptbl, TR_ALWAYS, "%s (id %d)", e->name, e->id);
    if (e->idMap) {
	xTrace0(ptbl, TR_ALWAYS, "  upper protocols:");
	mapForEach(e->idMap, dispHlp, 0);
    }
    return 1;
}


void
protTblDisplayMap()
{
    xTrace0(ptbl, TR_ALWAYS, "protocol table:");
    mapForEach(ptblNameMap, dispEntry, 0);
}

#endif XK_DEBUG

