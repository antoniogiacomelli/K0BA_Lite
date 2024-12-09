/******************************************************************************
 *
 *     [[K0BA - Kernel 0 For Embedded Applications] | [VERSION: 0.3.0]]
 *
 ******************************************************************************
 ******************************************************************************
 *  Module           : Inter-task Communication
 *  Depends on       : Inter-task Synchronisation
 *  Provides to      : Application and System Tasks
 *  Public API       : Yes
 *
 *  In this unit:
 *                  o Indirect Blocking/Full Synch Mailbox
 *                  o Indirect Asynchronous Mailbox
 *                  o Indirect Cyclical Asynchronous Pump-Drop Queue
 *                  o Indirect Blocking Message Queue
 *                  o Multi-reader-writer Pipes
 *
 *****************************************************************************/

#define K_CODE
#include "kconfig.h"
#include "kobjs.h"
#include "kitc.h"
#include "klist.h"
#include "kmem.h"
#include "kutils.h"
#include "kinternals.h"

/*******************************************************************************
 * BLOCKING/SYNCHRONOUS MAILBOX
 *******************************************************************************/
#if (K_DEF_MBOX==ON)
K_ERR kMboxInit(K_MBOX *const kobj, SIZE mailSize, ADDR initMailPtr)
{
	K_CR_AREA
	K_ENTER_CR
	K_ERR err = -1;
	if (kobj == NULL)
	{
		KFAULT(FAULT_NULL_OBJ);
		return (K_ERROR);
	}
	BOOL fullFlag = (initMailPtr == NULL) ? (FALSE) : (TRUE);
	kobj->mboxState = (fullFlag) ? (MBOX_FULL) : (MBOX_EMPTY);
	kobj->senderTID = 0;
	kobj->init = TRUE;
	if (fullFlag)
	{
		kobj->mailPtr = initMailPtr;
	}
	kobj->mailSize = mailSize;
	kListInit(&kobj->readersMailQueue, "rdMailQ");
	kListInit(&kobj->writersMailQueue, "wrMailQ");
	K_EXIT_CR
	return (err);
}



/******************************************************************************
 * mbox contract:
 * (1) a mailbox is either full or empty.
 * (2) a rder blocks on an empty box.
 * (3) a wrtr blocks on a full box.
 * (4) when a rder successfully access a mbox, it is now the owner, and mbox
 *     will be empty. a wrtr is free, if any.
 * (5) when a wrtr successfully access a mbox, it is now the owner, and mbox
 *     will be full. a rder is free, if any.
 * (6) from (1)-(5), a box cannot have a wrt and rdr ownr at once.
 * (7) when blocking, higher prio wanna-access-task propagates its prio to
 *     the owner.
 * (8) the owner restores its priority when the wanna-access gains access
 ****************************************************************************/
K_ERR kMboxPost(K_MBOX *const kobj, ADDR const sendPtr)
{
	K_CR_AREA
	if (kIsISR())
	{
		kErrHandler(FAULT_ISR_INVALID_PRIMITVE);
	}
	if ((kobj == NULL) || (sendPtr == NULL))
	{
		KFAULT(FAULT_NULL_OBJ);
	}
	if (!kobj->init)
	{
		KFAULT(FAULT_OBJ_NOT_INIT);
	}

	K_ENTER_CR
	/* a reader is yet to read */
	if (kobj->mboxState == MBOX_FULL)
	{
		/* not-empty blocks a writer */
#if(K_DEF_MBOX_ENQ==K_DEF_ENQ_FIFO)
		kTCBQEnq(&kobj->writersMailQueue, runPtr);
#else
		kTCBQEnqByPrio(&kobj->writersMailQueue, runPtr);
#endif
		runPtr->status = PENDING_FULL_MBOX;
		if ((kobj->owner != NULL) && (runPtr->priority < kobj->owner->priority))
		{
			kobj->owner->priority = runPtr->priority;
		}
		K_PEND_CTXTSWTCH
		K_EXIT_CR
		K_ENTER_CR
	}
	if ((kobj->owner != NULL)
			&& (kobj->owner->priority != kobj->owner->realPrio))
	{
		kobj->owner->priority = kobj->owner->realPrio;
	}
	kobj->owner = runPtr;
	kobj->senderTID = runPtr->uPid;
	kobj->mailPtr = sendPtr;
	kobj->mboxState = MBOX_FULL;

	/*  full: unblock a reader */
	if (kobj->readersMailQueue.size > 0)
	{
		K_TCB *freeReadPtr;
		kTCBQDeq(&kobj->readersMailQueue, &freeReadPtr);
		K_ERR err = kReadyCtxtSwtch(freeReadPtr);
		assert(err==0);
	}
	K_EXIT_CR
	return (K_SUCCESS);
}

