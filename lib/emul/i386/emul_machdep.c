/* 
 * Mach Operating System
 * Copyright (c) 1994,1993,1992,1991,1990,1989 Carnegie Mellon University
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
 **********************************************************************
 * HISTORY
 * $Log:	emul_machdep.c,v $
 * Revision 2.15  94/07/08  16:12:44  mrt
 * 	Updated copyright
 * 
 * Revision 2.14  92/07/05  23:25:45  dpj
 * 	Removed include of mach_object.h.
 * 	[92/05/10  01:37:26  dpj]
 * 
 * 	Converted to new C++ RPC package.
 * 	[92/05/10  00:39:31  dpj]
 * 
 * 	Added include of mach_object.h.
 * 	[92/04/17  16:04:46  dpj]
 * 
 * Revision 2.13  92/03/05  14:56:13  jms
 * 	Change the "is sensitive" signal logic to use the stack pointer to determine
 * 	if we are running in the emulation lib instead of using the IP.
 * 	Upgrade to use only "new" IPC
 * 	[92/02/26  17:28:45  jms]
 * 
 * Revision 2.12  91/11/13  17:16:48  dpj
 * 	Really eliminated lib/load and dyn_objects (previous jjc merge
 * 	had not done it)
 * 	[91/11/08            dpj]
 * 
 * Revision 2.11  91/11/06  11:33:32  jms
 * 	Changed to work with C++ objects.
 * 	[91/09/26  19:51:34  pjg]
 * 
 * Revision 2.10  91/10/06  22:27:05  jjc
 * 	Picked up changes from Dan Julin:
 * 		Added emul_setup().
 * 		Removed a bunch of dummy statements left by jms while fighting
 * 		a compiler problem.
 * 		Removed MACH3_UNIX case.
 * 		Eliminated dyn_methods and lib/load.
 * 	[91/10/01            jjc]
 * 
 * Revision 2.9  91/07/01  14:07:07  jms
 * 	No Further Changes
 * 	[91/06/24  16:30:32  jms]
 * 
 * 	Further fixes to error return values.
 * 	[91/05/07  11:26:29  jms]
 * 
 * 	Use the new interrupt mechinism for handling interrupts to threads currently
 * 	executing in the emulation libs/servers.
 * 	Strip out "uxsignal_do_handler".  Never worked,  correct version found in
 * 	"threads.c"
 * 	[91/04/15  17:26:52  jms]
 * 
 * 	Enable syscall retry for "except" subsystem errors.
 * 	Add active intr handler count args
 * 	[91/02/21  14:12:40  jms]
 * 
 * 	Upgrade to modified language exception handler interface.
 * 	[91/01/23  14:00:54  jms]
 * 
 * Revision 2.8  91/05/05  19:24:55  dpj
 * 	Fixed error reporting for execv() and friends.
 * 	[91/04/29            dpj]
 * 	Corrected an error code translation bug around E_JUSTRETURN.
 * 	Place the UNIX error code in eax on exit.
 * 	[91/04/28  09:50:25  dpj]
 * 
 * Revision 2.7  91/04/12  18:48:01  jjc
 * 	Removed valid_vm_address() because we're using COPYIN and
 * 	COPYOUT instead.
 * 	[91/04/01            jjc]
 * 	Picked up Paul Neves' changes
 * 	[91/03/29  15:49:01  jjc]
 * 
 * Revision 2.6  90/12/21  13:52:36  jms
 * 	Fixed setup of a forked child's entry point.
 * 	[90/12/17  14:28:34  neves]
 * 	merge for roy/neves @osf
 * 	[90/12/20  13:16:00  jms]
 * 
 * Revision 2.5  90/11/27  18:18:57  jms
 * 	Modified to support pure kernel syscalls and forking.
 * 	[90/11/20  13:04:55  jms]
 * 
 * 	Add code to make emul_fork work under the pure kernel.
 * 	[90/08/20  17:06:50  jms]
 * 
 * Revision 2.4  90/07/26  12:29:47  dpj
 * 	Fixed include of mach.h.
 * 	[90/07/24  14:06:37  dpj]
 * 
 * Revision 2.3  90/07/09  17:01:19  dorr
 * 	Stack frame for sig_return fixed.  Emul_wait added. Misc bug fixes.
 * 	[90/07/06  13:47:01  jms]
 * 
 * 	move emul_entry definition into syscall_define.h
 * 	[89/12/18  16:08:39  dorr]
 * 
 * Revision 2.1  90/03/19  17:34:25  dorr
 * Created.
 * 
 * Revision 2.3  90/01/02  21:59:52  dorr
 * 	move emul_entry definition into syscall_define.h
 * 
 * Revision 2.2  89/10/06  13:45:45  mbj
 * 	Hacked BSD-server version to make multi-server version.
 * 	[89/10/05  16:33:44  mbj]
 * 
 * 	Version for the multi-server emulator.
 * 	[89/09/11  16:08:02  mbj]
 * 
 * Revision 2.1  89/08/04  14:04:49  rwd
 * Created.
 * 
 */

