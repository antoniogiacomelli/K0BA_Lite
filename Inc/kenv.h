
#ifndef ENV_H
#define ENV_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * example:
#include <stm32f3xx_hal.h>
#include <cmsis_gcc.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
*/

/*#define K_DEF_PRINTF*/

#ifdef K_DEF_PRINTF
/* extern data, as peripheral handlers declarations, etc*/
extern UART_HandleTypeDef huart2;
#endif

#ifdef __cplusplus
}
#endif
#endif /*ENV_H*/
