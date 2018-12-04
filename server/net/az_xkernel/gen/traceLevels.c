/*
 * trace variable initialization
 *
 * Warning: this file is generated from graph.comp and is overwritten
 * every time 'make compose' is run
 */

#include "xk_debug.h"
#include "compose.h"
#include "protocols.h"

void
initTraceLevels()
{
#ifdef XK_DEBUG

    traceethp = TR_ERRORS;
    tracearpp = TR_ERRORS;
    tracevnetp = TR_ERRORS;
    traceipp = TR_ERRORS;
    traceicmpp = TR_ERRORS;
    tracevmuxp = TR_ERRORS;
    traceblastp = TR_ERRORS;
    tracevsizep = TR_ERRORS;
    traceudpp = TR_ERRORS;
    tracetcpp = TR_ERRORS;
    tracebidctlp = TR_ERRORS;
    tracebidp = TR_ERRORS;
    tracechanp = TR_ERRORS;
    traceprotocol = TR_ERRORS;
    traceprottest = TR_ERRORS;
    traceptbl = TR_ERRORS;
    traceinit = TR_ERRORS;

#endif XK_DEBUG
}