TID kMboxPend(K_MBOX *const kobj, ADDR* recvPPtr)
{
	K_CR_AREA

	if (kIsISR())
	{
		kErrHandler(FAULT_ISR_INVALID_PRIMITVE);
	}
	if ((kobj == NULL) || (recvPPtr == NULL))
	{
		KFAULT(FAULT_NULL_OBJ);
	}
	if (!kobj->init)
	{
		KFAULT(FAULT_OBJ_NOT_INIT);
	}

	K_ENTER_CR

	if (kobj->mboxState == MBOX_EMPTY)
	{
#if(K_DEF_MBOX_ENQ==K_DEF_ENQ_FIFO)
		kTCBQEnq(&kobj->readersMailQueue, runPtr);
#else
		kTCBQEnqByPrio(&kobj->readersMailQueue, runPtr);
#endif
		runPtr->status = PENDING_EMPTY_MBOX;
		runPtr->pendingMbox = kobj;
		/* priority inheritance */
		if ((kobj->owner != NULL) && (runPtr->priority < kobj->owner->priority))
		{
			kobj->owner->priority = runPtr->priority;
		}
		K_PEND_CTXTSWTCH
		K_EXIT_CR
		K_ENTER_CR
	}
	if ((kobj->owner != NULL)
			&& (kobj->owner->priority != kobj->owner->realPrio))
	{
		kobj->owner->priority = kobj->owner->realPrio;
	}
	kobj->owner = runPtr;
	*recvPPtr = kobj->mailPtr;
	kobj->mailPtr=NULL; /*destroy mesg in mbox*/
	kobj->mboxState = MBOX_EMPTY;

	/* empty: unblock a writer */
	if (kobj->writersMailQueue.size > 0)
	{
		K_TCB *freeWriterPtr;
		kTCBQDeq(&kobj->writersMailQueue, &freeWriterPtr);
		K_ERR err = kReadyCtxtSwtch(freeWriterPtr);
		assert(err==0);
	}

	K_EXIT_CR

	return (kobj->senderTID);
}

K_MBOX_STATUS kMboxQuery(K_MBOX *const kobj)
{

	return (kobj->mboxState);
}

SIZE kMboxGetSize(K_MBOX *const kobj)
{

	return (kobj->mailSize);
}

#endif

/*******************************************************************************
 * FULL ASYNCHRONOUS MAILBOX
 *******************************************************************************/
#if (K_DEF_AMBOX==ON)