/*
 * Take/Return from signals - machine-dependent.
 */
#include "emul_base.h"

#include "cthreads.h"
#include <mach.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include <sys/syscall.h>
#include <machine/psl.h>
#include <machine/vmparam.h>
#include <machine/psl.h>

#include <mach/exception.h>
#include <mach/thread_status.h>
#include <mach/machine/vm_param.h>
#include <mach_error.h>
#include <exception_error.h>
#include <interrupt.h>

/*
#include <event_methods.h>
#include <signal_methods.h>
*/

#include <machine/machine_address_space.h>

#include <syscall_table.h>

#define	E_JUSTRETURN	255	/* return without changing registers */

extern boolean_t	is_in_emulation_space();
extern boolean_t	emul_stacks_member();


/*
 * User's registers are stored on partially on the user's stack
 * and partially on the emulator's stack.
 */
struct emul_regs_2 {
	int	edx;
	int	ecx;
	int	eax;
	int	eflags;
	int	eip;
};

struct emul_regs {
	int	ebp;
	int	edi;
	int	esi;
	int	ebx;
	struct emul_regs_2 *uesp;
				/* pointer to rest of user registers */
};


typedef struct emul_sig {
	mach_error_fn_t	hand;
	int		sig;
	int		code;
	vm_offset_t	stack;
	int		old_mask;
	int		old_onstack;
	boolean_t	is_default_handler;
} * emul_sig_t;

/* the address(es) of the sigreturn routine for use in "taking" signals */
extern int (*sigreturnaddr)();
extern int *_sigreturn();

/* Assembler to do the child forking */
extern	int child_fork();

static	emul_sig_t	pending_sig = (emul_sig_t)0;
	char		sig_unsafe = 0;

void	take_signals();	/* forward */

/*
 * Emulation trap vectors - the size of the code vector is machine-dependent.
 */

/*
 * Child entry point. Complete child initialization and setup
 * for child's return from fork system call.
 */

void
child_init_md(regs)
	register struct emul_regs * regs;
{
	register struct emul_regs_2 *regs2;

	(void)emul_child_init(); 	/* XXX - What about errors? */
  
	regs2 = regs->uesp;
	regs2->eflags &= ~EFL_CF;
	regs2->eax = ESUCCESS;
	regs2->edx = 0;
}

/*
 * System calls enter here.
 */
