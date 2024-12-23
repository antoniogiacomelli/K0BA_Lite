/******************************************************************************
 *
 * [K0BA - Kernel 0 For Embedded Applications | [VERSION: 0.3.0]]
 *
 ******************************************************************************
 ******************************************************************************
 *  Module              : Inter-task Synchronisation
 *  Depends on          : Scheduler
 *  Provides to         : Message Passing, Appli
 *  Public API  	    : Yes
 *
 * 	In this unit:
 * 					o Pend/Suspend/Signal/Resume
 *					o Sleep/Wake-Up (on Events)
 *					o Semaphores
 *					o Mutexes
 *
 *
 *****************************************************************************/

#define K_CODE
#include "kconfig.h"
#include "ktypes.h"
#include "kobjs.h"
#include "klist.h"
#include "kmem.h"
#include "kitc.h"
#include "ksch.h"
#include "ktimer.h"
#include "kinternals.h"

/*******************************************************************************
 * DIRECT TASK PENDING/SIGNAL
 *******************************************************************************/
K_ERR kPend()
{

	if (kIsISR())
		kErrHandler(FAULT_ISR_INVALID_PRIMITVE);
	K_CR_AREA

	K_ENTER_CR

	K_ERR err = -1;

	err=kTCBQEnq(&sleepingQueue, runPtr);

	runPtr->status=PENDING;

	K_PEND_CTXTSWTCH;

	K_EXIT_CR

	return (err);
}

K_ERR kSignal(TID const taskID)
{

	K_ERR err = -1;
	K_CR_AREA
	K_ENTER_CR
	PID pid = kGetTaskPID(taskID);
	if (tcbs[pid].status == PENDING || tcbs[pid].status == SUSPENDED)
	{
		K_TCB *tcbGotPtr = &tcbs[pid];
		err = kTCBQRem(&sleepingQueue, &tcbGotPtr);
		assert(!err);
		err = kReadyCtxtSwtch(tcbGotPtr);
		assert(!err);
	}
	else
	{
		tcbs[pid].lostSignals += 1;
	}
	K_EXIT_CR
	return (err);
}

K_ERR kSuspend(TID const taskID)
{
	K_CR_AREA
	K_ENTER_CR
	K_ERR err = -1;
	PID pid = kGetTaskPID(taskID);
	if (runPtr->priority > tcbs[pid].priority)
		return (K_ERR_CANT_SUSPEND);
	if (tcbs[pid].status != READY)
		return (K_ERR_CANT_SUSPEND);

	err=kTCBQEnq(&sleepingQueue, runPtr);
	runPtr->status=SLEEPING;
	assert(err==0);
	K_EXIT_CR
	return (err);
}

#if (K_DEF_SLEEPWAKE==ON)
/******************************************************************************
 * SLEEP/WAKE ON EVENTS
 ******************************************************************************/
K_ERR kEventInit(K_EVENT* const kobj)
{

    if (kobj == NULL)
    {
        kErrHandler(FAULT_NULL_OBJ);
    }
    K_CR_AREA
	K_ENTER_CR
    kobj->eventID = (UINT32) kobj;
    assert( !kTCBQInit( & (kobj->waitingQueue), "eventQ"));
    kobj->init = TRUE;
	kobj->timeoutNode.nextPtr = NULL;
	kobj->timeoutNode.timeout = 0;
	kobj->timeoutNode.kobj = kobj;
	kobj->timeoutNode.objectType = EVENT;
    K_EXIT_CR
	return (K_SUCCESS);
}
VOID kEventSleep(K_EVENT *kobj, TICK timeout)
{


	if (kIsISR())
	{
		kErrHandler(FAULT_ISR_INVALID_PRIMITVE);
	}
	if (kobj == NULL)
	{
		kErrHandler(FAULT_NULL_OBJ);
	}
	if (kobj->init==FALSE)
	{
		kErrHandler(FAULT_OBJ_NOT_INIT);
	}

	K_CR_AREA
	K_ENTER_CR

	if (kobj->init == TRUE)
	{
		if (timeout > 0)
			kTimeOut(&kobj->timeoutNode, timeout);

		kTCBQEnq(&kobj->waitingQueue, runPtr);
		runPtr->status=SLEEPING;
		runPtr->pendingEv = kobj;
		K_PEND_CTXTSWTCH
		K_EXIT_CR
		return;
	}
	K_EXIT_CR
}

VOID kEventWake(K_EVENT *kobj)
{
	if (kobj == NULL)
	{
		kErrHandler(FAULT_NULL_OBJ);
	}

	if (kobj->waitingQueue.size == 0)
		return;
	K_CR_AREA
	K_ENTER_CR
	if (kobj->init==FALSE)
	{
		kErrHandler(FAULT_OBJ_NOT_INIT);
	}
	SIZE sleepThreads = kobj->waitingQueue.size;
	if (sleepThreads > 0)
	{
		for (SIZE i = 0; i < sleepThreads; ++i)
		{
			K_TCB *nextTCBPtr;
			kTCBQDeq(&kobj->waitingQueue, &nextTCBPtr);
			assert(!kReadyCtxtSwtch(nextTCBPtr));
			nextTCBPtr->pendingEv = NULL;
		}
	}
	K_EXIT_CR
	return;
}

