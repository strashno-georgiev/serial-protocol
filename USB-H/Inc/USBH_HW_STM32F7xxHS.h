/*********************************************************************
*                     SEGGER Microcontroller GmbH                    *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 2003 - 2022  SEGGER Microcontroller GmbH                 *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File        : USBH_HW_STM32F7xxHS.h
Purpose     : Header for the STM32F7 HighSpeed emUSB Host driver
-------------------------- END-OF-HEADER -----------------------------
*/

#ifndef USBH_HW_STM32F7XX_HS_H_
#define USBH_HW_STM32F7XX_HS_H_

#include "SEGGER.h"

#if defined(__cplusplus)
  extern "C" {                 // Make sure we have C-declarations in C++ programs
#endif

U32  USBH_STM32F7_HS_Add           (void * pBase);
U32  USBH_STM32F7_HS_AddEx         (void * pBase, U8 PhyType);
void USBH_STM32F7_HS_SetCacheConfig(const SEGGER_CACHE_CONFIG *pConfig, unsigned ConfSize);
void USBH_STM32F7_HS_SetCheckAddress(USBH_CHECK_ADDRESS_FUNC * pfCheckValidDMAAddress);

#if defined(__cplusplus)
  }
#endif

#endif // USBH_HW_STM32F7XX_HS_H_

/*************************** End of file ****************************/
