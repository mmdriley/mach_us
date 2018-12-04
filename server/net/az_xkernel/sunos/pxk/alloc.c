/* 
 * alloc.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.3 $
 * $Date: 1993/02/01 23:53:55 $
 */

/*
 * There are 2 debugging flags of interest.  DEBUGMALLOC checks very
 * carefully that allocated blocks are not being corrupted.  Additional
 * information is collected and checks are done on every alloc and free.
 * DEBUGMALLOCLEAKS turns on recording of stack backtraces for every
 * allocated block so that storage leaks can be found.
 */

#include "platform.h"
#include "assert.h"

#ifdef NDEBUG
#undef DEBUGMALLOC
#undef DEBUGMALLOCLEAKS
#else
#endif

#if defined(DEBUGMALLOCLEAKS)
#define DEBUGMALLOC
#endif

#if defined(DEBUGMALLOC) || defined(DEBUGMALLOCLEAKS)
#define ANYDEBUGMALLOC
#endif

int debuggingmalloc = 
#ifdef ANYDEBUGMALLOC
1;
#else
0;
#endif

#ifdef ANYDEBUGMALLOC
#define LMALLOC i_malloc
#define LFREE i_free
/* The number of extra longs, >= 1 */
#ifdef  DEBUGMALLOCLEAKS
#define MALLOC_NPCS	5
#else
#define MALLOC_NPCS	1
#endif
#ifdef DEBUGMALLOC
#define MALLOC_CHECK	4
#define MALLOC_EXTRAS 	10
#else
#define MALLOC_CHECK	1
#define MALLOC_EXTRAS	0
#endif
#define MALLOC_STUFF	((2 * MALLOC_NPCS) + MALLOC_CHECK)
/* The number of malloc blocks to store */
#define MAXBLOCKS	8192
#define FIRST_MALLOC_TAG	0x441199ee
#define SECOND_MALLOC_TAG	0x55ff0011
#define TAIL_MALLOC_TAG		0xdd007722
#else
#define LMALLOC malloc
#define LFREE free
#endif


#ifdef XSIMUL
#define SBRK(X) sbrk(X)
extern char *sbrk();
#else
#define SBRK(X) xsbrk(X)
extern char *xsbrk();
#ifdef USERLIB
#define abort() { printf("Abort\n"); xexit(); }
#else

#endif
#endif

#define USESPL
#ifdef GORP
#include <stdio.h>
#endif

/*  An implementation of malloc(3), free(3) using the QuickFit method.
 *  Guy Almes, May 1983.
 *
 *  Cf. MALLOC(3) in the Unix manual for external specifications.
 *
 *  Cf. Chuck Weinstock's PhD thesis for a discussion of the techniques
 *  used.  (Charles Weinstock, Dynamic Storage Allocation Techniques,
 *  April 1976, CMU)
 *  nb: Unlike the original QuickFit, this implementation assumes that
 *  TailFit always works.  Also, we want to allow for some other user to
 *  be using sbrk.
 *
 *  NOTE: change made on 11/29/84 by Mike Schwartz to call HoldSigs and
 *  ReleaseSigs at beginning and end of malloc if signals weren't
 *  already held (and running in Eden Kernel or EFT context).  This is needed
 *  in case a Unix routine that uses malloc is called, so that signals
 *  will be held as needed to protect the critical region variables during
 *  the allocation.
 *
 *  Hacked by oystr.  While the external specification was met, the
 *  internal behaviour of the standard malloc(3) was not.  To wit:
 *  areas free'd do not have their contents touched until reallocated
 *  (Almes actually fixed this after considerable pleading); it is
 *  possible to do multiple free's of the same area - that is "x = malloc();
 *  free(x); free(x);" causes no damage.  This turkey originally did
 *  not detect such a situation even though the multiple free's caused
 *  it to trash the list structures and blow up at some future malloc
 *  call.  Realloc was also a disaster area, making the assumption that
 *  the old block was in use, which is not necessarily the case.
 */
/*  The parts of Unix used by this module:
 */

/*  From BRK(2):
 */
    extern char *brk();      /* set break to addr */

/*  From END(3):
 */
    extern int end;                 /* first byte beyond bss region */

/*  Design parameters; these can be changed to tune the implementation
 *  to a specific application.
 */

#define GrainSize       8
#define LogGrainSize    3
#define HeaderSize      sizeof(int)
    /* number of bytes: every allocated block is
     * (k+1) * GrainSize, for k >= MinGrains
     * GrainSize == (1 << LogGrainSize)
     */