K_ERR kAmboxInit(K_AMBOX *const kobj, ADDR initMailPtr, BYTE initMailSize)
{

	K_CR_AREA
	K_ENTER_CR
	if (IS_NULL_PTR(kobj))
	{
		KFAULT(FAULT_NULL_OBJ);
		K_EXIT_CR
		return (K_ERROR);
	}
	if ((initMailPtr != NULL) && (initMailSize == 0))
	{
		K_EXIT_CR
		return (K_ERR_MBOX_SIZE);
	}
	if ((initMailPtr == NULL) && (initMailSize > 0))
	{
		K_EXIT_CR
		return (K_ERR_MBOX_SIZE);
	}
	kobj->init = TRUE;
	if (initMailPtr != NULL)
	{
		kobj->aMailSize = initMailSize;
	}
	else
	{
		kobj->aMailPtr = NULL;
		kobj->aMailSize = 0;
	}
	K_EXIT_CR
	return (K_SUCCESS);
}
K_ERR kAmboxRecvCpy(K_AMBOX *const kobj, ADDR const recvPtr)
{
	K_CR_AREA
	K_ENTER_CR
	if (IS_NULL_PTR(kobj))
	{
		KFAULT(FAULT_NULL_OBJ);
		K_EXIT_CR
		return (K_ERR_OBJ_NULL);
	}
	if (!kobj->init)
	{
		KFAULT(FAULT_OBJ_NOT_INIT);
		K_EXIT_CR
		return (K_ERR_OBJ_NOT_INIT);
	}
	if (kobj->aMailPtr == NULL)
	{
		K_EXIT_CR
		return (K_ERR_AMBOX_EMPTY);
	}
	if (kobj->aMailSize == 0)
	{
		K_EXIT_CR
		return (K_ERR_AMBOX_SIZE);
	}
	UINT32 r;
	CPY(recvPtr, kobj->aMailPtr, kobj->aMailSize, r);
	if (r != 0)
	{
		/* destroy data */
		kobj->aMailPtr = NULL;
		K_EXIT_CR
		return (K_SUCCESS);
	}
	K_EXIT_CR
	return (K_ERROR);
}

K_ERR kAmboxRecv(K_AMBOX *const kobj, ADDR* const recvPtr)
{
	K_CR_AREA
	K_ENTER_CR
	if (IS_NULL_PTR(kobj))
	{
		KFAULT(FAULT_NULL_OBJ);
		K_EXIT_CR
		return (K_ERR_OBJ_NULL);
	}
	if (!kobj->init)
	{
		KFAULT(FAULT_OBJ_NOT_INIT);
		K_EXIT_CR
		return (K_ERR_OBJ_NOT_INIT);
	}
	if (kobj->aMailPtr == NULL)
	{
		K_EXIT_CR
		return (K_ERR_AMBOX_EMPTY);
	}
	if (kobj->aMailSize == 0)
	{
		K_EXIT_CR
		return (K_ERR_AMBOX_SIZE);
	}
	UINT32 r;
	CPY(recvPtr, kobj->aMailPtr, kobj->aMailSize, r);
	if (r != 0)
	{
		/* destroy data */
		kobj->aMailPtr = NULL;
		K_EXIT_CR
		return (K_SUCCESS);
	}
	K_EXIT_CR
	return (K_ERROR);
}

/* it does not destroy the message*/
K_ERR kAmboxRecvKeep(K_AMBOX *const kobj, ADDR const recvPtr)
{

	K_CR_AREA
	K_ENTER_CR
	if (IS_NULL_PTR(kobj))
	{
		KFAULT(FAULT_NULL_OBJ);
	}
	if (!kobj->init)
	{
		KFAULT(FAULT_OBJ_NOT_INIT);
	}
	if (kobj->aMailPtr == NULL)
	{
		K_EXIT_CR
		return (K_ERR_AMBOX_EMPTY);
	}
	if (kobj->aMailSize == 0)
	{
		K_EXIT_CR
		return (K_ERR_AMBOX_SIZE);
	}
	UINT32 r;
	CPY(recvPtr, kobj->aMailPtr, kobj->aMailSize, r);
	/* keep the message here  */
	if (r != 0)
	{
		K_EXIT_CR
		return (K_SUCCESS);
	}
	K_EXIT_CR
	return (K_ERR_MESG_CPY);
}

