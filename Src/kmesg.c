/******************************************************************************
 *
 *     [[K0BA - Kernel 0 For Embedded Applications] | [VERSION: 0.3.1]]
 *
 ******************************************************************************
 ******************************************************************************
 *  Module           : Inter-task Communication
 *  Depends on       : Inter-task Synchronisation
 *  Provides to      : Application
 *  Public API       : Yes
 *
 *  In this unit:
 *  				 Message Passing
 *  				 Pipe
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
#include "ktimer.h"

/*******************************************************************************
 * MAILBOX
 ******************************************************************************/
#if (K_DEF_MBOX==ON)

K_ERR kMboxInit(K_MBOX *const kobj, BOOL initFull, ADDR initMailPtr)
{
	K_CR_AREA

	if (kobj == NULL)
	{
		KFAULT(FAULT_NULL_OBJ);
		return (K_ERROR);
	}
	K_ENTER_CR

	if (initFull)
	{
		if (initMailPtr == NULL)
		{
			K_EXIT_CR
			return (K_ERR_OBJ_NULL);
		}
		kobj->mailPtr = initMailPtr;
	}
	else
	{
		kobj->mailPtr = NULL;
	}

	K_ERR listerr;
	listerr = kListInit(&kobj->waitingQueue, "mailq");
	assert(listerr == 0);
	kobj->timeoutNode.nextPtr = NULL;
	kobj->timeoutNode.timeout = 0;
	kobj->timeoutNode.kobj = kobj;
	kobj->timeoutNode.objectType = MAILBOX;
	kobj->init = TRUE;
	K_EXIT_CR
	return (K_SUCCESS);
}

K_ERR kMboxSend(K_MBOX *const kobj, ADDR const sendPtr, TICK timeout)
{
	K_CR_AREA
	if (kIsISR())
	{
		KFAULT(FAULT_ISR_INVALID_PRIMITVE);
	}
	if ((kobj == NULL) || (sendPtr == NULL))
	{
		KFAULT(FAULT_NULL_OBJ);
	}
	if (kobj->init == FALSE)
	{
		KFAULT(FAULT_OBJ_NOT_INIT);
	}
	K_ENTER_CR
	/* a reader is yet to read */
	if (kobj->mailPtr != NULL)
	{

		if (timeout > 0)
		{
			kTimeOut(&kobj->timeoutNode, timeout);
		}
		if ((kobj->owner != NULL) && (runPtr->priority < kobj->owner->priority))
		{
			kobj->owner->priority = runPtr->priority;
		}
		do
		{
			/* not-empty blocks a writer */
#if(K_DEF_MBOX_ENQ==K_DEF_ENQ_FIFO)
			kTCBQEnq(&kobj->waitingQueue, runPtr);
#else
			kTCBQEnqByPrio(&kobj->waitingQueue, runPtr);
#endif
			runPtr->status = SENDING;
			K_PEND_CTXTSWTCH
			K_EXIT_CR
			K_ENTER_CR
			if (runPtr->timeOut)
			{
				runPtr->timeOut = FALSE;
				K_EXIT_CR
				return (K_ERR_TIMEOUT);
			}
		} while (kobj->mailPtr != NULL);
	}
	if (kobj->owner && (kobj->owner->priority != kobj->owner->realPrio))
		kobj->owner->priority = kobj->owner->realPrio;
	kobj->mailPtr = sendPtr;
	kobj->owner = runPtr;
	/*  full: unblock a reader, if any */
	if (kobj->waitingQueue.size > 0)
	{
		K_TCB *freeReadPtr;
		freeReadPtr = kTCBQPeek(&kobj->waitingQueue);
		if (freeReadPtr->status == RECEIVING)
		{
			kTCBQDeq(&kobj->waitingQueue, &freeReadPtr);
			assert(freeReadPtr != NULL);
			kTCBQEnq(&readyQueue[freeReadPtr->priority], freeReadPtr);
			freeReadPtr->status = READY;
			if (freeReadPtr->priority < runPtr->priority)
				K_PEND_CTXTSWTCH
		}
	}
	K_EXIT_CR
	return (K_SUCCESS);
}

