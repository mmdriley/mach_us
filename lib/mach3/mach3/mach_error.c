/* 
 * Mach Operating System
 * Copyright (c) 1994,1993,1992,1991,1990,1989,1988,1987 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator   or   Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they made and grant Carnegie Mellon the
 * rights to redistribute these changes.
 */
/*************************************************************
 *   mach_error.c
 *	interprets structured mach error codes and prints
 *	or returns a descriptive string. Will map old style
 *	unstructured error codes to new style if necessary.
 *
 * HISTORY:
 * $Log:	mach_error.c,v $
 * Revision 2.6  94/07/08  17:55:11  mrt
 * 	Updated copyrights
 * 
 * Revision 2.5  94/06/01  18:19:35  mrt
 * 	Changed include of <servers/errorlib.h> to <errorlib.h>
 * 	[94/02/14            mrt]
 * 
 * Revision 2.4  93/01/20  17:37:10  jms
 * 	Update to correct build env (stump->CMU)
 * 	[93/01/18  16:11:26  jms]
 * 
 * Revision 2.3  92/03/05  14:56:52  jms
 * 	Put old IPC error stuff onder the compile switch ERR_DO_COMPAT.
 * 	[92/02/26  17:47:54  jms]
 * 
 * Revision 2.2  90/07/26  12:37:37  dpj
 * 	First version
 * 	[90/07/24  14:29:06  dpj]
 * 
 * Revision 1.6  89/10/06  13:48:13  mbj
 * 	Fixed some fencepost errors in compatibility conversions.
 * 	[89/10/05  16:41:39  mbj]
 * 
 * Revision 1.5  89/06/30  18:32:27  dpj
 * 	Merged with mrt's version; lots of fence-post errors fixed.
 * 	[89/06/29  00:09:26  dpj]
 * 
 * Revision 1.4  89/05/17  16:31:50  dorr
 * 	include file cataclysm
 * 
 * Revision 1.3  89/05/04  17:48:32  mbj
 * 	Merge up to U3.
 * 	[89/04/17  15:23:56  mbj]
 * 
 * Revision 1.2.1.1  89/03/31  16:07:47  mbj
 * 	Create mbj_signal branch
 * 
 * Revision 1.2.1.1  89/03/30  16:51:02  mbj
 * 	Use fprintf instead of fputs and putc.
 * 
 * Revision 1.2  88/08/10  11:48:51  dorr
 * make sure error number is output if error message is not known.
 * 
 *
 *  12-May-88  Mary R. Thompson (mrt) at Carnegie Mellon
 *	Removed mach_error_print entry and added include
 *	 of mach_error.h	
 *
 *  09-Mar-88  Douglas Orr (dorr) at Carnegie Mellon
 *	Rewrote to deal with new structured error code.
 *	Added mach_error_type function and removed mach_errormsg
 *	which was the same as mach_error_string.
 ***********************************************************
 */
#include <stdio.h>
#include <mach/error.h>
#include <mach/message.h>	/* compatibility */
#include <mig_errors.h>		/* compatibility */
#include <servers/netname.h>	/* compatibility */
#include <servers/env_mgr.h>	/* compatibility */

#include <errorlib.h>
#include <mach_error.h>

static char * mach_error_string_int();

#ifdef ERR_DO_COMPAT
static
do_compat( org_err )
	mach_error_t		* org_err;
{
	mach_error_t		err = *org_err;

	/* 
	 * map old error numbers to 
	 * to new error sys & subsystem 
	 */
	/* -100 */
	if( (err <= SEND_ERRORS_START) && (err > SEND_ERRORS_START-100) ) {
		err = -(err - SEND_ERRORS_START) | IPC_SEND_MOD;
	} else	/* -200 */
	if( (err <= RCV_ERRORS_START) && (err > RCV_ERRORS_START-100) ) {
		err = -(err - RCV_ERRORS_START) | IPC_RCV_MOD;
	} else	/* -300 */
	if( (err <= MIG_TYPE_ERROR) && (err > MIG_TYPE_ERROR-100) ) {
		err = -(err - MIG_TYPE_ERROR) | IPC_MIG_MOD;
	} else	/* 1000 */
	if( (err >= NAME_NOT_YOURS) && (err < NAME_NOT_YOURS-1+100) ) {
		err = ((err - NAME_NOT_YOURS) + 1) | SERV_NETNAME_MOD;
	} else  /* 1600 */
	if( (err >= ENV_VAR_NOT_FOUND) && (err < ENV_VAR_NOT_FOUND-1+100) ) {
		err = ((err - ENV_VAR_NOT_FOUND) + 1) | SERV_ENV_MOD;
	} else	/* 27600 */
	if( (err >= IPC_ERROR_BASE) && (err < IPC_ERROR_BASE+100) ) {
		err = ((err - IPC_ERROR_BASE) + 1 ) | SERV_EXECD_MOD;
	};
	   

	*org_err = err;
}
#endif ERR_DO_COMPAT

typedef enum { false=0, true=1 } boolean;
#if	DEBUG
boolean mach_error_full_diag = true;
#else
boolean mach_error_full_diag = false;
#endif	DEBUG


void
mach_error( str, err )	
	char	*str;
	mach_error_t		err;
{
	char * err_str;
	char buf[1024];
	int diag;

#ifdef ERR_DO_COMPAT
	do_compat( &err );
#endif ERR_DO_COMPAT

	err_str=mach_error_string_int(err, &diag);

	if ( diag ) {
		sprintf( buf, "%s %s (%x)", mach_error_type(err), err_str, err );
		err_str = buf;
	}

	fprintf(stderr, "%s %s\n", str, err_str);
}


char *
mach_error_type( err )
	mach_error_t		err;
{
	int sub, system;

#ifdef ERR_DO_COMPAT
	do_compat( &err );
#endif ERR_DO_COMPAT

	sub = err_get_sub(err);
	system = err_get_system(err);

	if (system > err_max_system
	||  sub >= errors[system].max_sub ) return( "(?/?)" );
	return( errors[system].subsystem[sub].subsys_name );
}

static char *
mach_error_string_int( err, diag )
	mach_error_t		err;
	boolean			* diag;
{
	int sub, system, code;
	char * err_str;

#ifdef ERR_DO_COMPAT
	do_compat( &err );
#endif ERR_DO_COMPAT

	sub = err_get_sub(err);
	system = err_get_system(err);
	code = err_get_code(err);

	*diag = true;

	if (system > err_max_system) return( "(?/?) unknown error system" );
	if (sub >= errors[system].max_sub) return( errors[system].bad_sub );
	if (code >= errors[system].subsystem[sub].max_code) return ( NO_SUCH_ERROR );

	*diag = mach_error_full_diag;
	return( errors[system].subsystem[sub].codes[code] );
}

char *
mach_error_string( err )
	mach_error_t		err;
{
	boolean diag;

	return mach_error_string_int( err, &diag );

}
