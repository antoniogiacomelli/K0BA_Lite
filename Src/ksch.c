/******************************************************************************
 *
 *     [[K0BA - Kernel 0 For Embedded Applications] | [VERSION: 0.3.1]]
 *
 ******************************************************************************
 ******************************************************************************
 * 	Module           :  High-Level Scheduler / Kernel Initialisation
 * 	Provides to      :  All services
 *  Depends  on      :  Low-Level Scheduler / Kernel Initialisation
 *  Public API 		 :  Yes
 *
 * 	In this unit:
 * 					o Scheduler routines
 * 					o Tick Management
 * 		            o Task Queues Management
 *			 		o Task Control Block Management
 *
 ******************************************************************************/

#define K_CODE

#include "kversion.h"
#include "kitc.h"
#include "ksystasks.h"
#include "ktimer.h"
#include "kinternals.h"
#include "ksch.h"

/*****************************************************************************/

/* scheduler globals */
K_TCBQ readyQueue[K_DEF_MIN_PRIO + 2];
K_TCBQ sleepingQueue;
K_TCB *runPtr;
K_TCB tcbs[NTHREADS];
struct kRunTime runTime;
/* local globals  */
static PRIO highestPrio = 0;
static PRIO const lowestPrio = K_DEF_MIN_PRIO;
static PRIO nextTaskPrio = 0;
static PRIO idleTaskPrio = K_DEF_MIN_PRIO + 1;
static volatile UINT32 readyQBitMask;
static volatile UINT32 readyQRightMask;
static volatile UINT32 version;

/* fwded private helpers */
static inline VOID kReadyRunningTask_(VOID);
static inline PRIO kCalcNextTaskPrio_();
#if (K_DEF_SCH_TSLICE == ON)
static inline BOOL kDecTimeSlice_(void);
#endif
/*******************************************************************************
 * YIELD
 *******************************************************************************/

VOID kYield(VOID) /*  <- USE THIS =) */
{
	ISB
	asm volatile("svc #0xC5");
	DSB
}
/*******************************************************************************
 TASK QUEUE MANAGEMENT
 *******************************************************************************/

K_ERR kTCBQInit(K_TCBQ *const kobj, STRING listName)
{
	if (IS_NULL_PTR(kobj))
	{
		kErrHandler(FAULT_NULL_OBJ);
		return (K_ERR_OBJ_NULL);
	}
	return (kListInit(kobj, listName));
}

K_ERR kTCBQEnq(K_TCBQ *const kobj, K_TCB *const tcbPtr)
{
	K_CR_AREA
	K_ENTER_CR
	if (kobj == NULL || tcbPtr == NULL)
	{
		kErrHandler(FAULT_NULL_OBJ);
	}
	K_ERR err = kListAddTail(kobj, &(tcbPtr->tcbNode));
	if (err == 0)
	{
		if (kobj == &readyQueue[tcbPtr->priority])
			readyQBitMask |= 1 << tcbPtr->priority;
	}
	K_EXIT_CR
	return (err);
}

K_ERR kTCBQDeq(K_TCBQ *const kobj, K_TCB **const tcbPPtr)
{
	if (kobj == NULL)
	{
		kErrHandler(FAULT_NULL_OBJ);
	}
	K_LISTNODE *dequeuedNodePtr = NULL;
	K_ERR err = kListRemoveHead(kobj, &dequeuedNodePtr);

	if (err != K_SUCCESS)
	{
		kErrHandler(FAULT_LIST);
	}
	*tcbPPtr = K_LIST_GET_TCB_NODE(dequeuedNodePtr, K_TCB);

	if (*tcbPPtr == NULL)
	{
		kErrHandler(FAULT_NULL_OBJ);
		return (K_ERR_OBJ_NULL);
	}
	K_TCB *tcbPtr_ = *tcbPPtr;
	PRIO prio_ = tcbPtr_->priority;
	if ((kobj == &readyQueue[prio_]) && (kobj->size == 0))
		readyQBitMask &= ~(1U << prio_);
	return (K_SUCCESS);
}

K_ERR kTCBQRem(K_TCBQ *const kobj, K_TCB **const tcbPPtr)
{
	if (kobj == NULL || tcbPPtr == NULL)
	{
		kErrHandler(FAULT_NULL_OBJ);
	}
	K_LISTNODE *dequeuedNodePtr = &((*tcbPPtr)->tcbNode);
	K_ERR err = kListRemove(kobj, dequeuedNodePtr);
	if (err != K_SUCCESS)
	{
		kErrHandler(FAULT_LIST);
		return (err);
	}
	*tcbPPtr = K_LIST_GET_TCB_NODE(dequeuedNodePtr, K_TCB);
	if (*tcbPPtr == NULL)
	{
		kErrHandler(FAULT_NULL_OBJ);
	}
	return (K_SUCCESS);
}

