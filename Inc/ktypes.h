/******************************************************************************
 *
 * [K0BA - Kernel 0 For Embedded Applications] | [VERSION: 0.3.1]
 *
 ******************************************************************************
 * 	In this header:
 * 					o Kernel types definition
 *
 ******************************************************************************/

#ifndef KTYPES_H
#define KTYPES_H

#include "kenv.h"

/**
 * \brief C standard library primitives alias
 */
/*** these are immutable:                                                   */
typedef void VOID;
typedef char CHAR;
typedef unsigned char BYTE;
typedef int INT;	/* stack type */
typedef unsigned int UINT;

/*** typedef (there is no harm) the stdint for "stylistic"                  */
/*** readability purposes: both never a consensus.                          */
/*** still, K0BA needs you provide a stdint compatible to your environment  */
/*** and you can use the stdint for user-defined code                       */
typedef int8_t INT8;
typedef int32_t INT32;
typedef uint32_t UINT32;
typedef uint64_t UINT64;
typedef int64_t INT64;
typedef int16_t INT16;
typedef uint16_t UINT16;
typedef size_t SIZE;

/*** if you dont provide a stdbool                                          */
#if !defined(bool)
typedef unsigned int BOOL
#define false (unsigned int)0U;
#define true  (unsigned int)1U;
#define bool
#endif
typedef _Bool BOOL;
#define TRUE			true
#define FALSE			false

/**
 * \brief Primitve aliases
 */
/*** User-defined Task ID range: 0-255.                         */
/*** Priority range: 0-31 - tasks can have the same priority    */
typedef void *ADDR; /* Generic address type  */
typedef char const *STRING; /* Read-only String alias  */

typedef unsigned char PID; /* System defined Task ID type */
typedef unsigned char TID; /* User defined Task ID */
typedef unsigned char PRIO; /* Task priority type */
typedef unsigned int TICK; /* Tick count type */

/*** Func ptrs typedef */
typedef void (*TASKENTRY)(void); /* Task entry function pointer */
typedef void (*CALLOUT)(void*); /* Callout (timers)             */
typedef void (*CBK)(void*); /* Generic Call Back             */

/**
 *\brief Return values
 */
typedef enum kErr
{

	/* SUCCESSFUL: 0U */
	K_SUCCESS = 0U, /* No Error */

	/* INFO OR UNSUCCESSFUL-NON-FAULTY RETURN VALUES: positive   */
	K_ERR_TIMEOUT = 0x1,
	K_QUERY_MBOX_EMPTY = 0x2,
	K_QUERY_MBOX_FULL = 0x3,
	K_QUERY_MUTEX_LOCKED = 0x4,
	K_QUERY_MUTEX_UNLOCKED = 0x5,
	K_ERR_MBOX_FULL = 0x6,
	K_ERR_MBOX_SIZE = 0x7,
	K_ERR_MBOX_EMPTY = 0x8,
	K_ERR_MBOX_ISR = 0x9,
	K_ERR_MBOX_NO_WAITERS = 0xA,
	K_ERR_MESGQ_FULL = 0xB,
	K_ERR_MESGQ_EMPTY = 0xC,

	/* FAULTY RETURN VALUES: negative */
	K_ERROR = (int) 0xFFFFFFFF, /* (0xFFFFFFFF) Generic error placeholder */

	K_ERR_OBJ_NULL = (int) 0xFFFF0000, /* A null object was passed as a parameter */
	K_ERR_OBJ_NOT_INIT = (int) 0xFFFF0001, /* Tried to use an unitialised kernel object */

	K_ERR_LIST_ITEM_NOT_FOUND = (int) 0xFFFF0002, /* Item not found on a K_LIST */
	K_ERR_LIST_EMPTY = (int) 0xFFFF0003, /* Empty list */

	K_ERR_MEM_INIT = (int) 0xFFFF0004, /* Error initialising memory control block */
	K_ERR_MEM_FREE = (int) 0xFFFF0005, /* Error when freeing an allocated memory */
	K_ERR_MEM_ALLOC = (int) 0xFFFF0006, /* Error allocating memory */

	K_ERR_TIMER_POOL_EMPTY = (int) 0xFFFF0007,

	K_ERR_INVALID_TID = (int) 0xFFFF0008, /* Invalid user-assigned task IDs are 0 or 255*/
	K_ERR_INVALID_PRIO = (int) 0xFFFF0009, /* Valid task priority range: 0-31. */


	K_ERR_INVALID_QUEUE_SIZE = (int) 0xFFFF000A, /* Maximum message queue size is 255 items */
	K_ERR_INVALID_MESG_SIZE = (int) 0xFFFF000B, /* Maximum message for a message queue is 255 bytes */
	K_ERR_MESGQ_NO_BUFFER = (int) 0xFFFF0012, /* Trying to send/recv copy from a queue with
		 unknown buffering address */
	K_ERR_MESG_CPY = (int) 0xFFFF000C, /* Error when copying a chunk of bytes from one addr to other */

	K_ERR_PDBUF_SIZE = (int) 0xFFFF000D, /* Invalid size of mesg attached to a PD Buffer */


	K_ERR_SEM_INVALID_VAL = (int) 0xFFFF000E, /* Invalid semaphore value */

	K_ERR_TASK_NOT_RUNNING = (int) 0xFFFF0010,
	K_ERR_INVALID_TSLICE = (int) 0xFFFF0011,


	K_ERR_MBOX_INIT_MAIL = (int) 0xFFFF0013,
	K_ERR_MUTEX_REC_LOCK = (int) 0xFFFF0014,
	K_ERR_CANT_SUSPEND_PRIO = (int) 0xFFFF0015

} K_ERR;