#define MinBytes        0
#define MaxBytes        (1604 - HeaderSize)         /* = 1532 bytes */
#define MinGrains       0
#define MaxGrains       ((MaxBytes-MinBytes+1) >> LogGrainSize)
    /* The implementation is tuned to use ExactFit for blocks in the range
     * MinBytes .. MaxBytes
     */

#define BtoG(b) ( ((b)-(GrainSize-HeaderSize) + GrainSize-1) >> LogGrainSize )
    /* convert a number of bytes to a number of grains */
#define GtoB(g) ( ((g) << LogGrainSize) + (GrainSize-HeaderSize) )
    /* and vice versa */

#define Nil (0)
    /* a nil pointer; often coerced to various flavors of pointers */

#define SBrkChunk (8 * 1024)
    /* the number of bytes to get from sbrk when growing the Tail */
#define roundUp(a,b) ((char *) ( ((int)(a) + (b)-1) & ~((b)-1) ))
    /* round up an address to the nearest 1k boundary like sbrk does */

/* header for Big blocks allocated via MiscFit */
typedef struct bigHeader {
        struct bigHeader *bNext;  /* the next field links available blocks */
        unsigned bSize;        /* the size of the block in bytes */
} bHeader, *bHeaderPtr;

/* header for Small blocks allocated via ExactFit */
typedef union smallHeader {
        unsigned sSize;        /* the size of a used block in bytes */
        union smallHeader *sNext;  /* the next field links available blocks */
} sHeader, *sHeaderPtr;

static char *FirstTailMin = 0;/* first sbrk'd block of storage */
static char *TailMin = 0;     /* points to 1st byte of Tail */
static char *TailMax = 0;     /* points just beyond end of Tail */
    /* invariants: TailMin and TailMax are each on int boundaries.
     *      The area with addresses TailMin <= addr < TailMax is available.
     *      &end <= TailMin <= TailMax == sbrk(0).
     */

/*
 * Free/in use magic numbers.  WARNING: if we ever get into 16MB+
 * virtual addresses ( >24 bits), you will have to change the headers.
 * The upper byte of the size field contains INUSEMAGIC if the
 * block has been malloc'd but not freed, 0 if the block is free.
 */
#define INUSEMAGIC 0xdd
#define SETMAGIC   ((unsigned)0xdd000000)
#define CLRMAGIC   0x00FFFFFF

static long *TailFit(nBytes)
register unsigned nBytes;
{
    register char *oldTailMin;
#ifdef GORP
printf("   TailFit(%d) called.  Tail: %x - %x.\n", nBytes, TailMin, TailMax);
#endif
    while (TailMax < TailMin+nBytes) {
        register char *oldBreak = SBRK(SBrkChunk);
        if ((int) oldBreak == -1) {
#ifdef GORP
            printf("sbrk returned a -1!!");
#endif
            return ( Nil );
        }
        if (oldBreak != TailMax) {  /* someone else did an sbrk! */
#ifdef GORP
            printf(
               "TailFit: pushed TailMin from %x to %x.\n",
                 TailMin, roundUp(oldBreak, sizeof(int)));
#endif
            TailMin = roundUp(oldBreak, sizeof(int));
	    if (FirstTailMin == 0) FirstTailMin = TailMin;
        }
        TailMax = oldBreak + SBrkChunk;
        if (TailMax != roundUp(TailMax, SBrkChunk)) {
                                            /* bring TailMax to a page */
#ifdef GORP
            printf(
                "TailFit: rounding TailMax from %x to %x.\n",
                TailMax, roundUp(TailMax, SBrkChunk));
#endif
#ifndef BRKISFIXEDONSUNSUSINGGPROF
	    (void) SBRK(roundUp(TailMax, SBrkChunk) - TailMax);
	    TailMax = roundUp(TailMax, SBrkChunk);
#else
            TailMax = roundUp(TailMax, SBrkChunk);
            (void) brk(TailMax);
#endif
        }
    } /* we now know we have enough in the Tail */
    oldTailMin = TailMin;
    TailMin += nBytes;
#ifdef GORP
printf(
    "   TailFit returns %x.  Tail: %x - %x.\n", oldTailMin, TailMin, TailMax);
#endif
    return ((long *) oldTailMin );
} /* TailFit */

static bHeaderPtr BigList;
/* BigLists points to a ring of zero or more available blocks, each marked
 * with both a size and a next field.  This ring is used in the MiscFit
 * portion of the algorithm, which is a simple FirstFit for very large blocks.
 */