K_TCB* kTCBQPeek(K_TCBQ *const kobj)
{
	if (kobj == NULL)
	{
		kErrHandler(FAULT_NULL_OBJ);
	}
	K_LISTNODE *nodePtr = kobj->listDummy.nextPtr;
	return K_GET_CONTAINER_ADDR(nodePtr, K_TCB, tcbNode);
}

K_TCB* kTCBQSearchPID(K_TCBQ *const kobj, TID uPid)
{
	if (kobj == NULL || kobj->listDummy.nextPtr == &(kobj->listDummy))
	{
		kErrHandler(FAULT_NULL_OBJ);
	}
	else
	{
		K_LISTNODE *currNodePtr = kobj->listDummy.nextPtr;
		while (currNodePtr != &(kobj->listDummy))
		{
			K_TCB *currTcbPtr = K_LIST_GET_TCB_NODE(currNodePtr, K_TCB);
			if (currTcbPtr->uPid == uPid)
			{
				return (currTcbPtr);
			}
			currNodePtr = currNodePtr->nextPtr;
		}
	}
	return (NULL);
}
K_ERR kTCBQEnqByPrio(K_TCBQ *const kobj, K_TCB *const tcbPtr)
{
	K_ERR err;
	if (kobj == NULL || tcbPtr == NULL)
	{
		kErrHandler(FAULT_NULL_OBJ);
	}

	if (kobj->size == 0)
	{
		/* enq on tail */
		err = kTCBQEnq(kobj, tcbPtr);
		return (err);
	}
	/* start on the tail and traverse with >= cond,    */
	/*  so we use a single insertafter.                */
	K_LISTNODE *currNodePtr = kobj->listDummy.prevPtr;
	K_TCB *currTcbPtr = K_LIST_GET_TCB_NODE(currNodePtr, K_TCB);
	while (currTcbPtr->priority >= tcbPtr->priority)
	{
		currNodePtr = currNodePtr->nextPtr;
		/* list end */
		if (currNodePtr == &(kobj->listDummy))
		{
			break;
		}
		currTcbPtr = K_LIST_GET_TCB_NODE(currNodePtr, K_TCB);

	}
	err = kListInsertAfter(kobj, currNodePtr, &(tcbPtr->tcbNode));
	assert(err == 0);
	return (err);
}

#define INSTANT_PREEMPT_LOWER_PRIO
/* this function enq ready and pend ctxt swtch if ready > running */
K_ERR kReadyCtxtSwtch(K_TCB *const tcbPtr)
{
	K_CR_AREA
	K_ENTER_CR
	if (IS_NULL_PTR(tcbPtr))
	{
		kErrHandler(FAULT_NULL_OBJ);
		K_EXIT_CR
		return (K_ERR_OBJ_NULL);
	}

	if (kTCBQEnq(&readyQueue[tcbPtr->priority], tcbPtr) == K_SUCCESS)
	{
		tcbPtr->status = READY;
#ifdef INSTANT_PREEMPT_LOWER_PRIO
		if (READY_HIGHER_PRIO(tcbPtr))
		{
			assert(!kTCBQEnq(&readyQueue[runPtr->priority], runPtr));
			runPtr->status = READY;
			K_PEND_CTXTSWTCH
		}
#endif
		K_EXIT_CR
		return (K_SUCCESS);
	}
	K_EXIT_CR
	return (K_ERROR);
}

K_ERR kReadyQDeq(K_TCB **const tcbPPtr, PRIO priority)
{

	K_ERR err = kTCBQDeq(&readyQueue[priority], tcbPPtr);
	assert(err == 0);
	return (err);
}

/*******************************************************************************
 * Task Control Block Management
 *******************************************************************************/
static PID pPid = 0; /** system pid for each task 	*/

static K_ERR kInitStack_(INT *const stackAddrPtr, UINT32 const stackSize,
		TASKENTRY const taskFuncPtr); /* init stacks */

static K_ERR kInitTcb_(TASKENTRY const taskFuncPtr, INT *const stackAddrPtr,
		UINT32 const stackSize);

