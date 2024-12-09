/******************************************************************************
 *
 * [K0BA - Kernel 0 For Embedded Applications] | [VERSION: 0.3.0]
 *
 ******************************************************************************/

/**
 * \file     kapi.h
 * \brief    Kernel API
 * \version  0.3.0
 * \author   Antonio Giacomelli
 *
 *
 * \verbatim
 *  ________________________________________
 * |                                        |--
 * | [TASK1][TASK2]  ....         [TASKN]   | application.c
 * |----------------------------------------|--
 * |                 API                    |
 * |----------------------------------------|--
 * |                                        |
 * |             K0BA KERNEL                | k*.h k*.c
 * |________________________________________|--
 * |                                        |
 * |        BOARD SUPPORT PACKAGE           |
 * |________________________________________|
 * |                                        |
 * |                CMIS HAL                |
 * |________________________________________|--
 *
 * This is the kernel API to be included within any application
 * development. It provides methods to access the kernel services.
 *
 * By default, it is placed in "application.h"
 *
 *
 **/

#ifndef KAPI_H
#define KAPI_H

#include "kconfig.h"

#if (CUSTOM_ENV==1)
#include "kenv.h"
#endif

#include "ktypes.h"
#include "kobjs.h"
#include "kversion.h"

/******************************************************************************/

/**
 * \brief Create a new task. Before using you need to statically allocate an
 *        array of (unsigned) integers visible to the function call scope.
 *        Normally you will declare the array in the application file and
 *        declare it as an extern for the main function scope, or vice-versa.
 *
 * \param taskFuncPtr  Pointer to the task entry function.
 *
 * \param taskName     Task name. Keep it as much as 8 Bytes.
 *
 * \param id           user-defined Task ID - valid range: 1-254
 *                     (0 and 255 are reserved).
 * \param stackAddrPtr Pointer to the task stack (the array variable).
 *
 * \param stackSize    Size of the task stack (in WORDS. 1WORD=4BYTES)
 *
 * \param timeSlice    UNSIGNED integer for time-slice.
 * 					   If time-slice is ON, value 0 is invalid.
 *
 *
 * \param priority     Task priority - valid range: 0-31.
 *
 * \param runToCompl   If this flag is 'TRUE',  the task once dispatched
 *                     although can be interrupted by tick and other hardware
 *                     interrupt lines, won't be preempted by user tasks.
 *                     runToCompl tasks are normally the top half of interrupt
 *                     handlers that are not urgent enough to bypass the scheduler,
 *                     and can be deferred, still taking precedence above every other
 *                     user task.
 *
 * \return K_SUCCESS on success, K_ERROR on failure
 */
K_ERR kCreateTask(TASKENTRY const taskFuncPtr,
		STRING taskName,
		TID const id,
        UINT32* const stackAddrPtr,
		UINT32 const stackSize,
#if(K_DEF_SCH_TSLICE==ON)
        TICK const timeSlice,
#endif
		PRIO const priority,
		BOOL const runToCompl);

/**
 * \brief Yields the current task.
 *
 */
VOID kYield(VOID);

/*******************************************************************************
 SEMAPHORES
 *******************************************************************************/
#if (K_DEF_SEMA==ON)
/**
 *\brief       Initialise a semaphore
 *\param kobj  Semaphore address
 *\param value Initial value
 *\return      \see ktypes.h
 */

K_ERR kSemaInit(K_SEMA* const kobj, INT32 const value);

/**
 *\brief Wait on a semaphore
 *\param kobj Semaphore address
 */
VOID kSemaWait(K_SEMA* const kobj);

/**
 *\brief Signal a semaphore
 *\param kobj Semaphore address
 */
VOID kSemaSignal(K_SEMA* const kobj);

/**
 * \brief           Tries to acquire a semaphore for N ticks
 *
 * \param kobj      Semaphore object
 * \param nAttmpts  Number of ticks
 * \return          K_SUCCESS or K_ERR_ATTMPT_TIMEOUT
 */
K_ERR kSemaAttmpt(K_SEMA* const kobj, TICK nAttmpts);
/**
 * \brief           Returns the semaphore counter value
 *
 * \param kobj      Semaphore address
 * \return          Semaphore counter value
 */
INT32 kSemaQuery(K_SEMA* const kobj);

#endif
/*******************************************************************************
 * MUTEX
 *******************************************************************************/
#if (K_DEF_MUTEX==ON)
/**
 *\brief Init a mutex
 *\param kobj mutex address
 *\return K_SUCCESS / K_ERROR
 */

K_ERR kMutexInit(K_MUTEX* const kobj);

/**
 *\brief Lock a mutex
 *\param kobj mutex address
 *\return K_SUCCESS or a specific error \see ktypes.h
 */