static char *MiscFit(nBytes)
register unsigned nBytes;
{
    register bHeaderPtr CurrentHdr, PreviousHdr;
    register unsigned oldSize;
    register char *newBlock = Nil;
#ifdef GORP
printf("   MiscFit(%d) called.\n", nBytes);
#endif
    if (BigList != (bHeaderPtr) Nil) {
        PreviousHdr = BigList;
        CurrentHdr = PreviousHdr->bNext;
        oldSize = PreviousHdr->bSize;  /* save real size field, then ... */
        PreviousHdr->bSize = nBytes;        /* ... forge a stopper value */
        while (CurrentHdr->bSize < nBytes) {
            PreviousHdr = CurrentHdr;
            CurrentHdr = PreviousHdr->bNext;
        } /* this loop always terminates due to the stopper value */
        BigList->bSize = oldSize;    /* restore old size */
        if (CurrentHdr->bSize >= nBytes) {   /* MiscFit worked */
            PreviousHdr->bNext = CurrentHdr->bNext;
            BigList =
                (PreviousHdr==CurrentHdr ? (bHeaderPtr) Nil : PreviousHdr);
	    CurrentHdr->bSize |= SETMAGIC;
            newBlock = (char *) (CurrentHdr+1);
        }
    }
#ifdef GORP
printf("   MiscFit returns %x.\n", newBlock);
#endif
    return( newBlock );
} /* MiscFit */

/* p = malloc(nBytes);
 * where nBytes is unsigned number of bytes needed and p is some pointer
 */

static sHeaderPtr AvailList[MaxGrains-MinGrains+1];
/* AvailList[MinGrains .. MaxGrains] is for ExactFit blocks
 *
 * AvailList[MinGrains..MaxGrains] each point to a LIFO singly-linked list of
 * equal sized blocks.  These lists are used in the ExactFit portion of the
 * algorithm.
 */

char *LMALLOC(reqBytes)
unsigned reqBytes;
{
    register char *newBlock;
    register unsigned nGrains;
#ifdef USESPL
    int x = spl7();
#endif
    nGrains = BtoG(reqBytes);
#ifdef GORP
    printf("malloc(%d) called.\n", reqBytes);
#endif

    if (reqBytes > MaxBytes) {
        register unsigned nBytes = GtoB(nGrains);
        newBlock = MiscFit(nBytes);
        if (newBlock == Nil) {
            bHeaderPtr newRawBlock = 
                            (bHeaderPtr) TailFit(nBytes + sizeof(bHeader));
            if (newRawBlock == (bHeaderPtr) Nil) {
		abort();
                newBlock = Nil;
            } else {
                newBlock = (char *) (newRawBlock+1);
                newRawBlock->bSize = nBytes | SETMAGIC;
            }
        }
    } else {
        register sHeaderPtr newRawBlock = AvailList[nGrains];
        if (newRawBlock != (sHeaderPtr) Nil) {
            AvailList[nGrains] = newRawBlock->sNext;
            newRawBlock->sSize = GtoB(nGrains) | SETMAGIC;
            newBlock = (char *) (newRawBlock + 1);
        } else {
            newRawBlock = (sHeaderPtr) TailFit(GtoB(nGrains)+sizeof(sHeader));
            if (newRawBlock == (sHeaderPtr) Nil) {
		abort();
                newBlock = Nil;
            } else {
                newBlock = (char *) (newRawBlock + 1);
                newRawBlock->sSize = GtoB(nGrains) | SETMAGIC;
            }
        }
    }
#ifdef GORP
printf("malloc returns %x.\n", newBlock);
#endif


#ifdef USESPL
    splx(x);
#endif

    return( newBlock );
} /* malloc */

void LFREE(ptr)
char *ptr;
{
    register sHeaderPtr oldBlock;
    register unsigned nGrains;
#ifdef USESPL
    int x;
#endif

    if (! ptr) return;
    oldBlock  = ((sHeaderPtr) ptr) - 1;
    nGrains = BtoG( (oldBlock->sSize & CLRMAGIC) );
    xAssert((oldBlock->sSize & SETMAGIC) == SETMAGIC);
    if ( ! (oldBlock->sSize & SETMAGIC) ) {
	/* already free */
#ifdef GORP
printf("free(%x) called returning %d grains (already free!).\n",
	ptr, nGrains);
#endif
	return;
    }
    else oldBlock->sSize &= CLRMAGIC;

#ifdef USESPL
    x = spl7();
#endif
#ifdef GORP
printf("free(%x) called returning %d grains.\n", ptr, nGrains);
#endif

    if (nGrains <= MaxGrains) {
        oldBlock->sNext = AvailList[nGrains];
        AvailList[nGrains] = oldBlock;
    } else {
        register bHeaderPtr oBlock = ((bHeaderPtr) ptr) - 1;
        if (BigList == (bHeaderPtr) Nil) {
            BigList = oBlock;
            oBlock->bNext = oBlock;
        } else {
            oBlock->bNext = BigList->bNext;
            BigList->bNext = oBlock;
        }
    }

#ifdef USESPL
    splx(x);
#endif

} /* free */

