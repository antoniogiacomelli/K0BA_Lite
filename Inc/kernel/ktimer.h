#ifndef K_TIMER_H
#define K_TIMER_H
#include "kmem.h"
#include "kitc.h"

extern K_TIMER *dTimReloadList; /* periodic timers */
extern K_TIMER *dTimOneShotList; /* reload timers */
extern K_TIMER* dTimSleepList; /* sleep delay list */

extern volatile struct kRunTime runTime; /* record of run time */
K_ERR kTimerInit(STRING timerName, TICK const ticks, CALLOUT const funPtr,
        ADDR const argsPtr, BOOL const reload);

VOID kBusyDelay(TICK const delay);
#define BUSY(t) kBusyDelay(t)

VOID kSleep(TICK const ticks);
#define SLEEP(t) kSleep(t)

TICK kTickGet(VOID);
BOOL kTimerHandler(void);
K_ERR kTimerPut(K_TIMER* const);

/* Timer Reload / Oneshot optionss */

#define RELOAD      1
#define ONESHOT     0

#endif
