/*
 * select_i.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.3 $
 * $Date: 1993/02/01 22:32:14 $
 */

#ifndef select_i_h
#define select_i_h


#define SEL_OK 1
#define SEL_FAIL 2

typedef struct  {
    long	id;
    long	status;		/* only used in the RPC realm */
} SelHdr;

typedef struct {
    SelHdr	hdr;
} SState;

typedef struct {
    Map passiveMap;
    Map activeMap;
} PState;


typedef struct {
    long id;
    XObj lls;
} ActiveKey;
    
typedef long PassiveKey;

extern int	traceselectp;

#ifdef __STDC__

void		selectCommonInit( XObj );
XObj		selectCommonOpen( XObj, XObj, XObj, Part *, long );
xkern_return_t	selectCommonOpenDisable( XObj, XObj, XObj, Part *, long );
xkern_return_t 	selectCommonOpenEnable( XObj, XObj, XObj, Part *, long );
int		selectControlProtl( XObj, int, char *, int );
int		selectCommonControlSessn( XObj, int, char *, int );
xkern_return_t	selectCallDemux( XObj, XObj, Msg *, Msg * );
xkern_return_t	selectDemux( XObj, XObj, Msg * );

#else

void		selectCommonInit();
XObj		selectCommonOpen();
xkern_return_t	selectCommonOpenDisable();
xkern_return_t 	selectCommonOpenEnable();
int		selectControlProtl();
int		selectCommonControlSessn();
xkern_return_t	selectDemux();
xkern_return_t	selectCallDemux();

#endif __STDC__


#endif	! select_i_h

