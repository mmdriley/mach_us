/*     
 * $RCSfile: unixTcpClient.c,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/01 22:32:07 $
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>

#define TRIPS 50


main()
{
    struct  sockaddr_in	me;
    struct  sockaddr_in	to;
    int	s;
    char	buf[32*1024];
    int   sizes[] = { 1, 2 * 1024 , 4*1024 };
    int	i;
    int	sz_i;
    int   st, et;
    int	count = 0;
    int   on = 1;
    int bytesRcvd;
    
    setbuf(stdout, 0);
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	printf("cannot open socket\n");
	exit(1);
    }	
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on));
    me.sin_family = AF_INET;
    me.sin_addr.s_addr = INADDR_ANY;
    me.sin_port = 0;  /* Let system pick one */
    if (bind(s, &me, sizeof(me))) {
	perror("bind");
	exit(1);
    }
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = inet_addr("192.12.69.88");
    to.sin_port = htons(2001);
    if (connect(s, &to, sizeof(to))) {
	perror("connecting socket");
	exit(1);
    }
    printf("Connection established\n");
    for (sz_i=0; sz_i < sizeof(sizes) / sizeof(int); sz_i++) {
	st = time(0);
	for (i=0; i<TRIPS; i++) {
	    if (write(s, buf, sizes[sz_i]) < 0) {
		perror("Could not send");
		exit(1);
	    }
	    for (bytesRcvd = 0; bytesRcvd < sizes[sz_i];) {
		int n;
		
		if ((n = recv(s, buf, sizeof(buf), 0)) < 0) {
		    perror("could not receive");
		    exit(1);
		}
		bytesRcvd += n;
	    }
#ifdef DOTS
	    putchar('.');
	    if (++count % 50 == 0) {
		putchar('\n');
	    }
#endif
	}
	et = time(0);
	printf("\nlen %d  %d secs / %d trips\n",sizes[sz_i], et - st, i);
    }
    exit(0);
}