K_ERR kMboxRecv(K_MBOX *const kobj, ADDR *recvPPtr, TICK timeout)
{
	K_CR_AREA

	if (kIsISR())
	{
		return (K_ERR_MBOX_ISR);
	}
	if ((kobj == NULL) || (recvPPtr == NULL))
	{
		KFAULT(FAULT_NULL_OBJ);
	}
	if (kobj->init == FALSE)
	{
		KFAULT(FAULT_OBJ_NOT_INIT);
	}

	K_ENTER_CR

	if (kobj->mailPtr == NULL)
	{

		if (timeout > 0)
		{
			kTimeOut(&kobj->timeoutNode, timeout);
		}
		if ((kobj->owner != NULL) && (runPtr->priority < kobj->owner->priority))
		{
			kobj->owner->priority = runPtr->priority;
		}
		do
		{

#if(K_DEF_MBOX_ENQ==K_DEF_ENQ_FIFO)
			kTCBQEnq(&kobj->waitingQueue, runPtr);
#else
			kTCBQEnqByPrio(&kobj->waitingQueue, runPtr);
#endif
			runPtr->status = RECEIVING;
			K_PEND_CTXTSWTCH
			K_EXIT_CR
			K_ENTER_CR
			if (runPtr->timeOut)
			{ /* timed-out */
				runPtr->timeOut = FALSE;
				K_EXIT_CR
				return (K_ERR_TIMEOUT);
			}
		} while (kobj->mailPtr == NULL);
	}
	if (kobj->owner->priority != kobj->owner->realPrio)
		kobj->owner->priority = kobj->owner->realPrio;
	*recvPPtr = kobj->mailPtr;
	kobj->mailPtr = NULL;
	kobj->owner = runPtr;

	/* it only makes sense do deq a writer */
	if (kobj->waitingQueue.size > 0)
	{
		K_TCB *freeWriterPtr;
		freeWriterPtr = kTCBQPeek(&kobj->waitingQueue);
		if (freeWriterPtr->status == SENDING)
		{
			kTCBQDeq(&kobj->waitingQueue, &freeWriterPtr);
			kTCBQEnq(&readyQueue[freeWriterPtr->priority], freeWriterPtr);
			freeWriterPtr->status = READY;
			if ((freeWriterPtr->priority < runPtr->priority))
				K_PEND_CTXTSWTCH
		}
	}

	K_EXIT_CR

	return (K_SUCCESS);
}

#if (K_DEF_MBOX_SENDRECV==ON)
/* sender does: sendrecv(&mbox, &send, &recv, t);  */
/* receiver does:
 * recv(&mbox, &recv, NULL, t);
 * -assemble answer-
 * send(&mbox, &answer, t);
 *
 ***/