void
emul_syscall(regs)
	register struct emul_regs *regs;
{
	register int	syscode;
	int		opc;
	register int	error;
	register struct sysent *callp;
	int		rval[2];
#if	XXX
	boolean_t	interrupt;
#endif	XXX
	register struct emul_regs_2 *regs2;
	register int	*cargs;

	regs2 = regs->uesp;

	cargs = (int *)(regs2+1);	/* args on stack top */

	cargs++;	/* point to first argument - skip return address */

	syscode = regs2->eax;

	/*
	 * Save old PC for backup
	 */
	opc = regs2->eip - 7;	/* XXX from dbg single server emul lib */

	if (syscode == 0) {
	    /*
	     * Indirect system call.
	     */
	    syscode = *cargs++;
	}

	/*
	 * Find system call table entry for the system call.
	 */
	if (syscode >= nsysent)
	    callp = &sysent[63];	/* nosysent */
	else if (syscode >= 0)
	    callp = &sysent[syscode];
	else {
#if	XXX
	    /*
	     * Negative system call numbers are CMU extensions.
	     */
	    if (syscode == -33)
		callp = &sysent_task_by_pid;
	    else if (syscode == -59)
		callp = &sysent_htg_ux_syscall;
	    else if (syscode < -ncmusysent)
		callp = &sysent[63];	/* nosysent */
	    else
		callp = &cmusysent[-syscode];
#else	XXX
	    callp = &sysent[63];	/* nosysent */
#endif	XXX
	}

	/*
	 * Set up the initial return values.
	 */
	rval[0] = 0;
	rval[1] = regs2->edx;

	/*
	 * Call the routine, passing arguments according to the table
	 * entry.
	 */
	switch (callp->nargs) {
	    case 0:
		error = (*callp->routine)(
				rval);
		break;
	    case 1:
		error = (*callp->routine)(
				cargs[0],
				rval);
		break;
	    case 2:
		error = (*callp->routine)(
				cargs[0], cargs[1],
				rval);
		break;
	    case 3:
		error = (*callp->routine)(
				cargs[0], cargs[1], cargs[2],
				rval);
		break;
	    case 4:
		error = (*callp->routine)(
				cargs[0], cargs[1], cargs[2], cargs[3],
				rval);
		break;
	    case 5:
		error = (*callp->routine)(
				cargs[0], cargs[1], cargs[2], cargs[3], cargs[4],
				rval);
		break;
	    case 6:
		error = (*callp->routine)(
				cargs[0], cargs[1], cargs[2],
				cargs[3], cargs[4], cargs[5],
				rval);
		break;

	    case -1:	/* generic */
		error = (*callp->routine)(
				syscode,
				cargs,
				rval);
		break;

	    case -2:
	    case 1000:	/* pass registers to modify */
		error = (*callp->routine)(
				cargs,
				rval,
				regs);
		regs2 = regs->uesp;	/* if changed */
		break;
	}

	/*
	 * Set up return values.
	 */
	
	if (error) {
		if (unix_err(E_JUSTRETURN) == error) {
			/* do not alter registers */
			NULL;
		}
		else if (except_sub == (error & (~ code_emask))) {
			/* restart call */
			regs2->eip = opc;
			regs2->eflags &= ~EFL_CF;
		}
		else {
			/* default error */
			regs2->eflags |= EFL_CF;
			regs2->eax = rval[0];
		} 
	} 
	else {
		/* success */
		regs2->eflags &= ~EFL_CF;
		regs2->eax = rval[0];
		regs2->edx = rval[1];
	}
	
	/*
	 * handle interrupt request.  don't allow any
	 * more signal manoevering until we exit
	 *
	 * this will be ugly to multi-thread properly
	 */
	sig_unsafe = 1;

	if (pending_sig) {	/* XXX multi-thread */
		take_signals(regs, pending_sig);
		pending_sig = (emul_sig_t)0;
	}

	/* we are done with this syscall, cleanup up the interrupt stuff */
	intr_reset_self();
}


