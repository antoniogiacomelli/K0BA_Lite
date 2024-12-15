/******************************************************************************
 *
 * [K0BA - Kernel 0 For Embedded Applications] | [VERSION: 0.3.0]
 *
 ******************************************************************************
 ******************************************************************************
 * 	Module           : Application Timers
 * 	Depends on       : Scheduler, Inter-task Synchronisation
 *  Public API 		 : Yes
 * 	In this unit:
 * 			o Timer Pool Management
 *			o Timer Delta List
 *			o Timer Handler
 *			o Sleep delay
 *			o Busy-wait delay
 *
 *****************************************************************************/

#define K_CODE
#include "kconfig.h"
#include "kobjs.h"
#include "ktypes.h"
#include "ksch.h"
#include "kitc.h"
#include "kmem.h"
#include "kutils.h"
#include "kinternals.h"
#include "ktimer.h"

K_MEM timerMem;
K_TIMER* dTimReloadList = NULL; /* periodic timers */
K_TIMER* dTimOneShotList = NULL; /* reload	  timers */
K_TIMER* dTimSleepList = NULL;
K_TIMER timerPool[K_DEF_N_TIMERS];
static BOOL timerPoolInit = FALSE;

static K_ERR kTimerListAdd_(K_TIMER** dTimList, STRING timerName,
		TICK tickCount, CALLOUT funPtr, ADDR argsPtr, BOOL reload);
static inline void kTimerPoolInit_(VOID)
{
	if ( !timerPoolInit)
	{
		kMemInit( &timerMem, (BYTE*) timerPool, TIMER_SIZE,
				K_DEF_N_TIMERS);
		timerPoolInit = TRUE;
	}
}
K_TIMER* kTimerGet(VOID)
{

	K_TIMER* retValPtr;
	retValPtr = (K_TIMER*) kMemAlloc( &timerMem);
	return (retValPtr);
}

K_ERR kTimerPut(K_TIMER* const kobj)
{

	if (kMemFree( &timerMem, (ADDR) kobj) == 0)
	{
		return (K_SUCCESS);
	}
	return (K_ERROR);
}

K_ERR kTimerInit(STRING timerName, TICK ticks, CALLOUT funPtr, ADDR argsPtr,
		BOOL reload)
{
	K_CR_AREA
	K_ENTER_CR
	if (reload == TRUE)
	{
		assert(
				kTimerListAdd_( &dTimReloadList, timerName, ticks, funPtr,
						argsPtr, reload) == (K_SUCCESS));
		K_EXIT_CR
		return (K_SUCCESS);
	}
	else
	{
		assert(
				kTimerListAdd_( &dTimOneShotList, timerName, ticks, funPtr,
						argsPtr, reload) == (K_SUCCESS));
		K_EXIT_CR
		return (K_ERROR);
	}
}

static K_ERR kTimerListAdd_(K_TIMER** selfPtr, STRING timerName, TICK ticks,
		CALLOUT funPtr, ADDR argsPtr, BOOL reload)
{
	kTimerPoolInit_();
	K_TIMER* newTimerPtr = kTimerGet();
	if (newTimerPtr == NULL)
	{
		return (K_ERROR);
	}
	newTimerPtr->timerName = timerName;
	newTimerPtr->dTicks = ticks;
	newTimerPtr->ticks = ticks;
	newTimerPtr->funPtr = funPtr;
	newTimerPtr->argsPtr = argsPtr;
	newTimerPtr->reload = reload;

	if ( *selfPtr == NULL)
	{

		*selfPtr = newTimerPtr;
		return (K_SUCCESS);
	}
	K_TIMER* currListPtr = *selfPtr;
	K_TIMER* prevListPtr = NULL;

	/* traverse the delta list to find the correct position based on relative
	 time*/
	while (currListPtr != NULL && currListPtr->dTicks < newTimerPtr->dTicks)
	{
		newTimerPtr->dTicks -= currListPtr->dTicks;
		prevListPtr = currListPtr;
		currListPtr = currListPtr->nextPtr;
	}
	/* insert new timer */
	newTimerPtr->nextPtr = currListPtr;

	/* adjust delta */
	if (currListPtr != NULL)
	{
		currListPtr->dTicks -= newTimerPtr->dTicks;
	}
	/* im the head, here */
	if (prevListPtr == NULL)
	{
		*selfPtr = newTimerPtr;
	}
	else
	{
		prevListPtr->nextPtr = newTimerPtr;
	}
	return (K_SUCCESS);
}
static K_TIMER timReloadCpy = {0};
BOOL kTimerHandler(void)
{
	BOOL ret=FALSE;
	if (dTimOneShotList)
	{
		if (dTimOneShotList->dTicks > 0) dTimOneShotList->dTicks --;
	}
	if (dTimReloadList)
	{
		if (dTimReloadList->dTicks > 0) dTimReloadList->dTicks --;
	}
	while (dTimOneShotList != NULL && dTimOneShotList->dTicks == 0)
	{
		ret=TRUE;
				K_TIMER* expTimerPtr = dTimOneShotList;
		/* ... as long there is that tick, tick...*/
		expTimerPtr = dTimOneShotList;
		/* ... followed by that bump: */
		dTimOneShotList->funPtr(dTimOneShotList->argsPtr);
		kTimerPut(expTimerPtr);
		dTimOneShotList = dTimOneShotList->nextPtr;
	}

	K_TIMER* putRelTimerPtr = NULL;
	while (dTimReloadList->dTicks == 0 && dTimReloadList)
	{
		putRelTimerPtr = dTimReloadList;
		kMemCpy( &timReloadCpy, dTimReloadList, TIMER_SIZE);
		kTimerPut(putRelTimerPtr);
		dTimReloadList->funPtr(dTimReloadList->argsPtr);
		dTimReloadList = dTimReloadList->nextPtr;
		K_TIMER* insTimPtr = &timReloadCpy;
		kTimerInit(insTimPtr->timerName, insTimPtr->ticks, insTimPtr->funPtr,
				insTimPtr->argsPtr, insTimPtr->reload);
		if (dTimReloadList == NULL)
		{
			dTimReloadList = &timReloadCpy;
			break;
		}
	}
	return (ret);
}

/*******************************************************************************
 * SLEEP TIMER AND BUSY-WAIT-DELAY
 *******************************************************************************/
void kSleep(TICK ticks)
{
	if (runPtr->status != RUNNING)
	{
		assert(FAULT_TASK_INVALID_STATE);
	}

	K_CR_AREA

	K_ENTER_CR

	if ( !kTimerListAdd_( &dTimSleepList, "SleepTimer", ticks, NULL,
			(K_TCB*) runPtr, ONESHOT))
	{

		if ( !kTCBQEnq( &sleepingQueue, runPtr))
		{
			runPtr->status = SLEEPING;
			runPtr->pendingTmr = (K_TIMER*) (dTimSleepList);

			K_PEND_CTXTSWTCH

		}
	}

	K_EXIT_CR

}

VOID kBusyDelay(TICK delay)
{
	if (runPtr->busyWaitTime == 0)
	{
		runPtr->busyWaitTime = delay;
	}
	while (runPtr->busyWaitTime)
		;/* procrastinating here */
	return;
}
TICK kTickGet(void)
{
	return (runTime.globalTick);
}