K_ERR kAmboxSend(K_AMBOX *const kobj, ADDR const sendPtr, SIZE aMailSize)
{
	K_CR_AREA
	K_ENTER_CR
	if (IS_NULL_PTR(kobj) || IS_NULL_PTR(sendPtr))
	{
		KFAULT(FAULT_NULL_OBJ);
		return (K_ERR_OBJ_NULL);
	}
	if (!kobj->init)
	{
		KFAULT(FAULT_OBJ_NOT_INIT);
		K_EXIT_CR
	}
	if (kobj->aMailPtr != NULL)
	{
		K_EXIT_CR
		return (K_ERR_AMBOX_FULL);
	}
	if (aMailSize == 0)
	{
		K_EXIT_CR
		return (K_ERR_AMBOX_SIZE);
	}
	kobj->aMailPtr = sendPtr;
	kobj->aMailSize = aMailSize;
	K_EXIT_CR
	return (K_SUCCESS);
}
/* it doesnt check for a full box*/
K_ERR kAmboxSendOvw(K_AMBOX *const kobj, ADDR const sendPtr, BYTE aMailSize)
{
	K_CR_AREA
	K_ENTER_CR
	if (IS_NULL_PTR(kobj) || IS_NULL_PTR(sendPtr))
	{
		KFAULT(FAULT_NULL_OBJ);
		return (K_ERR_OBJ_NULL);
	}
	if (kobj->init == FALSE)
	{
		KFAULT(FAULT_OBJ_NOT_INIT);
	}
	if (aMailSize == 0)
	{
		K_EXIT_CR
		return (K_ERR_AMBOX_SIZE);
	}
	kobj->aMailPtr = sendPtr;
	kobj->aMailSize = aMailSize;
	K_EXIT_CR
	return (K_SUCCESS);
}

#endif

#if (K_DEF_PDQ == ON)

/******************************************************************************
 * PUMP-DROP QUEUE (CYLICAL ASYNCHRONOUS BUFFERS)
 ******************************************************************************/
/* [downbeat funky bass]      */
/* (back:make it drop!)       */
/* intro:                     */
/* embdded is full with       */
/* queues that clawg so i show*/
/* ya a solution i knowz      */
/* this isnt  mine, u can see */
/* the project HAR-TIK        */

/* nooow,                     */
/* first ya  reserve it       */
/* then ya attach             */
/* where resides a  chunk     */
/* with dat info to pass      */
/* then ya can pump it        */
/* yo, ya wanna pump no air   */
/* needa mind the scope       */
/* cuz' no copy  I pass       */
/* u best use d'allocator     */
/* my homie b4da55            */
/* here no heap will trip     */
/* we no fragment no address  */

K_ERR kPDQInit(K_PDQ *const kobj, K_PDBUF *bufPool, BYTE nBufs)
{
	K_CR_AREA
	K_ENTER_CR
	K_ERR err = K_ERROR;
	err = BLKPOOLINIT(&kobj->pdbufMemCB, bufPool, sizeof(K_PDBUF), nBufs);
	if (!err)
	{
		/* nobody is using anything yet */
		kobj->currBufPtr = NULL;
		kobj->init = TRUE;
	}
	K_EXIT_CR
	return (err);
}

K_PDBUF* kPDQReserve(K_PDQ *const kobj)
{
	K_CR_AREA
	K_ENTER_CR
	if (!kobj->init)
		KFAULT(FAULT_OBJ_INIT);
	K_PDBUF *allocPtr = NULL;
	allocPtr = BLKALLOC(&kobj->pdbufMemCB);
	if (!allocPtr)
	{
		kobj->failReserve++;
	}
	K_EXIT_CR
	return (allocPtr);
}

