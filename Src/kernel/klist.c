/******************************************************************************
 *
 *     [[K0BA - Kernel 0 For Embedded Applications] | [VERSION: 0.3.0]]
 *
 ******************************************************************************
 ******************************************************************************
 * 	Module          : N/A
 * 	Provides to     : Scheduler, Inter-task synchronisation, Inter-Task
 * 	                  Communication, Application Timers
 * 	Depends on      : N/A
 * 	Public API      : No.
 * 	In this unit:
 *					o ADT for Circular Doubly Linked List
 *
 ******************************************************************************

 This Linked List has a sentinel (dummy node), whose addres
 is fixed on initialisation. Thus, every node in the list
 has an anchored reference, making it easier to handle edge
 cases.
 This ADT is aggregated to other system structures whenever
 an ordered list is required.

				  HEAD                                TAIL
	   _____     _____     _____     _____           _____
	  |     |-->|     |-->|     |-->|     |-->   <--|     |
	  |DUMMY|   |  H  |   | H+1 |   | H+2 |  . . .  |  T  |
	<-|_____|<--|_____|<--|_____|<--|_____|         |_____|-->
	|________________________________________________________|


	 - INITIALISE

	 The list is initialised by declaring a node, and assigning its previous
	 and next pointers to itself. This is the anchored reference.
			  _____
		   __|__   |
		  |     |-->
		  |DUMMY|
		<-|_____|
		|____|


	 - INSERT AFTER

		When list is empty we are inserting the head (that is also the tail).
		If not empty:
		 To insert on the head, reference node is the dummy.
		 To insert on the tail, reference node is the current tail
		 (dummy's previous node).
				 _____     _____     _____
				| ref |-> | new |-> | old |->
		   . . .|     |   |next |   | next| . . .
			  <-|_____| <-|_____| <-|_____|

	 - REMOVE A NODE

	  	 To remove a node we "cut off" its next and previous links, rearranging
	  	 as: node.prev.next = node.next; node.next.prev = node.prev;

					     _______________
				 _____  |  _____     ___|_
				|     |-> |     x   |     |->
		   . . .|prev |   |node |   | next|# . . .
			  <-|_____|   x_____| <-|_____|
				   |______________|


	   To remove the head, we remove the dummy's next node
	   To remove the tail, we remove the dummy's previous node
	   In both cases, the list adjusts itself
******************************************************************************/
#if(0)


#define K_CODE
#include "kmacros.h"
#include "kconfig.h"
#include "kobjs.h"
#include "ktypes.h"
#include "klist.h"
#include "kitc.h"
#include "kinternals.h"

static inline void kListNodeDel_(K_LISTNODE* nodePtr)
{
    nodePtr->nextPtr->prevPtr = nodePtr->prevPtr;
    nodePtr->prevPtr->nextPtr = nodePtr->nextPtr;
    nodePtr->prevPtr=NULL;
    nodePtr->nextPtr=NULL;
}


/* init the list by setting the dummy node */
K_ERR kListInit(K_LIST* const kobj, STRING listName)
{
    if (kobj == NULL)
    {
        return K_ERR_OBJ_NULL;
    }
    kobj->listDummy.nextPtr = &(kobj->listDummy);
    kobj->listDummy.prevPtr = &(kobj->listDummy);
    kobj->listName = listName;
    kobj->size = 0U;
    __DMB(); /*guarantee data is updated before going*/
    return K_SUCCESS;
}

K_ERR kListInsertAfter(K_LIST* const kobj, K_LISTNODE* const refNodePtr,
		K_LISTNODE* const newNodePtr)
{
    if (kobj == NULL || newNodePtr == NULL || refNodePtr == NULL)
    {
        return K_ERR_OBJ_NULL;
    }
    newNodePtr->nextPtr = refNodePtr->nextPtr;
    refNodePtr->nextPtr->prevPtr = newNodePtr;
    newNodePtr->prevPtr = refNodePtr;
    refNodePtr->nextPtr = newNodePtr;
    kobj->size += 1U;

    __DMB();
    return K_SUCCESS;
}

K_ERR kListRemove(K_LIST* const kobj, K_LISTNODE* const remNodePtr)
{
    if (kobj == NULL || remNodePtr == NULL)
    {
        return K_ERR_OBJ_NULL;
    }
    if (kobj->size==0)
    {
    	assert(0);
        return K_ERR_LIST_EMPTY;
    }
    kListNodeDel_(remNodePtr);
    kobj->size -= 1U;
    __DMB();
    return K_SUCCESS;
}

/*dequeue if treated as queue*/
K_ERR kListRemoveHead(K_LIST* const kobj, K_LISTNODE** const remNodePPtr)
{
    if (kobj == NULL || remNodePPtr == NULL)
    {
        return K_ERR_OBJ_NULL;
    }

    if (kobj->size==0)
    {
        return K_ERR_LIST_EMPTY;
    }

    K_LISTNODE* currHeadPtr = kobj->listDummy.nextPtr;
    *remNodePPtr = currHeadPtr;
    kListNodeDel_(currHeadPtr);
    kobj->size -= 1U;
    __DMB();
    return K_SUCCESS;
}
/*enqueue if a queue*/
K_ERR kListAddTail(K_LIST* const kobj, K_LISTNODE* const newNodePtr)
{
    if (kobj == NULL || newNodePtr == NULL)
    {
        return K_ERR_OBJ_NULL;
    }
    return kListInsertAfter(kobj, kobj->listDummy.prevPtr, newNodePtr);
}

/* jam if a queue*/
K_ERR kListAddHead(K_LIST* const kobj, K_LISTNODE* const newNodePtr)
{
    if (kobj == NULL || newNodePtr == NULL)
    {
        return K_ERR_OBJ_NULL;
    }
    return kListInsertAfter(kobj, &kobj->listDummy, newNodePtr);
}

K_ERR kListRemoveTail(K_LIST* const kobj, K_LISTNODE** remNodePPtr)
{
    if (kobj == NULL || remNodePPtr == NULL)
    {
        return K_ERR_OBJ_NULL;
    }
    if (kobj->size==0)
    {
        return K_ERR_LIST_EMPTY;
    }

    K_LISTNODE* currTailPtr = kobj->listDummy.prevPtr;
    if (currTailPtr != NULL)
    {
		*remNodePPtr = currTailPtr;
		kListNodeDel_(currTailPtr);
		kobj->size -= 1U;
		__DMB();
		return K_SUCCESS;
	}
	return K_ERR_OBJ_NULL;
}


#endif
