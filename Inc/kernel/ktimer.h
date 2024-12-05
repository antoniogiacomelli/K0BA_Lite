#ifndef K_TIMER_H
#define K_TIMER_H
#include "kmem.h"
#include "kitc.h"

extern K_TIMER *dTimReloadList; /* periodic timers */
extern K_TIMER *dTimOneShotList; /* reload timers */
extern volatile struct kRunTime runTime; /* record of run time */
K_ERR kTimerInit(STRING timerName, TICK const ticks, CALLOUT const funPtr,
        ADDR const argsPtr, BOOL const reload);
VOID kBusyDelay(TICK const delay);
/*nice alias*/
#define BUSY(t) kBusyDelay(t)
VOID kSleep(TICK const ticks);
/*nice alias*/
#define SLEEP(t) kSleep(t)
TICK kTickGet(VOID);
BOOL kTimerHandler(void);
/* Timer Reload / Oneshot optionss */
#define RELOAD      1
#define ONESHOT     0

#endif /* INC_KERNEL_KTIMER_H_ */