K_ERR kPDBufWrite(K_PDBUF *bufPtr, ADDR srcPtr, SIZE dataSize)
{
	K_CR_AREA
	K_ENTER_CR
	K_ERR err = 0;
	if (srcPtr == NULL)
	{
		err = K_ERR_OBJ_NULL;
		goto EXIT;
	}
	if (dataSize == 0)
	{
		err = K_ERR_PDBUF_SIZE;
		goto EXIT;
	}
	bufPtr->dataPtr = srcPtr;
	bufPtr->dataSize = dataSize;
	EXIT:
	K_EXIT_CR
	return (err);
}

K_ERR kPDQPump(K_PDQ *kobj, K_PDBUF *buffPtr)
{
	K_CR_AREA
	K_ENTER_CR
	if (!kobj->init)
		KFAULT(FAULT_OBJ_INIT);
	/* first pump will skip this if */
	if (kobj->currBufPtr != NULL && kobj->currBufPtr->nUsers == 0)
	{ /*deallocate curr buf if not null and not used */
		K_ERR err = BLKFREE(&kobj->pdbufMemCB, kobj->currBufPtr);
		if (err)
		{
			K_EXIT_CR
			return (err);
		}
	}
	kobj->currBufPtr = buffPtr;
	K_EXIT_CR
	return (K_SUCCESS);
}

K_PDBUF* kPDQFetch(K_PDQ *const kobj)
{
	K_CR_AREA
	K_ENTER_CR
	if (!kobj->init)
		KFAULT(FAULT_OBJ_INIT);
	if (kobj->currBufPtr)
	{
		K_PDBUF *ret = kobj->currBufPtr;
		kobj->currBufPtr->nUsers++;
		K_EXIT_CR
		return (ret);
	}
	K_EXIT_CR
	return (NULL);
}

K_ERR kPDBufRead(K_PDBUF *bufPtr, ADDR destPtr)
{
	K_CR_AREA
	K_ENTER_CR
	K_ERR err = 0;
	if (bufPtr == NULL || destPtr == NULL)
	{
		err = K_ERR_OBJ_NULL;
		goto EXIT;
	}
	if (destPtr != NULL)
	{
		CPY(destPtr, bufPtr->dataPtr, bufPtr->dataSize, err);
	}
	EXIT:
	K_EXIT_CR
	return (err);
}

K_ERR kPDQDrop(K_PDQ *kobj, K_PDBUF *bufPtr)
{
	K_CR_AREA
	K_ENTER_CR
	if (!kobj->init)
		KFAULT(FAULT_OBJ_INIT);
	K_ERR err = K_ERROR;
	if (bufPtr)
	{
		if (bufPtr->nUsers > 0)
			bufPtr->nUsers--;
		/* deallocate if not used and not the curr buf */
		if ((bufPtr->nUsers == 0) && (kobj->currBufPtr != bufPtr))
		{
			err = BLKFREE(&kobj->pdbufMemCB, bufPtr);
		}
		K_EXIT_CR
		return (err);
	}
	K_EXIT_CR
	return (err);
}

#endif

/*******************************************************************************
 * PIPES
 *******************************************************************************/
#if (K_DEF_PIPE==ON)

K_ERR kPipeInit(K_PIPE *const kobj)
{
	if (IS_NULL_PTR(kobj))
		KFAULT(FAULT_NULL_OBJ);
	K_CR_AREA
	K_ENTER_CR
	K_ERR err = -1;
	kobj->head = 0;
	kobj->tail = 0;
	kobj->data = 0;
	kobj->room = K_DEF_PIPE_SIZE;
	err = EVNTINIT(&(kobj->evData));
	assert(err==0);
	err = EVNTINIT(&(kobj->evRoom));
	assert(err==0);
	K_EXIT_CR
	return (err);
}