K_ERR kMboxSendRecv(K_MBOX *const kobj, ADDR const sendPtr,
		ADDR *const recvPPtr, TICK timeout)
{
	K_CR_AREA
	if (kIsISR())
	{
		KFAULT(FAULT_ISR_INVALID_PRIMITVE);
	}
	if ((kobj == NULL) || (sendPtr == NULL))
	{
		KFAULT(FAULT_NULL_OBJ);
	}
	if (kobj->init == FALSE)
	{
		KFAULT(FAULT_OBJ_NOT_INIT);
	}
	K_ENTER_CR
	/* a reader is yet to read */
	if (kobj->mailPtr != NULL)
	{

		if (timeout > 0)
		{
			kTimeOut(&kobj->timeoutNode, timeout);
		}
		if ((kobj->owner != NULL) && (runPtr->priority < kobj->owner->priority))
		{
			kobj->owner->priority = runPtr->priority;
		}
		do
		{
			/* not-empty blocks a writer */
#if(K_DEF_MBOX_ENQ==K_DEF_ENQ_FIFO)
			kTCBQEnq(&kobj->waitingQueue, runPtr);
#else
			kTCBQEnqByPrio(&kobj->waitingQueue, runPtr);
#endif
			runPtr->status = RECEIVING;
			K_PEND_CTXTSWTCH
			K_EXIT_CR
			K_ENTER_CR
			if (runPtr->timeOut)
			{
				runPtr->timeOut = FALSE;
				K_EXIT_CR
				return (K_ERR_TIMEOUT);
			}
		} while (kobj->mailPtr != NULL);
	}
	if (kobj->owner && (kobj->owner->priority != kobj->owner->realPrio))
		kobj->owner->priority = kobj->owner->realPrio;
	kobj->mailPtr = sendPtr;
	kobj->owner = runPtr;
	/*  full: unblock a reader, if any */
	if (kobj->waitingQueue.size > 0)
	{
		K_TCB *freeReadPtr;
		freeReadPtr = kTCBQPeek(&kobj->waitingQueue);
		if (freeReadPtr->status == RECEIVING)
		{
			kTCBQDeq(&kobj->waitingQueue, &freeReadPtr);
			assert(freeReadPtr != NULL);
			kTCBQEnq(&readyQueue[freeReadPtr->priority], freeReadPtr);
			freeReadPtr->status = READY;
		}
		/* do not pend here */
	}
	/* will pend after waiting for a recv */
	do
	{
#if(K_DEF_MBOX_ENQ==K_DEF_ENQ_FIFO)
		kTCBQEnq(&kobj->waitingQueue, runPtr);
#else
		kTCBQEnqByPrio(&kobj->waitingQueue, runPtr);
#endif
		runPtr->status = RECEIVING;
		K_PEND_CTXTSWTCH
		K_EXIT_CR
		K_ENTER_CR
		if (runPtr->timeOut)
		{
			runPtr->timeOut = FALSE;
			K_EXIT_CR
			return (K_ERR_TIMEOUT);
		}
		/* while mailPtr does not change or is only read */
	} while (kobj->mailPtr == sendPtr);
	*recvPPtr = kobj->mailPtr;
	kobj->mailPtr = NULL; /* empty again */
	K_EXIT_CR
	return (K_SUCCESS);
}
#endif

#if (K_DEF_AMBOX==ON)
K_ERR kMboxAsend(K_MBOX *const kobj, ADDR const sendPtr)
{
	K_CR_AREA

	if ((kobj == NULL) || (sendPtr == NULL))
	{
		KFAULT(FAULT_NULL_OBJ);
	}
	if (!kobj->init)
	{
		KFAULT(FAULT_OBJ_NOT_INIT);
	}

	K_ENTER_CR

	if (kobj->mailPtr != NULL)
	{
		K_EXIT_CR
		return (K_ERR_MBOX_FULL);
	}

	kobj->mailPtr = sendPtr;
	K_EXIT_CR

	return (K_SUCCESS);
}

K_ERR kMboxAsendOvw(K_MBOX *const kobj, ADDR const sendPtr)
{
	K_CR_AREA

	if ((kobj == NULL) || (sendPtr == NULL))
	{
		KFAULT(FAULT_NULL_OBJ);
	}
	if (!kobj->init)
	{
		KFAULT(FAULT_OBJ_NOT_INIT);
	}

	K_ENTER_CR

	kobj->mailPtr = sendPtr;

	K_EXIT_CR

	return (K_SUCCESS);
}

