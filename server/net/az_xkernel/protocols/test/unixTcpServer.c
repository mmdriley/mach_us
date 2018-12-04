/*     
 * $RCSfile: unixTcpServer.c,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/01 22:33:03 $
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>

extern char *inet_ntoa(struct in_addr);

main()
{
    struct  sockaddr_in	addr;
    struct  sockaddr_in	from;
    int	on = 1;
    int	off = 0;
    int	s;
    int	size,n;
    char	buf[8*1024];
    int	len=8*1024, newlen;
    int	i;
    int   count = 0;
    int addrLen;
    
    setbuf(stdout, 0);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(2001);
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	printf("cannot open socket\n");
	exit(1);
    }	
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on));
    if (bind(s, &addr, sizeof(addr))) {
	printf("init_ether: cannot bind socket\n");
	exit(1);
    }
    if (listen(s, 1) < 0) {
	perror("listen");
	exit(1);
    }
    printf("Listening for connections\n");
    addrLen = sizeof(from);
    if ((s = accept(s, &from, &addrLen)) < 0) {
	perror("accept");
	exit(1);
    }
    printf("Received connection from %d.%d.%d.%d. (%d)\n",
	   (from.sin_addr.s_addr & 0xff000000) >> 24,
	   (from.sin_addr.s_addr & 0x00ff0000) >> 16,
	   (from.sin_addr.s_addr & 0x0000ff00) >> 8,
	   (from.sin_addr.s_addr & 0x000000ff),
	   from.sin_port);
    if (ioctl(s, FIONBIO, &off) < 0) {
	perror("ioctl");
	exit(1);
    }
    while (1) {
	if ((len = recv(s, &buf, sizeof(buf), 0)) < 0) {
	    perror("could not receive");
	    exit(1);
	}
	if (len == 0) {
	    exit(0);
	}
/*	printf("Received %d bytes\n", len); */
#ifdef DOTS    
	putchar('.');
	if (++count % 50 == 0) {
	    putchar('\n');
	}
#endif
	if (write(s, buf, len) < 0) {
	    perror("could not send");
	    exit(1);
	}
    }
    exit(0);
}
