#ifndef APPLICATION_H
#define APPLICATION_H
#include "kenv.h"
#include "kapi.h"
/*******************************************************************************
 * TASK STACKS EXTERN DECLARATION
 *******************************************************************************/
#define STACKSIZE 128 /*you can define each stack with a specific
 size*/

extern INT stack1[STACKSIZE];
extern INT stack2[STACKSIZE];
extern INT stack3[STACKSIZE];


/*******************************************************************************
 * TASKS ENTRY POINT PROTOTYPES
 *******************************************************************************/
VOID Task1(VOID);
VOID Task2(VOID);
VOID Task3(VOID);

/******************************************************************************
 * TASKS EXTERN OBJECTS
 **/



/*****/
#define K_DEF_PRINTF
#ifdef K_DEF_PRINTF
extern UART_HandleTypeDef huart2;
#define kprintf(...) \
			do \
			{ \
				__disable_irq();\
				printf(__VA_ARGS__); \
				__enable_irq(); \
			} while(0U)

#else
#define kprintf(...) (void)0
#endif

#endif /* APPLICATION_H */