/**
 *\brief Fault codes
 */
typedef enum kFault
{
	FAULT = 0x00,
	FAULT_READY_QUEUE = 0x01,
	FAULT_NULL_OBJ = 0x02,
	FAULT_LIST = 0x03,
	FAULT_KERNEL_VERSION = 0x04,
	FAULT_QUEUE_ROOM = 0x05,
	FAULT_QUEUE_MESG_CPY = 0x06,
	FAULT_QUEUE_MEM_FREE = 0x07,
	FAULT_QUEUE_LIST_ADD = 0x08,
	FAULT_QUEUE_LIST_REM = 0x09,
	FAULT_SYS_MESG_GET = 0x0A,
	FAULT_SYS_MESG_PUT = 0x0B,
	FAULT_MEM_ALLOC = 0x0C,
	FAULT_MEM_FREE = 0x0D,
	FAULT_OBJ_NOT_INIT = 0x0E,
	FAULT_MEM_CPY = 0x0F,
	FAULT_INVALID_TASK_PRIO = 0x1F,
	FAULT_INVALID_TASK_ID = 0x2F,
	FAULT_OBJ_INIT = 0x3F,
	FAULT_SYSMESG_N_EXCEEDED = 0x4F,
	FAULT_UNLOCK_OWNED_MUTEX = 0x5F,
	FAULT_ISR_INVALID_PRIMITVE = 0x6F,
	FAULT_TASK_INVALID_STATE = 0x7F,
	FAULT_INVALID_SVC = 0xFF
} K_FAULT;

/**
 * \brief Task status
 */
typedef enum kTaskStatus
{
	INVALID = 0, READY, RUNNING,
	/* WAITING */
	PENDING, SLEEPING, BLOCKED, SUSPENDED, SENDING, RECEIVING

} K_TASK_STATUS;

typedef struct kTcb K_TCB;
typedef struct kTimer K_TIMER;
typedef struct kMemBlock K_MEM;
typedef struct kList K_LIST;
typedef struct kListNode K_LISTNODE;
typedef K_LIST K_TCBQ;

#if (K_DEF_SEMA == ON)

typedef struct kSema K_SEMA;
#endif /*sema */

#if (K_DEF_MESGQ == ON)

typedef struct kMesgQ K_MESGQ;

#endif /*mesgq*/

#if (K_DEF_MBOX == ON)

typedef struct kMailbox K_MBOX;

#endif /* mbox */

#if (K_DEF_SLEEPWAKE==ON)

typedef struct kEvent K_EVENT;

#if (K_DEF_PIPE==ON)

typedef struct kPipe K_PIPE;

#endif

#endif

#if (K_DEF_PDQ== ON)

typedef struct kPumpDropBuf K_PDBUF;
typedef struct kPumpDropQueue K_PDQ;

#endif

#if (K_DEF_MUTEX == ON)

typedef struct kMutex K_MUTEX;

#endif

#endif/*ktypes*/
