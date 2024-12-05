#include "application.h"

/*******************************************************************************
 * Application Tasks Stacks
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
 * Declare  kernel objects: mutexes, seamaphores, timers, etc.
 *******************************************************************************/

UINT32 queue[64];
UINT32 head=0;
UINT32 tail=0;
K_MBOX mailbox; 

/*** Init kernel objects here */
VOID kApplicationInit(VOID)
{
    kMboxInit( &mailbox, 0, NULL);
}

VOID Task1(VOID)
{
    while (1)
    {
        queue[head]=head*2;
        kMboxPost( &mailbox, &queue[head], K_NACK);

        head++;
        head %= 64;
        kSleep(1);
    }
}

VOID Task2(VOID)
{
    UINT32* recvPtr=&queue[tail];

    while (1)
    {

        kMboxPend( &mailbox, (ADDR)&recvPtr);
        UINT32 recv=*recvPtr;
        UNUSED(recv);
        tail+=1;
        tail%=64;
        kSleep(1);

    }
}

VOID Task3(VOID)
{
    UINT32* recvPtr=&queue[tail];

    while (1)
    {

        kMboxPend( &mailbox, (ADDR)&recvPtr);
        UINT32 recv=*recvPtr;
        UNUSED(recv);
        tail+=1;
        tail%=64;
        kSleep(1);
    }
}