char *malloc();
void free();

char *realloc(oldBlock, nBytes)
register char *oldBlock;
unsigned int nBytes;
{
    register char *newBlock = malloc(nBytes);

    if (newBlock != Nil) {
        register int *newPtr = (int *) newBlock;
        register int *oldPtr = (int *) oldBlock;
	register unsigned oldSize = 
	  (((sHeaderPtr) oldPtr) - 1)->sSize & CLRMAGIC;
        register unsigned nGrains = 
	  oldSize > nBytes ? BtoG(nBytes) : BtoG(oldSize);
        while (nGrains > 0) {
#if GrainSize != 8
  Change this.  Inline expand enough of these to copy GrainSize bytes
#endif
            *newPtr++ = *oldPtr++;
            *newPtr++ = *oldPtr++;
            nGrains--;
        }
#if GrainSize != 8
  Change this.  Have one less than before
#endif
        *newPtr++ = *oldPtr++;
        free(oldBlock);
    }

    return( newBlock );
} /* realloc */

void alloc_stats(total, fre, wasted)
int *total, *fre, *wasted;
{
  register int lfree, lwasted, i, b;
  register sHeaderPtr p;
  register bHeaderPtr q;
  int x = spl7();

  *total = TailMax - FirstTailMin;
  lfree = TailMax - TailMin;
  lwasted = 0;

  /* collect from fixed-sized lists */
  for (i = MinGrains; i <= MaxGrains; i++) {
    b = GtoB(i);
    for (p = AvailList[i]; p != Nil; p = p->sNext) {
      lfree += b;
      lwasted += sizeof(sHeader);
    }
  }
  /* collect from big list */
  if (BigList != Nil) {
    q = BigList;
    do {
      lfree += q->bSize;
      lwasted += sizeof(bHeader);
      q = q->bNext;
    } while (q != BigList);
  }
  *fre = lfree;
  *wasted = lwasted;
  splx(x);
}

#ifdef ANYDEBUGMALLOC

static long *blocks[MAXBLOCKS];
static int nextblock = 1;

static void printBlock(mp)
long *mp;
{
  int i, num;
#define CHECKONE(S,V,E) if ((V) != (E)) printf("%s wrong, %X != %X\n",S,V,E);
  CHECKONE("first tag", *mp, FIRST_MALLOC_TAG);
  mp++;
  CHECKONE("second tag", *mp, SECOND_MALLOC_TAG);
  mp++;
  printf("Size = %d\n", num = *mp++);
  printf("Block index = %d\n", *mp++);
  printf("Freed from %X\n", *mp++);
  for (i = 1; i < MALLOC_NPCS; i++) {
    printf("      from %X\n", *mp++);
  }
  printf("Allocated from %X\n", *mp++);
  for (i = 1; i < MALLOC_NPCS; i++) {
    printf("      from %X\n", *mp++);
  }
  mp += (num + sizeof(long) - 1) / sizeof(long);
  for (i = 0; i < MALLOC_EXTRAS; i++) {
    if (*mp != TAIL_MALLOC_TAG) 
      printf("Final tag %d wrong %X != %X\n", i, *mp, TAIL_MALLOC_TAG);
    mp++;
  }
}

#ifdef DEBUGMALLOC
#ifdef PARANOID
#define M_STEP 1
#define M_INC(X) (X)
#else
#define M_STEP 17
#define M_INC(X) (++(X))
#endif
#undef assert
#define assert(X) { if (!(X)) { printBlock(baseptr); abort(); } }

