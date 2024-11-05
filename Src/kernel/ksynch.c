/******************************************************************************
 *
 * [K0BA - Kernel 0 For Embedded Applications | [VERSION: 1.1.0]]
 *
 ******************************************************************************
 ******************************************************************************
 *  Module              : Inter-task Synchronisation
 *  Depends on          : Scheduler
 *  Provides to         : All services.
 *  Application API     : Yes
 *
 * 	In this unit:
 * 					o Direct Task Pend/Signal
 *					o Generic Events
 *					o Sleep/Wake-Up (on Events)
 *					o Semaphores
 *					o Mutexes with priority inheritance
 *					o Condition Variables
 *
 *****************************************************************************/

#define K_CODE
#include "kconfig.h"
#include "ktypes.h"
#include "kobjs.h"
#include "kapi.h"
#include "kglobals.h"

/******************************************************************************
* TASK YIELD (here for convenience)
*******************************************************************************/
void kYield(void)
{
	K_CR_AREA;
	K_ENTER_CR
	;
	if (runPtr->status == RUNNING)
	{ /* if yielded, not blocked, make it ready*/
		kReadyQEnq(runPtr);
		K_EXIT_CR;
		return;
	}
	else
	{
		K_PEND_CTXTSWTCH;
		K_EXIT_CR;
	}
}


/*******************************************************************************
* DIRECT TASK PENDING/SIGNAL
*******************************************************************************/
void kPend(void)
{
	K_CR_AREA;
	K_ENTER_CR
	;
	K_ERR ret = kTCBQEnq(&sleepingQueue, runPtr);
	if (!ret)
	{
		runPtr->status = PENDING;
		K_PEND_CTXTSWTCH
		;
		K_EXIT_CR
		;
	}
	else
	{
		assert(0);

	}

}

void kSignal(PID const taskID)
{

	K_CR_AREA;
	K_ENTER_CR;
	PID pid = kGetTaskPID(taskID);
	if (tcbs[pid].status == PENDING)
	{
		K_TCB *tcbGotPtr = &tcbs[pid];
		kTCBQRem(&sleepingQueue, &tcbGotPtr);
		kReadyQEnq(tcbGotPtr);
	}
	K_EXIT_CR;
	return;
}
VOID kSignalFromISR(PID const taskID)
{

	PID pid = kGetTaskPID(taskID);
	if (tcbs[pid].status == PENDING)
	{
		K_TCB *tcbGotPtr = &tcbs[pid];
		kTCBQRem(&sleepingQueue, &tcbGotPtr);
		if (!kTCBQEnq(&readyQueue[tcbGotPtr->priority], tcbGotPtr))
			tcbGotPtr->status = READY;
		assert(!kTCBQEnq(&readyQueue[runPtr->priority], runPtr));
		runPtr->status = READY;
	}

}


/******************************************************************************
* SLEEP/WAKE ON EVENTS
*******************************************************************************/

#if (K_DEF_SLEEPWAKE==ON)
static K_ERR kEventInit_(K_EVENT *const self)
{

	if (IS_NULL_PTR(self))
	{
		kErrHandler(FAULT_NULL_OBJ);
		return K_ERROR;
	}
	self->eventID = (UINT32) self;
	assert(!kTCBQInit(&(self->queue), "eventQ"));
	self->init = TRUE;
	return K_SUCCESS;
}

K_ERR kSleep(K_EVENT *self)
{
	K_CR_AREA;
	K_ENTER_CR
	;

	if (IS_NULL_PTR(self))
	{
		kErrHandler(FAULT_NULL_OBJ);
		K_EXIT_CR
		;
		return K_ERROR;
	}
	if (self->init == FALSE)
	{
		kEventInit_(self);
	}
	if (self->init == TRUE)
	{
		if (runPtr->status == RUNNING)
		{

			if (!kTCBQEnq(&(self->queue), runPtr))
			{
				runPtr->status = SLEEPING;
				runPtr->pendingObj = (K_EVENT*) self;
				K_PEND_CTXTSWTCH;
				K_EXIT_CR;
				return K_SUCCESS;
			}
		}
	}
	K_EXIT_CR;
	return K_ERROR;
}

