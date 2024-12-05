/******************************************************************************
 *
 *     [[K0BA - Kernel 0 For Embedded Applications] | [VERSION: 0.3.0]]
 *
 ******************************************************************************
 ******************************************************************************
 * 	In this header:
 * 					o Private API: Error handling and Checks
 *
 *****************************************************************************/

#ifndef KERR_H
#define KERR_H
#ifdef __cplusplus
extern "C" {
#endif
extern volatile K_FAULT faultID;
VOID ITM_SendValue(UINT32);
VOID kErrHandler(K_FAULT);
#ifdef __cplusplus
}
#endif
#endif /* K_ERR_H*/
