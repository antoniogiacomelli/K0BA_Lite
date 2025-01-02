/*****************************************************************************
*
* [K0BA - Kernel 0 For Embedded Applications] | [VERSION: 0.3.1]
*
******************************************************************************
******************************************************************************
* Module : Utils
* Provides to : All
* Public API : Yes
*
* In this unit:
* o Misc generic utils
*
*****************************************************************************/

#define K_CODE
#include "kconfig.h"
#include "kenv.h"
#include "kobjs.h"
#include "ktypes.h"
#include "kitc.h"


SIZE kStrLen(STRING s)
{
	SIZE len = 0;
	while ( *s != '\0')
	{
		s ++;
		len ++;
	}
	return (len);
}

SIZE kMemCpy(ADDR destPtr, ADDR const srcPtr, SIZE size)
{
	if ((IS_NULL_PTR(destPtr)) || (IS_NULL_PTR(srcPtr)))
	{
		kErrHandler(FAULT_NULL_OBJ);
	}
	SIZE n = 0;
	BYTE* destTempPtr = (BYTE*) destPtr;
	BYTE const* srcTempPtr = (BYTE const*) srcPtr;
	for (SIZE i = 0; i < size; ++i)
	{
		destTempPtr[i] = srcTempPtr[i];
		n ++;
	}
	return (n);
}

/*****************************************************************************
 * the glamurous blocking printf
 * deceiving and botching for the good
 * since 1902
 *****************************************************************************/
extern UART_HandleTypeDef huart2;

int _write(int file, char* ptr, int len)
{
	(VOID)file;
	int ret = len;
	while (len)
	{
		while ( !(huart2.Instance->SR & UART_FLAG_TXE))
			;
		huart2.Instance->DR = (char) ( *ptr) & 0xFF;
		while ( !(huart2.Instance->SR & UART_FLAG_TC))
			;
		len --;
		ptr ++;
	}
	return (ret);
}
