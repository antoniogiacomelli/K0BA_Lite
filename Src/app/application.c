#include "application.h"

/*******************************************************************************
 *
 * Application Tasks Stacks
 *
 *******************************************************************************/

/*
 * Declare your stacks here
 * They must be already extern data on application.h
 */
UINT32 stack1[STACKSIZE];
UINT32 stack2[STACKSIZE];
UINT32 stack3[STACKSIZE];

/*******************************************************************************
 * Customise your application init
 * Declare/init app objects
 * Declare  kernel objects: mutexes, semaphores, timers, etc.
 *******************************************************************************/
#define FLAG_TEMP_SENSOR_UPDATE       (1U << 0)
#define FLAG_HUM_SENSOR_UPDATE    (1U << 1)
#define FLAG_CO2_SENSOR_UPDATE   (1U << 2)
#define FLAG_FLOW_SENSOR_UPDATE   (1U << 3)

typedef enum
{
	TEMPERATURE = 1, HUMIDITY, CO2, FLOW
} UPDATE_t;

K_MBOX mbox;

/*** Init kernel objects here */
VOID kApplicationInit(VOID)
{
	kMboxInit(&mbox, EMPTY, NULL);
}
		UINT32 sendFlag = 0;
		UPDATE_t updateType = 0;

VOID Task1(VOID)
{
	while (1)
	{

		while (1)
		{
			updateType = (updateType + 1);
			if (updateType > 4)
			{
				updateType = 1;
			}
			switch (updateType)
			{
			case (TEMPERATURE):
				sendFlag = FLAG_TEMP_SENSOR_UPDATE;
				break;
			case (HUMIDITY):
				sendFlag = FLAG_HUM_SENSOR_UPDATE;
				break;
			case (CO2):
				sendFlag = FLAG_CO2_SENSOR_UPDATE;
				break;
			case (FLOW):
				sendFlag = FLAG_FLOW_SENSOR_UPDATE;
				break;
			default:
				break;
			}
			K_ERR err = kMboxSend(&mbox, (ADDR) &sendFlag, K_WAIT_FOREVER);
			assert(err==0);
		}

	}
}
	UINT32 *rcvdFlag = 0;
	TID senderPid = 0;
	UINT32 wantedFlags = FLAG_TEMP_SENSOR_UPDATE | FLAG_HUM_SENSOR_UPDATE |
	FLAG_CO2_SENSOR_UPDATE |
	FLAG_FLOW_SENSOR_UPDATE;
	UINT32 rcvdFlags = 0;
VOID Task2(VOID)
{

	while (1)
	{
		 K_ERR err = kMboxRecv(&mbox, (ADDR) &rcvdFlag, &senderPid,
				 K_WAIT_FOREVER);
		if (err == 0)
		{
			if (senderPid == 1)
			{
				rcvdFlags |= *rcvdFlag;
				if (rcvdFlags == wantedFlags)
				{
					rcvdFlags = 0; /* clear rcvd flags */
					kprintf("Task synchronized\n\r");
					/* do work */
				}
				else
				{
					kprintf("Still missing a flag\n\r");
					kBusyDelay(5);
				}
			}

		}
	}
}


VOID Task3(VOID)
{
	while (1)
	{
		kSleep(1);

	}

}
