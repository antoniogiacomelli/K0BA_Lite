/******************************************************************************
 *
 * [K0BA - Kernel 0 For Embedded Applications] | [VERSION: 0.1.0]
 *
 ******************************************************************************
 ******************************************************************************
 * 	In this unit:
 *
 * 		o Tracer/Logger
 *
 *****************************************************************************/


#include <kapi.h>
#if (K_CONF_TRACE==ON)
K_TRACE kTracer;
void kTraceInit(void)
{
	kTracer.head		= 0;
	kTracer.tail		= 0;
	kTracer.nAdded		= 0;
	kTracer.nWrap		= 0;

	for(UINT32 i=0; i<TRACEBUFF_SIZE; ++i)
	{
		kTracer.buffer[i].event=0;
		kTracer.buffer[i].timeStamp=0;
		kTracer.buffer[i].info='\0';
	}
}

K_ERR kTrace(K_TRACEID event, CHAR* info)
{
	K_CR_AREA;
	K_ENTER_CR;

	kTracer.buffer[kTracer.tail].event=event;
	kTracer.buffer[kTracer.tail].timeStamp=kTickGet();
	kTracer.buffer[kTracer.tail].pid=runPtr->pid;
	kTracer.buffer[kTracer.tail].info = info;
	kTracer.tail   += 1U;
	kTracer.tail   %= TRACEBUFF_SIZE;
	kTracer.nAdded += 1U;
	if (kTracer.nAdded == TRACEBUFF_SIZE-1)
	{
		kTracer.nWrap += 1U;
	}
	K_EXIT_CR;
	return K_SUCCESS;
}

void ITM_SendValue(UINT32 value) {
#if (ITM_ON == 1)
	if ((ITM->TCR & ITM_TCR_ITMENA_Msk) && (ITM->TER & 1)) { // Check if ITM and Port 0 are enabled
		while (ITM->PORT[0].u32 == 0);  // Wait until ITM port is ready
		ITM->PORT[0].u32 = value;       // Write the 32-bit value
	}

#else
	(void)value;
	return;
#endif
}

#endif
