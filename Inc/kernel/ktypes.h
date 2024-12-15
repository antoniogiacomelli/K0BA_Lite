/******************************************************************************
 *
 * [K0BA - Kernel 0 For Embedded Applications] | [VERSION: 0.3.0]
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
typedef void                     VOID;
typedef char                     CHAR;
typedef unsigned char            BYTE;

/*** typedef (there is no harm) the stdint for "stylistic"                  */
/*** readability purposes: both never a consensus.                          */
/*** still, K0BA needs you provide a stdint compatible to your environment  */
/*** and you can use the stdint for user-defined code                       */
typedef int8_t                   INT8;
typedef int32_t                  INT32;
typedef uint32_t                 UINT32;
typedef uint64_t                 UINT64;
typedef int64_t                  INT64;
typedef int16_t                  INT16;
typedef uint16_t                 UINT16;
typedef size_t                   SIZE;


/*** if you dont provide a stdbool                                          */
#if !defined(bool)
typedef unsigned int  BOOL
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
typedef void*         ADDR;   /* Generic address type  */
typedef char const*   STRING; /* Read-only String alias  */

typedef unsigned char PID; /* System defined Task ID type */
typedef unsigned char TID; /* User defined Task ID */
typedef unsigned char PRIO; /* Task priority type */
typedef unsigned int  TICK; /* Tick count type */

/*** Func ptrs typedef */
typedef void (*TASKENTRY)(void); /* Task entry function pointer */
typedef void (*CALLOUT)(void*); /* Callout (timers)             */
typedef void (*CBK)(void*); /* Generic Call Back             */

/**
 *\brief Return values
 */
