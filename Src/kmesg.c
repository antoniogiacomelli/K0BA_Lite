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

#if (K_DEF_MBOX_CAPACITY==SINGLE)

K_ERR kMboxInit(K_MBOX *const kobj, ADDR initMailPtr)
{
	K_CR_AREA

	if (kobj == NULL)
	{
		KFAULT(FAULT_NULL_OBJ);
		return (K_ERROR);
	}
	K_ENTER_CR
	kobj->mailPtr = initMailPtr;
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

K_ERR kMboxPost(K_MBOX *const kobj, ADDR const sendPtr, TICK timeout)
{
	K_CR_AREA
	K_ENTER_CR

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
	/* a reader is yet to read */
	if (kobj->mailPtr != NULL)
	{
		if (timeout == 0)
		{
			K_EXIT_CR
			return (K_ERR_MBOX_FULL);
		}
		if ((timeout > 0) && (timeout < 0xFFFFFFFF))
		{
			kTimeOut(&kobj->timeoutNode, timeout);
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

	kobj->mailPtr = sendPtr;

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

K_ERR kMboxPend(K_MBOX *const kobj, ADDR *recvPPtr, TICK timeout)
{
	K_CR_AREA
	K_ENTER_CR

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
	if (kobj->mailPtr == NULL)
	{
		if (timeout == 0)
		{
			K_EXIT_CR
			return (K_ERR_MBOX_EMPTY);
		}
		if ((timeout > 0) && (timeout < 0xFFFFFFFF))
		{
			kTimeOut(&kobj->timeoutNode, timeout);
		}

		do
		{

#if(K_DEF_MBOX_ENQ==K_DEF_ENQ_FIFO)
			kTCBQEnq(&kobj->waitingQueue, runPtr);
#else
			kTCBQEnqByPrio(&kobj->waitingQueue, runPtr);
#endif
			runPtr->status = RECEIVING;
			runPtr->pendingMbox = kobj;
			K_PEND_CTXTSWTCH
			K_EXIT_CR
			K_ENTER_CR
			if (runPtr->timeOut)
			{ /* timed-out */
				runPtr->timeOut = FALSE;
				runPtr->pendingMbox = 0;
				K_EXIT_CR
				return (K_ERR_TIMEOUT);
			}
		} while (kobj->mailPtr == NULL);
	}

	*recvPPtr = kobj->mailPtr;
	kobj->mailPtr = NULL;
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

BOOL kMboxIsFull(K_MBOX *const kobj)
{

	return ((kobj->mailPtr == NULL) ? FALSE : TRUE);
}

#if (K_DEF_MBOX_SENDRECV==ON)
/* sender does: sendrecv(&mbox, &send, &recv, t);  */
/* receiver does:
 * recv(&mbox, &recv, NULL, t);
 * -assemble answer-
 * send(&mbox, &answer, t);
 *
 ***/
K_ERR kMboxPostPend(K_MBOX *const kobj, ADDR const sendPtr,
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
		if (timeout == 0)
		{
			K_EXIT_CR
			return (K_ERR_MBOX_FULL);
		}
		if ((timeout > 0) && (timeout < 0xFFFFFFFF))
		{
			kTimeOut(&kobj->timeoutNode, timeout);
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

	kobj->mailPtr = sendPtr;

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
#endif /* sendrecv */

#elif (K_DEF_MBOX_CAPACITY==(MULTI))

K_ERR kMboxInit(K_MBOX *const kobj, ADDR memPtr, SIZE maxItems)
{
	K_CR_AREA

	if (kobj == NULL || memPtr == NULL || maxItems == 0)
	{
		KFAULT(FAULT_NULL_OBJ);
		return (K_ERROR);
	}

	K_ENTER_CR

	kobj->mailQPtr = memPtr;
	kobj->headIdx = 0;
	kobj->tailIdx = 0;
	kobj->maxItems = maxItems;
	kobj->countItems = 0;
	kobj->init = TRUE;

	K_ERR listerr = kListInit(&kobj->waitingQueue, "qq");
	assert(listerr == 0);

	kobj->timeoutNode.nextPtr = NULL;
	kobj->timeoutNode.timeout = 0;
	kobj->timeoutNode.kobj = kobj;
	kobj->timeoutNode.objectType = MAILBOX;

	K_EXIT_CR
	return (K_SUCCESS);
}

K_ERR kMboxPost(K_MBOX *const kobj, ADDR sendPtr, TICK timeout)
{
	K_CR_AREA

	if (kobj == NULL || sendPtr == NULL)
	{
		KFAULT(FAULT_NULL_OBJ);
		return (K_ERROR);
	}
	if (!kobj->init)
	{
		KFAULT(FAULT_OBJ_NOT_INIT);
		return (K_ERROR);
	}
	K_ENTER_CR

	if (kobj->countItems == kobj->maxItems)
	{
		if (timeout == 0)
		{
			K_EXIT_CR
			return (K_ERR_MBOX_FULL);
		}

		do
		{
			kTCBQEnq(&kobj->waitingQueue, runPtr);
			runPtr->status = SENDING;
			kTimeOut(&kobj->timeoutNode, timeout);
			K_PEND_CTXTSWTCH
			K_EXIT_CR
			K_ENTER_CR

			if (runPtr->timeOut)
			{
				runPtr->timeOut = FALSE;
				K_EXIT_CR
				return (K_ERR_TIMEOUT);
			}
		} while (kobj->countItems == kobj->maxItems);
	}

	/* post on tail */

	ADDR *tailAddr = (ADDR*) ((UINT*) kobj->mailQPtr + kobj->tailIdx);
	*tailAddr = sendPtr;

	kobj->tailIdx = (kobj->tailIdx + 1) % kobj->maxItems;

	kobj->countItems++;

	/* unblock a receiver if any */
	if (kobj->waitingQueue.size > 0)
	{
		K_TCB *freeReadPtr = kTCBQPeek(&kobj->waitingQueue);
		if (freeReadPtr->status == RECEIVING)
		{
			kTCBQDeq(&kobj->waitingQueue, &freeReadPtr);
			kTCBQEnq(&readyQueue[freeReadPtr->priority], freeReadPtr);
			freeReadPtr->status = READY;
			if (freeReadPtr->priority < runPtr->priority)
			{
				K_PEND_CTXTSWTCH
			}
		}
	}

	K_EXIT_CR
	return (K_SUCCESS);
}

K_ERR kMboxPend(K_MBOX *const kobj, ADDR *recvPPtr, TICK timeout)
{
	K_CR_AREA
	K_ENTER_CR

	if (kobj == NULL || recvPPtr == NULL)
	{
		KFAULT(FAULT_NULL_OBJ);
		return (K_ERROR);
	}
	if (!kobj->init)
	{
		KFAULT(FAULT_OBJ_NOT_INIT);
		return (K_ERROR);
	}

	if (kobj->countItems == 0)
	{
		if (timeout == 0)
		{
			K_EXIT_CR
			return (K_ERR_MBOX_EMPTY);
		}

		do
		{
			kTCBQEnqByPrio(&kobj->waitingQueue, runPtr);
			runPtr->status = RECEIVING;
			kTimeOut(&kobj->timeoutNode, timeout);
			K_PEND_CTXTSWTCH
			K_EXIT_CR
			K_ENTER_CR

			if (runPtr->timeOut)
			{
				runPtr->timeOut = FALSE;
				K_EXIT_CR
				return (K_ERR_TIMEOUT);
			}
		} while (kobj->countItems == 0);
	}

	// Calculate the address of the head and retrieve the mail
	ADDR *headAddr = (ADDR*) ((UINT*) kobj->mailQPtr + kobj->headIdx);
	*recvPPtr = *headAddr;

	// Advance the head offset
	kobj->headIdx = (kobj->headIdx + 1) % kobj->maxItems;

	kobj->countItems--;

	if (kobj->waitingQueue.size > 0)
	{
		K_TCB *freeSendPtr = kTCBQPeek(&kobj->waitingQueue);
		if (freeSendPtr->status == SENDING)
		{
			kTCBQDeq(&kobj->waitingQueue, &freeSendPtr);
			kTCBQEnqByPrio(&readyQueue[freeSendPtr->priority], freeSendPtr);
			freeSendPtr->status = READY;
			if (freeSendPtr->priority < runPtr->priority)
			{
				K_PEND_CTXTSWTCH
			}
		}
	}

	K_EXIT_CR
	return (K_SUCCESS);
}

K_ERR kMboxPeek(K_MBOX *const kobj, ADDR *peekPPtr)
{
	K_CR_AREA
	K_ENTER_CR

	if (kobj == NULL || peekPPtr == NULL)
	{
		KFAULT(FAULT_NULL_OBJ);
		K_EXIT_CR
		return (K_ERROR);
	}

	if (!kobj->init)
	{
		KFAULT(FAULT_OBJ_NOT_INIT);
		K_EXIT_CR
		return (K_ERROR);
	}

	ADDR *headAddr = (ADDR*) ((UINT*) kobj->mailQPtr + kobj->headIdx);

	*peekPPtr = *headAddr;

	K_EXIT_CR

	return (K_SUCCESS);
}

SIZE kMboxMailCount(K_MBOX *const kobj)
{
	return (kobj->countItems);
}

BOOL kMboxIsFull(K_MBOX *const kobj)
{
	return (kobj->countItems == kobj->maxItems);
}

#endif /* mailbox type */
#endif /* mailbox */

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
	K_ERR err = kListInit(&kobj->waitingQueue, "waitingQueue");
	if (err != 0)
	{
		K_EXIT_CR
		return (K_ERROR);
	}
	kobj->timeoutNode.nextPtr = NULL;
	kobj->timeoutNode.timeout = 0;
	kobj->timeoutNode.kobj = kobj;
	kobj->timeoutNode.objectType = MESGQUEUE;
	kobj->init = 1;
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
		if (timeout == 0)
		{
			K_EXIT_CR
			return (K_ERR_MESGQ_FULL);
		}

		if ((timeout > 0) && (timeout < 0xFFFFFFFF))
			kTimeOut(&kobj->timeoutNode, timeout);
		{
#if(K_DEF_MESGQ_ENQ==K_DEF_ENQ_FIFO)
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
		}
		while (kobj->mesgCnt >= kobj->maxMesg)
			;
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
		if (timeout == 0)
		{
			K_EXIT_CR
			return (K_ERR_MESGQ_EMPTY);
		}

		if ((timeout > 0) && (timeout < 0xFFFFFFFF))
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
		} while (kobj->mesgCnt == 0)
			;
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
		if (timeout == 0)
		{
			K_EXIT_CR
			return (K_ERR_MESGQ_FULL);
		}

		if (timeout < 0xFFFFFFFF)
			kTimeOut(&kobj->timeoutNode, timeout);
		do
		{
#if(K_DEF_MESGQ_ENQ==K_DEF_ENQ_FIFO)
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

K_ERR kMesgQGetMesgCount(K_MESGQ *const kobj, UINT32 *const mesgCntPtr)
{
	K_CR_AREA
	if (kobj)
	{
		K_ENTER_CR
		*mesgCntPtr = kobj->mesgCnt;
		K_EXIT_CR
		return (K_SUCCESS);
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

K_ERR kPDQInit(K_PDQ *const kobj, K_MEM *const memCtrlPtr, K_PDBUF *bufPool,
		BYTE nBufs)
{
	K_CR_AREA
	K_ENTER_CR
	K_ERR err = K_ERROR;
	kobj->memCtrlPtr = memCtrlPtr;
	err = BLKPOOLINIT(kobj->memCtrlPtr, bufPool, sizeof(K_PDBUF), nBufs);
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
	if ((kobj->currBufPtr == NULL))
	{
		allocPtr = BLKALLOC(kobj->memCtrlPtr);
	}
	else if ((kobj->currBufPtr->nUsers == 0) && (kobj->currBufPtr != NULL))
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

K_ERR kPDQPump(K_PDQ *const kobj, K_PDBUF *const buffPtr)
{
	K_CR_AREA
	if (!kobj->init)
		return (K_ERR_OBJ_NULL);
	/* first pump will skip this if */
	K_ENTER_CR
	if ((kobj->currBufPtr != NULL) && (kobj->currBufPtr != buffPtr)
			&& (kobj->currBufPtr->nUsers == 0))
	{ /*deallocate curr buf if not null and not used */
		K_ERR err = BLKFREE(kobj->memCtrlPtr, kobj->currBufPtr);
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

K_ERR kPDQDrop(K_PDQ *const kobj, K_PDBUF *const bufPtr)
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
			err = BLKFREE(kobj->memCtrlPtr, bufPtr);
		}
		K_EXIT_CR
		return (err);
	}
	return (K_ERR_OBJ_NULL);
}

#endif
