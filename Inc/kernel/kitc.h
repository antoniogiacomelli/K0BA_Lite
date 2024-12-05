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

VOID kPend(VOID);
VOID kSignal(PID const);
VOID kSuspend(TID);
VOID kResume(TID);

#if(K_DEF_SEMA==ON)

K_ERR kSemaInit(K_SEMA* const, INT32 const);
VOID kSemaWait(K_SEMA* const);
VOID kSemaSignal(K_SEMA* const);

static inline VOID kSignalAttmpt_(PID pid)
{
    K_TCB* tcbGotPtr = &tcbs[pid];
    K_ERR err = kTCBQRem( &attmptQueue, &tcbGotPtr);
    if (err < 0)
        assert(0);
    err = kReadyCtxtSwtch(tcbGotPtr);
    if (err < 0)
        assert(0);
}

#endif

#if (K_DEF_MUTEX==ON)

K_ERR kMutexInit(K_MUTEX* const );
VOID kMutexLock(K_MUTEX* const );
VOID kMutexUnlock(K_MUTEX* const );
K_ERR kMutexQuery(K_MUTEX* const );

#endif

#if (K_DEF_SLEEPWAKE==ON)

static inline K_ERR kEventInit(K_EVENT* const kobj)
{
    if (kobj == NULL)
    {
        kErrHandler(FAULT_NULL_OBJ);
    }
    kobj->eventID = (UINT32) kobj;
    assert( !kTCBQInit( & (kobj->queue), "eventQ"));
    kobj->init = TRUE;
    return K_SUCCESS;
}

VOID kEventSleep(K_EVENT* const);
VOID kEventWake(K_EVENT* const);

#endif

static inline K_ERR kUnRun_(K_TCBQ* queuePtr, K_TCB* tcbPtr,
        K_TASK_STATUS status)
{
    if (runPtr->status == RUNNING)
    {
        if (runPtr->realPrio > runPtr->priority)
        {
            runPtr->priority = runPtr->realPrio;
        }

        K_ERR err = kTCBQEnq(queuePtr, tcbPtr);
        if ( !err)
        {
            tcbPtr->status = status;
            return (err);
        }
    }
    return (K_ERR_TASK_NOT_RUNNING);
}

#ifdef __cplusplus
}
#endif
#endif /* K_ITC_H */