typedef enum kErr
{
/*TODO: TRIM RET VALS FOR EACH KERNEL OBJECT:*/
/*TODO: PRUNE UNUSED RETURNS				*/

    /* SUCCESSFUL: 0U */
    K_SUCCESS              = 0U, /* No Error */

    /* INFO OR UNSUCCESSFUL-NON-FAULTY RETURN VALUES: positive   */
    K_ERR_ATTMPT_TIMEOUT   = 0x1,
    K_ERR_AMBOX_FULL       = 0x2, /* Writing to a nonblocking full mailbox */
    K_ERR_AMBOX_EMPTY      = 0x3, /* Reading from a nonblocking empty mailbox  */
    K_QUERY_MBOX_EMPTY     = 0x4,
    K_QUERY_MBOX_FULL      = 0x5,
    K_QUERY_MUTEX_LOCKED   = 0x6,
    K_QUERY_MUTEX_UNLOCKED = 0x7,

    /* FAULTY RETURN VALUES: negative */
    K_ERROR 			  	    = (int)0xFFFFFFFF, /* (0xFFFFFFFF) Generic error placeholder */
    K_ERR_OBJ_NULL 		 	    = (int)0xFFFFFFFE, /* A null object was passed as a parameter */
    K_ERR_LIST_ITEM_NOT_FOUND   = (int)0xFFFFFFFD, /* Item not found on a K_LIST */
    K_ERR_LIST_EMPTY 		    = (int)0xFFFFFFFC, /* Empty list */
    K_ERR_MESG_SIZE 		    = (int)0xFFFFFFFB, /* Invalid message size */
    K_ERR_MEM_INIT 			    = (int)0xFFFFFFFA, /* Error initialising memory control block */
    K_ERR_MEM_FREE 			    = (int)0xFFFFFFF9, /* Error when freeing an allocated memory */
    K_ERR_MEM_ALLOC 		    = (int)0xFFFFFFF8, /* Error allocating memory */
    K_ERR_INVALID_TID 		    = (int)0xFFFFFFF7, /* Invalid user-assigned task IDs are 0 or 255*/
    K_ERR_INVALID_Q_SIZE 	    = (int)0xFFFFFFF6, /* Maximum message queue size is 255 items */
    K_ERR_INVALID_QMESG_SIZE    = (int)0xFFFFFFF5, /* Maximum message for a message queue is 255 bytes */
    K_ERR_INVALID_BYTEPOOL_SIZE = (int)0xFFFFFFF4, /* Maximum byte pool size is 255 bytes (254 effective) */
    K_ERR_OBJ_NOT_INIT 			= (int)0xFFFFFFF3, /* Tried to use an unitialised kernel object */
    K_ERR_SYS_MESG_GET 		    = (int)0xFFFFFFF2, /* Could not get a mesg buffer from the system pool */
    K_ERR_SYS_MESG_PUT 			= (int)0xFFFFFFF1, /* Could not return a mesg buffer to the system pool */
    K_ERR_MEM_CPY 				= (int)0xFFFFFFF0, /* Error when copying a chunk of bytes from one addr to other */
    K_ERR_INVALID_PRIO 			= (int)0xFFFFFFEF, /* Valid task priority range: 0-31. */
    K_ERR_MUTEX_NOT_LOCKED 		= (int)0xFFFFFFEE, /* Tried to lock an unlocked mutex */
    K_ERR_MUTEX_NOT_OWNER 		= (int)0xFFFFFFED, /* Tried to unlock an owned mutex */
    K_ERR_RESERVED 				= (int)0xFFFFFFE8, /* 'reserved' often is used so design mistakes seem upcoming features */
    K_ERR_MESG_CPY 				= (int)0xFFFFFFE5, /* Failure when copying (recv/send) mesg/mail */
    K_ERR_PDBUF_SIZE 			= (int)0xFFFFFFE4, /* Invalid size of mesg attached to a PD Buffer */
    K_ERR_SEM_INVALID_VAL 		= (int)0xFFFFFFE3, /* Invalid semaphore value */
    K_ERR_QUERY_UNDEFINED 		= (int)0xFFFFFFE2, /* Undefined query state for a kobj */
    K_ERR_TASK_NOT_RUNNING 		= (int)0xFFFFFFE1,
    K_ERR_INVALID_TSLICE 		= (int)0xFFFFFFE0,
	K_ERR_MESGQ_NO_BUFFER 		= (int)0xFFFFFFD9, /* Trying to send/recv copy from a queue with
													  unknown buffering address */
	K_ERR_MBOX_INIT 			= (int)0xFFFFFFD8,
	K_ERR_NAMED_MBOX_SEND_SELF  = (int)0xFFFFFFD7, /* named channel cant sent do itself */
	K_ERR_NAMED_MBOX_RECV		= (int)0xFFFFFFD6, /* a named mbox can only accept recv its assigned task */
	K_ERR_MBOX_EMPTY			= (int)0xFFFFFFD5,  /* isr read from an empty mailbox */
	K_ERR_MBOX_FULL				= (int)0xFFFFFFD4,
	K_ERR_MBOX_SIZE				= (int)0xFFFFFFE3,
	K_ERR_MBOX_ISR				= (int)0xFFFFFFE2,

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

typedef enum
{
    MBOX_EMPTY =    0,
    MBOX_FULL  =    1
}K_MBOX_STATUS;

typedef enum
{
    MESGQ_EMPTY 	=    0,
	MESGQ_PARTIAL   =    1,
    MESGQ_FULL		=    2
}K_MESGQ_STATUS;

typedef enum
{
	MAILBOX,
	AMAILBOX,
	MESGQUEUE,
	PDQUEUE,
	PIPE
}K_CHAN_TYPE;

/**
 * \brief Task status
 */
typedef enum kTaskStatus
{
    INVALID = 0,
    /*---------------------------------------------------------*/
    READY,
    /*---------------------------------------------------------*/
    RUNNING,
    /*------------------------WAITING--------------------------*/
    PENDING,
    SLEEPING,
    BLOCKED,
    SENDING,
    RECEIVING,
    SUSPENDED,
    /*---------------------------------------------------------*/
    ATTEMPTING /*** Trying to acquire a resource.              */

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

#if (K_DEF_AMBOX == ON)

    typedef struct kAsynchMbox K_AMBOX;

#endif /* ambox */

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
