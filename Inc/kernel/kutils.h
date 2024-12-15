/******************************************************************************
 *
 *     [[K0BA - Kernel 0 For Embedded Applications] | [VERSION: 0.3.0]]
 *
 ******************************************************************************
 ******************************************************************************
 *  In this header:
 *                  o It is called 'utils' but not really.
 *****************************************************************************/

#ifndef K_UTILS_H
#define K_UTILS_H
#ifdef __cplusplus
extern "C" {
#endif


SIZE kStrLen(STRING s);
SIZE kMemCpy(ADDR destPtr, ADDR const srcPtr, SIZE size);

#if (K_DEF_PRINTF == ON)

#if (CUSTOM_ENV == 1)
extern UART_HandleTypeDef huart2;
int _write(int file, char* ptr, int len);

#endif

#endif
#ifdef __cplusplus
 }
#endif
#endif
