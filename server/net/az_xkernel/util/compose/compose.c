/* 
 * compose.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.41 $
 * $Date: 1993/02/02 00:00:04 $
 */

#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include ARCH_INCLUDE
#include "global.h"

/* 
 * System prototypes
 */
#ifdef __STDC__
int	system( char * );
char *	mktemp( char * );
int	getopt( int, char **, char * );
#endif __STDC__

typedef struct {
    char *	name;
    char *	value;
} TraceVar;

#define Array(type, size)  struct { type a[(size)]; int n; }

static Array(PROTOCOL, 200)	instance;
static Array(PROTOCOL *, 200) 	local_p;
static Array(PROTOCOL *, 200) 	unique_p;
static Array(TraceVar, 200)	traceVar;
static Array(char *, 50)	protTbl;   /* Names of protocol table files */
static Array(char *, 50)	localdir;  /* dirs containing local prots   */
static int 	nerrors = 0;
static int	fastFlag;
static int	makeFileFlag;
static char	*makeFileName = "protocols.sed";
static char *	libraries[20];
static int	last_driver = -1;
static FILE *	m_file;
static FILE *	h_file;
static FILE *	prot_file;
static FILE *	trace_file;
static FILE *	tbl_file;
static FILE *	tmp_file;
static char *	tmpFileName = "/tmp/composeNamesXXXXXX";
#define volatileWarning \
  " *\n * Warning: this file is generated from graph.comp and is overwritten\n * every time 'make compose' is run\n"


#define strsame(A,B) (!(strcmp((A),(B))))

static void	checkConsistency( void );
static int	find_index( ProtName * );
static int	findPath( PROTOCOL * );
static void	gen( void );
static void	gen_code( FILE * );
static void	gen_defs( FILE * );
static void	gen_make( FILE * );
static void	gen_sed( FILE * );
static void	gen_trace_file( FILE * );
static void	gen_tbl_file( FILE * );
static int	genProtlCode( FILE *, PROTOCOL * );
static void	initTmpFile( void );
static char*	make_define( char * );
#if 0
static void	print_protocol( PROTOCOL * );
#endif
static char*	suffix( char * );


static void
usage()
{
    fprintf(stderr, "usage: compose [-h] [-f] [-m mfile] [library ...]\n");
}


void
main( int argc, char **argv )
{
    extern	char *	optarg;
    extern	int	optind;
    char	c;
    int 	i=0; 
    
    while ((c = getopt(argc, argv, "fm:h")) != -1 ) {
      switch (c) {
	case 'f':
	  fastFlag = 1;
	  break;
	case 'm':
	  makeFileFlag = 1;
	  makeFileName = optarg;
	  break;
	case '?':
	case 'h':
	  usage();
	  exit(1);
      }
    }
    for (; optind < argc; optind++) {
	libraries[i++] = argv[optind];
    }

    libraries[i] = (char *)0;
    parse();
    checkConsistency();
    gen();
    exit(0);
}


static FILE *
createFile( char *name )
{
    FILE *	f;

    if ( ! strcmp(name, "-") ) {
	return stdout;
    }
    unlink(name);
    f = fopen(name, "w");
    if (f == NULL) {
	errorFile(name);
    }
    return f;
}


static void
initTmpFile()
{
    char	cmd[200];
    int		i;

    for ( i=0; libraries[i]; i++ ) {
	sprintf(cmd, "nm -g %s | grep '_init$' >> %s",
		libraries[i], tmpFileName);
	system(cmd);
    }
}


static void
gen()
{
    m_file = createFile(makeFileName);
    h_file = createFile("protocols.h");
    prot_file = createFile("protocols.c");
    tbl_file = createFile("protTbl.c");
    trace_file = createFile("traceLevels.c");
    mktemp(tmpFileName);
    tmp_file = createFile(tmpFileName);
    
    if ( ! ( m_file && h_file && prot_file && tbl_file && trace_file &&
	     tmp_file) ) {
	exit (1);
    }
    
    if ( ! fastFlag ) {
	initTmpFile();
    }
    if ( makeFileFlag ) {
	gen_make(m_file);
    } else {
	gen_sed(m_file);
    }
    fclose(m_file);
    gen_defs(h_file);
    fclose(h_file);
    gen_code(prot_file);
    fclose(prot_file);
    gen_tbl_file(tbl_file);
    fclose(tbl_file);
    gen_trace_file(trace_file);
    fclose(trace_file);
    if ( ! fastFlag ) {
	unlink(tmpFileName);
    }
}


