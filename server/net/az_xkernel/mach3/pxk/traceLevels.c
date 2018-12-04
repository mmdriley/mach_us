/*
 * trace variable initialization
 *
 * Warning: this file is generated from graph.comp and is overwritten
 * every time 'make compose' is run
 *
 * For the XKMACHKERNEL version, this must configured manually when
 * the Mach kernel is built; perhaps someday a version of compose
 * will exist for generating all this automatically
 *
 */

#ifdef XKMACHKERNEL
#include "debug.h"

extern int traceethp;
extern void eth_init();
extern int tracearpp;
extern void arp_init();
extern int traceipp;
extern void ip_init();
extern int traceicmpp;
extern void icmp_init();
extern int traceudpp;
extern void udp_init();
extern int tracetcpp;
extern void tcp_init();

void
initTraceLevels()
{
#ifndef NDEBUG

    traceprocesscreation = TR_ERRORS;

#endif /* ! NDEBUG */
}
#endif XKMACHKERNEL
