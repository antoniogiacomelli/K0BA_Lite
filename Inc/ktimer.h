#ifndef K_TIMER_H
#define K_TIMER_H
#include "kmem.h"
#include "kitc.h"
/* Timer Reload / Oneshot optionss */

#define RELOAD      1
#define ONESHOT     0


extern K_TIMER *dTimReloadList; /* periodic timers */
extern K_TIMER *dTimOneShotList; /* one-shot timers */
extern K_TIMER *dTimSleepList; /* sleep delay list */

extern K_TIMEOUT_NODE* timeOutListHeadPtr ;
VOID kTimeOut(K_TIMEOUT_NODE *timeOutNode, TICK timeout);
VOID kRemoveTaskFromMbox(ADDR kobj);
void kRemoveTaskFromSema(void *kobj);
VOID kRemoveTaskFromMutex(ADDR kobj);
VOID kRemoveTaskFromQueue(ADDR kobj);
BOOL kHandleTimeoutList(void);
VOID kRemoveTaskFromEvent(ADDR kobj);

extern struct kRunTime runTime; /* record of run time */

K_ERR kTimerInit(STRING timerName, TICK const ticks, CALLOUT const funPtr,
		ADDR const argsPtr, BOOL const reload);

VOID kBusyDelay(TICK const delay);

#define BUSY(t) kBusyDelay(t)

VOID kSleep(TICK const ticks);

#define SLEEP(t) kSleep(t)

TICK kTickGet(VOID);

BOOL kTimerHandler(void);

K_ERR kTimerPut(K_TIMER* const);
#if (K_DEF_SCH_TSLICE==OFF)
VOID kSleepUntil(TICK const);
#endif
#endif
