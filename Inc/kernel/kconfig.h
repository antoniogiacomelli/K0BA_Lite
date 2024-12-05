/******************************************************************************
 *
 * [K0BA - Kernel 0 For Embedded Applications] | [VERSION: 0.3.0]
 *
 ******************************************************************************/
/**
 * \file     kconfig.h
 * \brief    Kernel Configuration File
 * \version  0.3.0
 * \author   Antonio Giacomelli
 * \details  Kernel configuration definitions.
 *
 *
 * ##Important Parameters##
 *
 * - **Number of User Tasks:**  (`K_DEF_N_USRTASKS`)
 *
 *    Number of tasks user will create.
 *
 * - **Maximum priority:**      (`K_DEF_N_MINPRIO`)
 *   Priorities range are from `0` to ``K_DEF_N_MINPRIO`.
 *   Note, the lower the number the highest ¨the effective
 *   priority. (0 is highest effective priority)
 *   Besides, the IDLE TASK is a system task with a priority
 *   number one unit above the maximum user-defined number,
 *   so it will always be lowest-priority task.
 *
 * - **Number of timers:**     (`K_DEF_N_TIMERS`)
 *   Consider using the number of tasks that will rely on timers
 *   +1. This a safe configuration. Remember kSleep(t) uses
 *   timers.
 *
 *
 * - **Tick Period:**        (`K_DEF_TICK_PERIOD`)
 *   Pre-defined values are `TICK_1MS`, `TICK_5MS` and `TICK_10MS`.
 *   Users can define it, as they wish, by configuring SysTick.
 *   Recommended value is 5ms.
 *
 ******************************************************************************/

#ifndef KCONFIG_H
#define KCONFIG_H
#include "kinternals.h"


#define ON     (1)
#define OFF    (0)

#define CUSTOM_ENV (0) /* include headers for HAL and compiler in kenv.h */
/* and set this macro to 1                        */


/******************************************************************************/
/*** [Time Quantum] ***********************************************************/
/******************************************************************************/
/*** The time in milliseconds between one tick and another.                   */
/*** Remember the lower this value, the higher the overhead.                  */
/*** Getting this number right is crucial. I recommend 5ms as a default, any  */
/*** thing less than that might be counter-productive and error-prone.        */

#define K_DEF_TICK_PERIOD            (TICK_5MS)


/******************************************************************************/
/*** [User assigned tasks] ****************************************************/
/******************************************************************************/
/*** Define the number of user assigned tasks here. Any errouneous behaviour  */
/*** such as a task never running or hard faults when resuming, this is the   */
/*** first parameter to re-check. If correct, you start wondering about stack */
/*** size.                                                                    */

#define K_DEF_N_USRTASKS    	     (3)

/*** The lowest effective priority, that is the highest value for user-defined*/
/**** tasks                                                                   */

#define K_DEF_MIN_PRIO	           	 (3)


/******************************************************************************/
/*** [Time-Slice] *************************************************************/
/******************************************************************************/

/*** Scheduler with time-slice.                                               */
/*** If OFF, time-slice value assigned when creating a task will be ignored   */
/*** but the API does not change                                              */
/*** If ON, keep in mind 0 and 1 as slice will have the same effect: 1 time   */
/*** for the task                                                             */

#define K_DEF_SCH_TSLICE			 (OFF)


/******************************************************************************/
/*** [App Timers] *************************************************************/
/******************************************************************************/
/*** Number of application timers. If all your tasks use the kSleep(t)        */
/*** primitive, a safe number is (N tasks + 1)                                */

#define K_DEF_N_TIMERS                (K_DEF_N_USRTASKS+1)


/******************************************************************************/
/*** [Allocator] *************************************************************/
/******************************************************************************/
/*** This macro will round-up to a multiple of 4 bytes any memory blocks      */
/*** you will manage with thea allocator                                      */

#define K_DEF_MEMBLOCK_ALIGN_4        (ON)

/******************************************************************************/
/*** [Semaphores] *************************************************************/
/******************************************************************************/

#define K_DEF_SEMA                    (ON)

#if (K_DEF_SEMA==ON)

#if (DEADCODE) /* TBI */
/*** When a semaphore blocks a task it enqueues it on its control queue.      */
/*** This enqueue can be ordered by priority or on a FIFO                     */
/*** This option for a FIFO might alleviate any low priority tasks from       */
/*** starving.                                                                */

#define K_DEF_SEMA_ENQ_PRIO  (1)
#define K_DEF_SEMA_ENQ_FIFO  (2)

#define K_DEF_SEMA_ENQ       (0)
#endif

#endif

/******************************************************************************/
/*** [Mutexes] ****************************************************************/
/******************************************************************************/
#define K_DEF_MUTEX                   (OFF)


/******************************************************************************/
/*** [Slee/Wake Events] *******************************************************/
/******************************************************************************/

#define K_DEF_SLEEPWAKE               (ON)

/******************************************************************************/
/*** [Pipes] ******************************************************************/
/******************************************************************************/
#if (K_DEF_SLEEPWAKE==ON)

#define K_DEF_PIPE                    (ON)  /* Pipes ON/OFF */
#define K_DEF_PIPE_SIZE               (32) /* Pipe size (bytes) */

#endif

/******************************************************************************/
/*** [Indirect (Blocking) Message Queue] **************************************/
/******************************************************************************/
/*** Indirect Blocking Message Queues depend on semaphore primitives          */
#if (K_DEF_SEMA==ON)

#define K_DEF_MESGQ 			      (OFF)

#if (K_DEF_MESGQ == ON)

/*** Number of system mesg records: this number might more or equal to sum   */
/*** of all your message queue sizes                                         */

#define K_DEF_N_MESG                  (32)

/*** You can choose if a sender blocks or not a full queue. If not, messages  */
/*** not delivered will be overwritten starting on the tail                   */
#define K_DEF_MESGQ_BLOCK_FULL        (ON)

#endif /*mesgq*/
#endif

/******************************************************************************/
/*** [Indirect Blocking (Synch) Mailbox] **************************************/
/******************************************************************************/
#define K_DEF_MBOX	                  (ON)   /* Synch Mailbox ON/OFF */

/******************************************************************************/
/*** [Indirect Asynch Mailbox] ************************************************/
/******************************************************************************/
#define K_DEF_AMBOX                   (ON)

/******************************************************************************/
/*** [Pump-Drop Queues] *******************************************************/
/******************************************************************************/
#define K_DEF_PDQ                     (ON)

/******************************************************************************/
/*** [MISC] *******************************************************************/
/******************************************************************************/
/*** Turn ON if you configured a _write() function for using with kprintf()   */
#define K_DEF_PRINTF                  (OFF)

/*[EOF]*/

#endif /* KCONFIG_H */