static char *
suffix(char *s)
{
  return strsame(&s[strlen(s) - 2], "_s") ? "S" : "c";
}


/*
 * findPath -- searches for the protocol's source code if the location
 * was not explicitly specified. 
 * Forms list of search directories from those in the 'paths'
 * array with the substitutions:
 * 	a '?' is replaced with the name of the protocol
 * 	a '*' is a wildcard
 * There can be at most one of the substitution characters in a 'paths'
 * entry.  If a directory in the search list containing the source code is
 * found, it is assigned to p->path.
 * 	RETURNS:
 * 		1 if a directory was found.
 *		0 if no directory was found.
 */
static int
findPath(PROTOCOL *p)
{
  static char *paths[] = {
    "./",
    "./?/",
#if 0
    "../protocols/",
    "../protocols/?/",
    "../drivers/*/",
#endif
    0
    };
  char *t, *where, *f, buffer[128], dir[128];
  DIR *directory;
  struct DIRENT *dd;
  int i;

  if (! p->numfiles) {
    p->files[0] = p->n.name;
    p->numfiles = 1;
  }
  if (p->path) {
    /*
     * A path was explicitly specified in the graph file
     */
    sprintf(dir, "%s/", p->path);
    p->path = xerox(dir);
    if (! opendir(p->path)) {
      warnCouldNotAccess(p->path);
    } else {
      for (i=0; i < p->numfiles; i++) {
	sprintf(buffer, "%s%s.%s", p->path, p->files[i], suffix(p->files[i]));
	if (access(buffer, R_OK)) {
	  warnCouldNotAccess(buffer);
	}
      }
    }
    return 1;
  }
  f = p->files[0];
  for (i = 0; (t = paths[i]); i++) {
    if (where = index(t, '*')) {
      *where = '\0';
      directory = opendir(t);
      while ((dd = readdir(directory)) != NULL) {
	sprintf(dir, "%s%s/", t, dd->d_name);
	sprintf(buffer, "%s%s.%s", dir, f, suffix(f));
	if (access(buffer, 4) == 0) {
	  *where = '*';
	  closedir(directory);
	  p->path = xerox(dir);
	  return 1;
	}
      }
      *where = '*';
      closedir(directory);
    } else if (where = index(t, '?')) {
      *where = '\0';
      sprintf(dir, "%s%s/", t, p->n.name);
      sprintf(buffer, "%s%s.%s", dir, f, suffix(f));
      if (access(buffer, 4) == 0) {
	*where = '?';
	p->path = xerox(dir);
	return 1;
      }
      *where = '?';
    } else {
      sprintf(buffer, "%s%s.%s", t, f, suffix(f));
      if (access(buffer, 4) == 0) {
	p->path = xerox(t);
	return 1;
      }
    }
  }
  return 0;
}


/* 
 * mkLocalProts -- Find out which protocols are to be compiled
 * "locally" (in the user's build directory).  Add an entry to local_p
 * for each of these protocols (but multiple instances have only one
 * entry.)  Also add an entry to localdirs for each local directory in
 * which protocols will be compiled.
 */
