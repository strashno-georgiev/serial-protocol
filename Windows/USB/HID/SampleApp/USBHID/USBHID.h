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
File    : USBHID.h
Purpose : Definition of the USBHID functions
---------------------------END-OF-HEADER------------------------------
*/
#ifndef _USBHID_H
#define _USBHID_H

#include "Global.h"

#if defined(__cplusplus)
  extern "C" {          /* Make sure we have C-declarations in C++ programs */
#endif


/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
#ifndef   USB_MAX_DEVICES
  #define USB_MAX_DEVICES              32
#endif

/*********************************************************************
*
*       Defines, non configurable
*
**********************************************************************
*/

/*********************************************************************
*
*       Function prototypes
*
**********************************************************************
*/

/*********************************************************************
*
*       USB HID basic functions
*/
void     USBHID_Close    (unsigned Id);
int      USBHID_Open     (unsigned Id);
void     USBHID_Init     (U8 VendorPage);
void     USBHID_Exit     (void);

/*********************************************************************
*
*       USB HID direct input/output functions
*/
int      USBHID_Read     (unsigned Id, void       * pBuffer, unsigned NumBytes);
int      USBHID_Write    (unsigned Id, const void * pBuffer, unsigned NumBytes);
int      USBHID_GetReport(unsigned Id, void * pBuffer, unsigned NumBytes, unsigned ReportId);

/*********************************************************************
*
*       USB HID control functions
*/
unsigned USBHID_GetNumAvailableDevices (U32 * pMask);
int      USBHID_GetProductName         (unsigned Id, char * pBuffer, unsigned NumBytes);
int      USBHID_GetSerialNumber        (unsigned Id, char * pBuffer, unsigned NumBytes);
int      USBHID_GetInputReportSize     (unsigned Id);
int      USBHID_GetOutputReportSize    (unsigned Id);
U16      USBHID_GetProductId           (unsigned Id);
U16      USBHID_GetVendorId            (unsigned Id);
void     USBHID_RefreshList            (void);
void     USBHID_SetVendorPage          (U8 Page);


#if defined(__cplusplus)
  }     /* Make sure we have C-declarations in C++ programs */
#endif


#endif

/*************************** End of file ****************************/