event_is_sensitive_md(thread)
	thread_t		thread;
{
	struct i386_thread_state	regs;
	int				count;
	mach_error_t		err;
	boolean_t		is_on;

	while ( sig_unsafe ) {
		/*
		 *  oh shit ... someone's in nasty code on their way out
		 *  of the emulator.  spin until they leave (don't forget
		 *  to wake them up so that CAN leave)
		 */
		if (thread_resume(thread)) {	
			/* very mysterious.  give up */
			ERROR((Diag,"event_is_sensitive: thread_resume fails--%s\n",
			       mach_error_string(err)));
			return FALSE;
		}

		while (sig_unsafe) {
			swtch();
		}

		if ( thread_suspend(thread) ) {
			return FALSE;
		}
	}

	count = i386_THREAD_STATE_COUNT;
	err = thread_get_state(thread, i386_THREAD_STATE, &regs, &count);
	if (err) {
		ERROR((Diag,"event_is_sensitive: thread_get_state fails (thread 0x%x)--%s\n",
		       thread, mach_error_string(err)));
		return TRUE;
	}

	/*
	 *  see if the current stack is an emulation stack
	 */
	is_on = emul_stacks_member(regs.uesp);

DEBUG0(TRUE,(Diag,"event_is_sensitive: is%s in emul lib\n",
	     is_on ? "" : " not"));

	return is_on;
}

		
struct sigframe {
    int		(*sf_retadr)();
    int		sf_signum;
    int		sf_code;
    struct sigcontext *sf_scp;
    struct sigcontext *sf_scpcopy;	/* for return */
} *fp;

/*
 *	uxsignal_call:  arrange for the target thread to call
 *	some routine.  if it's on_exit, modify the exit scp.  otherwise
 *	modify the target thread.
 */
mach_error_t uxsignal_call_md(thread, stack, hand, sig, code, mask, on_stack, 
			   on_exit, is_default_handler)
	thread_t		thread;
	vm_offset_t		stack;
	mach_error_fn_t		hand;
	int			sig;
	int			code;
	int			mask;
	boolean_t		on_stack;
	boolean_t		on_exit;
	boolean_t		is_default_handler;
{
	mach_error_t		err = ERR_SUCCESS;
	int			h_cnt = 0;

	if (on_exit) {
		/*
		 * XXX do we want to handle multiple signals while in the emulator?
		 */
		static struct emul_sig	p;


		if (pending_sig) {
			/*
			 *  FOR NOW, only have one pending signal at a time
			 */
			return KERN_FAILURE;
		}

		p.hand = hand;
		p.stack = stack;
		p.sig = sig;
		p.code = code;
		p.old_mask = mask;
		p.old_onstack = on_stack;
		p.is_default_handler = is_default_handler;

		pending_sig = &p;

	} else {
		/*
		 *  approximate solutions for approximate people
		 */

		struct i386_thread_state	regs;
		vm_offset_t sp;
		struct sigcontext * scp;
		struct sigframe * fp;
		int count;

		/*
		 *  ... in case you get a fault wazzing the user's stack
		 */
		INTR_REGION(INTR_SYNC_IMMEDIATE)
			/*
			 * whap 'im
			 */
			count = i386_THREAD_STATE_COUNT;

			err = thread_get_state(thread, i386_THREAD_STATE, &regs, 
				       &count);
			if (err) goto finish;
 
			/*
			 *  get a signal stack	
			 */
			if (stack == (vm_offset_t)0) {
				sp = (vm_offset_t)regs.uesp;
			} else {
				sp = stack;
			}

DEBUG0(TRUE,(Diag,"uxsignal_call_md: send sig=%d on stack=%#x to thread %d\n", 
	     sig, stack, thread));

			/*
			 *  push a sigcontext and sigframe on the user's stack
			 */
			scp = ((struct sigcontext *)sp) - 1;
			fp  = ((struct sigframe *)scp) - 1;

			/*
			 * Build the argument list for the signal handler.
			 */
			fp->sf_signum = sig;
			fp->sf_code = code;
			fp->sf_scp = scp;

			/*
			 * Build the stack frame to be used to call sigreturn.
			 */
			fp->sf_scpcopy = scp;
			fp->sf_retadr = (int (*)())
			((unsigned long) sigreturnaddr & ~0x80000000);

			/*
			 * Build the signal context to be used by sigreturn.
			 */
			scp->sc_onstack = on_stack;
			scp->sc_mask = mask;

			scp->sc_gs = regs.gs;
			scp->sc_fs = regs.fs;
			scp->sc_es = regs.es;
			scp->sc_ds = regs.ds;

			scp->sc_edi = regs.edi;
			scp->sc_esi = regs.esi;
			scp->sc_ebp = regs.ebp;
			scp->sc_ebx = regs.ebx;
			scp->sc_edx = regs.edx;
			scp->sc_ecx = regs.ecx;
			scp->sc_eax = regs.eax;
			scp->sc_eip = regs.eip;
			scp->sc_cs = regs.cs;
			scp->sc_efl = regs.efl;
			scp->sc_uesp = regs.uesp;
			scp->sc_ss = regs.ss;

			/*
			 *  force the user to call our handler on our stack
			 */
			regs.uesp = (int)fp;
			regs.eip = (int)hand;

			/* just in case the poor sucker's doing a mach call */
			(void)thread_abort(thread);

			err = thread_set_state(thread, i386_THREAD_STATE, &regs, 
					       i386_THREAD_STATE_COUNT);
		    finish:
			0;

		INTR_HANDLER
			INTR_RETURN(INTR_ERROR);
		INTR_END
	}

	return err;
}


