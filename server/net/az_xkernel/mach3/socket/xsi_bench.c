/*
 * $RCSfile: xsi_bench.c,v $
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/02 00:09:34 $
 * $Author: menze $
 *
 * $Log: xsi_bench.c,v $
 * Revision 1.2  1993/02/02  00:09:34  menze
 * copyright change
 *
 * Revision 1.1  1992/12/01  22:16:40  menze
 * Initial revision
 *
 */
#include "xsi_bench.h"
#include "util.h"


#define DEFAULT_TRIPS		(100)
#define DEFAULT_RTRIP_LEN	(1)
#define DEFAULT_TRUPUT_LEN	(1024*1024)
#define USECS_PER_MSEC		(1000)
#define USECS_PER_SEC		(1000*USECS_PER_MSEC)
#define BUF_SIZE		(8192)

bool xsi_i_am_server = FALSE;
bool xsi_i_am_client = FALSE;
int  xsi_fixed_prio  = 0;

static char *hostname = "localhost";
static int pid;
static int rounds = DEFAULT_TRIPS;
static int len = DEFAULT_RTRIP_LEN;
static bool throughput = FALSE;
static bool udp = FALSE;
static bool len_specified = FALSE;
static int port = 8999;
static long server_addr;


static void
usage(void)
{
    fprintf(stderr, "usage: %s [ -f pri ]\n", globalArgv[0]);
    fprintf(stderr,
  "       %s -c saddr [ -f pri ] [ -r rounds ] [ -l length ] [ -tu ]\n",
	    globalArgv[0]);
    fprintf(stderr,
	    "       %s -s [ -f pri ] [ -tu ]\n", globalArgv[0]);
    fprintf(stderr, "\t-f: run with fixed priority PRI (if PRI > 0)\n");
    fprintf(stderr,
     "\t-c: run as benchmark client talking to host SADDR\n");
    fprintf(stderr, "\t    (SADDR expected in numeric form)\n");
    fprintf(stderr, "\t-s: run as benchmark server\n");
    fprintf(stderr, "\t-r: number of rounds to execute (round-trip only)\n");
    fprintf(stderr, "\t-l: length buffers to be exchanged in round-trip\n");
    fprintf(stderr, "\t    size of data to be transmitted in throughput\n");
    fprintf(stderr, "\t-t: run throughput test instead of round-trip\n");
    fprintf(stderr, "\t-u: use UDP instead of TCP (round-trip only)\n");
    exit(1);
} /* usage */


void
xsi_bench_process_options(void)
{
    int c;
    extern int optind;
    extern char *optarg;

    while ((c = getopt(globalArgc, globalArgv, "c:f:sr:l:tu")) != EOF) {
	switch (c) {
	  case 'c':
	    xsi_i_am_server = FALSE;
	    xsi_i_am_client = TRUE;
	    server_addr = inet_addr(optarg);
	    hostname = optarg;
	    break;
	  case 'f':
	    xsi_fixed_prio = atoi(optarg);
	    break;
	  case 's':
	    xsi_i_am_client = FALSE;
	    xsi_i_am_server = TRUE;
	    break;
	  case 'r':
	    rounds = atoi(optarg);
	    break;
	  case 'l':
	    len = atoi(optarg);
	    len_specified = TRUE;
	    break;
	  case 't':
	    throughput = !throughput;
	    if (throughput) {
		port = 8998;
	    } else {
		port = 8999;
	    } /* if */
	    if (!len_specified) {
		len = DEFAULT_TRUPUT_LEN;
	    } /* if */
	    break;
	  case 'u':
	    udp = !udp;
	    break;
	  case '?':
	  default:
	    usage();
	} /* switch */
    } /* while */
    if (optind < globalArgc) {
	usage();
    } /* if */
} /* xsi_bench_process_options */


