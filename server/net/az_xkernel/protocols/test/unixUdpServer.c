/* 
 * unixUdpServer.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.3 $
 * $Date: 1993/02/01 22:25:50 $
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>

main()
{
  struct  sockaddr_in	addr;
  struct  sockaddr_in	from;
  int	on = 1;
  int	s;
  int	size,n;
  char	buf[8*1024];
  int	len=8*1024, newlen;
  int	i;
  int   count = 0;

  setbuf(stdout, 0);
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(2001);
  if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    printf("cannot open socket\n");
    exit(1);
  }	
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on));
  if (bind(s, &addr, sizeof(addr))) {
    printf("init_ether: cannot bind socket\n");
    exit(1);
  }
  while (1) {
    size = sizeof(from);
    newlen = recvfrom(s, buf, len, 0, &from, &size);
#ifdef DOTS    
    putchar('.');
    if (++count % 50 == 0) {
      putchar('\n');
    }
#endif
    sendto(s, buf, newlen, 0, &from, size);
  }
  exit(0);
}
