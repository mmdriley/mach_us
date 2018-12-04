/* 
 * ether_i.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.3 $
 * $Date: 1993/02/02 00:01:39 $
 */

#define	EMAXPAK	1266
#define	EADLEN	6

#define	EHLEN	14
#define	EDLEN	EMAXPAK-EHLEN

#define RCVBUFSIZE 50*1024		/* increased size - menze */

struct	eheader {
  ETHhost	e_dest;
  ETHhost	e_src;
  short	e_ptype;
};
	
struct  epacket {
  struct	eheader hdr;
  char		data[EDLEN];
};

typedef u_short PassiveID;
typedef ETHaddr ActiveID;
