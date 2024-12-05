/******************************************************************************
 *
 *     [[K0BA - Kernel 0 For Embedded Applications] | [VERSION: 0.3.0]]
 *
 ******************************************************************************
 ******************************************************************************
 * 	In this header:
 * 					o System Tasks and Deferred Handlers function
 * 					  prototypes.
 *
 *****************************************************************************/
#ifndef KSYSTASKS_H
#define KSYSTASKS_H
#ifdef __cplusplus
extern "C" {
#endif
void IdleTask(void);
void TimerHandlerTask(void);
BOOL kTimerHandler(void);
#ifdef __cplusplus
 }
#endif
#endif /* KSYSTASKS_H */