static K_ERR kInitStack_(INT *const stackAddrPtr, UINT32 const stackSize,
		TASKENTRY const taskFuncPtr)
{
	if (stackAddrPtr == NULL || stackSize == 0 || taskFuncPtr == NULL)
	{
		return (K_ERROR);
	}
	stackAddrPtr[stackSize - PSR_OFFSET] = 0x01000000;
	stackAddrPtr[stackSize - PC_OFFSET] = (INT32) taskFuncPtr;
	stackAddrPtr[stackSize - LR_OFFSET] = 0x14141414;
	stackAddrPtr[stackSize - R12_OFFSET] = 0x12121212;
	stackAddrPtr[stackSize - R3_OFFSET] = 0x03030303;
	stackAddrPtr[stackSize - R2_OFFSET] = 0x02020202;
	stackAddrPtr[stackSize - R1_OFFSET] = 0x01010101;
	stackAddrPtr[stackSize - R0_OFFSET] = 0x00000000;
	stackAddrPtr[stackSize - R11_OFFSET] = 0x11111111;
	stackAddrPtr[stackSize - R10_OFFSET] = 0x10101010;
	stackAddrPtr[stackSize - R9_OFFSET] = 0x09090909;
	stackAddrPtr[stackSize - R8_OFFSET] = 0x08080808;
	stackAddrPtr[stackSize - R7_OFFSET] = 0x07070707;
	stackAddrPtr[stackSize - R6_OFFSET] = 0x06060606;
	stackAddrPtr[stackSize - R5_OFFSET] = 0x05050505;
	stackAddrPtr[stackSize - R4_OFFSET] = 0x04040404;
	/*stack painting*/
	for (UINT32 j = 17; j < stackSize; j++)
	{
		stackAddrPtr[stackSize - j] = (INT) 0xBADC0FFE;
	}
	stackAddrPtr[0] = 0x0BADC0DE;
	return (K_SUCCESS);
}

K_ERR kInitTcb_(TASKENTRY const taskFuncPtr, INT *const stackAddrPtr,
		UINT32 const stackSize)
{
	if (kInitStack_(stackAddrPtr, stackSize, taskFuncPtr) == K_SUCCESS)
	{
		tcbs[pPid].stackAddrPtr = stackAddrPtr;
		tcbs[pPid].sp = &stackAddrPtr[stackSize - R4_OFFSET];
		tcbs[pPid].stackSize = stackSize;
		tcbs[pPid].status = READY;
		tcbs[pPid].pid = pPid;

		return (K_SUCCESS);
	}
	return (K_ERROR);
}

K_ERR kCreateTask(TASKENTRY const taskFuncPtr, STRING taskName, TID const id,
		INT *const stackAddrPtr, UINT32 const stackSize,
#if(K_DEF_SCH_TSLICE==ON)
        TICK const timeSlice,
#endif
		PRIO const priority, BOOL const runToCompl)
{
	if (id == TIMHANDLER_ID || id == IDLETASK_ID)
	{
		kErrHandler(FAULT_INVALID_TASK_ID);
	}
	/* if private PID is 0, system tasks hasn't been started yet */
	if (pPid == 0)
	{

		/* initialise IDLE TASK */
		assert(kInitTcb_(IdleTask, idleStack, IDLE_STACKSIZE) == K_SUCCESS);

		tcbs[pPid].priority = idleTaskPrio;
		tcbs[pPid].realPrio = idleTaskPrio;
		tcbs[pPid].taskName = "IdleTask";
		tcbs[pPid].uPid = IDLETASK_ID;
		tcbs[pPid].runToCompl = FALSE;
#if(K_DEF_SCH_TSLICE==ON)

        tcbs[pPid].timeSlice = 0;
#endif
		pPid += 1;

		/* initialise TIMER HANDLER TASK */
		assert(
				kInitTcb_(TimerHandlerTask, timerHandlerStack, TIMHANDLER_STACKSIZE) == K_SUCCESS);

		tcbs[pPid].priority = 0;
		tcbs[pPid].realPrio = 0;
		tcbs[pPid].taskName = "TimHandlerTask";
		tcbs[pPid].uPid = TIMHANDLER_ID;
		tcbs[pPid].runToCompl = TRUE;
#if(K_DEF_SCH_TSLICE==ON)

        tcbs[pPid].timeSlice = 0;
#endif

		pPid += 1;

	}
	/* initialise user tasks */
	if (kInitTcb_(taskFuncPtr, stackAddrPtr, stackSize) == K_SUCCESS)
	{
#if(K_DEF_SCH_TSLICE==ON)
        {
            assert(timeSlice>0);
            return (K_ERR_INVALID_TSLICE);
        }
#endif
		if (priority > idleTaskPrio)
		{
			assert(FAULT_INVALID_TASK_PRIO);
		}

		tcbs[pPid].priority = priority;
		tcbs[pPid].realPrio = priority;
		tcbs[pPid].taskName = taskName;
		tcbs[pPid].lastWakeTime=0;
#if(K_DEF_SCH_TSLICE==ON)

        tcbs[pPid].timeSlice = timeSlice;
        tcbs[pPid].timeLeft = timeSlice;
#endif
		tcbs[pPid].uPid = id;
		tcbs[pPid].runToCompl = runToCompl;
		pPid += 1;
		return (K_SUCCESS);
	}

	return (K_ERROR);
}
/*******************************************************************************
 * CRITICAL REGIONS
 *******************************************************************************/

