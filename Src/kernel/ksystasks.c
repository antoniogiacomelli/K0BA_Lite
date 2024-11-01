/******************************************************************************
 *
 *     [[K0BA - Kernel 0 For Embedded Applications] | [VERSION: 1.1.0]]
 *
 ******************************************************************************
 ******************************************************************************
 * 	Module       : Nucleus
 * 	Sub-module   : System Tasks
 * 	Depends on   : Scheduler
 * 	Provides to  : Application Timers
 *
 * 	In this unit :
 * 					o  System Tasks and Deferred ISR handler tasks
 *
 *****************************************************************************/

#define K_CODE
#include "ksys.h"

UINT32 idleStack[IDLE_STACKSIZE];
UINT32 timerHandlerStack[TIMHANDLER_STACKSIZE];

void IdleTask(void)
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

		if (dTimOneShotList || dTimReloadList)
		{

			kTimerHandler();
		}
	}
}
