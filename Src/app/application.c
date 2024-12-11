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

#define QUEUE_SIZE (64)

static UINT32 queue[QUEUE_SIZE];
static UINT32 head=0;
static UINT32 tail=0;

K_MBOX mailbox; 

/*** Init kernel objects here */
VOID kApplicationInit(VOID)
{
    kMboxInit( &mailbox, 0, NULL);
}

static VOID send(UINT32* mesg)
{
	queue[head]=*mesg;
	kMboxPost(&mailbox, &queue[head]);
	head+=1U;
	head %= QUEUE_SIZE;
}

static VOID recv(UINT32** recvpptr)
{
	UINT32* tailptr = 	&queue[tail];
	kMboxPend(&mailbox, (ADDR*)&tailptr);
	*recvpptr = tailptr;
	tail+=1U;
	tail %= QUEUE_SIZE;

}

VOID Task1(VOID)
{
	UINT32 sendmesg=0;
    while (1)
    {
    	send(&sendmesg);
    	sendmesg++;
    	kSleep(1);
    }
}

VOID Task2(VOID)
{


    while (1)
    {

        kSleep(1);

    }
}


VOID Task3(VOID)
{
	UINT32* recvptr;

    while (1)
    {
    	recv(&recvptr);
    	(VOID)recvptr;
    	kSleep(1);
    }
}

