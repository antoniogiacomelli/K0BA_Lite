/******************************************************************************
 *
 *     [[K0BA - Kernel 0 For Embedded Applications] | [VERSION: 0.3.1]]
 *
 ******************************************************************************
 ******************************************************************************
 *  In this header:
 *                  o Inter-task synchronisation and communication kernel
 *                    module.
 *
 *****************************************************************************/

#ifndef K_ITC_H
#define K_ITC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "kconfig.h"
#include "ktypes.h"
#include "kobjs.h"
#include "kerr.h"
#include "klist.h"
#include "ksch.h"

K_ERR kPend(VOID);
K_ERR kSignal(TID const);



#if (K_DEF_SLEEPWAKE==ON)

K_ERR kEventInit(K_EVENT* const);
VOID kEventSleep(K_EVENT* const, TICK);
VOID kEventWake(K_EVENT* const);

#endif


/*
                \o_´
 	   ____  ´  (
	  /_|__\...(.\...~~~~´´´´´....                   ,.~~~~~~~..'´
	  \_|__/                       ´´.__ ,.~~~~~~~..'´


 	   ____
	  /_|__\.´´(´(...~~~~´´´´´....                   ,.~~~~~~~..'´
	  \_|__/     \                 ´´.__ ,.~~~~~~~..'´
	          `  o  ´
             `  // ´
             `
               ´
 	   ____
	  /_|__\.´´(´(...~~~~´´´´´....                   ,.~~~~~~~..'´
	  \_|__/    o                 ´´.__ ,.~~~~~~~..'´
	         `  / `
               ((
             ` `   `

*/

#ifdef __cplusplus
}
#endif
#endif /* K_ITC_H */
