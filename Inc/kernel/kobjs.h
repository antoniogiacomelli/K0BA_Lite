/**
 *****************************************************************************
 *
 * [K0BA - Kernel 0 For Embedded Applications] | [VERSION: 0.3.0]
 *
 ******************************************************************************
 *\file kobjs.h
 *\brief    Kernel objects
 *\version  0.3.0
 *\author   Antonio Giacomelli
 *****************************************************************************/
#ifndef KOBJS_H
#define KOBJS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ktypes.h"
/**
 *\brief Node structure for general circular doubly linked list
 */
struct kListNode
{
	struct kListNode* nextPtr; /* Pointer to next node */
	struct kListNode* prevPtr; /* Pointer to previous node */
};

/**
 *\brief  Circular doubly linked list structure
 */
struct kList
{
	struct kListNode listDummy; /* Dummy node for head/tail management */
	STRING listName; /* List name */
	UINT32 size; /* Number of items in the list */
	BOOL init;
};

/******************************************************************************/

/**
 * \brief Task Control Block
 */
struct kTcb
{
	UINT32* sp;           /* Saved stack pointer */
	K_TASK_STATUS status; /* Task status */
	UINT32 runCnt;        /* Dispatch count */
	UINT32* stackAddrPtr; /* Stack address */
	UINT32 stackSize;     /* Stack size */
	PID pid;              /* System-defined task ID */
	TID uPid;             /* User-defined   task ID */
	PRIO priority;        /* Task priority (0-31) 32 is invalid */
	PRIO realPrio;        /* Real priority (for prio inheritance) */
#if (K_DEF_SCH_TSLICE == ON)
	TICK timeSlice;       /* Time-slice duration 		   */
	TICK timeLeft;        /* Remaining time-slice 	   */
#endif
	STRING taskName;      /* Task name */
    BOOL runToCompl;      /* Cooperative-only task     */
	TICK busyWaitTime;    /* Busy-Delay in ticks 			   */
    TICK   attmptCntr;      /* Trying to access CR */

/*--------------------------------------------------------*/
#if (K_DEF_SEMA == ON)
    K_SEMA* pendingSema;  /* Pending sema    */
#endif
#if (K_DEF_MUTEX==ON)
	K_MUTEX* pendingMutx;  /* Pending mutex          */
#endif
#if (K_DEF_SLEEPWAKE==ON)
	K_EVENT* pendingEv;   /* Pending event */
#endif
	K_MBOX* pendingMbox;   /* Pending mbox */
	K_TIMER* pendingTmr;  /* Pending timer */
	UINT32 lostSignals;   /* Number of lost direct signals */
	UINT32 nPreempted;    /* Preemption count  */
	PID    preemptedBy;   /* PID that preempted */
    UINT32  nPrioBoost;   /* n of prio inhert   */
/*---------------------------------------------------------*/

    struct kListNode tcbNode; /* Aggregated list node 	   */
} __attribute__((aligned));

/******************************************************************************/

/**
 *\brief Record of ticks
 */
struct kRunTime
{
	TICK globalTick; /* Global system tick */
	UINT32 nWraps; /* Number of tick wraps */
};

/*****************************************************************************/

#if (K_DEF_SEMA==ON)
/**
 *\brief  Counter Semaphore
 */
struct kSema
{
	INT32 value; /* Semaphore value */
	struct kList queue; /* Semaphore waiting queue */
    struct kTcb* ownerPtr; /* Task owning the mutex */
	BOOL   init;
};
#endif

/*****************************************************************************/

/**
 *\brief Mutex
 */
#if (K_DEF_MUTEX == ON)
struct kMutex
{
	struct kList queue; /* Mutex waiting queue */
	BOOL lock; /* 0=unlocked, 1=locked */
	struct kTcb* ownerPtr; /* Task owning the mutex */
	BOOL init; /* Init state */
};
#endif

/*****************************************************************************/

#if (K_DEF_SLEEPWAKE==ON)
/**
 *\brief Generic Event
 */