/*
 * Take a signal.
 */
void
take_signals(regs, p)
	register struct emul_regs *regs;
	emul_sig_t p;
{
	/*
	 *  approximate solutions for approximate people
	 */
	struct emul_regs_2	save_regs2;
	register struct emul_regs_2	*regs2;

	register struct sigcontext *scp;
	register struct sigframe * fp;
	extern void*		uxsignal_obj;

	int	old_mask, old_onstack, sig, code, handler, new_sp;

	/*
	 * Get anything valuable off user stack first.
	 */
	save_regs2 = *regs->uesp;

	sig = p->sig;
	code = p->code;
	old_mask = p->old_mask;
	old_onstack = p->old_onstack;
	handler = (int)p->hand;
	new_sp = p->stack;

	/*
	 * If there really were no signals to take, return.
	 */
	if (sig == 0)
	    return;

	/* Just a default handler */
	if (p->is_default_handler) {
		((mach_error_fn_t)handler)(sig, code, NULL);
		(void)signal_return(uxsignal_obj, old_onstack, old_mask);
		return;
	}

	/*
	 * Put the signal context and signal frame on the signal stack.
	 */
	if (new_sp == 0) {
	    /*
	     * Build signal frame and context on user's stack.
	     */
	    new_sp = (int)regs->uesp;
	}

DEBUG0(TRUE,(Diag,"take_signals: taking signal %d on %#x\n", sig, new_sp));

	scp = ((struct sigcontext *)new_sp) - 1;
	fp  = ((struct sigframe *)scp) - 1;

	/*
	 * Build the argument list for the signal handler.
	 */
	fp->sf_signum = sig;
	fp->sf_code = code;
	fp->sf_scp = scp;

	/*
	 * Build the stack frame to be used to call sigreturn.
	 */
	fp->sf_scpcopy = scp;
	fp->sf_retadr = (int (*)())
		((unsigned long) sigreturnaddr & ~0x80000000);

	/*
	 * Build the signal context to be used by sigreturn.
	 */
	scp->sc_onstack = old_onstack;
	scp->sc_mask = old_mask;

/*	scp->sc_gs = ... */
/*	scp->sc_fs = ... */
/*	scp->sc_es = ... */
/*	scp->sc_ds = ... */

	scp->sc_edi = regs->edi;
	scp->sc_esi = regs->esi;
	scp->sc_ebp = regs->ebp;
	scp->sc_esp = 0;			/* kernel stack XXX */
	scp->sc_ebx = regs->ebx;
	scp->sc_edx = save_regs2.edx;
	scp->sc_ecx = save_regs2.ecx;
	scp->sc_eax = save_regs2.eax;
/*	scp->sc_trapno = ... */
	scp->sc_err = 0;
	scp->sc_eip = save_regs2.eip;
/*	scp->sc_cs = ... */
	scp->sc_efl = save_regs2.eflags;
	scp->sc_uesp = (int)(regs->uesp+1);	/* user sp after return */
/*	scp->sc_ss = ... */

	/*
	 * Set up the new stack and handler addresses.
	 */
	regs2 = ((struct emul_regs_2 *)fp) - 1;
	*regs2 = save_regs2;
	regs->uesp = regs2;
	regs2->eip = handler;
}