UINT32 kPipeRead(K_PIPE *const kobj, BYTE *destPtr, UINT32 nBytes)
{

	K_CR_AREA
	K_ENTER_CR
	if (IS_NULL_PTR(kobj))
		KFAULT(FAULT_NULL_OBJ);
	UINT32 readBytes = 0;
	if (nBytes == 0)
		return (0);
	if (kobj->tail == kobj->head)
	{
		SLPEVNT(&(kobj->evData)); /* wait for data from writers */
		K_EXIT_CR

		K_ENTER_CR
	}
	while (kobj->data)
	{

		*destPtr = kobj->buffer[kobj->tail]; /* read from the tail  */
		destPtr++;
		kobj->tail += 1;
		kobj->tail %= K_DEF_PIPE_SIZE; /* wrap around */
		kobj->data -= 1; /* decrease data       */
		readBytes += 1; /* increase read bytes */
		kobj->room += 1;
	}
	if (readBytes)
	{
		WKEVNT(&(kobj->evRoom));
		K_EXIT_CR
		DMB
		return (readBytes); /* return number of read bytes    */
	}
	else
	{
		K_EXIT_CR
		return (0);
	}
}

UINT32 kPipeWrite(K_PIPE *const kobj, BYTE *srcPtr, UINT32 nBytes)
{
	K_CR_AREA
	K_ENTER_CR
	if (IS_NULL_PTR(kobj))
		KFAULT(FAULT_NULL_OBJ);
	UINT32 writeBytes = 0;
	if (nBytes == 0)
		return (0);
	if (nBytes <= 0)
		return (0);
	if (kobj->head != kobj->tail)
	{
		SLPEVNT(&(kobj->evRoom));
		K_EXIT_CR
		K_ENTER_CR
	}
	while (kobj->room)
	{
		kobj->buffer[kobj->head] = *srcPtr;
		kobj->head += 1;
		kobj->head %= K_DEF_PIPE_SIZE;
		kobj->data++;
		kobj->room -= 1;
		srcPtr++;
		writeBytes++;
		if (writeBytes >= nBytes)
		{
			break;
		}
	}
	WKEVNT(&(kobj->evData));
	K_EXIT_CR
	DMB
	return (writeBytes);
}
#endif /*K_DEF_PIPES*/

/*******************************************************************************
 * INDIRECT BLOCKING MESSAGE QUEUE
 *******************************************************************************/

#if(K_DEF_MESGQ==ON)

/*SysMesg Poolf*/

static MESG sysmesgPool[K_DEF_N_MESG]; /* reserved memory  */
static QUEUE sysmesgQueue; /* the double-list as a queue */
static BOOL sysmesgPoolInit = FALSE;
static UINT32 sysmesgCnt = K_DEF_N_MESG;
static UINT32 sysmesgReq = 0;


static inline VOID kMesgPoolInit_()
{
    QUEUEINIT( &sysmesgQueue, "sysmesglist");
    SIZE idx = 0;
    for (idx = 0; idx < K_DEF_N_MESG; ++idx)
        ENQUEUE( &sysmesgQueue, &sysmesgPool[idx].mesgNode);
}

static inline K_MESG* kMesgGet_(VOID)
{
    K_CR_AREA
    K_ENTER_CR
    /* dont need a lock */
    if (sysmesgCnt == 0)
    {
        K_EXIT_CR
        return NULL;
    }
    K_LISTNODE* listNodePtr = NULL;
    K_ERR err = -1;
    err = DEQUEUE( &sysmesgQueue, &listNodePtr);
    assert(err==0);
    K_MESG* kMesgPtr = K_LIST_GET_SYSMESG_NODE(listNodePtr);
    if (kMesgPtr)
    {
        kMesgPtr->mesgNode = *listNodePtr;
        sysmesgCnt --;
        K_EXIT_CR
        return (kMesgPtr);
    }
    K_EXIT_CR
    return (NULL);
}

static inline K_ERR kMesgPut_(K_MESG* const kMesgPtr)
{
    K_CR_AREA
    K_ENTER_CR
    K_ERR err = ENQUEUE( &sysmesgQueue, &kMesgPtr->mesgNode);
    assert(err==0);
    sysmesgCnt ++;
    K_EXIT_CR
    return (K_SUCCESS);
}

