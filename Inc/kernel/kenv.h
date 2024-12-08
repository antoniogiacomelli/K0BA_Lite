
#ifndef ENV_H
#define ENV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stm32f4xx_hal.h>
#include <stm32f401xe.h>
#include <cmsis_gcc.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

/* extern data, as peripheral handlers declarations, etc*/
extern UART_HandleTypeDef huart2;


#ifdef __cplusplus
}
#endif
#endif /*ENV_H*/
