/*
 * msg_internal.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.20 $
 * $Date: 1993/02/01 23:57:32 $
 */

/* this should be a power of two; see PAGEROUND in msg.c */

#define MSG_STATISTICS

#ifndef XKMACHKERNEL
#define PageSize 4096
#define PAGEROUND(len)  ((len + MNodeLeafSize + (PageSize-1)) / PageSize)
#else
/* this hack is more efficient for funky inkernel malloc/free */
#define PageSize 2048
/* was 4096 -- rcs */
#define PAGEROUND(len)  ((len + MNodeLeafSize + (PageSize-1) + sizeof(int)) / PageSize)
#endif ! XKMACHKERNEL
/* 
 * Size of virtual space for tree nodes
 */
#ifdef MSGCACHEING
static int MNodePairVSpace = 1 * PageSize;
static int MNodeBufVSpace =  1 * PageSize;
static int MNodePageVSpace = 126 * PageSize;

static char *mWindow;
#endif MSGCACHEING


/* enum NodeType */

enum NodeType {
  t_MNodeJunk,
  t_MNodePair,
  t_MNodePage,
  t_MNodeUsrPage,
  t_MNodeBuf,
#ifdef MSG_NEW_ALG
  t_MNodeDummy
#endif MSG_NEW_ALG
};

/* These sizes are computed from the MNode structure definition */
#define MNodeSize (sizeof(enum NodeType) + sizeof(long))
#define MNodePairSize (MNodeSize + 2 * sizeof(struct nlink))
#define MNodeLeafSize (MNodeSize + sizeof(long) + sizeof(char *))
#define MNodePageSize (PageSize)
#define MNodePageDataSize (MNodePageSize - MNodeLeafSize)
#define MNodeUsrPageSize (MNodeLeafSize + sizeof(void (*)()))
#define MNodeBufSize  (MNodeLeafSize + sizeof(void (*)()))

#define mprsize MNodePairSize
#define mpgsize MNodePageSize
#define mupsize MNodeUsrPageSize
#define mbfsize MNodeBufSize

#define mprtype t_MNodePair
#define mpgtype t_MNodePage
#define muptype t_MNodeUsrPage
#define mbftype t_MNodeBuf

/*
 *     the left-right node pair constituent
 */

struct nlink {
  long offset;
  long length;
  MNode  *node;
};

/*
 * struct MNode
 * all tree nodes
 */
struct MNode {
  enum NodeType type;
  unsigned long refCnt;
  union {
    struct {
      struct nlink l, r;
    } pair;
    struct {
      long  size;     /* size of buffer */
      char *data;     /* the buffer */
    } leaf;
    struct {
      long  size;     /* size of buffer */
      char *data;     /* the buffer */
      char buffer[MNodePageDataSize];
    } page;
    struct {
      long  size;     /* size of buffer */
      char *data;     /* the buffer */
      void (*bFree)(/* void* */);  /* the buffer's deallocator function */
    } usrpage;
    struct {
      long  size;     /* size of buffer */
      char *data;     /* the buffer */
      void (*bFree)(/* void* */);  /* the buffer's deallocator function */
    } buf;
  } b;
};



/*
 * struct MNodePair
 */
#ifdef MSGCACHEING
static MNodePair* mpr_freeStore;
static int        mpr_numNodes;
static int        mpr_lastFreeNode;
#endif
static int	  mpr_used;
static int	  mpr_hiwat;

/*
 * struct MNodePage
 * a leaf node on one vm page
 */
#ifdef MSGCACHEING
static MNodePage* mpg_freeStore;
static int        mpg_numNodes;
static int        mpg_lastFreeNode;
#endif
static int	  mpg_used;
static int	  mpg_hiwat;

/*
 * struct MNodeUsrPage
 * a leaf node with a user-supplied buffer
 */
#ifdef MSGCACHEING
static MNodeUsrPage* mup_freeStore;
static int        mup_numNodes;
static int        mup_lastFreeNode;
#endif
static int	  mup_used;
static int	  mup_hiwat;

/*
 * struct MNodeBuf
 * a leaf node that refers to a user-provided buffer
 */

#ifdef MSGCACHEING
static MNodeBuf*  mbf_freeStore;
static int        mbf_numNodes;
static int        mbf_lastFreeNode;
#endif
static int	  mbf_used;
static int	  mbf_hiwat;

extern MNodeBuf  *newMNodeBuf();

extern void msgShow();

#ifdef MSG_NEW_ALG
static MNodeLeaf   dummyStack;
#if 0 
static char       *vmWindow;
#endif
/* limit for number of nodes allocated to a Msg */
#ifdef CONTROL_RESOURCES
static int numNodesHardLimit = 1000;
#endif CONTROL_RESOURCES
static int numNodesSoftLimit = 100;
static int msgSpace_used, msgSpace_hiwat;


#endif MSG_NEW_ALG