K_ERR kWake(K_EVENT *self)
{
	if (IS_NULL_PTR(self))
	{
		kErrHandler(FAULT_NULL_OBJ);
		return K_ERR_NULL_OBJ;
	}
	K_CR_AREA;
	K_ENTER_CR;
	SIZE sleepThreads = self->queue.size;
	if (sleepThreads > 0)
	{
		for (SIZE i = 0; i < sleepThreads; ++i)
		{
			K_TCB *nextTCBPtr;
			kTCBQDeq(&self->queue, &nextTCBPtr);
			kReadyQEnq(nextTCBPtr);
		}
		K_EXIT_CR;
		return K_SUCCESS;
	}
	return K_ERROR;
}
#endif
/******************************************************************************
* SEMAPHORES
******************************************************************************/

K_ERR kSemaInit(K_SEMA *const self, INT32 const value)
{
	K_CR_AREA;
	K_ENTER_CR;
	if (IS_NULL_PTR(self))
	{
		kErrHandler(FAULT_NULL_OBJ);
		K_EXIT_CR;
		return K_ERR_NULL_OBJ;
	}
	self->value = value;
	if (kTCBQInit(&(self->queue), "semaQ") != K_SUCCESS)
	{
		kErrHandler(FAULT_LIST);
		K_EXIT_CR;
		return K_ERROR;
	}
	K_EXIT_CR;
	return K_SUCCESS;
}

K_ERR kSemaWait(K_SEMA *const self)
{
	K_CR_AREA;
	K_ENTER_CR;
	if (IS_NULL_PTR(self))
	{
		kErrHandler(FAULT_NULL_OBJ);
		return K_ERR_NULL_OBJ;
	}
	(self->value) = (self->value) - 1;
	if ((self->value) < 0)
	{
		if (!kTCBQEnq(&(self->queue), runPtr))
		{
			runPtr->status = BLOCKED;
			runPtr->pendingObj = (K_SEMA*) self;
			K_PEND_CTXTSWTCH;
			K_EXIT_CR;
			return K_SUCCESS;
		}
		kErrHandler(FAULT_LIST);
		K_EXIT_CR;
		return K_ERROR;
	}
	K_EXIT_CR;
	return K_SUCCESS;
}

K_ERR kSemaSignal(K_SEMA *const self)
{
	K_CR_AREA;
	K_ENTER_CR;
	if (IS_NULL_PTR(self))
	{
		kErrHandler(FAULT_NULL_OBJ);
		K_EXIT_CR;
		return K_ERROR;
	}
	K_TCB *nextTCBPtr = NULL;
	(self->value) = (self->value) + 1;
	if ((self->value) <= 0)
	{
		kTCBQDeq(&(self->queue), &nextTCBPtr);
		if (IS_NULL_PTR(nextTCBPtr))
		{
			kErrHandler(FAULT_NULL_OBJ);
			K_EXIT_CR;
			return K_ERROR;
		}
		nextTCBPtr->pendingObj = NULL;
		if (!kReadyQEnq(nextTCBPtr))
		{
			K_EXIT_CR;
			return K_SUCCESS;
		}
		K_EXIT_CR;
		kErrHandler(FAULT_READY_QUEUE);
		return K_ERROR;
	}
	K_EXIT_CR;
	return K_SUCCESS;
}
/*******************************************************************************
* MUTEX
*******************************************************************************/
K_ERR kMutexInit(K_MUTEX *const self)
{
	K_CR_AREA;
	K_ENTER_CR;
	if (IS_NULL_PTR(self))
	{
		kErrHandler(FAULT_NULL_OBJ);
		K_EXIT_CR;
		return K_ERROR;
	}
	self->lock = FALSE;
	if (kTCBQInit(&(self->queue), "mutexQ") != K_SUCCESS)
	{
		K_EXIT_CR;
		kErrHandler(FAULT_LIST);
		return K_ERROR;
	}
	self->init = TRUE;
	K_EXIT_CR;
	return K_SUCCESS;
}
K_ERR kMutexLock(K_MUTEX *const self)
{
	K_CR_AREA;
	K_ENTER_CR;
	if (self->init == FALSE)
	{
		assert(0);
	}
	if (IS_NULL_PTR(self))
	{
		kErrHandler(FAULT_NULL_OBJ);
		K_EXIT_CR;
		return K_ERR_NULL_OBJ;
	}
	if (self->lock == FALSE)
	{
		/* lock mutex and set the owner */
		self->lock = TRUE;
		self->ownerPtr = runPtr;
		K_EXIT_CR;
		return K_SUCCESS;
	}

	if ((self->ownerPtr != runPtr) && (self->ownerPtr != NULL))
	{
		if (self->ownerPtr->priority > runPtr->priority)
		{

			/* mutex owner has lower priority than the tried-to-lock-task
			 * thus, we boost owner priority, to avoid an intermediate
			 * priority task that does not need lock to preempt
			 * this task, causing an unbounded delay */

			self->ownerPtr->priority = runPtr->priority;

		}
		runPtr->status = BLOCKED;
		runPtr->pendingObj = (K_MUTEX*) self;
		kTCBQEnq(&(self->queue), runPtr);
		K_PEND_CTXTSWTCH;
		K_EXIT_CR;
	}
	K_EXIT_CR;
	return K_SUCCESS;
}