UINT32 kEventQuery(K_EVENT *const kobj)
{
	if (kobj == NULL)
	{
		kErrHandler(FAULT_NULL_OBJ);
	}
	return (kobj->waitingQueue.size);
}

#endif

#if (K_DEF_SEMA == ON)
/******************************************************************************
 * SEMAPHORES
 ******************************************************************************/
K_ERR kSemaInit(K_SEMA *const kobj, INT32 const value)
{
	K_CR_AREA
	K_ENTER_CR
	if (kobj == NULL)
	{
		kErrHandler(FAULT_NULL_OBJ);
		K_EXIT_CR
		return (K_ERR_OBJ_NULL);
	}
	if (value < 0)
		kErrHandler(FAULT);
	kobj->value = value;
	if (kTCBQInit(&(kobj->waitingQueue), "semaQ") != K_SUCCESS)
	{
		kErrHandler(FAULT_LIST);
		K_EXIT_CR
		return (K_ERROR);
	}
	kobj->init = TRUE;
	kobj->ownerPtr = NULL;
	kobj->timeoutNode.nextPtr = NULL;
	kobj->timeoutNode.timeout = 0;
	kobj->timeoutNode.kobj = kobj;
	kobj->timeoutNode.objectType = SEMAPHORE;
	K_EXIT_CR
	return (K_SUCCESS);
}

K_ERR kSemaWait(K_SEMA *const kobj, TICK const timeout)
{
	if (kIsISR())
	{
		kErrHandler(FAULT_ISR_INVALID_PRIMITVE);
	}
	if (kobj->init == FALSE)
	{
		kErrHandler(FAULT_OBJ_NOT_INIT);
	}

	if (kobj == NULL)
	{
		kErrHandler(FAULT_NULL_OBJ);
	}

	K_CR_AREA
	K_ENTER_CR
	kobj->value--;
	DMB

	if (kobj->value < 0)
	{
#if(K_DEF_SEMA_ENQ==K_DEF_ENQ_FIFO)
		kTCBQEnq(&kobj->waitingQueue, runPtr);
#else
		kTCBQEnqByPrio(&kobj->waitingQueue, runPtr);
#endif
		runPtr->status = BLOCKED;
		runPtr->pendingSema = kobj;
		DMB

		kTimeOut(&kobj->timeoutNode, timeout);
		if (kobj->ownerPtr)
		{
			if (runPtr->priority < kobj->ownerPtr->priority)
			{
				kobj->ownerPtr->priority = runPtr->priority;
			}
		}
		K_PEND_CTXTSWTCH
		K_EXIT_CR
		K_ENTER_CR
		if (runPtr->timeOut)
		{
			runPtr->timeOut = FALSE;
			kobj->value += 1;
			K_EXIT_CR
			return (K_ERR_TIMEOUT);
		}
	}
	if (kobj->ownerPtr)
	{
		/* restore owner priority */
		kobj->ownerPtr->priority = kobj->ownerPtr->realPrio;
	}
	/*the task reaching this point is the sema owner*/
	kobj->ownerPtr = runPtr;
	K_EXIT_CR
	return (K_SUCCESS);
}

VOID kSemaSignal(K_SEMA *const kobj)
{
	K_CR_AREA
	K_ENTER_CR
	if (kobj == NULL)
	{
		kErrHandler(FAULT_NULL_OBJ);
		K_EXIT_CR
	}
	if (kobj->init == FALSE)
	{
		kErrHandler(FAULT_OBJ_NOT_INIT);
	}
	K_TCB *nextTCBPtr = NULL;
	(kobj->value) = (kobj->value) + 1;
	DMB
	if ((kobj->value) <= 0)
	{
		/* if here, the semaphore has sleeping tasks            */
		K_ERR err = kTCBQDeq(&(kobj->waitingQueue), &nextTCBPtr);
		assert(err==0);
		assert(nextTCBPtr != NULL);
		nextTCBPtr->pendingSema = NULL;
		/* a task will resume on the waiting queue, gaining access*/
		err = kReadyCtxtSwtch(nextTCBPtr);
		goto EXIT;
	}
	/* if there are no waiters */
	kobj->ownerPtr = NULL; /* clear owner */
	EXIT:
	K_EXIT_CR
	return;
}

INT32 kSemaQuery(K_SEMA *const kobj)
{
	if (kobj->init == FALSE)
	{
		kErrHandler(FAULT_OBJ_NOT_INIT);
	}

	if (kobj == NULL)
	{
		kErrHandler(FAULT_NULL_OBJ);
	}
	return (kobj->value);
}

#endif /*sema*/

#if (K_DEF_MUTEX == ON)
/*******************************************************************************
 * MUTEX
 *******************************************************************************/