K_ERR kMboxArecv(K_MBOX *const kobj, ADDR *recvPPtr)
{
	K_CR_AREA

	if ((kobj == NULL) || (recvPPtr == NULL))
	{
		KFAULT(FAULT_NULL_OBJ);
	}

	if (!kobj->init)
	{
		KFAULT(FAULT_OBJ_NOT_INIT);
	}

	K_ENTER_CR

	if (kobj->mailPtr == NULL)
	{
		K_EXIT_CR
		return (K_ERR_MBOX_EMPTY);
	}

	*recvPPtr = kobj->mailPtr;
	kobj->mailPtr = NULL;

	K_EXIT_CR

	return (K_SUCCESS);
}

K_ERR kMboxArecvKeep(K_MBOX *const kobj, ADDR *recvPPtr, TID *senderTIDPtr)
{
	K_CR_AREA

	if ((kobj == NULL) || (recvPPtr == NULL))
	{
		KFAULT(FAULT_NULL_OBJ);
	}

	if (!kobj->init)
	{
		KFAULT(FAULT_OBJ_NOT_INIT);
	}

	K_ENTER_CR

	if (kobj->mailPtr == NULL)
	{
		K_EXIT_CR
		return (K_ERR_MBOX_EMPTY);
	}

	*recvPPtr = kobj->mailPtr;

	K_EXIT_CR

	return (K_SUCCESS);
}

#endif /*asynch mbox*/

BOOL kMboxIsFull(K_MBOX *const kobj)
{

	return ((kobj->mailPtr == NULL) ? FALSE : TRUE);
}

#endif

/*******************************************************************************
 * MESSAGE QUEUE
 *******************************************************************************/
#if(K_DEF_MESGQ==ON)

K_ERR kMesgQInit(K_MESGQ *const kobj, ADDR const buffer, SIZE const mesgSize,
		SIZE const nMesg)
{
	K_CR_AREA

	if ((kobj == NULL) || (buffer == NULL))
	{
		return (K_ERR_OBJ_NULL);
	}
	if (mesgSize == 0)
	{
		return (K_ERR_INVALID_MESG_SIZE);
	}
	if (nMesg == 0)
	{
		return (K_ERR_INVALID_QUEUE_SIZE);
	}

	K_ENTER_CR

	kobj->buffer = buffer;
	kobj->mesgSize = mesgSize;
	kobj->maxMesg = nMesg;
	kobj->mesgCnt = 0;
	kobj->readIndex = 0;
	kobj->writeIndex = 0;

#if (K_DEF_SYNCH_MESGQ==ON)

	kobj->owner = NULL;

	K_ERR err = kListInit(&kobj->waitingQueue, "waitingQueue");
	if (err != 0)
	{
		K_EXIT_CR
		return (K_ERROR);
	}

	kobj->init = 1;
	kobj->timeoutNode.nextPtr = NULL;
	kobj->timeoutNode.timeout = 0;
	kobj->timeoutNode.kobj = kobj;
	kobj->timeoutNode.objectType = MESGQUEUE;

#endif

	K_EXIT_CR

	return (K_SUCCESS);
}

K_ERR kMesgQPeek(K_MESGQ *const kobj, ADDR recvPtr)
{
	K_CR_AREA

	if ((kobj == NULL) || (recvPtr == NULL) || (kobj->init == 0))
	{
		return (K_ERROR);
	}

	K_ENTER_CR

	if (kobj->mesgCnt == 0)
	{
		K_EXIT_CR
		return (K_ERR_MESGQ_EMPTY);
	}
	BYTE const *src = kobj->buffer + (kobj->readIndex * kobj->mesgSize);
	BYTE *dest = (BYTE*) recvPtr;
	SIZE err = 0;
	CPYQ(dest, src, kobj->mesgSize, err);
	if (err != kobj->mesgSize)
	{
		kobj->readIndex = (kobj->readIndex + 1) % kobj->maxMesg;
		K_EXIT_CR
		return (K_ERR_MESG_CPY);
	}
	K_EXIT_CR
	return (K_SUCCESS);
}

#endif

#if (K_DEF_SYNCH_MESGQ==ON)

