/******************************************************************************
 *
 *     [[K0BA - Kernel 0 For Embedded Applications] | [VERSION: 0.3.0]]
 *
 ******************************************************************************
 ******************************************************************************
 *  In this header:
 *                  o Inter-task synchronisation and communication kernel
 *                    module.
 *
 *****************************************************************************/

#ifndef K_ITC_H
#define K_ITC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "kconfig.h"
#include "ktypes.h"
#include "kobjs.h"
#include "kerr.h"
#include "klist.h"
#include "ksch.h"

K_ERR kPend(VOID);
K_ERR kSignal(TID const);

static inline K_ERR kUnRun_(K_TCBQ* queuePtr, K_TCB* tcbPtr,
        K_TASK_STATUS status)
{
    if (runPtr->status == RUNNING)
    {
        K_ERR err = kTCBQEnq(queuePtr, tcbPtr);
        if ( !err)
        {
            tcbPtr->status = status;
            return (err);
        }
    }
    return (K_ERR_TASK_NOT_RUNNING);
}


#if (K_DEF_SLEEPWAKE==ON)

static inline K_ERR kEventInit(K_EVENT* const kobj)
{
    if (kobj == NULL)
    {
        kErrHandler(FAULT_NULL_OBJ);
    }
    kobj->eventID = (UINT32) kobj;
    assert( !kTCBQInit( & (kobj->waitingQueue), "eventQ"));
    kobj->init = TRUE;
    return (K_SUCCESS);
}
VOID kEventSleep(K_EVENT* const, TICK);
VOID kEventWake(K_EVENT* const);

#endif


/*
                \o_´
 	   ____  ´  (
	  /_|__\...(.\...~~~~´´´´´....                   ,.~~~~~~~..'´
	  \_|__/                       ´´.__ ,.~~~~~~~..'´


 	   ____
	  /_|__\.´´(´(...~~~~´´´´´....                   ,.~~~~~~~..'´
	  \_|__/     \                 ´´.__ ,.~~~~~~~..'´
	          `  o  ´
             `  // ´
             `
               ´
 	   ____
	  /_|__\.´´(´(...~~~~´´´´´....                   ,.~~~~~~~..'´
	  \_|__/    o                 ´´.__ ,.~~~~~~~..'´
	         `  / `
               ((
             ` `   `

*/

#ifdef __cplusplus
}
#endif
#endif /* K_ITC_H */