VOID kMutexLock(K_MUTEX* const kobj);

/**
 *\brief Unlock a mutex
 *\param kobj mutex address
 */
VOID kMutexUnlock(K_MUTEX* const kobj);

/**
 * \brief Return the state of a mutex (locked/unlocked)
 */
K_ERR kMutexQuery(K_MUTEX* const kobj);

#endif

/******************************************************************************/
/* MESSAGE QUEUE                                                              */
/******************************************************************************/
#if (K_DEF_MESGQ == ON)

/**
 * \brief  Initialise an indirect blocking message queue.

 * \param kobj          Pointer to the message queue kernel object.
 * \param mesgPoolPtr   Address of the allocated memory for the message
 *                      contents.
 * \param queueSize     Number of items the queue support before blocking full.
 * \param mesgSize      Size of each item/message.
 * \return              See ktypes.h
 */
K_ERR kMesgQInit(K_MESGQ* const kobj, ADDR const mesgPoolPtr,
        BYTE const queueSize, BYTE const mesgSize);


/**
 * \brief            Send a message to the queue. The contents will be copied
 *                   to a queue buffer.
 * \param kobj       Address of the queue kernel object.
 * \param mesgPtr    Address of the message to be sent. Beware the scope.
 * \param mesgSize   Size of the message - non-zero, up to the value you decla
 *                   red.
 * \return           See ktypes.h
 */
K_ERR kMesgQSend(K_MESGQ* const kobj, ADDR const mesgPtr, BYTE const mesgSize);


/**
 * \brief              Receive a copy of a message from a queue.
 *
 * \param kobj         Message Queue address.
 * \param recvMesgPtr  Pointer to where the message will be copied.
 * \param senderTIDPtr Pointer to the variable to store the sender task id.
 * \return             See ktypes.h
 */
K_ERR kMesgQRecv(K_MESGQ* const kobj, ADDR recvMesgPtr, TID* senderTIDPtr);

/**
 * \brief                 Send a message copy to the queue front.
 * \param kobj            Message Queue address.
 * \param mesgPtr		  Message addr.
 * \param mesgSize        Size.
 * \return                See ktypes.h
 */

K_ERR kMesgQJam(K_MESGQ* const kobj, ADDR const mesgPtr, BYTE const mesgSize);



#endif /*K_DEF_MESGQ*/

/*******************************************************************************
 * SYNCH MAILBOX
 *******************************************************************************/

#if (K_DEF_MBOX == ON)

/**
 * \brief               Initialises an indirect blocking mailbox.
 *
 * \param kobj          Mailbox address.
 * \param mailSize      Size of the message it will store.
 * \param initMailPtr   If initialised full, give the address of the initial
 *                      message.
 * \return              See ktypes.h
 */
K_ERR kMboxInit(K_MBOX* const kobj, SIZE mailSize,
        ADDR initMailPtr);

/**
 * \brief               Post to a mailbox. Task blocks when full.
 * \param kobj          Mailbox address.
 * \param sendPtr       Message adddress.
 * \return              See ktypes.h
 */
K_ERR kMboxPost(K_MBOX* const kobj, ADDR const sendPtr);

/**
 * \brief               Pend on a mailbox to receive a message. Block if empty.
 *
 * \param kobj          Mailbox address.
 * \param recvPPtr      Pointer to copy the message.
 * \return              See ktypes.h
 */
TID kMboxPend(K_MBOX* const kobj, ADDR* recvPtr);

/**
 * \brief       		Return status of a mailbox: full, empty.
 *
 */
K_MBOX_STATUS kMboxQuery(K_MBOX* const kobj);

/**
 * \brief 				Return the size set for a message within a mailbox, if any.
 */
SIZE kMboxGetSize(K_MBOX *const kobj);

#endif

#if (K_DEF_AMBOX==ON)

/**
 * \brief               Asynchronous mailboxes are meant to be flexible. They
 *                      can be employed 'as is' or the user can create the synch
 *                      mechanisms, and use it on ISRs.
 * \param kobj          Asynch Mailbox address.
 * \param initMailPtr   If you provide a pointer for a message, the mailbox will
 *                      be initialised as 'full'.
 * \param initMailSize  If you provide a initial pointer to a message you need
 *                      to provide its size.
 * \return              See ktypes.h
 */
K_ERR kAmboxInit(K_AMBOX* const kobj, ADDR initMailPtr, BYTE initMailSize);
/**
 * \brief               Send a message to an asynchronous mailbox.

 * \param kobj          Asynch mailbox address.
 * \param sendPtr       Message address.
 * \param nMailSize     Message size.
 * \return              It will return indicating if the message was sent or
 *                      if the mailbox was full and the message was not success
 *                      fuly placed.
 */
