/*
 *
 * hdr-utils.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/03 07:21:36 $
 */

/*
 *
 *  Support functions for converting between different byte orders
 *
 *    This file represents work in progress
 *
 */

#include "xk_debug.h"
#define HDRUTILS
#include "hdr-utils.h"

extern int tracemachripcp;

unsigned int   nop_1(n) char *n; { return *n; }

long  nop_64(n) char *n;  { return (nop_32(n) << 32) | nop_32(n+4) ; }
int   nop_32(n) char *n; { return ((((((*n & 0xff)<<8) | (*(n+1)&0xff)) <<8) | (*(n+2)&0xff)) << 8) | (*(n+3) & 0xff); }
short nop_16(n) char *n; { return ((*n & 0xff)<<8) | (*(n+1)&0xff); }
char  nop_8(n)  char *n;  { return *n; }

long  swap_64(n) char *n;  { 
  return 
#ifdef HASLONGLONGS
/* make this have two 32-bit quantities */
    swap_32(n + 8) | swap_32(n)
#else
  0
#endif HASLONGLONGS
    ;
 }
int   swap_32(n) char *n; { return ((*n & 0xffff0000) << 16) | (*(n+2) & 0xffff); }
short swap_16(n) char *n; { return ((*n & 0xff) << 8) | (*(n+1) & 0xff); }
char  swap_8(n)  char *n;  { return *n & 0xff; }

long  twist_64(n) char *n;  {  return twist_64(n)<<32 | twist_64(n+8); }
int   twist_32(n) char *n; {  return ((((((*n & 0xff) << 8) | (*(n+1) & 0xff)) << 8) | (*(n+2) & 0xff)) << 8) | (*(n+3)& 0xff); }
short twist_16(n) char *n; { return *n; }
char  twist_8(n)  char *n;  { return *n & 0xff; }

long  unscramble_64(n) char *n; { return unscramble_32(n)<<32 | unscramble_32(n+8); };
int   unscramble_32(n) char *n;  { return ((((((*n & 0xff) << 8) | (*(n+1) & 0xff)) << 8) | (*(n+2) & 0xff)) << 8) | (*(n+3)& 0xff); }
short unscramble_16(n) char *n; { return *n; }
char  unscramble_8(n)  char *n;  { return *n & 0xff; }

float real_error(n) int n;
{ 
  xTrace0(machripcp,
	  TR_ALWAYS, "machnetipc: data conversion: cannot convert reals");
  return 0.0;
}

long  (*unpermute_int64[4])() = { nop_64, swap_64, twist_64, unscramble_64 };
int   (*unpermute_int32[4])() = { nop_32, swap_32, twist_32, unscramble_32 };
short (*unpermute_int16[4])() = { nop_16, swap_16, twist_16, unscramble_16 };
float (*unpermute_real8[4])() = { real_error, real_error, real_error, real_error };

int
arch_unpermute_index(arch_type) enum SOURCE_BYTE_ARCH arch_type;
{
  switch (arch_type) {
   case MN_ARCH_MARKER:   return(0);
   case MN_OTHER_ENDIAN:  return(1);
   case MN_WORD_SWAP:     return(2);
   case MN_SCRAMBLE:      return(3);
  }
  xTrace1(machripcp, TR_ERRORS, "machnetipc: arch_unpermute_index: bad arch_type 0x%x", arch_type);
  return 0;
}

/*
 *  set_convert_vector (arch, vec)
 *
 *   Sets up a vector of functions for converting from a given
 *   source architecture to the local architecture.
 *   The resulting vector is indexed by the data type ---
 *      
 *     vec[MACH_MSG_TYPE_INTEGER_16] is a function to convert
 *       from an incoming representation of a 16-bit integer
 *       to a local integer.
 */
void
set_convert_vector( arch, vec )
     enum SOURCE_BYTE_ARCH arch;
     Pfv *vec;
{
  int i;
  int archin = arch_unpermute_index(arch);

  vec[0] = (Pfv)nop_8;
  vec[1] = (Pfv)unpermute_int16[archin];
  vec[2] = (Pfv)unpermute_int32[archin];
  vec[3] = (Pfv)nop_8;
  vec[4] = (Pfv)nop_8;
  vec[5] = (Pfv)nop_8;
  vec[8] = (Pfv)unpermute_real8[archin];
  vec[9] = (Pfv)nop_8;
  vec[10] = (Pfv)nop_8;
  vec[11] = (Pfv)nop_8;
  vec[12] = (Pfv)nop_8;
}

Pfv
real_conversion_func(enum SOURCE_BYTE_ARCH arch, int real_arch)
{
  xTrace0(machripcp, TR_ERRORS, "machnetipc: real_conversion_func called: no action");
  return 0;
}