static void
mkLocalProts()
{
    int	i, j;
    char cmd[200];

    /*
     * Current directory should always be in the "localdir" list so a
     * compilation rule will be generated for it since
     * some files are compiled there (localinit, protocols) even if no
     * user-specified files are.
     */
    localdir.a[localdir.n++] = "./";
    for (i = 0; i < instance.n; i++) {
	PROTOCOL *p = &instance.a[i];

	if (findPath(p)) {
	    /* 
	     * See if this protocol already has a local_p entry
	     */
	    for ( j=0; j < local_p.n; j++ ) {
		if ( strsame(local_p.a[j]->n.name, p->n.name) ) {
		    break;
		}
	    }
	    if ( j != local_p.n ) {
		continue;
	    }
	    local_p.a[local_p.n++] = p;
	    /* 
	     * See if this protocol's local directory is already in
	     * the localdir array.
	     */
	    for (j=0; j < localdir.n; j++) {
		if (strsame(localdir.a[j], p->path)) {
		    break;
		}
	    }
	    if (j == localdir.n) {
		localdir.a[localdir.n++] = p->path;
	    }
	} else {
	    /*
	     * look for protocol init routine in the libraries and warn
	     * if it is not found
	     */
	    if ( ! fastFlag ) {
		for (j=0; libraries[j]; j++) {
		    sprintf(cmd, "grep '%s_init$' %s > /dev/null",
			    p->n.name, tmpFileName);
		    if (! system(cmd)) 
		      break;
		}
		if (! libraries[j]) {
		    warnProtNotFound(p->n.name); 
		}
	    }
	}
    }
}


/* 
 * Make sure all other protocols referenced by a protocol are defined.
 * Also sets the 'index' field of the ProtName structures in the
 * 'down' field of each protocol, as well as the 'index' field of the
 * protocol itself.
 */
static void
checkConsistency()
{
    int i, j;
    int di;
    
    for (i = 0; i < instance.n; i++) {
	for (j = 0; j < instance.a[i].numdown; j++) {
	    di = find_index(&instance.a[i].down[j]);
	    if (di == -1) {
		errorProtlUndefined(instance.a[i].down[j].name,
				    instance.a[i].n.name);
		nerrors++;
	    }
	    instance.a[i].down[j].index = di;
	}
	instance.a[i].n.index = i;
    }
    if (nerrors) exit(1);
}



static void
gen_sed(FILE *fptr)
{
    int i, j;
    PROTOCOL	*p;
    
    mkLocalProts();
    /*
     * Add dependencies for locally compiled files
     */
    fprintf(fptr, "/^# PCDEFINITIONS BEGIN HERE/,/^# PCDEFINITIONS END HERE/c\\\n");
    fprintf(fptr, "# PCDEFINITIONS BEGIN HERE\\\n");
    fprintf(fptr, "# Don't edit these lines, compose uses them\\\n");
    fprintf(fptr, "PCDOTOS = ");
    for (i = 0; i < local_p.n; i++) {
	p = local_p.a[i];
	if (p->numfiles) {
	    for (j = 0; j < p->numfiles; j++) {
		fprintf(fptr, "%s${HOW}/%s.o \\\\\\\n", p->path, p->files[j]);
	    }
	} else {
	    fprintf(fptr, "%s${HOW}/%s.o \\\\\\\n", p->path, p->n.name);
	}
    }
    fprintf(fptr, "\\\n");
    fprintf(fptr, "PCDOTCS = ");
    for (i = 0; i < local_p.n; i++) {
	p = local_p.a[i];
	if (p->numfiles) {
	    for (j = 0; j < p->numfiles; j++) {
		if (suffix(p->files[j])[0] == 'c') {
		    fprintf(fptr, "%s%s.c \\\\\\\n", p->path, p->files[j]);
		}
	    }
	} else {
	    if (suffix(p->n.name)[0] == 'c') {
		fprintf(fptr, "%s%s.c \\\\\\\n", p->path, p->n.name);
	    }
	}
    }
    fprintf(fptr, "\\\n");
    fprintf(fptr, "PCDOTSS = ");
    for (i = 0; i < local_p.n; i++) {
	p = local_p.a[i];
	if (p->numfiles) {
	    for (j = 0; j < p->numfiles; j++) {
		if (suffix(p->files[j])[0] == 'S') {
		    fprintf(fptr, "%s%s.S ", p->path, p->files[j]);
		}
	    }
	} else {
	    if (suffix(local_p.a[i]->n.name)[0] == 'S') {
		fprintf(fptr, "%s%s.S ", p->path, p->n.name);
	    }
	}
    }
    fprintf(fptr, "\\\n");
    fprintf(fptr, "# PCDEFINITIONS END HERE\n");
    
    /*
     * One implicit rule per local directory
     */
    fprintf(fptr, "/^# IMPLICIT RULES BEGIN HERE/,/^# IMPLICIT RULES END HERE/c\\\n");
    fprintf(fptr, "# IMPLICIT RULES BEGIN HERE\\\n");
    fprintf(fptr, "# Don't edit these lines, compose uses them\\\n\\\n");
    for (i = 0; i < localdir.n; i++) {
	fprintf(fptr, "%s$(HOW)/%%.o : %s%%.c\\\n",
		localdir.a[i], localdir.a[i]);
	fprintf(fptr, "\\\t$(DEFAULTRULE)\\\n\\\n");
    }
    fprintf(fptr, "# IMPLICIT RULES END HERE\n");
}