K_ERR kAmboxSend(K_AMBOX* const kobj, ADDR const sendPtr, BYTE nMailSize);
/**
 * \brief               Receive from an asynchronous mailbox. It copies the
 *                      message to a buffer provided by the receiver.
 *                      The message in box is destroyed afterwards.
 * \param kobj          Asynch mailbox address.
 * \param recvPtr       Address to store the message.
 * \return              It will return indicating if the message could have
 *                      been received or if the mailbox was empty.
 */
K_ERR kAmboxRecv(K_AMBOX* const kobj, ADDR const recvPtr);
/**
 * \brief               Receive from an asynchronous mailbox. It copies the
 *                      message to a buffer provided by the receiver, but
 *                      KEEP the message in the box. So, the box will still
 *                      be full.
 * \param kobj          Asynch mailbox address.
 * \param recvPtr       Address to store the message.
 * \return              It will return indicating if the message could have
 *                      been received or if the mailbox was empty.
 */
K_ERR kAmboxRecvKeep(K_AMBOX* const kobj, ADDR const recvPtr);

/**
 * \brief				Send to an asynchronous mailbox, overwriting the
 * 						current message.
 * \param kobj		    Asynch mailbox address.
 * \param sendPtr       Message address.
 * \param nMailSize     Message size.
 * \return				K_SUCCESS or an error in case sendPtr is NULL or
 * 						aMailSize is 0.
 */
K_ERR kAmboxSendOvw(K_AMBOX *const kobj, ADDR const sendPtr, BYTE aMailSize);

#endif

/*******************************************************************************
 * PUMP-DROP QUEUE (CYCLIC ASYNCHRONOUS BUFFERS - CABs)
 *******************************************************************************/

#if (K_DEF_PDQ == ON)

/**
 * \brief          Pump-drop queue initialisation.
 *
 * \param kobj     PD queue address.
 * \param bufPool  Pool of PD Buffers, statically allocated.
 * \param nBufs    Number of buffers for this queue.
 * \return         see ktypes.h
 */
K_ERR kPDQInit(K_PDQ* const kobj, K_PDBUF* bufPool, BYTE nBufs);

/**
 * \brief          Reserves a pump-drop buffer before writing on it.
 *
 * \param kobj     Queue address.
 * \return         see ktypes.h
 */
K_PDBUF* kPDQReserve(K_PDQ* const kobj);


/**
 * \brief           Writes into a PD buffer the source address and the size
 *                  of a data message.
 *
 * \param bufPtr    Buffer address.
 * \param srcPtr    Message address.
 * \param dataSize  Message size.
 * \return
 */
K_ERR kPDBufWrite(K_PDBUF* bufPtr, ADDR srcPtr, SIZE dataSize);

/**
 * \brief          Pump a buffer into the queue - make it available for readers.
 *
 * \param kobj     Queue address.
 * \return         Address of a written PD buffer.
 */
K_ERR kPDQPump(K_PDQ* const kobj, K_PDBUF* bufPtr);

/**
 * \brief          Fetches the most recent buffer pumped in the queue.
 *
 * \param kobj     Queue address.
 * \return         Address of a PD buffer available for reading.
 */
K_PDBUF* kPDQFetch(K_PDQ* const kobj);

/**
 * \brief          Copies the message from a PD Buffer to a chosen address.
 *
 * \param bufPtr   Address of the PD buffer.
 * \param destPtr  Address that will store the message.
 * \return         see ktypes.h
 */
K_ERR kPDBufRead(K_PDBUF* bufPtr, ADDR destPtr);

/**
 * \brief         Called by reader to indicate it has consumed the
 *                message from a buffer. If there are no more readers
 *                using the buffer, and it is not the last buffer pumped
 *                in the queue, it will be reused.
 *
 * \param kobj    Queue address;
 * \return        see ktypes.h
 */
K_ERR kPDQDrop(K_PDQ* const kobj, K_PDBUF* bufPtr);


#endif

/*******************************************************************************
 * PIPES
 *******************************************************************************/

#if (K_DEF_PIPE==ON)

/**
 *\brief Initialise a pipe
 *\param kobj Pointer to the pipe
 */
K_ERR kPipeInit(K_PIPE* const kobj);

/**
 *\brief Read a stream of bytes from a pipe
 *\param kobj Pointer to a pipe
 *\param destPtr Address to store the read data
 *\param nBytes Number of bytes to be read
 *\retval Number of read bytes if success. -1 if fails.
 */
UINT32 kPipeRead(K_PIPE* const kobj, BYTE* srcPtr, UINT32 nBytes);

