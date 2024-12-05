/******************************************************************************
 *
 *     [[K0BA - Kernel 0 For Embedded Applications] | [VERSION: 0.3.0]]
 *
 ******************************************************************************
 ******************************************************************************
 *  In this header:
 *                  o System internals defines
 *
 *****************************************************************************/

#ifndef SYS_ALIAS
#define SYS_ALIAS
#include "kenv.h"
#ifdef __cplusplus
extern "C" {
#endif
/*
 * brief This is the offset w.r.t the top of a stack frame
 * The numbers are unsigned.
 * */

#define PSR_OFFSET  1 /* Program Status Register offset */
#define PC_OFFSET   2 /* Program Counter offset */
#define LR_OFFSET   3 /* Link Register offset */
#define R12_OFFSET  4 /* R12 Register offset */
#define R3_OFFSET   5 /* R3 Register offset */
#define R2_OFFSET   6 /* R2 Register offset */
#define R1_OFFSET   7 /* R1 Register offset */
#define R0_OFFSET   8 /* R0 Register offset */
#define R11_OFFSET  9 /* R11 Register offset */
#define R10_OFFSET  10 /* R10 Register offset */
#define R9_OFFSET   11 /* R9 Register offset */
#define R8_OFFSET   12 /* R8 Register offset */
#define R7_OFFSET   13 /* R7 Register offset */
#define R6_OFFSET   14 /* R6 Register offset */
#define R5_OFFSET   15 /* R5 Register offset */
#define R4_OFFSET   16 /* R4 Register offset */

#define IDLE_STACKSIZE         64
#define TIMHANDLER_STACKSIZE  128
#define TIMHANDLER_ID        255
#define IDLETASK_ID           0

#define MSGBUFF_SIZE sizeof(K_MESG)
#define TIMER_SIZE   sizeof(K_TIMER)

#define _N_SYSTASKS          2 /*idle task + tim handler*/
#define NTHREADS             (K_DEF_N_USRTASKS + _N_SYSTASKS)
#define TICK_10MS           (SystemCoreClock/1000)  /*  Tick period of 10ms */
#define TICK_5MS            (SystemCoreClock/2000)  /* Tick period of 5ms */
#define TICK_1MS            (SystemCoreClock/10000) /*  Tick period of 1ms */
#define NPRIO               (K_DEF_MIN_PRIO + 1)

/* Mapped API */

typedef struct kSema SEMA;
typedef struct kMutex LOCK;
typedef struct kMesg MESG;
typedef struct kEvent EVNT;
typedef struct kList QUEUE;
typedef struct kListNode NODE;
typedef struct kTcb TCB;


/*
 *                                               .~~~~~~~~~~~~~~~~~<<>>
 * the kite is         nutz:     ,....,,........´
 *                             ,'
 *                           o//
 *                     ~~   /
 *                         / \
 *
 */

/* SYNCH PRIMITIVES */

#define SEMINIT                      kSemaInit
#define SEMUP                        kSemaSignal
#define SEMDWN                       kSemaWait
#define LOCK                         kMutexLock
#define UNLOCK                       kMutexUnlock
#define LOCKINIT                     kMutexInit
#define SLPEVNT                      kEventSleep
#define WKEVNT                       kEventWake
#define EVNTINIT                     kEventInit

/* List */
#define QUEUEINIT 			    	 kListInit
#define ENQUEUE		 			     kListAddTail
#define DEQUEUE				    	 kListRemoveHead
#define ENQJAM 				    	 kListAddHead

/* Mem Block Pool */
#define BLKPOOLINIT                  kMemInit
#define BLKALLOC 				     kMemAlloc
#define BLKFREE  					 kMemFree

/* Sys Mesg Record */
#define MESGGET 					 kMesgGet_
#define MESGPUT 					 kMesgPut_

/* Misc */
#define KFAULT					 	 kErrHandler
#define MEMCPY              	     kMemCpy

/* inline asm */
#define DMB								asm volatile ("dmb 0xF":::"memory");
#define DSB								asm volatile ("dsb 0xF":::"memory");
#define ISB								asm volatile ("isb 0xF":::"memory");
#define NOP                             asm volatile ("nop");

/*  helpers */
/* we use this macro so we dont mind reentrancy */

#define CPY(d,s,z, r)                                 \
 do                                                   \
 {                                                    \
      BYTE* dt = (BYTE*) d;                           \
      BYTE const* st = (BYTE const*) s;               \
      r=0;                                            \
     for (SIZE i = 0; i < z; ++i)                     \
      {                                               \
          dt[i] = st[i];                              \
          r ++;                                       \
      }                                               \
  } while(0U)

/*
 * brief Macro to get the address of the container structure
 * param memberPtr Pointer to the member
 * param containerType The type of the container structure
 * param memberName The name of the member inside the structure
 */

#define K_GET_CONTAINER_ADDR(memberPtr, containerType, memberName) \
    ((containerType *)((unsigned char *)(memberPtr) - \
     offsetof(containerType, memberName)))

/* brief Critical region macro to declare a variable for critical
 * region state */
#define K_CR_AREA  UINT32 crState;

/* brief Macro to enter critical region */
#define K_ENTER_CR crState = kEnterCR();

/* brief Macro to exit critical region */
#define K_EXIT_CR  kExitCR(crState);

/* brief Trigger Context Switch */
#define K_PEND_CTXTSWTCH K_TRAP_PENDSV;

#define READY_HIGHER_PRIO(ptr) ((ptr->priority < nextTaskPrio) ? 1 : 0)

/*
 * brief Max value for tick type
 */
#define K_TICK_TYPE_MAX ((1ULL << (8 * sizeof(typeof(TICK)))) - 1)

/*
 * brief Max value for priority type
 */
#define K_PRIO_TYPE_MAX ((1ULL << (8 * sizeof(typeof(PRIO)))) - 1)

/*
 * brief Gets the address of the container structure of a node
 * param nodePtr Pointer to the node
 * param containerType Type of the container
 */


#define IS_NULL_PTR(ptr) ((ptr) == NULL ? 1 : 0)

/*
 * brief Software interrupt (trap) for PendSV
 */
#define K_TRAP_PENDSV  \
                       \
    __ISB();           \
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; \
    __DSB();

/*
 * brief Software interrupt (trap) for Supervisor Call
 * param N Supervisor call number
 */

#define K_TRAP_SVC(N)  \
    do { asm volatile ("svc %0" :: "i" (N)); } while(0U)

/*
 * brief Enables the system tick
 */
#define K_TICK_EN  SysTick->CTRL |= 0xFFFFFFFF;

/*
 * brief Disables the system tick
 */
#define K_TICK_DIS SysTick->CTRL &= 0xFFFFFFFE;

/*
 * brief Macro to convert version numbers to string.
 *
 */
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

/*
 * brief Macro to get the version as a string
 */
#define VERSION_STRING \
    TOSTRING(VERSION.major) "." \
    TOSTRING(VERSION.minor) "." \
    TOSTRING(VERSION.patch)

#define IS_INIT(obj) (obj)->init) ? (1) : (0)

#define DEADCODE (0)

__STATIC_FORCEINLINE unsigned kIsISR()
{
    unsigned ipsr_value;
    asm("MRS %0, IPSR" : "=r"(ipsr_value));
    return (ipsr_value);
}

/*  ~~~~~~~~~~~~~~~~~<<>> */
#ifdef __cplusplus
}
#endif
#endif
