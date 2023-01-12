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
File        : USBH_MIDI.h
Purpose     : MIDI class driver.
-------------------------- END-OF-HEADER -----------------------------
*/

#ifndef USBH_MIDI_H
#define USBH_MIDI_H

#include "USBH.h"
#include "SEGGER.h"

#if defined(__cplusplus)
extern "C" {                 // Make sure we have C-declarations in C++ programs
#endif

#define USBH_MIDI_INVALID_HANDLE                   NULL

/*********************************************************************
*
*       USBH_MIDI_HANDLE
*/

typedef struct _USBH_MIDI_INST * USBH_MIDI_HANDLE;

/*********************************************************************
*
*       USBH_MIDI_DEVICE_INFO
*
*  Description
*    Structure containing information about a MIDI device.
*/
typedef struct {
  U16                   VendorId;        // Vendor ID of the device.
  U16                   ProductId;       // Product ID of the device.
  unsigned              DevIndex;        // Device index.
  U16                   bcdDevice;       // BCD-coded device version.
  USBH_SPEED            Speed;           // USB speed of the device, see USBH_SPEED.
  USBH_INTERFACE_ID     InterfaceId;     // USBH interface ID.
  unsigned              NumInCables;     // Number of MIDI IN cables
  unsigned              NumOutCables;    // Number of MIDI OUT cables
} USBH_MIDI_DEVICE_INFO;

/*********************************************************************
*
*       Public functions
*
**********************************************************************
*/

/*********************************************************************
*
*       Component management
*/
U8                USBH_MIDI_Init                     (void);
void              USBH_MIDI_Exit                     (void);

/*********************************************************************
*
*       Device management
*/
USBH_STATUS       USBH_MIDI_AddNotification          (USBH_NOTIFICATION_HOOK * pHook, USBH_NOTIFICATION_FUNC * pfNotification, void * pContext);
USBH_STATUS       USBH_MIDI_RemoveNotification       (const USBH_NOTIFICATION_HOOK * pHook);
void              USBH_MIDI_ConfigureDefaultTimeout  (U32 RdTimeout, U32 WrTimeout);
//
USBH_MIDI_HANDLE  USBH_MIDI_Open                     (unsigned Index);
USBH_STATUS       USBH_MIDI_Close                    (USBH_MIDI_HANDLE hDevice);
USBH_STATUS       USBH_MIDI_GetDeviceInfo            (USBH_MIDI_HANDLE hDevice, USBH_MIDI_DEVICE_INFO * pDevInfo);
USBH_STATUS       USBH_MIDI_GetStatus                (USBH_MIDI_HANDLE hDevice);
USBH_STATUS       USBH_MIDI_SetTimeouts              (USBH_MIDI_HANDLE hDevice, U32 ReadTimeout, U32 WriteTimeout);
USBH_STATUS       USBH_MIDI_SetBuffer                (USBH_MIDI_HANDLE hDevice, U8 *pBuffer, unsigned BufferLen);
int               USBH_MIDI_GetIndex                 (USBH_INTERFACE_ID InterfaceID);

/*********************************************************************
*
*       Device I/O
*/
USBH_STATUS       USBH_MIDI_RdEvent                  (USBH_MIDI_HANDLE hDevice, U32 *pEvent);
USBH_STATUS       USBH_MIDI_RdData                   (USBH_MIDI_HANDLE hDevice, U8 *pData, U32 DataLen, U32 *pRdDataLen);
USBH_STATUS       USBH_MIDI_WrEvent                  (USBH_MIDI_HANDLE hDevice, U32 Event);
USBH_STATUS       USBH_MIDI_WrData                   (USBH_MIDI_HANDLE hDevice, const U8 *pData, U32 DataLen, U32 *pWrDataLen);
USBH_STATUS       USBH_MIDI_Send                     (USBH_MIDI_HANDLE hDevice);
USBH_STATUS       USBH_MIDI_GetQueueStatus           (USBH_MIDI_HANDLE hDevice, U32 *pRxBytes);

#if defined(__cplusplus)
}
#endif

#endif // USBH_MIDI_H

/*************************** End of file ****************************/