TID kMesgGetSenderID(K_MESG* const kMesgPtr)
{
    if (kMesgPtr == NULL)
        return (IDLETASK_ID); /*impossible, idletask is illiterate*/
    return (kMesgPtr->senderTid);
}

ADDR kMesgGetPtr(K_MESG* const kMesgPtr)
{
    if (kMesgPtr == NULL)
        return (NULL);
    return (kMesgPtr->mesgPtr);
}

K_ERR kMesgQInit(K_MESGQ* const kobj, ADDR const mesgPoolPtr,
        BYTE const queueSize, BYTE const mesgSize)
{

    if (IS_NULL_PTR(kobj) || IS_NULL_PTR(mesgPoolPtr))
        return (K_ERR_OBJ_NULL);
    if (queueSize == 0)
        return (K_ERR_INVALID_Q_SIZE);
    if (mesgSize == 0)
        return (K_ERR_INVALID_QMESG_SIZE);
    if (queueSize > K_DEF_N_MESG)
        return (K_ERR_INVALID_Q_SIZE);
    K_CR_AREA
    K_ENTER_CR
    sysmesgReq += queueSize;
    if (sysmesgReq >= K_DEF_N_MESG)
    {
        KFAULT(FAULT_SYSMESG_N_EXCEEDED);
    }
    if ( !sysmesgPoolInit)
    {
        kMesgPoolInit_();
    }
    K_ERR err = -1;
    err = SEMINIT( & (kobj->semaItem), 0);
    assert(err==0);
#if (K_DEF_MESGQ_BLOCK_FULL==ON)
    err = SEMINIT( &kobj->semaRoom, queueSize);
    assert(err==0);
#endif
    if (mesgPoolPtr!=NULL)
    {
    	err = BLKPOOLINIT( & (kobj->mesgqMcb), (ADDR ) mesgPoolPtr,
    			(BYTE ) mesgSize, queueSize);
    	assert(err==0);
    }
    else
    {
    	KFAULT(FAULT_NULL_OBJ);
    }
    err = QUEUEINIT( &kobj->mesgqList, "qlist");
    assert(err==0);
    kobj->itemSize = mesgSize;
    kobj->init = TRUE;
    K_EXIT_CR
    return (K_SUCCESS);
}

K_ERR kMesgQSend(K_MESGQ* const kobj, ADDR const mesgPtr, BYTE const mesgSize)
{

    K_CR_AREA
    if (IS_NULL_PTR(kobj) || IS_NULL_PTR(mesgPtr))
        KFAULT(FAULT_NULL_OBJ);
    if (kobj->init == FALSE)
        KFAULT(FAULT_OBJ_NOT_INIT);
    if (kobj->itemSize < mesgSize)
        return (K_ERR_MESG_SIZE);
    if (kobj->mesgqMcb.init==FALSE)
    {
    	return (K_ERR_MESGQ_NO_BUFFER);
    }
#if (K_DEF_MESGQ_BLOCK_FULL==ON)
    SEMDWN( &kobj->semaRoom);
#endif
    K_ENTER_CR
    K_MESG* kMesgPtr = MESGGET();

    if (kMesgPtr == NULL)
    {
        KFAULT(FAULT_MEM_ALLOC);
    }
    kMesgPtr->mesgPtr = BLKALLOC( &kobj->mesgqMcb);
    kMesgPtr->senderTid = runPtr->uPid;
    kMesgPtr->mesgSize = mesgSize;
    UINT32 r;
    CPY(kMesgPtr->mesgPtr, mesgPtr, mesgSize, r);
    if (r == 0)
    {
        KFAULT(FAULT_MEM_CPY);
        K_EXIT_CR
        return (K_ERR_MEM_CPY);
    }
    /*add sysmesg on queue list */
    K_ERR err = ENQUEUE( &kobj->mesgqList, & (kMesgPtr->mesgNode));
    assert(err==0);
    SEMUP( & (kobj->semaItem));
    K_EXIT_CR
    DMB
    return (K_SUCCESS);
}