boolean_t uxsignal_xlate_exception_md(exception, code, subcode, unix_signal, 
				      unix_code)
int	exception, code, subcode;
int	*unix_signal, *unix_code;
{
	switch(exception) {

	    case EXC_BAD_INSTRUCTION:
	        *unix_signal = SIGILL;
		switch (code) {
		    case EXC_I386_INVOP:
			*unix_code = ILL_RESOP_FAULT;
			break;
		    default:
			return(FALSE);
		}
		break;
		
	    case EXC_ARITHMETIC:
	        *unix_signal = SIGFPE;
		switch (code) {
		    case EXC_I386_INTO:
			*unix_code = FPE_INTOVF_TRAP;
			break;
		    case EXC_I386_DIV:
			*unix_code = FPE_INTDIV_TRAP;
			break;
		    case EXC_I386_BOUND:
			*unix_code = FPE_SUBRNG_TRAP;
			break;
		    case EXC_I386_NOEXT:
		    case EXC_I386_EXTOVR:
		    case EXC_I386_EXTERR:
		    case EXC_I386_EMERR:
			*unix_code = 0;
			break;
		    default:
			return(FALSE);
		}
		break;
		
	    case EXC_BREAKPOINT:
		*unix_signal = SIGTRAP;
		break;
		
	    default:
		return(FALSE);
	}
	return(TRUE);
}


/*
 *	uxsignal_thread_stack, uxsignal_self_stack: return the 
 *	base of the stack for a thread, using
 *	the standard emulation library stack size
 */

vm_address_t uxsignal_thread_stack(thread)
	thread_t		thread;
{
	
	struct i386_thread_state	regs;
	int		count;

	if (thread == MACH_PORT_NULL)
		return 0;

	/*
	 * make the target thread call a longjmp handler
	 */
	count = i386_THREAD_STATE_COUNT;
	if (thread_get_state(thread, i386_THREAD_STATE, &regs, 
			       &count))
		return 0;

DEBUG0(TRUE,(Diag,"thread_stack: returning %#x\n", regs.uesp & cthread_stack_mask));

	return regs.uesp & cthread_stack_mask;
}

/*
 *	uxsignal_thread_stack, uxsignal_self_stack: return the 
 *	base of the stack for a thread, using
 *	the standard emulation library stack size
 */
vm_address_t uxsignal_self_stack()
{

DEBUG0(TRUE,(Diag,"thread_stack: returning %#x\n", cthread_sp() & cthread_stack_mask));
	return cthread_sp() & cthread_stack_mask;
}


/*
 * Exec starts here to save registers.
 */
struct execa {
    char	*fname;
    char	**argp;
    char	**envp;
};

int
emul_execv(argp, rval, regs)
	register struct execa	*argp;
	syscall_val_t		*rval;
	struct emul_regs	*regs;
{
	mach_error_t		err;

	err = emul_execvx(argp, FALSE, regs);

	if (err)
		rval->rv_val1 = emul_error_to_unix(err);
	else
		rval->rv_val1 = 0;

	return(err);
}

int
emul_execve(argp, rval, regs)
	register struct execa	*argp;
	syscall_val_t		*rval;
	struct emul_regs	*regs;
{
	mach_error_t		err;

	err = emul_execvx(argp, TRUE, regs);

