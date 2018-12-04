/*     
 * $RCSfile: start_thread.c,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/01 23:54:41 $
 */

/* 
 * Included multiple times by process.c with different argument
 * declarations.  
 */


void
xkernel_limit_threads/**/NUMARGS( prio, stack, r COMMA ARGS )
    short prio;
    stkalign_t *stack;
    Pfi r;
{
    thread_t tmp_thread;
    
    if (xk_current_threads > MAX_XK_THREADS && prio < (LWP_MAXPRIO - STD_PRIO)) {
	xTrace1(processcreation, TR_ERRORS,
		"CreateProcess%d Too many threads; dying",
		NUMARGS);
	return;
    }
    if (xk_current_threads > MAX_XK_THREADS)
      xTrace3(processcreation, TR_ERRORS,
	      "%d Forcing thread creation past limit %d prio %d",
	      NUMARGS, xk_current_threads, prio);
    if (lwp_create(&tmp_thread, process_enter_monitor/**/NUMARGS, prio, 0, 
		   stack, NUMARGS + 1, r COMMA ARGS) == -1) {
      sprintf(errBuf, "CreateProcess%d error in lwp_create.", NUMARGS);
      Kabort(errBuf);
    } else {
      xk_current_threads++;
    }
    xTrace2(processcreation, TR_GROSS_EVENTS, "%d Created thread %d",
	    NUMARGS, tmp_thread.thread_id);
    xTrace2(processswitch, TR_GROSS_EVENTS, "%d Created thread %d",
	    NUMARGS, tmp_thread.thread_id);
    
}


CreateProcess/**/NUMARGS( r, prio COMMA ARGS )
     Pfi r;
     short prio;
{
  stkalign_t *stack;

  /* xkernel and LWP are switched for priorities */
  prio = LWP_MAXPRIO - prio;

  xTrace3(processcreation, TR_MAJOR_EVENTS,
	  "Sparc CreateProcess%d: r %x prio %d", NUMARGS, r, prio);

  if ((stack = lwp_newstk()) == 0)
    Kabort("fork: Bad stack allocate.");

  xkernel_limit_threads/**/NUMARGS(prio, stack, r COMMA ARGS);
}


process_enter_monitor/**/NUMARGS(user_fn COMMA ARGS )
     Pfi user_fn;
{
  Process *pd;

  MONITOR(master_monitor);

  if ((pd = (Process *)xMalloc(sizeof(Process))) == NULL) 
    Kabort("process_enter_monitor: no memory");

  pd->link = NULL;
  if (lwp_self(&pd->lwp) == -1)
    Kabort("process_enter_monitor: Couldn't get self.");

  Active = pd;
  (user_fn)( ARGS );
  Active = NULL;
  xk_current_threads--;

  xFree((char *)pd);
}
