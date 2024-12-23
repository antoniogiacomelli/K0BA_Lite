/******************************************************************************
 *
 * [K0BA - Kernel 0 For Embedded Applications] | [VERSION: 0.3.0]
 *
 ******************************************************************************
 * ##Important Parameters##
 *
 * - **Number of User Tasks:**  (`K_DEF_N_USRTASKS`)
 *
 *    Number of user tasks.
 *
 * - **Lowest effective priority:**      (`K_DEF_N_MINPRIO`)
 *   Priorities range are from `0` to ``K_DEF_N_MINPRIO`.
 *   (0 is highest effective priority)
 *
 * - **Number of timers:**     (`K_DEF_N_TIMERS`)
 *   Consider Number of Tasks + 1
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

/* include headers for HAL and compiler in kenv.h */
/* and set this macro to 1                        */
#define CUSTOM_ENV (1)

/**/
/*** [ Time Quantum ] *********************************************************/
#define K_DEF_TICK_PERIOD            (TICK_5MS)

/**/
/*** [ Number of user-defined tasks ] ******************************************/
#define K_DEF_N_USRTASKS    	     (3)

/**/
/*** [The lowest effective priority, that is the highest user-defined value]  */
#define K_DEF_MIN_PRIO	           	 (2)

/**/
/*** [ Time-Slice Scheduling ]*************************************************/
#define K_DEF_SCH_TSLICE			 (OFF)

/**/
/*** [ App Timers ] ***********************************************************/
#define K_DEF_N_TIMERS                (K_DEF_N_USRTASKS+1)

/**/
/*** [ Semaphores ] ***********************************************************/
#define K_DEF_SEMA                    (ON)

/*** Semaphore enqueing discipline	 */
/*** Options: 					  	 */
/*** (K_DEF_ENQ_PRIO) 		         */

/*** Default: K_DEF_ENQ_PRIO 	     */
#define K_DEF_SEMA_ENQ       (K_DEF_ENQ_PRIO)

/**/
/*** [ Mutexes ] ***************************************************************/
#define K_DEF_MUTEX                   (ON)
#define K_DEF_MUTEX_ENQ				  (K_DEF_ENQ_FIFO)


/**/
/*** [Sleep/Wake Events] *******************************************************/
#define K_DEF_SLEEPWAKE               (ON)

/**/
/*** [ Pipes ] ******************************************************************/

#if (K_DEF_SLEEPWAKE==ON)

#define K_DEF_PIPE                    (ON)
#define K_DEF_PIPE_SIZE               (32)

#endif

/**/

/*** [ Message Queue ] **************************************/
#if (K_DEF_SEMA==ON)

#define K_DEF_MESGQ 			      (ON)

#if (K_DEF_MESGQ == ON)

/*** Synch methods					*/
#define K_DEF_SYNCH_MESGQ				(ON)

#if (K_DEF_SYNCH_MESGQ==ON)

/*** Queue Discipline				*/
#define K_DEF_MESGQ_ENQ	   (K_DEF_ENQ_PRIO)

#endif

/*** Asynch methods					*/
#define K_DEF_ASYNCH_MESGQ		  	(ON)

#endif /*mesgq*/

#endif

/**/
/*** [ Mailbox ] ****************************************************************/
#define K_DEF_MBOX	                  (ON)

/* Synch Mailbox queue discipline
 Default: K_DEF_ENQ_PRIO  		    */
#define K_DEF_MBOX_ENQ       (K_DEF_ENQ_PRIO)

/* Enable asynchronous methods */
#define K_DEF_AMBOX                   (ON)


/**/
/*** [ Pump-Drop Queues ] *******************************************************/

#define K_DEF_PDQ                     (ON)


/**/


/*[EOF]*/

#endif /* KCONFIG_H */
