/******************************************************************************
 *
 * [K0BA - Kernel 0 For Embedded Applications] | [VERSION: 0.3.0]
 *
 ******************************************************************************
 ******************************************************************************
 * 	Module           : Tracer
 * 	Provides to      : Every other module can use this service.
 *  Public API       : Yes
 *
 * 	In this unit:
 *
 * 		o Tracer/Logger
 *
 *****************************************************************************/


#if DEADCODE
#define K_DEF_TRACE          ON         /* Trace ON/OFF      */
#if (K_DEF_TRACE==ON)
#define K_DEF_TRACEBUFF_SIZE      64   /* Trace Buffer size*/
#define K_DEF_TRACE_MAX_CHAR      16    /* Max info bytes  */
#define K_DEF_TRACE_NO_INFO       OFF   /* No custom info on trace buffer */
#endif

#define K_CODE
#include "kconfig.h"
#include "ktypes.h"
#include "kobjs.h"
#include "kapi.h"
#include "kglobals.h"
static BOOL first;

#if (K_DEF_TRACE==ON)
K_TRACE kTracer;
void kTraceInit(void)
{
	kTracer.head = 0;
	kTracer.tail = 0;
	kTracer.nAdded = 0;
	kTracer.nWrap = 0;
	first= TRUE;
	for (UINT32 i = 0; i < K_DEF_TRACEBUFF_SIZE; ++i)
	{
		kTracer.buffer[i].event = 0;
		kTracer.buffer[i].timeStamp = 0;
#if (K_DEF_TRACE_NO_INFO==OFF)
		kTracer.buffer[i].info='\0';
#endif
	}
}
K_ERR kTrace(K_TRACEID event, CHAR *info)
{
	K_CR_AREA;
	K_ENTER_CR
	;
	if (!first)
	{
		if (kTracer.buffer[kTracer.tail].timeStamp == kTracer.buffer[kTracer.tail - 1].timeStamp)
		{
			K_EXIT_CR;
			return 0;
		}
	}
	kTracer.buffer[kTracer.tail].event = event;
	kTracer.buffer[kTracer.tail].timeStamp = kTickGet();
	kTracer.buffer[kTracer.tail].pid = runPtr->pid;
#if (K_DEF_TRACE_NO_INFO==OFF)
	kTracer.buffer[kTracer.tail].info = info;
#endif
	kTracer.tail += 1U;
	kTracer.tail %= K_DEF_TRACEBUFF_SIZE;
	kTracer.nAdded += 1U;
	if (kTracer.nAdded == K_DEF_TRACEBUFF_SIZE - 1)
	{
		kTracer.nWrap += 1U;
	}
	first=FALSE;
	K_EXIT_CR
	;
	return K_SUCCESS;
}

void ITM_SendValue(UINT32 value)
{
#if (ITM_ON == 1)
	if ((ITM->TCR & ITM_TCR_ITMENA_Msk) && (ITM->TER & 1)) { // Check if ITM and Port 0 are enabled
		while (ITM->PORT[0].u32 == 0);  // Wait until ITM port is ready
		ITM->PORT[0].u32 = value;       // Write the 32-bit value
	}

#else
	(void) value;
	return;
#endif
}

#endif

#if (K_DEF_TRACE == ON)
typedef enum kTraceID
{
    NONE = 0,
    TIMER_EN,
    TIMER_OUT,
    PREEMPTED,
    PRIORITY_BOOST,
    PRIORITY_RESTORED,
    INTERRUPT,
    TSIGNAL_RCVD,
    TSIGNAL_SENT,
    TPEND_SEMA,
    TPEND_MUTEX,
    TPEND_CVAR,
    TPEND_SLEEP,
    TWAKE,
    TPEND,
    MESG_RCVD,
    MESG_SENT,
    LIST_INSERT,
    LIST_REMOVED,
    REMOVE_EMPTY_LIST
} K_TRACEID;
#endif

#endif /*DEADCODE*/

/*[EOF]*/