K_ERR kMesgQSend(K_MESGQ *const kobj, ADDR const sendPtr, TICK const timeout)
{

	K_CR_AREA

	if ((kobj == NULL) || (sendPtr == NULL) || (kobj->init == 0))
	{
		return (K_ERROR);
	}
	if (kIsISR())
		KFAULT(FAULT_ISR_INVALID_PRIMITVE);

	K_ENTER_CR
	if (kobj->mesgCnt >= kobj->maxMesg) /*full*/
	{
		if ((kobj->owner != NULL) && (runPtr->priority < kobj->owner->priority))
		{
			kobj->owner->priority = runPtr->priority;
		}
		if (timeout > 0)
			kTimeOut(&kobj->timeoutNode, timeout);
		do
		{
#if(K_DEF_MBOX_ENQ==K_DEF_ENQ_FIFO)
			kTCBQEnq(&kobj->waitingQueue, runPtr);
#else
			kTCBQEnqByPrio(&kobj->waitingQueue, runPtr);
#endif
			runPtr->status = SENDING;

			K_PEND_CTXTSWTCH
			K_EXIT_CR
			K_ENTER_CR
			if (runPtr->timeOut)
			{
				runPtr->timeOut = FALSE
				;
				K_EXIT_CR
				return (K_ERR_TIMEOUT);
			}
		} while (kobj->mesgCnt >= kobj->maxMesg);
	}
	BYTE *dest = kobj->buffer + (kobj->writeIndex * kobj->mesgSize);
	BYTE const *src = (BYTE const*) sendPtr;
	SIZE err = 0;
	CPYQ(dest, src, kobj->mesgSize, err);

	if (err != kobj->mesgSize)
	{
		K_EXIT_CR

		return (K_ERR_MESG_CPY);
	}
	kobj->writeIndex = (kobj->writeIndex + 1) % kobj->maxMesg;
	kobj->mesgCnt++;

	if ((kobj->owner != NULL)
			&& (kobj->owner->priority != kobj->owner->realPrio))
	{
		kobj->owner->priority = kobj->owner->realPrio;
	}
	kobj->owner = runPtr;

	/* was empty ?*/
	if ((kobj->waitingQueue.size > 0) && (kobj->mesgCnt == 1))
	{
		K_TCB *freeTaskPtr;
		kTCBQDeq(&kobj->waitingQueue, &freeTaskPtr);
		kTCBQEnq(&readyQueue[freeTaskPtr->priority], freeTaskPtr);
		freeTaskPtr->status = READY;

		if (freeTaskPtr->priority < runPtr->priority)
		{
			K_PEND_CTXTSWTCH
		}
	}
	K_EXIT_CR
	return (K_SUCCESS);
}