K_ERR kMesgQRecv(K_MESGQ* const kobj, ADDR const recvMesgPtr, TID* senderTIDPtr)
{

    K_CR_AREA
    if (IS_NULL_PTR(kobj))
        KFAULT(FAULT_NULL_OBJ);
    if (kobj->init == FALSE)
        return (K_ERR_OBJ_NOT_INIT);
    K_ERR err = K_SUCCESS;
    if (kobj->mesgqMcb.init==FALSE)
    {
    	return (K_ERR_MESGQ_NO_BUFFER);
    }
    SEMDWN( &kobj->semaItem);
    K_ENTER_CR
    /* copy data */
    /* get K_MESG from the queue private list */
    K_LISTNODE* dequeuedNodePtr = NULL;
    err = DEQUEUE( &kobj->mesgqList, &dequeuedNodePtr);
    if (err < 0)
        KFAULT(FAULT_LIST);
    K_MESG* kMesgPtr = K_LIST_GET_SYSMESG_NODE(dequeuedNodePtr);
    if (kMesgPtr == NULL)
        KFAULT(FAULT_SYS_MESG_GET);
    UINT32 r = 0;
    CPY(recvMesgPtr, kMesgPtr->mesgPtr, kMesgPtr->mesgSize, r);
    if (r == 0)
        KFAULT(FAULT_QUEUE_MESG_CPY);
    *senderTIDPtr = kMesgPtr->senderTid;
    /*return dynamic allocated objs to their pools */
    err = MESGPUT(kMesgPtr); /* return to the system pool */
    if (err < 0)
        KFAULT(FAULT_SYS_MESG_GET);
    /* return to the queue pool */
    err = (BLKFREE( &kobj->mesgqMcb, kMesgPtr->mesgPtr));
    if (err < 0)
        KFAULT(FAULT_MEM_FREE);
#if (K_DEF_MESGQ_BLOCK_FULL == ON)
    SEMUP( &kobj->semaRoom);
#endif
    K_EXIT_CR
    DMB
    return (K_SUCCESS);
}

K_ERR kMesgQJam(K_MESGQ* const kobj, ADDR const mesgPtr, BYTE const mesgSize)
{

    K_CR_AREA

    if (IS_NULL_PTR(kobj) || IS_NULL_PTR(mesgPtr))
        KFAULT(FAULT_NULL_OBJ);
    if (kobj->init == FALSE)
        return (K_ERR_OBJ_NOT_INIT);
    if (kobj->itemSize < mesgSize)
        return (K_ERR_MESG_SIZE);
#if (K_DEF_MESGQ_BLOCK_FULL == ON)
    SEMDWN( &kobj->semaRoom);
#endif

    K_ENTER_CR
    K_MESG* kMesgPtr = MESGGET();
    if (kMesgPtr == NULL)
    {
        KFAULT(FAULT_MEM_ALLOC);
    }
    kMesgPtr->mesgPtr = BLKALLOC( &kobj->mesgqMcb);
    kMesgPtr->senderTid = runPtr->uPid;
    kMesgPtr->mesgSize = mesgSize;
    UINT32 r;
    CPY(kMesgPtr->mesgPtr, mesgPtr, mesgSize, r);
    if (r == 0)
    {
        KFAULT(FAULT_MEM_CPY);
        K_EXIT_CR
        return (K_ERR_MEM_CPY);
    }
    /* add sysmesg on queue list, on the HEAD */
    K_ERR err = ENQJAM( &kobj->mesgqList, & (kMesgPtr->mesgNode));
    if (err < 0)
        KFAULT(FAULT_LIST);
    SEMUP( & (kobj->semaItem));
    K_EXIT_CR
    return (K_SUCCESS);
}


#endif /*K_DEF_MESGQ*/
