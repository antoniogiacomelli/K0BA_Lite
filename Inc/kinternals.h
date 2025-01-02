/******************************************************************************
 *
 *     [[K0BA - Kernel 0 For Embedded Applications] | [VERSION: 0.3.1]]
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

#define N_SYSTASKS          2 /*idle task + tim handler*/

/*** Config values */
#define NTHREADS             (K_DEF_N_USRTASKS + N_SYSTASKS)
#define NPRIO               (K_DEF_MIN_PRIO + 1)
#define K_DEF_ENQ_PRIO  (0)
#define K_DEF_ENQ_FIFO  (1)
#define TICK_10MS           (SystemCoreClock/1000)  /*  Tick period of 10ms */
#define TICK_5MS            (SystemCoreClock/2000)  /* Tick period of 5ms */
#define TICK_1MS            (SystemCoreClock/10000) /*  Tick period of 1ms */


/* Mapped API */

typedef struct kSema SEMA;
typedef struct kMutex LOCK;
typedef struct kMesg MESG;
typedef struct kEvent EVNT;
typedef struct kList QUEUE;
typedef struct kListNode NODE;
typedef struct kTcb TCB;


/*                                                                    ____
 *                                                  .~~~~.,      ..~~/__|_\
 *                                ,....,,.   ....´`.´      ´´..-'    \__|_/
 *                              ,'        `.´
 *                            o//
 *                           /
 *                          ))
 *******************************************************************************/


#define SLPEVNT                      kEventSleep
#define WKEVNT                       kEventWake
#define EVNTINIT                     kEventInit


/* Mem Block Pool */
#define BLKPOOLINIT                  kMemInit
#define BLKALLOC 				     kMemAlloc
#define BLKFREE  					 kMemFree

/* Misc */
#define KFAULT					 	 kErrHandler
#define MEMCPY              	     kMemCpy

/* inline asm */
#define DMB								asm volatile ("dmb 0xF":::"memory");
#define DSB								asm volatile ("dsb 0xF":::"memory");
#define ISB								asm volatile ("isb 0xF":::"memory");
#define NOP                             asm volatile ("nop");

/*_ means assembly hardwired code parms */
#define _K_SWTCH asm volatile("svc #0xC5");
#define _K_STUP asm volatile("svc #0xAA");

/*  helpers */
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

/* for the mesg queue, pointers are incremented altogether */
#define CPYQ(d, s, z, r)                                \
do {                                                    \
    BYTE* __d = (BYTE*)(d);                             \
    BYTE const* __s = (BYTE const*)(s);                 \
    *(BYTE*)(__d)++ = *(BYTE const*)(__s)++;            \
    (r)++;												\
    SIZE __z = z;										\
    if ((__z) > 1U) 										\
	{                                     				\
        while (--(__z)) 									\
		{			                                    \
            *(BYTE*)(__d)++ = *(BYTE const*)(__s)++;    \
            (r)++;                                      \
        }                                               \
    }                                                   \
} while (0U)

__STATIC_FORCEINLINE unsigned kIsISR()
{
    unsigned ipsr_value;
    asm("MRS %0, IPSR" : "=r"(ipsr_value));
    DMB
    return (ipsr_value);
}

#define K_GET_CONTAINER_ADDR(memberPtr, containerType, memberName) \
    ((containerType *)((unsigned char *)(memberPtr) - \
     offsetof(containerType, memberName)))
#define K_CR_AREA  UINT32 crState_;
#define K_ENTER_CR crState_ = kEnterCR();
#define K_EXIT_CR  kExitCR(crState_);
#define K_PEND_CTXTSWTCH K_TRAP_PENDSV

#define READY_HIGHER_PRIO(ptr) ((ptr->priority < nextTaskPrio) ? 1 : 0)
#define K_TICK_TYPE_MAX ((1ULL << (8 * sizeof(typeof(TICK)))) - 1)
#define K_PRIO_TYPE_MAX ((1ULL << (8 * sizeof(typeof(PRIO)))) - 1)
#define IS_NULL_PTR(ptr) ((ptr) == NULL ? 1 : 0)

#define K_TRAP_PENDSV  \
                       \
    ISB		           \
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; \
    DSB

#define K_TRAP_SVC(N)  \
    do { asm volatile ("svc %0" :: "i" (N)); } while(0U)

#define K_TICK_EN  SysTick->CTRL |= 0xFFFFFFFF;
#define K_TICK_DIS SysTick->CTRL &= 0xFFFFFFFE;
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define IS_INIT(obj) (obj)->init) ? (1) : (0)
#define IS_VALID_TID(id) ((id == (IDLETASK_ID)) || (id == (TIMHANDLER_ID))) ? (0) : (1)

#define DEADCODE (0)



/*
 *
 *                                                                    ____
 *                                                  .~~~~.,      ..~~/__|_\
 *                                ,....,,.   ....´`.´      ´´..-'    \__|_/
 *                              ,'        `.´
 *                ,.~~~~~~~..'´
 *           ~.o./
 *        ~´`-´
 *        ~-'
 *
 *      `.´
 *      '
 *     .
 ******************************************************************************/

#ifdef __cplusplus
}
#endif
#endif
