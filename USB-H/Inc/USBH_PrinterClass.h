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
File        : USBH_PrinterClass.h
Purpose     : API of the USB host stack
-------------------------- END-OF-HEADER -----------------------------
*/

#ifndef USBH_PRINTERCLASS_H_
#define USBH_PRINTERCLASS_H_

#include "USBH.h"
#include "SEGGER.h"

#if defined(__cplusplus)
  extern "C" {                 // Make sure we have C-declarations in C++ programs
#endif

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/
#define USBH_PRINTER_INVALID_HANDLE   0u

/*********************************************************************
*
*       USBH_PRINTER
*/
typedef U32 USBH_PRINTER_HANDLE;

typedef struct {
  U16  VendorId;          // The printer's vendor ID.
  U16  ProductId;         // The printer's product ID.
  U16  bcdDevice;         // Binary Coded Decimal device version.
  U16  MaxPacketSize_OUT; // Maximum packet size of the bulk OUT EP.
  U16  MaxPacketSize_IN;  // Maximum packet size of the bulk IN EP. If this value is zero it means that the printer does not have an IN endpoint.
  U8   acSerialNo[255];   // The printer's serial number.
} USBH_PRINTER_DEVICE_INFO;


U8                  USBH_PRINTER_Init(void);
void                USBH_PRINTER_Exit(void);
USBH_PRINTER_HANDLE USBH_PRINTER_Open(const char * sName);
USBH_PRINTER_HANDLE USBH_PRINTER_OpenByIndex(unsigned Index);
USBH_STATUS         USBH_PRINTER_Write(USBH_PRINTER_HANDLE hDevice, const U8 * pData, unsigned NumBytes);
USBH_STATUS         USBH_PRINTER_Read(USBH_PRINTER_HANDLE hDevice, U8 * pData, unsigned NumBytes);
USBH_STATUS         USBH_PRINTER_GetPortStatus(USBH_PRINTER_HANDLE hDevice, U8 * pStatus);
USBH_STATUS         USBH_PRINTER_ExecSoftReset(USBH_PRINTER_HANDLE hDevice);
USBH_STATUS         USBH_PRINTER_GetDeviceId(USBH_PRINTER_HANDLE hDevice, U8 * pData, unsigned NumBytes);
USBH_STATUS         USBH_PRINTER_Close(USBH_PRINTER_HANDLE hDevice);
int                 USBH_PRINTER_GetNumDevices(void);
void                USBH_PRINTER_RegisterNotification(USBH_NOTIFICATION_FUNC * pfNotification, void * pContext);
USBH_STATUS         USBH_PRINTER_AddNotification(USBH_NOTIFICATION_HOOK * pHook, USBH_NOTIFICATION_FUNC * pfNotification, void * pContext);
USBH_STATUS         USBH_PRINTER_RemoveNotification(const USBH_NOTIFICATION_HOOK * pHook);
void                USBH_PRINTER_ConfigureTimeout(U32 Timeout);
USBH_STATUS         USBH_PRINTER_GetDeviceInfo(USBH_PRINTER_HANDLE hDevice, USBH_PRINTER_DEVICE_INFO * pDevInfo);
USBH_STATUS         USBH_PRINTER_SendVendorRequest(USBH_PRINTER_HANDLE hDevice, U8 RequestType, U8 Request, U16 wValue, void * pData, U32 * pNumBytes, U32 Timeout);
USBH_STATUS         USBH_PRINTER_Receive(USBH_PRINTER_HANDLE hDevice, U8 * pData, U32 * pNumBytesRead, U32 Timeout);
USBH_STATUS         USBH_PRINTER_WriteEx(USBH_PRINTER_HANDLE hDevice, const U8 * pData, U32 NumBytes, U32 * pNumBytesWritten, U32 Timeout);
USBH_STATUS         USBH_PRINTER_AddCustomDeviceMask(const U16 * pVendorIds, const U16 * pProductIds, U16 NumIds);
#if defined(__cplusplus)
  }
#endif

#endif // USBH_PRINTERCLASS_H_

/*************************** End of file ****************************/
