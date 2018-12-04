/* 
 * hoststr.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.18 $
 * $Date: 1993/02/01 23:58:23 $
 */

#include "xkernel.h"
#include "ip.h"
#include "eth.h"
#ifndef XKMACHKERNEL
#include "x_stdio.h"
#endif ! XKMACHKERNEL

#ifdef XKMACHKERNEL
static int
sscanf4(str, format, a1, a2, a3, a4)
char *str, format;
int *a1, *a2, *a3, *a4;
{
  int n;

  *a1 = *a2 = *a3 = *a4 = 0;
  while (*str >= '0' && *str <= '9')
    *a1 = 10*(*a1) + (*str++ - '0');
  if (*str++ != '.') return 1;
  while (*str >= '0' && *str <= '9')
    *a2 = 10*(*a2) + (*str++ - '0');
  if (*str++ != '.') return 2;
  while (*str >= '0' && *str <= '9')
    *a3 = 10*(*a3) + (*str++ - '0');
  if (*str++ != '.') return 3;
  while (*str >= '0' && *str <= '9')
    *a4 = 10*(*a4) + (*str++ - '0');
  return(4);
}

static int
hexdigit(a)
char a;
{
  if (a >= '0' && a <= '9') return(a-'0');
  if (a >= 'a' && a <= 'f') return(a-'a'+10);
  if (a >= 'A' && a <= 'F') return(a-'A'+10);
  return(-1);
}

static int
sscanf6(str, format, a1, a2, a3, a4, a5, a6)
char *str, format;
int *a1, *a2, *a3, *a4, *a5, *a6;
{
  int n;

/*  printf("Sscanf6 converting %s\n", str); */
  *a1 = *a2 = *a3 = *a4 = *a5 = *a6 = 0;
  while ((n=hexdigit(*str))>=0)
    (*a1 = 16*(*a1) + n, str++);
  if (*str++ != ':') return 1;
  while ((n=hexdigit(*str))>=0)
    (*a2 = 16*(*a2) + n, str++);
  if (*str++ != ':') return 2;
  while ((n=hexdigit(*str))>=0)
    (*a3 = 16*(*a3) + n, str++);
  if (*str++ != ':') return 3;
  while ((n=hexdigit(*str))>=0)
    (*a4 = 16*(*a4) + n, str++);
  if (*str++ != ':') return 4;
  while ((n=hexdigit(*str))>=0)
    (*a5 = 16*(*a5) + n, str++);
  if (*str++ != ':') return 5;
  while ((n=hexdigit(*str))>=0)
    (*a6 = 16*(*a6) + n, str++);
/*
  printf("Sscanf6 results: %d %d %d %d %d %d\n", *a1, *a2, *a3, *a4, *a5, *a6);
 */
  return(6);
}



int
sscanf1(str, format, a1)
char *str, format;
int *a1;
{
  int n;

  *a1=0;
  while (*str >= '0' && *str <= '9')
    *a1 = 10*(*a1) + (*str++ - '0');
  return(1);
}


#endif XKMACHKERNEL

xkern_return_t
str2ipHost(h, s)
    IPhost *h;
    char *s;
 {
    int a, b, c, d;

#ifndef XKMACHKERNEL
    if ( sscanf(s, "%d.%d.%d.%d", &a, &b, &c, &d) < 4 ) {
#else
    if ( sscanf4(s, "%d.%d.%d.%d", &a, &b, &c, &d) < 4 ) {
#endif ! XKMACHKERNEL
	return XK_FAILURE;
    }
    h->a = a;
    h->b = b;
    h->c = c;
    h->d = d;
    return XK_SUCCESS;
}


char *
ipHostStr(h)
    IPhost *h;
{
    static char str[4][32];
    static int i=0;
    
    i = ++i % 4;
    if ( h == 0 ) {
	return "<null>";
    }
    if ( h == (IPhost *)ANY_HOST ) {
	return "ANY_HOST";
    }
    sprintf(str[i], "%d.%d.%d.%d", h->a, h->b, h->c, h->d);
    return str[i];
}


xkern_return_t
str2ethHost(h, s)
    ETHhost *h;
    char *s;
 {
     int	a[6], i;
     
     if ( 
#ifndef XKMACHKERNEL
	 sscanf
#else
	 sscanf6
#endif ! XKMACHKERNEL
	 (s, "%x:%x:%x:%x:%x:%x",
		 a, a+1, a+2, a+3, a+4, a+5) < 6 ) {
	 return XK_FAILURE;
     }
     for ( i=0; i < 6; i++ ) {
	 if ( a[i] > 0xff ) {
	     return XK_FAILURE;
	 }
     }
     h->high = htons((a[0] << 8) + a[1]);
     h->mid = htons((a[2] << 8) + a[3]);
     h->low = htons((a[4] << 8) + a[5]);
     return XK_SUCCESS;
}


char *
ethHostStr(h)
    ETHhost *h;
{
    static char str[4][32];
    static int i=0;
    
    if ( h == 0 ) {
	return "<null>";
    }
    if ( h == (ETHhost *)ANY_HOST ) {
	return "ANY_HOST";
    }
    i = ++i % 4;
    sprintf(str[i], "%x.%x.%x",
	    ntohs(h->high), ntohs(h->mid), ntohs(h->low));
    return str[i];
}