static void checkBlocks()
{
  register int i, j;
  static int start = 0;
  long *lmp, *pmp, *baseptr;
  long num, block, *callerspc, *freeerspc;

  for (i = (M_INC(start) >= M_STEP ? start = 0: start);
       i < MAXBLOCKS;
       i += M_STEP) {
    if (lmp = blocks[i]) {
      baseptr = lmp;
      for (pmp = lmp; *pmp != FIRST_MALLOC_TAG; --pmp) ;
      assert(*lmp++ == FIRST_MALLOC_TAG);
      assert(*lmp++ == SECOND_MALLOC_TAG);
      num = *lmp++;
      block = *lmp++;
      freeerspc = (long *)*lmp++;
#ifdef DEBUGMALLOCLEAKS
      lmp += MALLOC_NPCS -1;
#endif
      callerspc = (long *)*lmp++;
#ifdef DEBUGMALLOCLEAKS
      lmp += MALLOC_NPCS -1;
#endif
      lmp += (num + sizeof(long) - 1) / sizeof(long);
      for (j = 0; j < MALLOC_EXTRAS; j++) assert(*lmp++ == TAIL_MALLOC_TAG);
    }
  }
}
#else
#define checkBlocks() 
#endif

char *malloc(num)
unsigned int num;
{
  register long *callerspc;		/* r11 on vax, a5 on sun */
  register long *mp;
  register char *answer;
  int startblock = nextblock;
  int i;

  checkBlocks();
#ifdef vax
  asm("	movl 16(fp), r11");
#endif
#ifdef sun
  asm("	movl a6@(4), %0" : "=d" (callerspc));
#endif

  mp = (long *) i_malloc (num + (MALLOC_EXTRAS + MALLOC_STUFF) * sizeof(long));
  while (blocks[nextblock]) {
    if (++nextblock >= MAXBLOCKS) nextblock = 1;
    if (nextblock == startblock) {
      nextblock = 0;
      break;
    }
  }
  blocks[nextblock] = mp;
#ifdef DEBUGMALLOC
  *mp++ = FIRST_MALLOC_TAG;
  *mp++ = SECOND_MALLOC_TAG;
  *mp++ = num;
#endif
  *mp++ = nextblock;
  /* Freeers PCs */
  *mp++ = (long) 0;
#ifdef DEBUGMALLOCLEAKS
  for (i = 1; i < MALLOC_NPCS; i++) *mp++ = 0;
#endif
  /* Allocators PCs */
  *mp++ = (long) callerspc;
#ifdef DEBUGMALLOCLEAKS
  fillPCs(mp);
  mp += MALLOC_NPCS - 1;
#endif
  answer = (char *) mp;
#ifdef DEBUGMALLOC
  mp += (num + sizeof(long) - 1) / sizeof(long);
  for (i = 0; i < MALLOC_EXTRAS; i++) *mp++ = TAIL_MALLOC_TAG;
#endif
  return answer;
}

char *calloc(n, size)
int n, size;
{
  register long *callerspc;		/* r11 on vax, a5 on sun */
  register long *mp;
  register char *answer;
  register int num = n * size;
  int startblock = nextblock;
#if defined(DEBUGMALLOC) || defined(DEBUGMALLOCLEAKS)
  int i;
#endif

  checkBlocks();
#ifdef vax
  asm("	movl 16(fp), r11");
#endif
#ifdef sun
  asm("	movl a6@(4), %0" : "=d" (callerspc));
#endif

  mp = (long *) i_malloc (num + (MALLOC_EXTRAS + MALLOC_STUFF) * sizeof(long));
  while (blocks[nextblock]) {
    if (++nextblock >= MAXBLOCKS) nextblock = 0;
    if (nextblock == startblock) {
      nextblock = -1;
      break;
    }
  }
  blocks[nextblock] = mp;
#ifdef DEBUGMALLOC
  *mp++ = FIRST_MALLOC_TAG;
  *mp++ = SECOND_MALLOC_TAG;
  *mp++ = num;
#endif
  *mp++ = nextblock;
  /* Freeers PCs */
  *mp++ = (long) 0;
#ifdef DEBUGMALLOCLEAKS
  for (i = 1; i < MALLOC_NPCS; i++) *mp++ = 0;
#endif
  /* Allocators PCs */
  *mp++ = (long) callerspc;
#ifdef DEBUGMALLOCLEAKS
  fillPCs(mp);
  mp += MALLOC_NPCS - 1;
#endif
  answer = (char *) mp;
#ifdef DEBUGMALLOC
  mp += (num + sizeof(long) - 1) / sizeof(long);
  for (i = 0; i < MALLOC_EXTRAS; i++) *mp++ = TAIL_MALLOC_TAG;
#endif
  bzero(answer, num);
  return answer;
}

