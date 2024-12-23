/******************************************************************************
 *
 *     [[K0BA - Kernel 0 For Embedded Applications] | [VERSION: 0.3.0]]
 *
 ******************************************************************************
 ******************************************************************************
 * 	Module           : Memory Block Allocator
 * 	Provides to      : Inter-task Communication, Application Timers
 * 	Depends on       : Inter-task Synchronisation
 *  Public API       : Yes
 * 	In this unit:
 * 					o Memory Block Allocator
 *
 *****************************************************************************/

#define K_CODE
#include "kconfig.h"
#include "kobjs.h"
#include "ktypes.h"
#include "kitc.h"
#include "kerr.h"
#include "kinternals.h"
#include "kmem.h"

K_ERR kMemInit(K_MEM* const kobj, ADDR const memPoolPtr,
        BYTE blkSize, BYTE const numBlocks)
{
    K_CR_AREA

    K_ENTER_CR

    if (IS_NULL_PTR(kobj))
    {
        KFAULT(FAULT_NULL_OBJ);
        K_EXIT_CR
        return (K_ERR_MEM_INIT);
    }
    /*round up to next value multiple of 4 (if not a multiple)*/
    blkSize = (blkSize + 0x03) & 0xFC;

    /* initialise freelist of blocks */

    BYTE* blockPtr = (BYTE*) memPoolPtr; /* first byte address in the pool  */
    ADDR* nextAddrPtr = (ADDR*) memPoolPtr; /* next block address */

    for (BYTE i = 0; i < numBlocks - 1; i ++)
    {
        /* init pool by advancing each block addr by blkSize bytes */
        blockPtr += blkSize;
        /* save blockPtr addr as the next */
        *nextAddrPtr = (ADDR) blockPtr;
        /* update  */
        nextAddrPtr = (ADDR*) (blockPtr);

    }
    *nextAddrPtr = NULL;

/* init the control block */
    kobj->blkSize = blkSize;
    kobj->nMaxBlocks = numBlocks;
    kobj->nFreeBlocks = numBlocks;
    kobj->freeListPtr = memPoolPtr;
    kobj->poolPtr = memPoolPtr;
#if(MEMBLKLAST)
    kobj->lastUsed = NULL;
#endif
    kobj->init = TRUE;
    K_EXIT_CR
    return (K_SUCCESS);
}

ADDR kMemAlloc(K_MEM* const kobj)
{

    if (kobj->nFreeBlocks == 0)
    {
        return (NULL);
    }
    K_CR_AREA

    K_ENTER_CR
    ADDR allocPtr = kobj->freeListPtr;
    kobj->freeListPtr = *(ADDR*) allocPtr;
    #if(MEMBLKLAST)
    kobj->lastUsed = allocPtr;
    #endif
    if (allocPtr != NULL)
        kobj->nFreeBlocks -= 1;
    K_EXIT_CR
    return (allocPtr);
}

K_ERR kMemFree(K_MEM* const kobj, ADDR const blockPtr)
{

    if (kobj->nFreeBlocks == kobj->nMaxBlocks)
    {
        return (K_ERR_MEM_FREE);
    }
    if (IS_NULL_PTR(kobj) || IS_NULL_PTR(blockPtr))
    {
        return (K_ERR_MEM_FREE);
    }
    K_CR_AREA
    K_ENTER_CR
    *(ADDR*) blockPtr = kobj->freeListPtr;
    kobj->freeListPtr = blockPtr;
    kobj->nFreeBlocks += 1;
    K_EXIT_CR
    return (K_SUCCESS);
}