static void
gen_make(FILE *fptr)
{
    int i, j;
    PROTOCOL	*p;
    
    mkLocalProts();
    /*
     * Add dependencies for locally compiled files
     */
    fprintf(fptr, "PCDOTOS = ");
    for (i = 0; i < local_p.n; i++) {
	p = local_p.a[i];
	if (p->numfiles) {
	    for (j = 0; j < p->numfiles; j++) {
		fprintf(fptr, "%s${HOW}/%s.o \\\n", p->path, p->files[j]);
	    }
	} else {
	    fprintf(fptr, "%s${HOW}/%s.o \\\n", p->path, p->n.name);
	}
    }
    fprintf(fptr, "\n");
    fprintf(fptr, "PCDOTCS = ");
    for (i = 0; i < local_p.n; i++) {
	p = local_p.a[i];
	if (p->numfiles) {
	    for (j = 0; j < p->numfiles; j++) {
		if (suffix(p->files[j])[0] == 'c') {
		    fprintf(fptr, "%s%s.c \\\n", p->path, p->files[j]);
		}
	    }
	} else {
	    if (suffix(p->n.name)[0] == 'c') {
		fprintf(fptr, "%s%s.c \\\n", p->path, p->n.name);
	    }
	}
    }
    fprintf(fptr, "\n");
    fprintf(fptr, "PCDOTSS = ");
    for (i = 0; i < local_p.n; i++) {
	p = local_p.a[i];
	if (p->numfiles) {
	    for (j = 0; j < p->numfiles; j++) {
		if (suffix(p->files[j])[0] == 'S') {
		    fprintf(fptr, "%s%s.S ", p->path, p->files[j]);
		}
	    }
	} else {
	    if (suffix(local_p.a[i]->n.name)[0] == 'S') {
		fprintf(fptr, "%s%s.S ", p->path, p->n.name);
	    }
	}
    }
    fprintf(fptr, "\n\n");
    
    /* 
     * Create inclusion flags for local directories
     */
    fprintf(fptr, "COMPOSE_INCLUDES = ");
    for (i = 0; i < localdir.n; i++) {
	char	dir[1024];

	/* 
	 * The local directory is stored with a trailing '/' which we
	 * need to strip off.
	 */
	strcpy(dir, localdir.a[i]);
	dir[strlen(dir) - 1] = 0;
	fprintf(fptr, "-I%s ", dir);
    }
    fprintf(fptr, "\n\n");

    /*
     * One implicit rule per local directory
     */
    for (i = 0; i < localdir.n; i++) {
	fprintf(fptr, "%s$(HOW)/%%.o : %s%%.c\n",
		localdir.a[i], localdir.a[i]);
	fprintf(fptr, "\t$(DEFAULTRULE)\n\n");
    }
}


static int
find_index( ProtName *pName )
{
    int 	i;
    ProtName	*instName;

    for (i = 0; i < instance.n; i++) {
	instName = &instance.a[i].n;
	if ( strsame(pName->name, instName->name) &&
	     strsame(pName->instance, instName->instance) ) {
	    return i;
	}
    }
    return -1;
}