void free(p)
char *p;
{
  register long *freeerspc;		/* r11 on vax, a5 on sun */
  long *baseptr = (long *)p - MALLOC_STUFF, *lmp = baseptr;
  long num, block, *callerspc;
  register int i;

#ifdef vax
  asm("	movl 16(fp), r11");
#endif
#ifdef sun
  asm("	movl a6@(4), %0" : "=d" (freeerspc));
#endif
  assert(freeerspc != 0);

  checkBlocks();
#ifdef DEBUGMALLOC
  assert(*lmp++ == FIRST_MALLOC_TAG);
  assert(*lmp++ == SECOND_MALLOC_TAG);
  num = *lmp++;
#endif
  block = *lmp++;
  assert(*lmp == 0);
  *lmp++ = (long) freeerspc;
#ifdef DEBUGMALLOCLEAKS
  fillPCs(lmp);
  lmp += MALLOC_NPCS - 1;
#endif
  
  callerspc = (long *)*lmp++;
#ifdef DEBUGMALLOCLEAKS
  lmp += MALLOC_NPCS -1;
#endif
#ifdef DEBUGMALLOC
  lmp += (num + sizeof(long) - 1) / sizeof(long);
  for (i = 0; i < MALLOC_EXTRAS; i++) assert(*lmp++ == TAIL_MALLOC_TAG);
#endif
  blocks[block] = 0;
  i_free(baseptr);
}

#ifdef DEBUGMALLOCLEAKS
static unsigned long *currentFp;

compareblocks(ap, bp)
long **ap, **bp;
{
  long *a = *ap, *b = *bp;
  int i;
  if (a == b) return 0;
  if (!a) return  1;
  if (!b) return -1;
  
  a += MALLOC_CHECK + MALLOC_NPCS;
  b += MALLOC_CHECK + MALLOC_NPCS;
  for (i = 0; i < MALLOC_NPCS; i++) {
    if (*a < *b) return -1;
    if (*a > *b) return 1;
    a++; b++;
  }
  return 0;
}

allocTopPCs(n, x)
int n, x;
{
  int i, j, k;
  long last[MALLOC_NPCS], *b;
  static long *lblocks[MAXBLOCKS];
  if (n > MALLOC_NPCS) n = MALLOC_NPCS;
  if (n < 1) n = 1;
  bcopy(blocks, lblocks, sizeof(blocks));
  qsort(lblocks, MAXBLOCKS, sizeof(long *), compareblocks);
  for (i = 0, j = 0; i < MAXBLOCKS; i++, j++) {
    b = lblocks[i];
    if (!b || bcmp(b + MALLOC_CHECK + MALLOC_NPCS, last, n * sizeof(long))) {
      if (j >= x) {
	printf("%d at ", j);
	for (k = 0; k < MALLOC_NPCS; k++) {
	  printf("%X ", last[k]);
	}
	printf("\n");
      }
      if (b) bcopy(b + MALLOC_CHECK + MALLOC_NPCS, last, MALLOC_NPCS * sizeof(long));
      j = 0;
    }
    if (!b) break;
  }
}

fillPCs(mp)
unsigned long *mp;
{
  unsigned long *returnPc, *baseFp;
  unsigned long *stackTop;
  int i;

#ifdef sun
  asm("movl	a6, _currentFp");
# define PREVFPOFFSET 0
# define PREVPCOFFSET 1
#endif
#ifdef vax
  asm("movl	fp, _currentFp");
# define PREVFPOFFSET 3
# define PREVPCOFFSET 4
#endif
#define FETCHPREVFP(XX) ((unsigned long *) (*((XX)+PREVFPOFFSET)))
#define FETCHPREVPC(XX) ((unsigned long *) (*((XX)+PREVPCOFFSET)))

  baseFp = currentFp;
  stackTop = currentFp + (0x1000 / 4);
  /* Simulate a return twice */
  currentFp = FETCHPREVFP(currentFp);
  currentFp = FETCHPREVFP(currentFp);

  for (i = 0; i < MALLOC_NPCS - 1 && currentFp != 0; i++) {
    if (currentFp > stackTop) return;
    if (currentFp < baseFp) return;
    returnPc = FETCHPREVPC(currentFp);
    *mp++ = (long) returnPc;
    if (FETCHPREVFP(currentFp) < currentFp ) return;
    currentFp = FETCHPREVFP(currentFp);
  }
}
#else
allocTopPCs(n, x)
int n, x;
{}
#endif DEBUGMALLOCLEAKS
#else
/*ARGSUSED*/
allocTopPCs(n, x)
int n, x;
{}
#endif ANYDEBUGMALLOC
