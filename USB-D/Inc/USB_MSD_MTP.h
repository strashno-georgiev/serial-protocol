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
File    : USB_MSD_MTP.h
Purpose : Public header of USB MSD/MTP combination feature.
--------  END-OF-HEADER  ---------------------------------------------
*/

#ifndef USB_MSD_MTP_H_          /* Avoid multiple inclusion */
#define USB_MSD_MTP_H_

#include "USB_Private.h"
#if USB_SUPPORT_MSD_MTP_COMBINATION > 0
#include <stdio.h>
#include "SEGGER.h"
#include "USB_MSD_Private.h"
#include "USB_MTP_Private.h"

#if defined(__cplusplus)
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif


/*********************************************************************
*
*       defines, non-configurable
*
**********************************************************************
*/
#define USBD_MSD_MTP_MODE_NOT_INITED  (0uL)
#define USBD_MSD_MTP_MODE_MSD         (1uL)
#define USBD_MSD_MTP_MODE_MTP         (2uL)

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

int   USBD_MSD_MTP_Add      (const USB_MSD_INIT_DATA * pMSDInitData, const USB_MTP_INIT_DATA * pMTPInitData);
int   USBD_MSD_MTP_GetMode  (void);
void  USBD_MSD_MTP_Task     (void);
#if defined(__cplusplus)
  }              /* Make sure we have C-declarations in C++ programs */
#endif
#else  // USB_SUPPORT_MSD_MTP_COMBINATION > 0
void USB_MSD_MTP_c(void);
#endif // USB_SUPPORT_MSD_MTP_COMBINATION > 0
#endif /* Avoid multiple inclusion */

/*************************** End of file ****************************/