K_ERR kMesgQRecv(K_MESGQ *const kobj, ADDR recvPtr, TICK const timeout)
{
	K_CR_AREA

	if ((kobj == NULL) || (recvPtr == NULL) || (kobj->init == 0))
	{
		return (K_ERROR);
	}
	if (kIsISR())
		KFAULT(FAULT_ISR_INVALID_PRIMITVE);

	K_ENTER_CR

	if (kobj->mesgCnt == 0)
	{
		if ((kobj->owner != NULL) && (runPtr->priority < kobj->owner->priority))
		{
			kobj->owner->priority = runPtr->priority;
		}
		if (timeout > 0)
			kTimeOut(&kobj->timeoutNode, timeout);
		do
		{
			kTCBQEnq(&kobj->waitingQueue, runPtr);
			runPtr->status = RECEIVING;
			K_PEND_CTXTSWTCH
			K_EXIT_CR
			K_ENTER_CR
			if (runPtr->timeOut == TRUE)
			{
				runPtr->timeOut = FALSE;
				K_EXIT_CR
				return (K_ERR_TIMEOUT);
			}
		} while (kobj->mesgCnt == 0);
	}
	BYTE const *src = kobj->buffer + (kobj->readIndex * kobj->mesgSize);
	BYTE *dest = (BYTE*) recvPtr;
	SIZE err = 0;
	CPYQ(dest, src, kobj->mesgSize, err);
	if (err != kobj->mesgSize)
	{
		K_EXIT_CR
		return (K_ERR_MESG_CPY);
	}
	kobj->readIndex = (kobj->readIndex + 1) % kobj->maxMesg;
	kobj->mesgCnt--;
	if ((kobj->owner != NULL)
			&& (kobj->owner->priority != kobj->owner->realPrio))
	{
		kobj->owner->priority = kobj->owner->realPrio;
	}

	kobj->owner = runPtr;

	/* was full ? unblock */
	if ((kobj->waitingQueue.size > 0) && (kobj->mesgCnt == (kobj->maxMesg - 1)))
	{
		K_TCB *freeTaskPtr;
		kTCBQDeq(&kobj->waitingQueue, &freeTaskPtr);
		kTCBQEnq(&readyQueue[freeTaskPtr->priority], freeTaskPtr);
		freeTaskPtr->status = READY;

		if (freeTaskPtr->priority < runPtr->priority)
		{
			K_PEND_CTXTSWTCH
		}
	}

	K_EXIT_CR
	return (K_SUCCESS);
}

K_ERR kMesgQJam(K_MESGQ *const kobj, ADDR const sendPtr, TICK timeout)
{
	K_CR_AREA
	if ((kobj == NULL) || (sendPtr == NULL) || (kobj->init == 0))
	{
		return (K_ERROR);
	}
	if (kIsISR())
		KFAULT(FAULT_ISR_INVALID_PRIMITVE);

	K_ENTER_CR

	if (kobj->mesgCnt >= kobj->maxMesg) /*full*/
	{
		if ((kobj->owner != NULL) && (runPtr->priority < kobj->owner->priority))
		{
			kobj->owner->priority = runPtr->priority;
		}
		if (timeout > 0)
			kTimeOut(&kobj->timeoutNode, timeout);
		do
		{
#if(K_DEF_MBOX_ENQ==K_DEF_ENQ_FIFO)
			kTCBQEnq(&kobj->waitingQueue, runPtr);
#else
			kTCBQEnqByPrio(&kobj->waitingQueue, runPtr);
#endif
			runPtr->status = SENDING;

			K_PEND_CTXTSWTCH
			K_EXIT_CR
			K_ENTER_CR
			if (runPtr->timeOut)
			{
				runPtr->timeOut = FALSE
				;
				K_EXIT_CR
				return (K_ERR_TIMEOUT);
			}
		} while (kobj->mesgCnt >= kobj->maxMesg);
	}
	kobj->readIndex =
			(kobj->readIndex == 0) ?
					(kobj->maxMesg - 1) : (kobj->readIndex - 1);
	BYTE *dest = kobj->buffer + (kobj->readIndex * kobj->mesgSize);
	BYTE const *src = (BYTE const*) sendPtr;
	SIZE err = 0;
	CPYQ(dest, src, kobj->mesgSize, err);
	if (err != kobj->mesgSize)
	{
		/* restore the read index on failure */
		kobj->readIndex = (kobj->readIndex + 1) % kobj->maxMesg;
		K_EXIT_CR
		return (K_ERR_MESG_CPY);
	}
	/*succeded */
	kobj->mesgCnt++;
	if ((kobj->owner != NULL)
			&& (kobj->owner->priority != kobj->owner->realPrio))
	{
		kobj->owner->priority = kobj->owner->realPrio;
	}

	kobj->owner = runPtr;

	/*was empty?*/
	if ((kobj->mesgCnt == 1) && (kobj->waitingQueue.size > 0))
	{
		K_TCB *freeTaskPtr;
		kTCBQDeq(&kobj->waitingQueue, &freeTaskPtr);
		kTCBQEnq(&readyQueue[freeTaskPtr->priority], freeTaskPtr);
		freeTaskPtr->status = READY;
		if (freeTaskPtr->priority < runPtr->priority)
		{
			K_PEND_CTXTSWTCH
		}
	}
	K_EXIT_CR
	return (K_SUCCESS);
}