UINT32 kEnterCR(VOID)
{
	asm volatile("DSB");

	UINT32 crState;

	crState = __get_PRIMASK();
	if (crState == 0)
	{
		asm volatile("DSB");
		asm volatile ("CPSID I");
		asm volatile("ISB");

		return (crState);
	}
	asm volatile("DSB");
	return (crState);
}

VOID kExitCR(UINT32 crState)
{
	asm volatile("DSB");
	__set_PRIMASK(crState);
	asm volatile ("ISB");

}

/******************************************************************************
 HELPERS
 ******************************************************************************/
/*TODO: improve naive search*/
PID kGetTaskPID(TID const taskID)
{
	PID pid = 0;
	for (pid = 0; pid < NTHREADS; pid++)
	{
		if (tcbs[pid].uPid == taskID)
			break;
	}
	return (pid);
}

PRIO kGetTaskPrio(TID const taskID)
{
	PID pid = kGetTaskPID(taskID);
	return (tcbs[pid].priority);
}

/* get user-assigned running task id */
TID kGetRunningTID(VOID)
{
	return (runPtr->uPid);
}

/* get kernel-assigned running task id */
TID kGetRunningPID(VOID)
{
	return (runPtr->pid);
}

PRIO kGetRunningPRIO(VOID)
{
	return (runPtr->priority);
}


/******************************************************************************
 * KERNEL INITIALISATION
 *******************************************************************************/

static void kInitRunTime_(void)
{
	runTime.globalTick = 0;
	runTime.nWraps = 0;
}
static K_ERR kInitQueues_(void)
{
	K_ERR err = 0;
	for (PRIO prio = 0; prio < NPRIO + 1; prio++)
	{
		err |= kTCBQInit(&readyQueue[prio], "ReadyQ");
	}
	err |= kTCBQInit(&sleepingQueue, "SleepQ");
	assert(err == 0);
	return (err);
}

void kInit(void)
{

	version = kGetVersion();
	if (version != K_VALID_VERSION)
		kErrHandler(FAULT_KERNEL_VERSION);
	kInitQueues_();
	kInitRunTime_();
	highestPrio = tcbs[0].priority;
	for (int i = 0; i < NTHREADS; i++)
	{
		if (tcbs[i].priority < highestPrio)
		{
			highestPrio = tcbs[i].priority;
		}
	}

	for (int i = 0; i < NTHREADS; i++)
	{
		kTCBQEnq(&readyQueue[tcbs[i].priority], &tcbs[i]);
	}

	kReadyQDeq(&runPtr, highestPrio);
	assert(runPtr->status == READY);
	assert(tcbs[IDLETASK_ID].priority == lowestPrio+1);
	kApplicationInit();
	__enable_irq();

	_K_STUP
	/* calls low-level scheduler for start-up */
}

/*******************************************************************************
 *  TICK MANAGEMENT
 *******************************************************************************/
static inline VOID kReadyRunningTask_(VOID)
{
	if (runPtr->status == RUNNING)
	{
		assert(
				(kTCBQEnq(&readyQueue[runPtr->priority], runPtr) == (K_SUCCESS)));
		runPtr->status = READY;
	}
}

#if (K_DEF_SCH_TSLICE == ON)
static BOOL kDecTimeSlice_(void)
{
    if ( (runPtr->status == RUNNING) && (runPtr->runToCompl == FALSE))
    {
        {
            if (runPtr->timeLeft > 0)
            {
                runPtr->timeLeft -= 1U;
            }
            if (runPtr->timeLeft == 0)
            {
                runPtr->timeLeft = runPtr->timeSlice;
                kReadyRunningTask_();
                return (TRUE);
            }
        }
    }
    return (FALSE);
}
#endif
static BOOL kSleepHandle_(VOID)
{
	K_CR_AREA

	K_ENTER_CR

	BOOL ret = FALSE;
	if (dTimSleepList)
	{
		if (dTimSleepList->dTicks > 0)
			dTimSleepList->dTicks--;

		while (dTimSleepList != NULL && dTimSleepList->dTicks == 0)
		{
			K_TIMER *expTimerPtr = dTimSleepList;
			K_TCB *tcbToWakePtr = NULL;
			tcbToWakePtr = (K_TCB*) (dTimSleepList->argsPtr);
			if (tcbToWakePtr->status == SLEEPING)
				assert(tcbToWakePtr != NULL);
			assert(!kTCBQRem(&sleepingQueue, &tcbToWakePtr));
			assert(tcbToWakePtr != NULL);
			kTCBQEnq(&readyQueue[tcbToWakePtr->priority], tcbToWakePtr);
			tcbToWakePtr->status = READY;
			tcbToWakePtr->pendingTmr = NULL;
			kTimerPut(expTimerPtr);
			dTimSleepList = dTimSleepList->nextPtr;
			K_EXIT_CR
			return (TRUE);
		}
	}

	K_EXIT_CR

	return (ret);

}

