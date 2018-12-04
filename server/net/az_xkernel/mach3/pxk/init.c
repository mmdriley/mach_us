/*
 * init.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.27 $
 * $Date: 1993/02/02 00:05:04 $
 */

/*
 * Mach3
 */

#include "process.h"
#include "upi.h"
#include "xk_debug.h"
#ifndef XKMACHKERNEL
#include <stdio.h>
#endif XKMACHKERNEL
#include "site.h"
#include "platform.h"
#include "netmask.h"
#include "prottbl.h"
#include "x_util.h"

/* the event scheduling interval granularity */
int  event_granularity = 100; 

int traceinit = 0;

extern build_pgraph();

#ifndef XKMACHKERNEL

#  ifdef __STDC__

int	main( int, char ** );
static void	initRom( void );

#  endif

#endif XKMACHKERNEL

#ifdef MACH
#define SHEPNUM 10 
#endif


/* extern	int 	_init_user(); */

int globalArgc;
char **globalArgv;
char errBuf[200];
char *rom[ROM_MAX_LINES + 1][ROM_MAX_FIELDS + 1];  

extern	char *protocolTables[];
static	char *protocolTablesDefault[2] = {
    PROTOCOL_TABLE,
    0
};


#ifdef XKMACHKERNEL
/*
 * Maximum number of args:
 */
#define NMAXARGS	16

/* initialization of testArgs determines default args: */
#if 0
char *testArgs = "-testudp -s";
#else
char *testArgs = "-testudp -c192.12.69.99";
#endif

static char *testArgv[NMAXARGS]	= {"xkernel"};
static int testArgc = 1;

/* the inkernel version needs a pre-parsed initRom table */
#include "initRom.c"

/*
 * Split "testArgs" into individual arguments and store them in
 * testArgv[]; testArgc is the number of args in testArgv[];
 * testArgv[0] is always "xkernel".
 */
static void
split_args()
{
    extern char* index();
    char *cp = testArgs;

    /* split args and store them in testArgc/testArgv: */

    while (cp && *cp) {
	testArgv[testArgc++] = cp;
	if (testArgc >= NMAXARGS) {
	    Kabort("init.c: too many actual args (increase NMAXARGS)");
	} /* if */
	cp = index(cp, ' ');
	if (cp) {
	    *cp++ = '\0';
	    while (*cp == ' ') {
		++cp;
	    } /* while */
	} /* if */
    } /* while */
    testArgv[testArgc] = 0;
} /* split_args */

#endif XKMACHKERNEL

int
#ifndef XKMACHKERNEL
main(argc,argv)
#else
xkInit(argc, argv)
#endif
int	argc;
char	*argv[];
{
    char **protTbl;

#ifndef XKMACHKERNEL    
    setbuf(stdout, 0);
    globalArgc = argc;
    globalArgv = argv;
#else
    split_args();

    globalArgc = testArgc;
    globalArgv = testArgv;
#endif ! XKMACHKERNEL    

    xTraceInit();
    map_init();
    msgInit();
    threadInit();
    evInit(event_granularity);
    inputThreadInit();
    xTrace0(init, TR_EVENTS, "Calling initRom");
    initRom();
    netMaskInit();
    xMallocInit();
    if ( *protocolTables == 0 ) {
	/* 
	 * No tables defined in graph.comp
	 */
	protTbl = protocolTablesDefault;
    } else {
	protTbl = protocolTables;
    }
    while ( *protTbl != 0 ) {
	if ( protTblBuild(*protTbl) ) {
	    Kabort("Error building protocol table");
	}
	protTbl++;
    }
    upiInit();
    xTrace1(init, TR_EVENTS,
	    "Creating thread to build protocol graph (func %x)",
	    build_pgraph );
    CreateProcess( build_pgraph, STD_PRIO, 0 );
    
    xTrace0(init, TR_EVENTS, "init thread exits");

#ifndef XKMACHKERNEL
    cthread_exit((void *)0);
    return 0;
#else
    thread_terminate( current_thread() );
    thread_halt_self();
    panic( "The zombie xkInit strolls" );
#endif XKMACHKERNEL
}

static char *
savestr(s)
    char *s;
{
    char *r;
    r = (char *) xMalloc((unsigned)(strlen(s)+1));
    strcpy(r, s);
    return(r);
}


#define ROM_LEN 200

#ifndef XKMACHKERNEL
static void
initRom()
{
    char buf[ROM_LEN + 2];
    char *p;
    FILE *f;
    int	i, j;
    
    if ((f = fopen("rom", "r")) == NULL) {
	xTrace0(init, TR_MAJOR_EVENTS, "not loading ROM file");
	return;
    } else {
	xTrace0(init, TR_MAJOR_EVENTS, "loading ROM file");
    }
    
    for ( i=0; fgets(buf, ROM_LEN + 2, f); i++ ) {
	if ( i > ROM_MAX_LINES  ) {
	    sprintf(errBuf, "ROM file has too many lines (max %d)",
			   ROM_MAX_LINES);
	    Kabort(errBuf);
	}
	if ( strlen(buf) > ROM_LEN ) {
	    sprintf(errBuf, "ROM entry in line %d is too long (max %d chars)",
			   i, ROM_LEN);
	    Kabort(errBuf);
	}
	p = savestr(buf);
	/* 
	 * Put a '\0' after each field and set the rom array to these
	 * fields 
	 */
	for ( j=0; *p; j++ ) {
	    if ( j >= ROM_MAX_FIELDS ) {
		sprintf(errBuf,
			"ROM entry on line %d has too many fields (max %d)",
			i, ROM_MAX_FIELDS);
		Kabort(errBuf);
	    }
	    rom[i][j] = p;
	    /* 
	     * Find and mark the end of this field
	     */
	    while ( *p && ! isspace(*p) ) {
		p++;
	    }
	    if ( *p ) {
		*p++ = 0;
		/* 
		 * Find the start of the next field
		 */
		while ( *p && isspace(*p) ) {
		    p++;
		}
	    }
	}
    }
    fclose(f);
}
#endif XKMACHKERNEL
