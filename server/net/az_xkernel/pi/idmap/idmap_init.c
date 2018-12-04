/* 
 * idmap_init.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.7 $
 * $Date: 1993/02/01 23:55:54 $
 */

  map_functions[KEYSIZE].bind    = BINDNAME;
  map_functions[KEYSIZE].remove  = REMOVENAME;
  map_functions[KEYSIZE].resolve = RESOLVENAME;
  map_functions[KEYSIZE].unbind  = UNBINDNAME;
#undef KEYSIZE
#undef BINDNAME
#undef REMOVENAME
#undef RESOLVENAME
#undef UNBINDNAME