struct kEvent
{
	struct kList queue; /* Waiting queue */
	BOOL init; /* Init flag */
	UINT32 eventID; /* event ID */
};
#if (K_DEF_PIPE == ON) /* still within kdefsleepwake*/
struct kPipe
{
    UINT32 tail; /* read index */
    UINT32 head; /* write index */
    UINT32 data; /* Number of bytes in the buffer */
    UINT32 room; /* Number of free slots in the buffer (bytes) */
    struct kEvent evRoom; /* Cond Var for writers */
    struct kEvent evData; /* Cond Var for readers */
    BYTE buffer[K_DEF_PIPE_SIZE]; /* Data buffer */
    BOOL init;
};
#endif /* K_DEF_PIPES */
#endif /* K_DEF_SLEEPWAKE */

/******************************************************************************/

#define MEMBLKLAST (1)
/**
 *\brief Fixed-size pool memory control block (BLOCK POOL)
 */
struct kMemBlock
{
	BYTE* freeListPtr; /* Pointer to the head of the free list*/
	BYTE* poolPtr;
	BYTE blkSize; /* Size of each block (in bytes) */
	BYTE nMaxBlocks; /* Total number of blocks in the pool */
	BYTE nFreeBlocks; /* Current number of free blocks available */
#if (MEMBLKLAST)
	BYTE* lastUsed;
#endif
	BOOL init;
};
/******************************************************************************/

#if (K_DEF_MUTEX==ON)
/* RIP */
#define      MBOX_LOCK      (0)
#define      AMBOX_LOCK     (0)
#define      MESGQ_LOCK     (0)
#endif

/******************************************************************************/

#if (K_DEF_MBOX==ON)
/**
 * \brief Indirect Blocking or Fully Synchronous Mailbox
 */
struct kMailbox
{
    /*[CONFIG]*/
    BOOL init; /* Init 			  */
    /*[CONTROL]*/
    K_MBOX_STATUS mboxState;
    K_TCB* owner;
    struct kList writersMailQueue;
    struct kList readersMailQueue;
    /*EXCHANGE*/
    TID senderTID;
    BYTE   mailSize;
    ADDR   mailPtr;
    BOOL   needAck;
} __attribute__((aligned(4)));
#endif

/******************************************************************************/

#if (K_DEF_AMBOX==ON)
/**
 * \brief Indirect Asynchronous Mailbox
 */
struct kAsynchMbox
{
    /*[CONFIG]*/
    BOOL init; /* Init */
    /*EXCHANGE*/
    BYTE aMailSize;
    ADDR aMailPtr;

} __attribute__((aligned(4)));

#endif

/******************************************************************************/

#if ((K_DEF_MESGQ==ON))

/**
 *\brief Message Record
 */
struct kMesg
{
    TID senderTid; /* Sender Task ID*/
    ADDR mesgPtr; /* Pointer to message contents */
    SIZE mesgSize; /* Mesg size */
    struct kListNode mesgNode; /* Mesg Queue Node */
} __attribute__((aligned));


/**
 *\brief Indirect Message Queue
 */
struct kMesgQ
{
	struct kSema semaItem; /* Semaphore indicating a new message */
#if (K_DEF_MESGQ_BLOCK_FULL==ON)
	struct kSema semaRoom;
#endif
	struct kMemBlock mesgqMcb; /* Mem Ctrl Block of the associated mesg pool */
	struct kList mesgqList;
	BYTE   itemSize;
	BOOL   init;

};

#endif /*K_DEF_MSG_QUEUE*/

/******************************************************************************/
#if (K_DEF_PDQ== ON)
struct kPumpDropBuf
{

     ADDR   dataPtr;
     SIZE   dataSize;                /* mesg size in this buf */
     UINT32 nUsers;                  /* number of tasks using */
};
struct kPumpDropQueue
{
    struct kMemBlock        pdbufMemCB;    /* associated allocator */
    struct kPumpDropBuf*    currBufPtr;    /* current buffer   */
    UINT32 failReserve;
    BOOL                    init;
};
#endif
/*****************************************************************************/
/**
 *\brief Application Timer
 */
struct kTimer
{
	STRING timerName; /* Timer name */
	TICK dTicks; /* Delta ticks */
	TICK ticks; /* Abs ticks */
	BOOL reload; /* Reload/Oneshot */
	CALLOUT funPtr; /* Callback */
	ADDR argsPtr; /* Arguments */
	TID taskID;
	K_TIMER* nextPtr; /* Next timer in the queue */
	BOOL init;
} __attribute__((aligned));

/*****************************************************************************/


/*[EOF]*/


#ifdef __cplusplus
}
#endif

#endif /* KOBJS_H */