K_ERR kMutexInit(K_MUTEX *const kobj)
{

	if (kobj == NULL)
	{
		kErrHandler(FAULT_NULL_OBJ);
		return (K_ERROR);
	}
	kobj->lock = FALSE;
	if (kTCBQInit(&(kobj->waitingQueue), "mutexQ") != K_SUCCESS)
	{
		kErrHandler(FAULT_LIST);
		return (K_ERROR);
	}
	kobj->init = TRUE;
	kobj->timeoutNode.nextPtr = NULL;
	kobj->timeoutNode.timeout = 0;
	kobj->timeoutNode.kobj = kobj;
	kobj->timeoutNode.objectType = MUTEX;
	return (K_SUCCESS);
}

K_ERR kMutexLock(K_MUTEX *const kobj, TICK timeout)
{
	K_CR_AREA
	K_ENTER_CR
	if (kobj->init == FALSE)
	{
		assert(0);
	}
	if (kobj == NULL)
	{
		kErrHandler(FAULT_NULL_OBJ);
	}
	if (kIsISR())
	{
		kErrHandler(FAULT_ISR_INVALID_PRIMITVE);
	}
	if (kobj->lock == FALSE)
	{
		/* lock mutex and set the owner */
		kobj->lock = TRUE;
		kobj->ownerPtr = runPtr;
		K_EXIT_CR
	}
	if ((kobj->ownerPtr != runPtr) && (kobj->ownerPtr != NULL))
	{
		if (kobj->ownerPtr->priority > runPtr->priority)
		{
			/* mutex owner has lower priority than the tried-to-lock-task
			 * thus, we boost owner priority, to avoid an intermediate
			 * priority task that does not need lock to preempt
			 * this task, causing an unbounded delay */

			kobj->ownerPtr->priority = runPtr->priority;
		}
#if(K_DEF_MUTEX_ENQ==K_DEF_ENQ_FIFO)
		kTCBQEnq(&kobj->waitingQueue, runPtr);
#else
		kTCBQEnqByPrio(&kobj->waitingQueue, runPtr);
#endif
		if (timeout > 0)
			kTimeOut(&kobj->timeoutNode, timeout);
		runPtr->status = BLOCKED;
		runPtr->pendingMutx = (K_MUTEX*) kobj;
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
	else
	{
		if (kobj->ownerPtr == runPtr)
		{ /* recursive lock ? why ? WHYYYYYYYYYYY*/
			K_EXIT_CR
			return (K_ERR_MUTEX_REC_LOCK);
		}
	}

	K_EXIT_CR
	return (K_SUCCESS);
}

VOID kMutexUnlock(K_MUTEX *const kobj)
{
	K_CR_AREA
	K_ENTER_CR
	K_TCB *tcbPtr;
	if (kobj == NULL)
	{
		kErrHandler(FAULT_NULL_OBJ);
	}
	if (kobj->init == FALSE)
	{
		assert(0);
	}
	if ((kobj->lock == FALSE))
	{
		return;
	}
	if (kobj->ownerPtr != runPtr)
	{
		assert(FAULT_UNLOCK_OWNED_MUTEX);
		K_EXIT_CR
		return;
	}
	/* runPtr is the owner and mutex was locked */
	if (kobj->waitingQueue.size == 0)
	{
		kobj->lock = FALSE;
		kobj->ownerPtr->priority = kobj->ownerPtr->realPrio;
		kobj->ownerPtr->pendingMutx = NULL;
		tcbPtr = kobj->ownerPtr;
		kobj->ownerPtr = NULL;
	}
	else
	{
		/*there are waiters, unblock a waiter set new mutex owner.
		 * mutex is still locked */
		kTCBQDeq(&(kobj->waitingQueue), &tcbPtr);
		if (IS_NULL_PTR(tcbPtr))
			kErrHandler(FAULT_NULL_OBJ);
		/* here only runptr can unlock a mutex*/
		if (runPtr->priority < runPtr->realPrio)
		{
			runPtr->priority = runPtr->realPrio;
		}
		if (!kReadyCtxtSwtch(tcbPtr))
		{
			kobj->ownerPtr = tcbPtr;
			tcbPtr->pendingMutx = NULL;
			K_EXIT_CR
			return;
		}
		else
		{
			kErrHandler(FAULT_READY_QUEUE);
		}
	}
	K_EXIT_CR
	return;
}

K_ERR kMutexQuery(K_MUTEX *const kobj)
{
	K_CR_AREA
	K_ENTER_CR
	K_ERR err = -1;
	if (kobj == NULL)
	{
		err = (K_ERR_OBJ_NULL);
		goto EXIT;
	}
	if (kobj->lock == TRUE)
	{
		err = (K_QUERY_MUTEX_LOCKED);
		goto EXIT;
	}
	if (kobj->lock == FALSE)
	{
		err = (K_QUERY_MUTEX_UNLOCKED);
		goto EXIT;
	}
	if (kobj->init == FALSE)
	{
		err = (K_ERR_OBJ_NOT_INIT);
		goto EXIT;
	}
	if (err == -1)
	{
		err = (K_ERR_QUERY_UNDEFINED);
	}
	EXIT:
	K_EXIT_CR
	return (err);
}
#endif /* mutex */