#endif

#if (K_DEF_ASYNCH_MESGQ==ON)

K_ERR kMesgQArecv(K_MESGQ *const kobj, ADDR recvPtr, SIZE *mesgCntPtr)
{
	K_CR_AREA

	if ((kobj == NULL) || (recvPtr == NULL) || (kobj->init == 0))
	{
		return (K_ERROR);
	}

	K_ENTER_CR

	if (kobj->mesgCnt == 0)
	{
		K_EXIT_CR
		return (K_ERR_MESGQ_EMPTY);
	}
	BYTE const *src = kobj->buffer + (kobj->readIndex * kobj->mesgSize);
	BYTE *dest = (BYTE*) recvPtr;
	SIZE err = 0;
	CPYQ(dest, src, kobj->mesgSize, err);
	if (err != kobj->mesgSize)
	{
		K_EXIT_CR
		return (K_ERR_MESG_CPY);
	}
	kobj->readIndex = (kobj->readIndex + 1) % kobj->maxMesg;
	kobj->mesgCnt--;
	K_EXIT_CR
	return (K_SUCCESS);
}

K_ERR kMesgQAsend(K_MESGQ *const kobj, ADDR const sendPtr)
{
	K_CR_AREA

	if ((kobj == NULL) || (sendPtr == NULL))
	{
		KFAULT(FAULT_NULL_OBJ);
	}
	if (!kobj->init)
	{
		KFAULT(FAULT_OBJ_NOT_INIT);
	}
	K_ENTER_CR
	if (kobj->mesgCnt >= kobj->maxMesg) /*full*/
	{
		K_EXIT_CR
		return (K_ERR_MESGQ_FULL);
	}
	BYTE *dest = kobj->buffer + (kobj->writeIndex * kobj->mesgSize);
	BYTE const *src = (BYTE const*) sendPtr;
	SIZE err = 0;
	CPYQ(dest, src, kobj->mesgSize, err);
	if (err != kobj->mesgSize)
	{
		K_EXIT_CR
		return (K_ERR_MESG_CPY);
	}
	kobj->writeIndex = (kobj->writeIndex + 1) % kobj->maxMesg;
	kobj->mesgCnt++;
	K_EXIT_CR
	return (K_SUCCESS);
}

K_ERR kMesgQReset(K_MESGQ *kobj)
{
	K_CR_AREA
	if (kobj)
	{
		K_ENTER_CR
		kobj->mesgCnt = 0;
		kobj->readIndex = 0;
		kobj->writeIndex = 0;
		K_EXIT_CR
	}
	return (K_ERR_OBJ_NULL);
}

K_ERR kMesgQGetMesgCont(K_MESGQ *const kobj, UINT32 *const mesgCntPtr)
{
	K_CR_AREA
	if (kobj)
	{
		K_ENTER_CR
		*mesgCntPtr = kobj->mesgCnt;
		K_EXIT_CR
	}
	return (K_ERR_OBJ_NULL);
}

#endif /*K_DEF_MESGQ*/

#if (K_DEF_PDQ == ON)

/******************************************************************************
 * PUMP-DROP QUEUE (CYLICAL ASYNCHRONOUS BUFFERS)
 ******************************************************************************
 * Producer:
 * reserve - write - pump
 *
 * Consumer:
 * fetch - read - drop
 *
 **/

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

	if (!kobj->init)
		KFAULT(FAULT_OBJ_INIT);
	K_CR_AREA
	K_ENTER_CR
	K_PDBUF *allocPtr = NULL;
	if ((kobj->currBufPtr==NULL))
	{
		allocPtr = BLKALLOC(&kobj->pdbufMemCB);
	}
	else if ((kobj->currBufPtr->nUsers == 0) && (kobj->currBufPtr!=NULL))
	{
		allocPtr = kobj->currBufPtr;
	}

	if (!allocPtr)
	{
		kobj->failReserve++;
	}
	K_EXIT_CR
	return (allocPtr);
}

