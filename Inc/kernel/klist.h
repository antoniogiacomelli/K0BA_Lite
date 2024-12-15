/******************************************************************************
 *
 *     [[K0BA - Kernel 0 For Embedded Applications] | [VERSION: 0.3.0]]
 *
 ******************************************************************************
 ******************************************************************************
 * 	In this header:
 * 					o Private API: List ADT
 *
 *****************************************************************************
 This Linked List has a sentinel (dummy node), whose address
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
 . . .|prev |   |node |   | next| . . .
    <-|_____|   x_____| <-|_____|
         |______________|


 To remove the head, we remove the dummy's next node
 To remove the tail, we remove the dummy's previous node
 In both cases, the list adjusts itself
 ******************************************************************************/

#ifndef KLIST_H
#define KLIST_H
#ifdef __cplusplus
extern "C" {
#endif

#define K_CODE


#define K_LIST_GET_TCB_NODE(nodePtr, containerType) \
    K_GET_CONTAINER_ADDR(nodePtr, containerType, tcbNode)

/*brief Helper macro to get a message buffer from a mesg list */
#define K_LIST_GET_SYSMESG_NODE(nodePtr) \
    K_GET_CONTAINER_ADDR(nodePtr, K_MESG, mesgNode)

#define KLISTNODEDEL(nodePtr) \
    do \
    \
    { \
    nodePtr->nextPtr->prevPtr = nodePtr->prevPtr; \
    nodePtr->prevPtr->nextPtr = nodePtr->nextPtr; \
    nodePtr->prevPtr=NULL; \
    nodePtr->nextPtr=NULL; \
    \
    }while(0U)

__attribute__((always_inline)) static inline K_ERR kListInit(K_LIST* const kobj,
        STRING listName)
{
    if (kobj == NULL)
    {
        return (K_ERR_OBJ_NULL);
    }
    kobj->listDummy.nextPtr = & (kobj->listDummy);
    kobj->listDummy.prevPtr = & (kobj->listDummy);
    kobj->listName = listName;
    kobj->size = 0U;
    kobj->init=TRUE;
    DMB/*guarantee data is updated before going*/
    return K_SUCCESS;
}
__attribute__((always_inline))    static inline K_ERR kListInsertAfter(
        K_LIST* const kobj, K_LISTNODE* const refNodePtr,
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

    DMB
    return K_SUCCESS;
}
__attribute__((always_inline))    static inline K_ERR kListRemove(K_LIST* const kobj,
        K_LISTNODE* const remNodePtr)
{
    if (kobj == NULL || remNodePtr == NULL)
    {
        return K_ERR_OBJ_NULL;
    }
    if (kobj->size == 0)
    {
        return K_ERR_LIST_EMPTY;
    }
    KLISTNODEDEL(remNodePtr);
    kobj->size -= 1U;
    DMB
    return K_SUCCESS;
}

__attribute__((always_inline))    static inline K_ERR kListRemoveHead(
        K_LIST* const kobj, K_LISTNODE** const remNodePPtr)
{

    if (kobj->size == 0)
    {
        return K_ERR_LIST_EMPTY;
    }

    K_LISTNODE* currHeadPtr = kobj->listDummy.nextPtr;
    *remNodePPtr = currHeadPtr;
    KLISTNODEDEL(currHeadPtr);
    kobj->size -= 1U;
    DMB
    return K_SUCCESS;
}
__attribute__((always_inline))    static inline K_ERR kListAddTail(
        K_LIST* const kobj, K_LISTNODE* const newNodePtr)
{
    return (kListInsertAfter(kobj, kobj->listDummy.prevPtr, newNodePtr));
}

__attribute__((always_inline))    static inline K_ERR kListAddHead(
        K_LIST* const kobj, K_LISTNODE* const newNodePtr)
{

    return (kListInsertAfter(kobj, &kobj->listDummy, newNodePtr));
}

__attribute__((always_inline))

static inline K_ERR kListRemoveTail(K_LIST* const kobj, K_LISTNODE** remNodePPtr)
{
    if (kobj->size == 0)
    {
        return K_ERR_LIST_EMPTY;
    }

    K_LISTNODE* currTailPtr = kobj->listDummy.prevPtr;
    if (currTailPtr != NULL)
    {
        *remNodePPtr = currTailPtr;
        KLISTNODEDEL(currTailPtr);
        kobj->size -= 1U;
        DMB
        return K_SUCCESS;
    }
    return K_ERR_OBJ_NULL;
}

K_ERR kTCBQInit(K_LIST* const, STRING);
K_ERR kTCBQEnq(K_LIST* const, K_TCB* const);
K_ERR kTCBQDeq(K_TCBQ* const, K_TCB** const);
K_ERR kTCBQRem(K_TCBQ* const, K_TCB** const);
K_ERR kReadyCtxtSwtch(K_TCB* const);
K_ERR kReadyQDeq(K_TCB** const, PRIO);
K_TCB* kTCBQPeek(K_TCBQ* const);
K_ERR kTCBQEnqByPrio(K_TCBQ* const, K_TCB* const);

#ifdef __cplusplus
}
#endif
#endif /* KLIST_H */
