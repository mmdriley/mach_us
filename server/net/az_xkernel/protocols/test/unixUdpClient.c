/* 
 * unixUdpclient.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.3 $
 * $Date: 1993/02/01 22:26:25 $
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>

#define TRIPS 1


main()
{
  struct  sockaddr_in	from;
  struct  sockaddr_in	to;
  int	s;
  int	rcvLen;
  char	buf[2*1024];
  int   sizes[] = { 2 * 1024 };
  int	i;
  int	sz_i;
  int   st, et;
  int	count = 0;
  int   fromSize;
  int   on = 1;

  setbuf(stdout, 0);
  for (sz_i=0; sz_i < sizeof(sizes) / sizeof(int); sz_i++) {
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
      printf("cannot open socket\n");
      exit(1);
    }	
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on));
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = inet_addr("192.12.69.49");
    to.sin_port = htons(2001);
    st = time(0);
    for (i=0; i<TRIPS; i++) {
#ifdef DOTS
      putchar('.');
      if (++count % 50 == 0) {
	putchar('\n');
      }
#endif
      if (sendto(s, buf, sizes[sz_i], 0, &to, sizeof(to)) < 0) {
	perror("Could not send");
	exit(1);
      }
      fromSize = sizeof(from);
      if ( recvfrom( s, buf, sizeof(buf), 0, &from, &fromSize ) < 0 ) {
	perror("Could not receive");
	exit(1);
      }
    }
    et = time(0);
    printf("len %d  %d secs / %d trips\n",sizes[sz_i], et - st, i);
  }
  exit(0);
}
