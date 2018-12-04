/*
 * $RCSfile: truput.c,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/02 00:16:39 $
 */

#include <ctype.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>

#define DEFAULT_LEN	(1024*1024)
#define BUF_SIZE	(8192)
#define USECS_PER_SEC	(1000*1000)

extern char *optarg;
extern int optind;

static char *pgm_name;
static char hostname[MAXHOSTNAMELEN];

/* throughput measurement of a connection */

static void
usage()
{
    fprintf(stderr, 
	    "usage: %s -c hostname [ -f pri ] [ -l length ]\n", pgm_name);
    fprintf(stderr, "       %s -s [ -f pri ]\n", pgm_name);
    fprintf(stderr, "\t-c: run as client talking to host HOSTNAME\n");
    fprintf(stderr,
	  "\t    (HOSTNAME can be either internet number or official name)\n");
    fprintf(stderr, "\t-s: run as server\n");
    fprintf(stderr,
	    "\t-f: run with fixed scheduling priority PRI (if PRI > 0)\n");
    fprintf(stderr, "\t-l: transmit LENGTH bytes\n");
    fprintf(stderr,
	    "\tdefaults: client, no fixed sche, %d bytes of data\n",
	    DEFAULT_LEN);
    exit(1);
} /* usage */


#ifdef MACH

#include <mach.h>

void
fixed_priority_scheduling(int priority)
{
    kern_return_t kr;
    processor_set_t set;
    processor_set_name_t name;

    /* a priority of 0 is interpreted as no fixed priority scheduling: */
    if (!priority) {
	return;
    } /* if */

    kr = thread_get_assignment(mach_thread_self(), &name);
    if (kr != KERN_SUCCESS) {
	quit(1, "fixed_priority_scheduling: thread_get_assignment(): %s\n",
	     mach_error_string(kr));
    } /* if */

    kr = host_processor_set_priv(mach_host_priv_self(), name, &set);
    if (kr != KERN_SUCCESS) {
	quit(1, "fixed_priority_scheduling: host_processor_set_priv(): %s\n",
	     mach_error_string(kr));
    } /* if */

    kr = processor_set_policy_enable(set, POLICY_FIXEDPRI);
    if (kr != KERN_SUCCESS) {
	quit(1,
	     "fixed_priority_scheduling: processor_set_policy_enable(): %s\n",
	     mach_error_string(kr));
    } /* if */

    kr = thread_policy(mach_thread_self(), POLICY_FIXEDPRI, priority);
    if (kr != KERN_SUCCESS) {
	quit(1, "fixed_priority_scheduling: thread_policy: %s\n",
	     mach_error_string(kr));
    } /* if */
} /* fixed_priority_scheduling */

#else /* MACH */

void
fixed_priority_scheduling(int priority)
{
    /* a priority of 0 is interpreted as no fixed priority scheduling: */
    if (!priority) {
	return;
    } /* if */

    fprintf(stderr,
	    "%s: fixed priority scheduling supported for Mach systems only\n",
	    pgm_name);
    exit(1);

} /* fixed_priority_scheduling */

#endif /* MACH */


void
client(int s, int len)
{
    char buf[BUF_SIZE];
    struct timeval start, stop;
    double delta;
    int toread;
    int nread;

    /* tell server how much data it has to send: */
    write(s, &len, sizeof(len));

    toread = len;

    /* start timer: */
    if (gettimeofday(&start, (struct timezone*) 0) < 0) {
	perror("gettimeofday");
	exit(1);
    } /* if */

    /* start receiving messages: */
    while (toread) {
	/* receive message from server */
	nread = read(s, buf, sizeof(buf));
	if (nread > 0) {
	    toread -= nread;
	    if (!toread) {
		break;
	    } /* if */
	} else {
	    perror("read");
	    exit(1);
	} /* if */
    } /* while */

    /* stop timer */
    if (gettimeofday(&stop, (struct timezone *) 0) < 0) { 
	perror("gettimeofday");
	exit(1);
    } /* if */

    delta = (stop.tv_sec - start.tv_sec) +
      (stop.tv_usec - start.tv_usec) / (double) USECS_PER_SEC;

    printf("Remote Host: %s, Protocol: TCP, Length: %d\n", 
	   hostname, len); 
    printf("Average throughput: %g KB/sec\n",
	   (len / 1024.0) / delta);
} /* client */