static void
tcp_truput_server(int s)
{
    int len;
    int i;
    char buf[BUF_SIZE];

    /* initialize buffer with something: */
    for (i = 0; i < BUF_SIZE; i++) {
	buf[i] = i;
    } /* for */

    do_listen(0, pid, s, 1, &errno);
    if (errno != ESUCCESS) {
	perror("do_listen");
	exit(1);
    } /* if */

    while (1) {
	void *bufp;
	int ns = s + 1;
	int towrite;
	int nwritten;
	int nread;
	struct sockaddr fromaddr;
	int fromlen;

	do_accept(0, pid, s, &fromaddr, &fromlen, ns, &errno);
	if (errno != ESUCCESS) {
	    perror("do_accept");
	    exit(1);
	} /* if */

	bufp = &len;
	nread = sizeof(len);
	do_read(0, pid, ns, &bufp, &nread, sizeof(len), &errno);
	if (nread != sizeof(len)) {
	    perror("do_read");
	    exit(1);
	} /* if */

	while (1) {
	    towrite = sizeof(buf);
	    if (towrite > len) {
		towrite = len;
	    } /* if */
	    do_write(0, pid, ns, buf, towrite, &nwritten, &errno);
	    if (nwritten != towrite) {
		perror("do_write");
		exit(1);
	    } /* if */
	    len -= towrite;
	    if (!len) {
		do_close(0, pid, ns, &errno);
		break;
	    } /* if */
	} /* while */
    } /* while */
} /* tcp_truput_server */