K_ERR kMutexUnlock(K_MUTEX *const self)
{
	K_CR_AREA;
	K_ENTER_CR;
	K_TCB *tcbPtr;
	if (IS_NULL_PTR(self))
	{
		kErrHandler(FAULT_NULL_OBJ);
		K_EXIT_CR;
		return K_ERR_NULL_OBJ;
	}
	if (self->init == FALSE)
	{
		assert(0);
	}
	if ((self->lock == FALSE))
	{
		K_EXIT_CR;
		return K_ERR_MUTEX_NOT_LOCKED;
	}
	if (self->ownerPtr != runPtr)
	{
		K_EXIT_CR;
		return K_ERR_MUTEX_NOT_OWNER;
	}
	/* runPtr is the owner and mutex was locked */
	if (self->queue.size == 0)  //no waiters
	{
		self->lock = FALSE;
		self->ownerPtr->priority = self->ownerPtr->realPrio;
		self->ownerPtr->pendingObj = NULL;
		tcbPtr = self->ownerPtr;
		self->ownerPtr = NULL;
	}
	else
	{
		/*there are waiters, unblock a waiter set new mutex owner.
		 * mutex is still locked */
		kTCBQDeq(&(self->queue), &tcbPtr);
		if (IS_NULL_PTR(tcbPtr))
			kErrHandler(FAULT_NULL_OBJ);
		if (runPtr->priority < runPtr->realPrio)
		{
			runPtr->priority = runPtr->realPrio;
		}
		if (!kReadyQEnq(tcbPtr))
		{
			self->ownerPtr = tcbPtr;
			tcbPtr->pendingObj = NULL;
		}
		else
		{
			kErrHandler(FAULT_READY_QUEUE);
		}
	}
	K_EXIT_CR;
	return K_SUCCESS;
}
/******************************************************************************
 * CONDITION VARIABLES
 ******************************************************************************/
#if (K_DEF_COND==ON)
K_ERR kCondInit(K_COND *const self)
{
	if (IS_NULL_PTR(self))
	{
		kErrHandler(FAULT_NULL_OBJ);
		return K_ERR_NULL_OBJ;
	}
	assert(!kMutexInit(&self->condMutex));
	assert(!kTCBQInit(&self->queue, "cvQ"));
	return K_SUCCESS;
}

VOID kCondWait(K_COND *const self)
{
	if (IS_NULL_PTR(self))
	{
		kErrHandler(FAULT_NULL_OBJ);
	}
	K_CR_AREA;
	K_ENTER_CR;
	kMutexUnlock(&self->condMutex);
	runPtr->status = SLEEPING;
	kTCBQEnq(&self->queue, runPtr);
	K_PEND_CTXTSWTCH;
	K_EXIT_CR;
	kMutexLock(&self->condMutex);
}

VOID kCondSignal(K_COND *const self)
{
	if (IS_NULL_PTR(self))
		kErrHandler(FAULT_NULL_OBJ);
	K_TCB *nextTCBPtr = NULL;
	kMutexLock(&self->condMutex);
	K_CR_AREA;
	K_ENTER_CR;
	kTCBQDeq(&self->queue, &nextTCBPtr);
	if (IS_NULL_PTR(nextTCBPtr))
		kErrHandler(FAULT_NULL_OBJ);
	kReadyQEnq(nextTCBPtr);
	kMutexUnlock(&self->condMutex);
	K_EXIT_CR;
}

VOID kCondWake(K_COND *const self)
{
	if (IS_NULL_PTR(self))
		kErrHandler(FAULT_NULL_OBJ);
	kMutexLock(&self->condMutex);
	SIZE nThreads = self->queue.size;
	for (SIZE i = 0; i < nThreads; i++)
	{
		kCondSignal(self);
	}
	kMutexUnlock(&self->condMutex);
}
#endif

