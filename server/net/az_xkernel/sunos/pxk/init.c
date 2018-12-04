/* 
 * init.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.28 $
 * $Date: 1993/02/01 23:54:31 $
 */

#include <ctype.h>
#include "x_stdio.h"
#include "process.h"
#include "upi.h"
#include "xk_debug.h"
#include "event_i.h"
#include "site.h"
#include "event.h"
#include "machine.h"
#include "compose.h"
#include "platform.h"
#include "prottbl.h"
#include "x_libc.h"
#include "x_util.h"
#include "netmask.h"

int globalArgc;
char **globalArgv;
char errBuf[200];
char *rom[ROM_MAX_LINES + 1][ROM_MAX_FIELDS + 1];  

int	traceinit;


static void	initRom( void );

static	char *protocolTablesDefault[2] = {
    PROTOCOL_TABLE,
    0
};


static void
pgraphStub(ev, arg)
    Event	ev;
    VOID 	*arg;
{
    build_pgraph();
}


int
main(argc,argv)
int	argc;
char	*argv[];
{
    char **protTbl;
    
    extern void clock_ih(), InitProtocols();
    
    setbuf(stdout, 0);
    globalArgc = argc;
    globalArgv = argv;
    
    LWP_Init();			/* cjt */
    
    xTraceInit();
    map_init();
    msgInit();
    evInit();
    init_clock(clock_ih, (long)(EVENT_INTERVAL/1000));
    initRom();
    netMaskInit();
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
    build_pgraph_dev();
    evDetach( evSchedule( pgraphStub, 0, 0 ) );
    
    return 0;
}


/* 
 * Save all but the last character (the newline)
 */
static char *
savestr( char *s )
{
    char 	*r;
    unsigned	len;

    len = strlen(s);
    r = (char *) xMalloc(len);
    strncpy(r, s, len - 1);
    return r;
}


#define ROM_LEN	200

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
    i = 0;
    while ( fgets(buf, ROM_LEN + 2, f) ) {
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
	 * Clear out initial white space
	 */
	while ( *p && isspace(*p) ) {
	    p++;
	}
	if ( strlen(p) == 0 ) {
	    continue;
	}
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
	i++;
    }
    fclose(f);
}