static void
end_driver_build(FILE *fptr)
{
  static int done = 0;
  if (! done) {
    fprintf(fptr, "}\n\n");
    fprintf(fptr, "\nvoid\nbuild_pgraph()\n");
    fprintf(fptr, "{\n");
    done = 1;
  }
}


static int
genProtlCode( fptr, p )
    FILE	*fptr;
    PROTOCOL	*p;
{
    PROTOCOL	*llp;
    int		i;

    if ( p->initState == INIT_DONE ) {
	return 0;
    }
    if ( p->initState == INIT_WORKING ) {
	/* 
	 * We have a cycle.
	 */
	errorCycle();
	errorCycleName(p->n.name);
	return -1;
    }
    assert(p->initState == INIT_NOT_DONE);
    p->initState = INIT_WORKING;
    for ( i = 0; i < p->numdown; i++ ) {
	llp = &instance.a[p->down[i].index];
	if ( genProtlCode(fptr, llp) ) {
	    errorCycleName(p->n.name);
	    return -1;
	}
    }
    fprintf(fptr, "\n  /* building protocol %s */\n", make_define(p->n.name));
    for ( i = 0; i < p->numdown; i++ ) {
	fprintf(fptr, "  argv[%d] = protl_tab[%d];\n", i, p->down[i].index);
    }
    fprintf(fptr, "  protl_tab[%d] = xCreateProtl(%s_init, \"%s\", \"%s\",",
	    p->n.index, p->n.name, p->n.name, p->n.instance);
    fprintf(fptr, " %d, argv);\n", p->numdown);
    fprintf(fptr, "  if ( protl_tab[%d] == ERR_XOBJ ) {\n", p->n.index);
    fprintf(fptr, "    Kabort(\"Could not create %s protocol\");\n",
	    p->n.name);
    fprintf(fptr, "  }\n");
    fprintf(fptr, "\n");
    p->initState = INIT_DONE;
    return 0;
}


static void
gen_code(FILE *fptr)
{
    int i;
    
    fprintf(fptr, "/*\n * protocol initialization calls\n");
    fprintf(fptr, volatileWarning);
    fprintf(fptr, " */\n\n");
    fprintf(fptr, "#include \"upi.h\"\n");
    fprintf(fptr, "#include \"x_util.h\"\n");
    fprintf(fptr, "#include \"compose.h\"\n");
    fprintf(fptr, "#include \"protocols.h\"\n");
    fprintf(fptr, "\n\n");
    fprintf(fptr, "XObj\t\tprotl_tab[%d+1];\n", instance.n);
    fprintf(fptr, "static XObj\targv[%d];\n", instance.n);
    
    fprintf(fptr, "\n\nvoid\nbuild_pgraph_dev()\n");
    fprintf(fptr, "{\n");
    
    for (i = 0; i < instance.n; i++) {
	PROTOCOL	*p = &instance.a[i];
	if (i == (last_driver + 1)) {
	    end_driver_build(fptr);
	}
	if ( genProtlCode(fptr, p) ) {
	    finishErrorCycle();
	}
    }
    end_driver_build(fptr);
    fprintf(fptr, "}\n");
}


static void
gen_defs( FILE *fptr )
{
    int i;
    
    fprintf(fptr, "/*\n * trace variable and init function declarations\n");
    fprintf(fptr, volatileWarning);
    fprintf(fptr, " */\n\n");
    for ( i=0; i < traceVar.n; i++ ) {
	fprintf(fptr, "extern int %s;\n", traceVar.a[i].name);
    }
    for ( i=0; i < unique_p.n; i++ ) {
	char *name = unique_p.a[i]->n.name;
	fprintf(fptr, "extern void %s_init();\n", name);
    }
}


char *
make_define(char *cptr)
{
  char buf[200];
  int i;

  for (i = 0; i < strlen(cptr); i++) {
    if (islower(cptr[i])) {
      buf[i] = toupper(cptr[i]);
    } else {
      buf[i] = cptr[i];
    }
  }
  buf[i] = '\0';
  return (xerox(buf));
}

