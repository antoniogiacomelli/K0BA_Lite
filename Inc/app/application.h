#ifndef APPLICATION_H
#define APPLICATION_H
#include "kenv.h"
#include "kapi.h"
#if (CUSTOM_ENV==1)
	#include <stm32f4xx_hal.h>
	#include <stm32f401xe.h>
	#include <cmsis_gcc.h>
	extern UART_HandleTypeDef huart2;

	#if (K_DEF_PRINTF == ON)
		extern K_MUTEX printMutex;
		extern UART_HandleTypeDef huart2;
		#define kprintf(...) \
			do \
			{ \
				K_TICK_DIS \
				kMutexLock(&printMutex); \
				printf(__VA_ARGS__); \
				kMutexUnlock(&printMutex); \
				K_TICK_EN \
			} while(0U)
	#else
    	#define kprintf(...) (void)(0U);
	#endif

#endif


/*******************************************************************************
 * TASK STACKS EXTERN DECLARATION
 *******************************************************************************/
#define STACKSIZE 128 /*you can define each stack with a specific
 size*/

extern UINT32 stack1[STACKSIZE];
extern UINT32 stack2[STACKSIZE];
extern UINT32 stack3[STACKSIZE];
extern UINT32 stack4[STACKSIZE];


/*******************************************************************************
 * TASKS ENTRY POINT PROTOTYPES
 *******************************************************************************/
VOID Task1(VOID);
VOID Task2(VOID);
VOID Task3(VOID);
VOID Task4(VOID);


/******************************************************************************
 * TASKS EXTERN OBJECTS
 **/


#endif /* APPLICATION_H */