	if (err)
		rval->rv_val1 = emul_error_to_unix(err);
	else
		rval->rv_val1 = 0;

	return(err);
}

int
emul_execvx(argp, use_env, regs)
	register struct execa	*argp;
	struct emul_regs	*regs;
	boolean_t		use_env;
{
	register struct emul_regs_2 *regs2;
	int		entry[2];
	unsigned int	entry_count;
	vm_address_t	arg_addr;
	register int	error;

	/*
	 * Do not have to save user registers on old stack;
	 * they will all be cleared.
	 */

	/*
	 * Call exec.  If error, return without changing registers.
	 */
	entry_count = 2;
	error = emul_exec_call(argp->fname,
			       argp->argp,
			       (use_env) ? argp->envp : (char **)0,
			       &arg_addr,
			       entry,&entry_count);
	if (error)
	    return (error);

	/*
	 * Put new user stack just below arguments.
	 */
	regs2 = ((struct emul_regs_2 *)arg_addr) - 1;

	/*
	 * Clear user registers except for sp, pc, sr.
	 */
	regs2->edx = 0;
	regs2->ecx = 0;
	regs2->eax = 0;
	regs2->eflags = EFL_USERSET;
	regs2->eip = entry[0];

	regs->edi = 0;
	regs->esi = 0;
	regs->ebx = 0;
	regs->uesp = regs2;

	/*
	 * Clear FP state?
	 */

	/*
	 * Return to new stack.
	 */
	return(unix_err(E_JUSTRETURN));
}


int
emul_sigreturn(argp, rval, regs)
	int			*argp;
	int			*rval;
	struct emul_regs	*regs;
{
	return emul_osigcleanup(argp, rval, regs);
}

/*
 * SUN seems to make no distinction between sigreturn and osigcleanup.
 */
int
emul_osigcleanup(argp, rval, regs)
	int			*argp;
	int			*rval;
	struct emul_regs	*regs;
{
	register struct sigcontext	*scp;
	struct emul_regs_2		save_regs2;
	register struct emul_regs_2	*regs2;
	register int			rc;
	struct sigcontext		sc;
	extern void*		uxsignal_obj;
	struct a {
	    struct sigcontext *sigcp;
	} *uap = (struct a *)argp;

	/*
	 * Copy in signal context and regs2.  Pointer to sigcontext is
	 * on top of user stack (unlike all other syscalls).
	 */
	save_regs2 = *regs->uesp;
	scp = uap->sigcp;
	sc = *scp;

	/*
	 * Change registers.
	 */
	regs2 = ((struct emul_regs_2 *)sc.sc_uesp) - 1;

	*regs2 = save_regs2;
	regs->uesp = regs2;

/*	... = sc.sc_gs; */
/*	... = sc.sc_fs; */
/*	... = sc.sc_es; */
/*	... = sc.sc_ds; */
	regs->edi = sc.sc_edi;
	regs->esi = sc.sc_esi;
	regs->ebp = sc.sc_ebp;
	regs->ebx = sc.sc_ebx;
	regs2->edx = sc.sc_edx;
	regs2->ecx = sc.sc_ecx;
	regs2->eax = sc.sc_eax;
	regs2->eip = sc.sc_eip;
/*	... = sc.sc_cs; */
	regs2->eflags = (sc.sc_efl & ~EFL_USERCLR) | EFL_USERSET;
/*	... = sc.sc_ss; */

	/*
	 * Change signal stack and mask.  If new signals are pending,
	 * do not take them until we switch user stack.
	 */
	rc = signal_return(uxsignal_obj, sc.sc_onstack, sc.sc_mask);

	return(unix_err(E_JUSTRETURN));	/* XXX Hack, will not work with interuptable syscalls */
}

/*
 * Wait has a weird parameter passing mechanism.
 */
mach_error_t
emul_wait(argp, rval, regs)
	int			*argp;
	int			*rval;
	struct emul_regs	*regs;
{
	register struct emul_regs_2 *regs2 = regs->uesp;