BOOL kTickHandler(void)
{
	/* return is short-circuit to !runToCompl & */
	BOOL runToCompl = FALSE;
#if (K_DEF_SCH_TSLICE==ON)
    BOOL tsliceDue = FALSE;
#endif
	BOOL readySleepTask = FALSE;
	BOOL timeOutTask = FALSE;
	BOOL ret = FALSE;
	runTime.globalTick += 1U;
	if (runPtr->busyWaitTime > 0)
	{
		runPtr->busyWaitTime -= 1U;
	}
	if (runTime.globalTick == K_TICK_TYPE_MAX)
	{
		runTime.globalTick = 0U;
		runTime.nWraps += 1U;
	}
	/* sleep delay */
	if (dTimSleepList)
	{
		readySleepTask = kSleepHandle_();
	}

	/* handle time out list */
	timeOutTask = kHandleTimeoutList();

	/* a run-to-completion task is a first-class citizen not prone to tick
	 truculence.*/
	if (runPtr->runToCompl && (runPtr->status == RUNNING))
		/* this flag toggles, short-circuiting the */
		/* return value  to FALSE                  */
		runToCompl = TRUE;

	/* this is the deferred handler for timers. it has priority 0. any user */
	/* task with priority also 0 will be postponed. */
	if (runPtr->pid != TIMHANDLER_ID)
	{
		if (dTimOneShotList || dTimReloadList)
		{
			kSignal(TIMHANDLER_ID); /* ready the TIMHANDLER */
			kReadyRunningTask_();
		}
	}

	/* if time-slice is enabled, decrease the time-slice. */
#if (K_DEF_SCH_TSLICE==ON)
    tsliceDue = kDecTimeSlice_();
    if (tsliceDue == FALSE)
    {
        /* check if there is a higher priority task in the queue   */
        /* this check accounts for any tasks with higher priority  */
        /* than runPtr when there is no timer handler  */
        /* or run-to-compl task pending */
        PRIO nextPrio_ = kCalcNextTaskPrio_();
        if (runPtr->priority > nextPrio_)
        {
            kReadyRunningTask_();
        }
    }
#else
	/* only READY the idle task if tslice is off*/
	if (runPtr->pid == IDLETASK_ID)
	{
		kReadyRunningTask_();
	}
#endif
	ret = (!runToCompl)
			& ((runPtr->status == READY) | readySleepTask | timeOutTask
					| (runPtr->yield == TRUE));
	return (ret);
}

/*******************************************************************************
 TASK SWITCHING LOGIC
 *******************************************************************************/
static inline PRIO kCalcNextTaskPrio_()
{
	if (readyQBitMask == 0U)
	{
		return (idleTaskPrio);
	}
	readyQRightMask = readyQBitMask & -readyQBitMask;
	PRIO prio = (PRIO) (__getReadyPrio(readyQRightMask));

	return (prio);
	/* return __builtin_ctz(readyQRightMask); */
}

VOID kSchSwtch(VOID)
{
	K_TCB *nextRunPtr = NULL;
	K_TCB *prevRunPtr = runPtr;
	if (runPtr->status == RUNNING)
	{
		kReadyRunningTask_();
	}
	nextTaskPrio = kCalcNextTaskPrio_(); /* get the next task priority */
	kTCBQDeq(&readyQueue[nextTaskPrio], &nextRunPtr);
	if (nextRunPtr == NULL)
	{
		kErrHandler(FAULT_NULL_OBJ);
	}
	runPtr = nextRunPtr;
	if (nextRunPtr->pid != prevRunPtr->pid)
	{
		runPtr->nPreempted += 1U;
		prevRunPtr->preemptedBy = runPtr->pid;
	}
	if (runPtr->yield)
	{
		runPtr->yield = FALSE;
	}
	return;
}

