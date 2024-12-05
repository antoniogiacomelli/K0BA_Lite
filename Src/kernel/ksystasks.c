/******************************************************************************
 *
 *     [[K0BA - Kernel 0 For Embedded Applications] | [VERSION: 0.3.0]]
 *
 ******************************************************************************
 ******************************************************************************
 * 	Module       : System Tasks
 * 	Depends on   : Scheduler
 * 	Provides to  : Application Timers
 *  Public API	 : N/A
 * 	In this unit :
 * 					o  System Tasks and Deferred ISR handler tasks
 *
 *****************************************************************************/

#define K_CODE
#include "kenv.h"
#include "kconfig.h"
#include "kobjs.h"
#include "ktypes.h"
#include "kitc.h"
#include "ktimer.h"
#include "kinternals.h"


UINT32 idleStack[IDLE_STACKSIZE];
UINT32 timerHandlerStack[TIMHANDLER_STACKSIZE];

void IdleTask(void) /*SAD: on a well designed system this fella runs for
                      >30% of the time. still, they call it ''idle''.
                      where are the unions? */
{

	while (1)
	{
		__DSB();
		__WFI();
		__ISB();
	}
}

void TimerHandlerTask(void)
{
	while (1)
	{
		kPend();
		K_CR_AREA
		K_ENTER_CR
		if (dTimOneShotList || dTimReloadList)
		{
			if (kTimerHandler())
			    kYield();
		}
		K_EXIT_CR
	}
}