	int		new_args[3];
	mach_error_t	rtn;

	if ((regs2->eflags & EFL_ALLCC) == EFL_ALLCC) {
	    new_args[1] = regs2->ecx;	/* options */
	    new_args[2] = regs2->edx;	/* rusage_p */
	}
	else {
	    new_args[1] = 0;
	    new_args[2] = 0;
	}
	rtn = emul_wait_common(&new_args[0], new_args[1], new_args[2], rval);
	rval[1] = new_args[0];
	return(rtn);
}

mach_error_t
emul_vfork(argp, rval, regs)
	int			*argp;
	int			*rval;
	struct emul_regs	*regs;
{
    return(emul_fork(argp, rval, regs));
}

mach_error_t
emul_fork(argp, rval, regs)
	int			*argp;
	int			*rval;
	struct emul_regs	*regs;
{
        register struct emul_regs_2 *regs2 = regs->uesp;
        register int error;

        struct i386_thread_state        child_regs;
        int                             seg_regs[6];

        extern int      child_fork();
        extern void     get_seg_regs();
	mach_error_t	ret;

        /*
         * Copy the registers for child.
         */
        get_seg_regs(seg_regs);

        child_regs.gs = seg_regs[5];
        child_regs.fs = seg_regs[4];
        child_regs.es = seg_regs[3];
        child_regs.ds = seg_regs[2];
        child_regs.edi = regs->edi;
        child_regs.esi = regs->esi;
        child_regs.ebp = regs->ebp;
     /* child_regs.esp */
        child_regs.ebx = regs->ebx;

        child_regs.edx = regs2->edx;
        child_regs.ecx = regs2->ecx;
        child_regs.eax = regs2->eax;
        child_regs.uesp = (int)regs->uesp;
        child_regs.efl = regs2->eflags;

     /* child_regs.eip */
        child_regs.cs = seg_regs[0];
        child_regs.ss = seg_regs[1];

        /* FP regs!!!! */


        /*
         * Create the child.
         */
        error = emul_fork_common((int *)&child_regs,
	                         i386_THREAD_STATE_COUNT,
	                         rval);

        if (error == 0)
            rval[1] = 0;

        return (error);
}

/*
 *  Put registers into the child thread for fork.
 */

boolean_t
emul_fork_set_thread_state(child_thread, child_state, child_state_count, parent_pid, rc)
        thread_t        child_thread;
        thread_state_t  child_state;
        unsigned int    child_state_count;
        int             parent_pid, rc;
{
        struct i386_thread_state *regs = (struct i386_thread_state *)child_state;
	mach_error_t	ret;

        if (child_state_count != i386_THREAD_STATE_COUNT)
            return (FALSE);

        regs->eax = parent_pid;
        regs->edx = rc;
        regs->eip = (int)child_fork;

        ret = thread_set_state(child_thread, i386_THREAD_STATE,
                                child_state, child_state_count);
        return (ret == KERN_SUCCESS);
}



int	emul_low_entry = -9;
int	emul_high_entry = 181;

extern emul_common();

void
emul_setup()
{
	int		i;
	kern_return_t	rc;
	task_t		task = mach_task_self();

	for (i = emul_low_entry;
	     i <= emul_high_entry;
	     i++) {
		rc = task_set_emulation(task,
					emul_common,
					i);
	}
	rc = task_set_emulation(task,
			emul_common,
			-33);
	rc = task_set_emulation(task,
			emul_common,
			-34);
	rc = task_set_emulation(task,
			emul_common,
			-41);
	rc = task_set_emulation(task,
			emul_common,
			-52);

#ifdef	notdef	XXX
	/*
	 * Change sigvec to point to our routine - the signal
	 * trampoline address is passed in VERY strangely.
	 */
	sysent[SYS_sigvec] = i386_syscall_sigvec;
#endif	notdef
}