/**
 *\brief Write a stream of bytes to a pipe
 *\param kobj Pointer to a pipe
 *\param srcPtr Address to get data
 *\param nBytes Number of bytes to be write
 *\retval Number of written bytes if success. -1 if fails.
 */
UINT32 kPipeWrite(K_PIPE* const kobj, BYTE* srcPtr, UINT32 nBytes);

#endif /*K_DEF_PIPES*/

/******************************************************************************
 * DIRECT TASK SIGNAL
 ******************************************************************************/
/**
 * \brief A caller task goes to a PENDING state, waiting for a kSignal.
 */
VOID kPend(VOID);

/**
 * \brief A task suspends another task. The task goes to a SUSPENDED state.
 *
 */
VOID kSuspend(TID const taskID);

/**
 * \brief Direct Signal a task either PENDING or SUSPENDED. Target task is resu
 *        med.
 * \param taskID The ID of the task to signal
 */
VOID kSignal(PID const taskID);

/******************************************************************************
 * SLEEP/WAKE-UP ON EVENTS
 ******************************************************************************/

#if (K_DEF_SLEEPWAKE==ON)

/**
 * \brief Suspends a task waiting for a specific event
 * \param kobj Pointer to a K_EVENT object
 */
VOID kEventSleep(K_EVENT* const kobj);

/**
 * \brief Wakes a task waiting for a specific event
 * \param kobj Pointer to a K_EVENT object
 * \return K_SUCCESS/K_ERROR
 */
VOID kEventWake(K_EVENT* const kobj);

/**
 * \brief  Return the number of tasks sleeping on an event.
 */
UINT32 kEventQuery(K_EVENT* const kobj);

#endif

/*******************************************************************************
 * APPLICATION TIMER AND DELAY
 ******************************************************************************/
/**
 * \brief Initialises an application timer
 * \param timerName a STRING (const char*) label for the timer
 * \param ticks initial tick count
 * \param funPtr The callback when timer expires
 * \param argsPtr Address to callback function arguments
 * \param reload TRUE for reloading after timer-out. FALSE for an one-shot
 * \return K_SUCCESS/K_ERROR
 */

/* Timer Reload / Oneshot optionss */
#define RELOAD      1
#define ONESHOT     0

K_ERR kTimerInit(STRING timerName, TICK const ticks, CALLOUT const funPtr,
        ADDR const argsPtr, BOOL const reload);

/**
 * \brief Busy-wait the task for a specified delay in ticks.
 *        Task does not suspend.
 * \param delay The delay time in ticks
 */
VOID kBusyDelay(TICK const delay);
/**
 * \brief Put the current task to sleep for a number of ticks.
 *        Task switches to SLEEPING state.
 * \param ticks Number of ticks to sleep
 */
VOID kSleep(TICK const ticks);


/**
 * \brief Gets the current number of  ticks
 * \return Global system tick value
 */
TICK kTickGet(VOID);

/*******************************************************************************
 * BLOCK MEMORY POOL
 ******************************************************************************/

/**
 * \brief Memory Pool Control Block Initialisation
 * \param kobj Pointer to a pool control block
 * \param memPoolPtr Address of a pool (typically an array) \
 * 		  of objects to be handled
 * \param blkSize Size of each block in bytes
 * \param numBlocks Number of blocks
 * \return K_ERROR/K_SUCCESS
 */
K_ERR kMemInit(K_MEM* const kobj, ADDR const memPoolPtr,
        BYTE blkSize, BYTE const numBlocks);

/**
 * \brief Allocate memory from a block pool
 * \param kobj Pointer to the block pool
 * \return Pointer to the allocated block, or NULL on failure
 */
ADDR kMemAlloc(K_MEM* const kobj);

/**
 * \brief Free a memory block back to the block pool
 * \param kobj Pointer to the block pool
 * \param blockPtr Pointer to the block to free
 * \return Pointer to the allocated memory. NULL on failure.
 */
K_ERR kMemFree(K_MEM* const kobj, ADDR const blockPtr);

/*******************************************************************************
 * MISC
 ******************************************************************************/
/**
 *\brief Gets a task system ID
 *\param taskID user-defined ID
 *\return Task system ID
 */
TID kGetTaskPID(TID const taskID);
/**
 * \brief Gets a task priorirty
 * \param taskID user-defined Task ID
 */
PRIO kGetTaskPrio(TID const taskID);

/**
 * \brief Returns the size of a string
 * \param string A pointer to char
 * \return string length - 1 (does not count '\0')
 */
unsigned int kGetVersion(void);
SIZE kMemCpy(ADDR destPtr, ADDR const srcPtr, SIZE size);
/*
 * brief Macro for unused variables
 */
#if !defined(UNUSED)
#define UNUSED(x) (void)x
#endif

/*[EOF]*/

#endif /* KAPI_H */
