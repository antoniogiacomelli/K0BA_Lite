/******************************************************************************
 *
 *     [[K0BA - Kernel 0 For Embedded Applications] | [VERSION: 0.3.1]]
 *
 ******************************************************************************
 ******************************************************************************
 *  In this header:
 *                  o High-level scheduler kernel module.
 *
 *****************************************************************************/
#ifndef KSCH_H
#define KSCH_H

#include "kconfig.h"
#include "kinternals.h"

/* shared data */
extern K_TCB* runPtr; /* Pointer to the running TCB */
extern K_TCB tcbs[NTHREADS]; /* Pool of TCBs */
extern volatile K_FAULT faultID; /* Fault ID */
extern INT idleStack[IDLE_STACKSIZE]; /* Stack for idle task */
extern INT timerHandlerStack[TIMHANDLER_STACKSIZE];
extern K_TCBQ readyQueue[K_DEF_MIN_PRIO + 2]; /* Table of ready queues */
extern K_TCBQ sleepingQueue;
extern K_TCBQ timeOutQueue;

BOOL kSchNeedReschedule(K_TCB*);
VOID kSchSwtch(VOID);
UINT32 kEnterCR(VOID);
VOID kExitCR(UINT32);
VOID kInit(VOID);
VOID kYield(VOID);
VOID kApplicationInit(VOID);
PID kGetTaskPID(TID const);
PRIO kGetTaskPrio(TID const);
extern unsigned __getReadyPrio(unsigned);


#ifdef __cplusplus
}
#endif

#endif /* KSCH_H */