#if 0

static void
print_protocol(PROTOCOL *p)
{
  int i;

  printf("name = %s\n", p->n.name);
  printf("numfiles = %d\n", p->numfiles);
  for (i = 0; i < p->numfiles; i++) {
    printf("file[%d] = %s\n", i, p->files[i]);
  }
  printf("numdown = %d\n", p->numdown);
  for (i = 0; i < p->numdown; i++) {
    printf("down[%d] = %s\n", i, p->down[i]);
  }
}

#endif


void
addProtTbl( char *name )
{
    char buf[256];

    assert(strlen(name) < 254);
    if ( strsame(name, "DEFAULT") ) {
	name = "PROTOCOL_TABLE";
    } else {
	sprintf(buf, "\"%s\"", name);
	name = xerox(buf);
    }
    protTbl.a[protTbl.n++] = name;
}


void
addTraceVar( char *name, char *value )
{
    int	i;
    /* 
     * Make sure there are no previous declarations of this trace variable
     * This can happen for multiple protocol instantiations and isn't really
     * an error.
     */
    for ( i=0; i < traceVar.n; i++ ) {
	if ( strsame(traceVar.a[i].name, name) ) {
	    return;
	}
    }
    traceVar.a[traceVar.n].name = name;
    traceVar.a[traceVar.n].value = value;
    traceVar.n++;
}


void
addInstance( PROTOCOL *p )
{
    int	i;

    /* 
     * If there are other previous instances of this protocol, make sure
     * this one doesn't have a path or files listing
     */
    for ( i=0; i < instance.n; i++ ) {
	if ( strsame(instance.a[i].n.name, p->n.name) ) {
	    /* 
	     * Previous instance exists
	     */
	    if ( p->path || p->numfiles ) {
		errorLaterInstanceFiles(p);
	    }
	    break;
	}
    }
    p->initState = INIT_NOT_DONE;
    instance.a[instance.n] = *p;
    if ( i == instance.n ) {
	/* 
	 * No previous instance exists, add this protocol to 'unique_p'
	 */
	unique_p.a[unique_p.n++] = &instance.a[instance.n];
    }
    instance.n++;
}


void
lastDriver()
{
    last_driver = instance.n - 1;
}


/* 
 * Generate a file which defines the array of protocol tables to be loaded 
 */
static void
gen_tbl_file( FILE *f )
{
    int	i;

    fprintf(f, "/*\n * user-defined protocol tables\n");
    fprintf(f, volatileWarning);
    fprintf(f, " */\n\n");
    fprintf(f, "#include \"site.h\"\n\n");
    fprintf(f, "#include \"compose.h\"\n\n");
    fprintf(f, "char *\nprotocolTables[%d] = {\n", protTbl.n + 1);
    if ( protTbl.n > 0 ) {
	for ( i = 0; i < protTbl.n; i++ ) {
	    fprintf(f, "    %s,\n", protTbl.a[i]);
	}
    }
    fprintf(f, "    0\n");
    fprintf(f, "};\n");
}


/* 
 * Generate a file which initializes the trace variables
 */
static void
gen_trace_file( FILE *f )
{
    int	i;

    fprintf(f, "/*\n * trace variable initialization\n");
    fprintf(f, volatileWarning);
    fprintf(f, " */\n\n");
    fprintf(f, "#include \"xk_debug.h\"\n");
    fprintf(f, "#include \"compose.h\"\n");
    fprintf(f, "#include \"protocols.h\"\n");
    fprintf(f, "\n");
    fprintf(f, "void\ninitTraceLevels()\n{\n");
    fprintf(f, "#ifdef XK_DEBUG\n\n");
    for ( i = 0; i < traceVar.n; i++ ) {
	fprintf(f, "    %s = %s;\n", traceVar.a[i].name, traceVar.a[i].value);
    }
    fprintf(f, "\n#endif XK_DEBUG\n");
    fprintf(f, "}\n");
}