void
tcp_server(int s)
{
    int len;
    int i;
    char buf[BUF_SIZE];

    /* initialize buffer with something: */
    for (i = 0; i < BUF_SIZE; i++) {
	buf[i] = i;
    } /* for */

    listen(s, 1);
    while (1) {
	int ns;
	int towrite;

	if ((ns = accept(s, 0, 0)) < 0) {
	    perror("accept");
	    exit(1);
	} /* if */

	if (read(ns, &len, sizeof(len)) != sizeof(len)) {
	    perror("read");
	    exit(1);
	} /* if */

	while (1) {
	    towrite = sizeof(buf);
	    if (towrite > len) {
		towrite = len;
	    } /* if */
	    if (write(ns, buf, towrite) != towrite) {
		perror("write");
		exit(1);
	    } /* if */
	    len -= towrite;
	    if (!len) {
		close(ns);
		break;
	    } /* if */
	} /* while */
    } /* while */
} /* tcp_server */


void
main(int argc, char **argv)
{
    struct sockaddr_in server; /* internet style socket address */
    struct hostent *hp;    /* host    */
    int s;                 /* socket */
    unsigned long host_addr = 0; /* host internet address */

    int is_server = 0;
    int c;
    int len = DEFAULT_LEN;	   /* length of messages to be transmitted */
    int port = 8998;
    int fixed_prio = 0;

    pgm_name = argv[0];

    /* check for arguments */
    while ((c = getopt(argc, argv, "c:f:sl:")) != -1) {
	switch (c) {
	  case 'c':
	    is_server = 0;
	    if (isdigit(optarg[0])) {
		host_addr = inet_addr(optarg);
	    } else {
		strcpy(hostname, optarg);
	    } /* if */
	    break;
	  case 'f':
	    fixed_prio = atoi(optarg);
	    break;
	  case 's':
	    is_server = 1;
	    break;
	  case 'l':
	    len = atoi(optarg);
	    if ((len < 1) || (len > BUF_SIZE)) {
		fprintf(stderr,
			"%s: message length should be in range 1..%d\n",
			pgm_name, BUF_SIZE);
		exit(1);
	    } /* if */
	    break;
	  case '?':
	    usage();
	} /* switch */
    } /* while */

    if (optind < argc) {
	usage();
    } /* if */

    fixed_priority_scheduling(fixed_prio);

    /* create an unnamed socket */
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	perror("socket/tcp");
	exit(1);
    } /* if */

    /* setup server address: */
    bzero((char *)&server, sizeof(server));
    server.sin_port = htons(port);
    
    if (is_server) {
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_family = AF_INET;
	if (bind(s, (struct sockaddr*) &server, sizeof(server)) < 0) {
	    perror("bind");
	    exit(1);
	} /* if */

	tcp_server(s);
    } else {
	if (host_addr) {
	    /* host address specified in numeric format: */
	    server.sin_addr.s_addr = host_addr;
	    server.sin_family = AF_INET;
	} else {
	    /* host address specified via its name: */
	    hp = gethostbyname(hostname);
	    if (hp == NULL) {
		fprintf(stderr, "%s: unknown host `%s'\n", pgm_name, hostname);
		exit(1);
	    } /* if */
	    bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);
	    server.sin_family = hp->h_addrtype;
	} /* if-elseif */

	/* connect to server: */
	if (connect(s, (struct sockaddr*) &server, sizeof(server)) < 0) {
	    perror("connect");
	    exit(1);
	} /* if */
	client(s, len);
    } /* if */

    /* discard the socket */
    close(s);
} /* main */

			/*** end of rtrip.c ***/