K_ERR kPDQPump(K_PDQ *kobj, K_PDBUF *buffPtr)
{
	K_CR_AREA
	if (!kobj->init)
		return (K_ERR_OBJ_NULL);
	/* first pump will skip this if */
	K_ENTER_CR
	if ((kobj->currBufPtr != NULL) && (kobj->currBufPtr != buffPtr)
			&& (kobj->currBufPtr->nUsers == 0))
	{ /*deallocate curr buf if not null and not used */
		K_ERR err = BLKFREE(&kobj->pdbufMemCB, kobj->currBufPtr);
		if (err)
		{
			K_EXIT_CR
			return (err);
		}
	}
	/* replace current buffer */
	kobj->currBufPtr = buffPtr;
	K_EXIT_CR
	return (K_SUCCESS);
}

K_PDBUF* kPDQFetch(K_PDQ *const kobj)
{
	K_CR_AREA
	if (!kobj->init)
		return (NULL);
	if (kobj->currBufPtr)
	{
		K_ENTER_CR
		K_PDBUF *ret = kobj->currBufPtr;
		kobj->currBufPtr->nUsers++;
		K_EXIT_CR
		return (ret);
	}
	return (NULL);
}

K_ERR kPDQDrop(K_PDQ *kobj, K_PDBUF *bufPtr)
{
	if (!kobj->init)
		return (K_ERR_OBJ_NULL);
	K_ERR err = 0;
	if (bufPtr)
	{
		K_CR_AREA
		K_ENTER_CR
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
	return (K_ERR_OBJ_NULL);
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
	if (kIsISR())
	{
		KFAULT(FAULT_ISR_INVALID_PRIMITVE);
	}
	K_CR_AREA
	K_ENTER_CR
	K_ERR err = -1;
	kobj->head = 0;
	kobj->tail = 0;
	kobj->data = 0;
	kobj->room = K_DEF_PIPE_SIZE;
	if (err < 0)
		return (err);
	err = EVNTINIT(&(kobj->evData));
	if (err < 0)
		return (err);
	err = EVNTINIT(&(kobj->evRoom));
	if (err < 0)
		return (err);
	kobj->init = TRUE;
	K_EXIT_CR
	return (K_SUCCESS);
}

UINT32 kPipeRead(K_PIPE *const kobj, BYTE *destPtr, UINT32 nBytes)
{

	if (IS_NULL_PTR(kobj) || IS_NULL_PTR(destPtr))
	{
		KFAULT(FAULT_NULL_OBJ);
	}
	if (kobj->init == FALSE)
	{
		KFAULT(FAULT_OBJ_NOT_INIT);
	}
	if (kIsISR())
	{
		KFAULT(FAULT_ISR_INVALID_PRIMITVE);
	}
	K_CR_AREA

	K_ENTER_CR
	UINT32 readBytes = 0;
	if (nBytes == 0)
		return (0);
	if (kobj->tail == kobj->head)
	{
		SLPEVNT(&(kobj->evData), 0); /* wait for data from writers */
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

	if (IS_NULL_PTR(kobj))
		KFAULT(FAULT_NULL_OBJ);
	if (kIsISR())
		KFAULT(FAULT_ISR_INVALID_PRIMITVE);

	K_CR_AREA
	K_ENTER_CR

	UINT32 writeBytes = 0;
	if (nBytes == 0)
		return (0);
	if (nBytes <= 0)
		return (0);
	if (kobj->head != kobj->tail)
	{
		SLPEVNT(&(kobj->evRoom), 0);
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