static void
truput_client(int s, int len)
{
    char buf[BUF_SIZE];
    struct timeval start, stop;
    double delta;
    int toread;
    int nread;
    int nwritten;
    void *bufp;

    /* tell server how much data it has to send: */
    do_write(0, pid, s, &len, sizeof(len), &nwritten, &errno);

    toread = len;

    /* start timer: */
    if (gettimeofday(&start, (struct timezone*) 0) < 0) {
	perror("gettimeofday");
	exit(1);
    } /* if */

    /* start receiving messages: */
    while (toread) {
	/* receive message from server */
	bufp = &buf;
	nread = sizeof(buf);
	do_read(0, pid, s, &bufp, &nread, nread, &errno);
	if (nread > 0) {
	    toread -= nread;
	    if (!toread) {
		break;
	    } /* if */
	} else {
	    perror("do_read");
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
} /* truput_client */


static void
tcp_rtrip_server(int s)
{
    do_listen(0, pid, s, 1, &errno);
    if (errno != ESUCCESS) {
	perror("do_listen");
	exit(1);
    } /* if */

    while (1) {
	int len;
	int ns = s + 1;
	char *bufp;
	char buf[BUF_SIZE];
	int nread;
	int toread;
	int nwritten;
	struct sockaddr fromaddr;
	int fromlen;

	do_accept(0, pid, s, &fromaddr, &fromlen, ns, &errno);
	if (errno != ESUCCESS) {
	    perror("do_accept");
	    exit(1);
	} /* if */

	bufp = (char*) &len;
	nread = sizeof(len);
	do_read(0, pid, ns, &bufp, &nread, sizeof(len), &errno);
	if (nread != sizeof(len)) {
	    perror("do_read");
	    exit(1);
	} /* if */

	while (1) {
	    toread = len;
	    bufp = buf;

	    while (1) {
		nread = sizeof(buf);
		do_read(0, pid, ns, &bufp, &nread, nread, &errno);
		if (nread > 0) {
		    toread -= nread;
		    bufp += nread;
		    if (!toread) {
			break;
		    } /* if */
		} else if (nread == 0) {
		    /* connection closed: */
		    do_close(0, pid, ns, &errno);
		    goto done;
		} else {
		    perror("do_read");
		    exit(1);
		} /* if */
	    } /* while */
	    /* send message back: */
	    do_write(0, pid, ns, buf, len, &nwritten, &errno);;
	} /* while */
      done:;
    } /* while */
} /* tcp_rtrip_server */


static void
udp_rtrip_server(int s)
{
    while (1) {
	int nread;
	int nsent;
	char buf[BUF_SIZE];
	char *bufp;
	struct sockaddr from;
	int fromlen;

	fromlen = sizeof(from);
	bufp = buf;
	nread = BUF_SIZE;
	do_recvfrom(0, pid, s, &bufp, &nread, nread, 0, &from, &fromlen,
		    &errno);
	if (nread > 0) {
	    do_sendto(0, pid, s, buf, nread, &nsent, 0, &from, fromlen,
		      &errno);
	    if (errno != ESUCCESS) {
		perror("do_sendto");
		exit(1);
	    } /* if */
	} else {
	    perror("do_recvfrom");
	    exit(1);
	} /* if */
    } /* while */
} /* udp_rtrip_server */


static void
rtrip_client(int s, int rounds, int len, bool udp)
{
    char buf[BUF_SIZE];
    char *bufp;
    struct timeval start, stop;
    double delta;
    int i;
    int n;
    int nread;
    int nwritten;

    /* initialize buffer with something: */
    for (i = 0; i < BUF_SIZE; i++) {
	buf[i] = i;
    } /* for */

    /* start timer */
    if (gettimeofday(&start, (struct timezone*) 0) < 0) {
	perror("gettimeofday");
	exit(1);
    } /* if */

    if (!udp) {
	nwritten = sizeof(len);
	do_write(0, pid, s, &len, nwritten, &nwritten, &errno);
	if (nwritten != sizeof(len)) {
	    perror("do_write");
	    exit(1);
	} /* if */
    } /* if */

    /* start sending and receiving messages */
    for (i = 0; i < rounds; i++) {  
	/* send message to server */
	do_write(0, pid, s, buf, len, &nwritten, &errno);
	if (nwritten < 0) {
	    perror("do_write");
	    exit(1);
	} /* if */

	/* receive answer from server */
	n = len;
	bufp = buf;
	while (n) {
	    nread = len;
	    do_read(0, pid, s, &bufp, &nread, nread, &errno);
	    if (nread <= 0) {
		if (nread == 0) {
		    fprintf(stderr, "do_read: eof unexpected\n");
		    exit(1);
		} else {
		    perror("do_read");
		    exit(1);
		} /* if */
	    } /* if */
	    n -= nread;
	    bufp += nread;
	} /* while */
    } /* for */

    /* stop timer */
    if (gettimeofday(&stop, (struct timezone *) 0) < 0) { 
	perror("gettimeofday");
	exit(1);
    } /* if */

    delta = USECS_PER_MSEC * (stop.tv_sec - start.tv_sec) +
      (stop.tv_usec - start.tv_usec) / (double) USECS_PER_MSEC;

    printf("Remote Host: %s, Protocol: %s, Rounds: %d, Length: %d\n", 
	   hostname, udp ? "UDP" : "TCP", rounds, len); 
    printf("Round-trip average time: %g milliseconds\n",
	   delta / rounds);
} /* bench_client */


void
xsi_benchmark(Event ev, void *arg)
{
    int s;
    struct sockaddr_in server;

    fixed_priority_scheduling(xsi_fixed_prio);
    
    if (throughput && udp) {
	fprintf(stderr, "%s: UDP version of throughput test doesn't exist\n",
		globalArgv[0]);
	usage();
    } /* if */

    pid = getpid();

    /* setup socket: */
    s = 0;
    do_socket(0, pid,
	      AF_INET, udp ? SOCK_DGRAM : SOCK_STREAM, 0,
	      s, &errno);
    if (errno != ESUCCESS) {
	perror("do_socket");
	exit(1);
    } /* if */

    /* setup server address: */
    bzero((char *)&server, sizeof(server));
    server.sin_port = htons(port);
    server.sin_family = AF_INET;

    if (xsi_i_am_server) {
	server.sin_addr.s_addr = INADDR_ANY;

	do_bind(0, pid,
		s, (struct sockaddr*) &server, sizeof(server),
		&errno);
	if (errno != ESUCCESS) {
	    perror("do_bind");
	    exit(1);
	} /* if */
	
	if (throughput) {
	    tcp_truput_server(s);
	} else {
	    if (udp) {
		udp_rtrip_server(s);
	    } else {
		tcp_rtrip_server(s);
	    } /* if */
	} /* if */
    } else {
	server.sin_addr.s_addr = server_addr;

	do_connect(0, pid,
		   s, (struct sockaddr*) &server, sizeof(server),
		   &errno);
	if (errno != ESUCCESS) {
	    perror("do_connect");
	    exit(1);
	} /* if */
	
	if (throughput) {
	    truput_client(s, len);
	} else {
	    rtrip_client(s, rounds, len, udp);
	} /* if */
	exit(0);
    } /* if */
} /* xsi_benchmark */

			/*** end of xsi_bench.h ***/
