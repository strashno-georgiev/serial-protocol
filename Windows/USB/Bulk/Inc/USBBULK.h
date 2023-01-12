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
File    : USBBULK.h
Purpose : USB functions
---------------------------END-OF-HEADER------------------------------
*/
#ifndef _USBBULK_H
#define _USBBULK_H

#include "Global.h"

#ifdef _WIN32
  #include <windows.h>

  #ifdef USBBULK_SIMPLE
    #define USBBULK_API
  #else
    #ifdef USBBULK_EXPORTS
      #define USBBULK_API __declspec(dllexport)
    #else
    #define USBBULK_API __declspec(dllimport)
    #endif
  #endif
  #define STDCALL __stdcall
#else
  #define USBBULK_API
  #define WINAPI
  #define STDCALL
  #define GUID int
#endif

#if defined(__cplusplus)
  extern "C" {          /* Make sure we have C-declarations in C++ programs */
#endif


/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
#define USBBULK_BUFFERSIZE            (64*1024)
#ifndef   USBBULK_MAX_DEVICES
  #define USBBULK_MAX_DEVICES               10
#endif

/*********************************************************************
*
*       Defines, non configurable
*
**********************************************************************
*/
#define USBBULK_MODE_BIT_ALLOW_SHORT_READ            (1 << 0)
#define USBBULK_MODE_BIT_ALLOW_SHORT_WRITE           (1 << 1)

#define USBBULK_MTYPE_INIT          (1UL <<  0)
#define USBBULK_MTYPE_CORE          (1UL <<  1)
#define USBBULK_MTYPE_DEVICE        (1UL <<  2)
#define USBBULK_MTYPE_API           (1UL <<  3)
#define USBBULK_MTYPE_READ          (1UL <<  4)
#define USBBULK_MTYPE_WRITE         (1UL <<  5)
#define USBBULK_MTYPE_LOCK          (1UL <<  6)
#define USBBULK_MTYPE_APPLICATION   (1UL << 31)

#define USBBULK_SETUP_REQUEST_HOST_TO_DEVICE              (0x00)
#define USBBULK_SETUP_REQUEST_DEVICE_TO_HOST              (0x80)

#define USBBULK_SETUP_REQUEST_VENDOR                      (0x40)
#define USBBULK_SETUP_REQUEST_CLASS                       (0x20)
#define USBBULK_SETUP_REQUEST_STANDARD                    (0x00)

#define USBBULK_SETUP_REQUEST_DEVICE                      (0x00)
#define USBBULK_SETUP_REQUEST_INTERFACE                   (0x01)
#define USBBULK_SETUP_REQUEST_ENDPOINT                    (0x02)
#define USBBULK_SETUP_REQUEST_OTHER                       (0x03)

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

typedef enum _USBBULK_DEVICE_EVENT {
  USBBULK_DEVICE_EVENT_ADD,
  USBBULK_DEVICE_EVENT_REMOVE
} USBBULK_DEVICE_EVENT;


#define USBBULK_SPEED_UNKNOWN       0
#define USBBULK_SPEED_LOW           1
#define USBBULK_SPEED_FULL          2
#define USBBULK_SPEED_HIGH          3
#define USBBULK_SPEED_SUPER         4

typedef struct _USBBULK_DEV_INFO {
  U16  VendorId;        // Vendor ID of the device.
  U16  ProductId;       // Product ID of the device.
  char acSN[256];       // 0-terminated string which holds the serial number of the device.
  char acDevName[255];  // 0-terminated string which holds the device name.
  U8   InterfaceNo;     // Interface number used by this device.
  U8   Speed;           // Device speed. One of the following:
                        // + USBBULK_SPEED_UNKNOWN
                        // + USBBULK_SPEED_LOW
                        // + USBBULK_SPEED_FULL
                        // + USBBULK_SPEED_HIGH
                        // + USBBULK_SPEED_SUPER
} USBBULK_DEV_INFO;

#ifdef _WIN32
  #pragma pack(1)
#endif
typedef struct _USBBULK_SETUP_REQUEST {
  U8 bRequestType;
  U8  bRequest;
  U16  wValue;
  U16  wIndex;
  U16  wLength;
} USBBULK_SETUP_REQUEST;
#ifdef _WIN32
  #pragma pack()
#endif


typedef int USB_BULK_HANDLE;

typedef void (STDCALL USBBULK_NOTIFICATION_FUNC)(void *  pContext, unsigned Index, USBBULK_DEVICE_EVENT Event);
typedef void (STDCALL USBBULK_LOG_FUNC) (const char * sLog);
typedef void (STDCALL USBBULK_WARN_FUNC)(const char * sWarn);

/*********************************************************************
*
*       Function prototypes
*
**********************************************************************
*/

/*********************************************************************
*
*       USB-Bulk basic functions
*/
USBBULK_API void            WINAPI USBBULK_Close                  (USB_BULK_HANDLE hDevice);
USBBULK_API USB_BULK_HANDLE WINAPI USBBULK_Open                   (unsigned DevIndex);
USBBULK_API USB_BULK_HANDLE WINAPI USBBULK_OpenTimed              (unsigned DevIndex, int Timeout);
USBBULK_API int             WINAPI USBBULK_GetDevInfoByIdx        (unsigned Idx, USBBULK_DEV_INFO * pDevInfo);

USBBULK_API void            WINAPI USBBULK_Init                   (USBBULK_NOTIFICATION_FUNC * pfNotification, void * pContext);
USBBULK_API void            WINAPI USBBULK_Exit                   (void);
USBBULK_API void            WINAPI USBBULK_SetUSBId               (U16 VendorId, U16 ProductId);
USBBULK_API void            WINAPI USBBULK_AddAllowedDeviceItem   (U16 VendorId, U16 ProductId);
USBBULK_API void            WINAPI USBBULK_RemoveAllowedDeviceItem(U16 VendorId, U16 ProductId);

/*********************************************************************
*
*       USB-Bulk direct input/output functions
*/
USBBULK_API int             WINAPI USBBULK_Read        (USB_BULK_HANDLE hDevice,             void       * pBuffer,   int NumBytes);
USBBULK_API int             WINAPI USBBULK_Write       (USB_BULK_HANDLE hDevice,             const void * pBuffer,   int NumBytes);
USBBULK_API void            WINAPI USBBULK_CancelRead  (USB_BULK_HANDLE hDevice);
USBBULK_API int             WINAPI USBBULK_ReadTimed   (USB_BULK_HANDLE hDevice,                   void * pBuffer,   int NumBytes, unsigned ms);
USBBULK_API int             WINAPI USBBULK_WriteTimed  (USB_BULK_HANDLE hDevice,             const void * pBuffer,   int NumBytes, unsigned ms);
USBBULK_API int             WINAPI USBBULK_WriteEx     (USB_BULK_HANDLE hDevice,             const void * pBuffer,   int NumBytes, unsigned ms, int Send0PacketIfRequired);
USBBULK_API int             WINAPI USBBULK_FlushRx     (USB_BULK_HANDLE hDevice);
USBBULK_API int             WINAPI USBBULK_SetupRequest(USB_BULK_HANDLE hDevice, USBBULK_SETUP_REQUEST * pSetupRequest, void * pBuffer, unsigned * pBufferSize, unsigned Timeout);

/*********************************************************************
*
*       USB-Bulk control functions
*/
USBBULK_API int             WINAPI USBBULK_GetConfigDescriptor        (USB_BULK_HANDLE hDevice, void * pBuffer, int Size);
USBBULK_API unsigned        WINAPI USBBULK_GetMode                    (USB_BULK_HANDLE hDevice);
USBBULK_API unsigned        WINAPI USBBULK_GetReadMaxTransferSize     (USB_BULK_HANDLE hDevice);
USBBULK_API unsigned        WINAPI USBBULK_GetWriteMaxTransferSize    (USB_BULK_HANDLE hDevice);
USBBULK_API unsigned        WINAPI USBBULK_GetReadMaxPacketSize       (USB_BULK_HANDLE hDevice);
USBBULK_API unsigned        WINAPI USBBULK_GetWriteMaxPacketSize      (USB_BULK_HANDLE hDevice);
USBBULK_API int             WINAPI USBBULK_ResetINPipe                (USB_BULK_HANDLE hDevice);
USBBULK_API int             WINAPI USBBULK_ResetOUTPipe               (USB_BULK_HANDLE hDevice);
USBBULK_API int             WINAPI USBBULK_ResetDevice                (USB_BULK_HANDLE hDevice);
USBBULK_API unsigned        WINAPI USBBULK_SetMode                    (USB_BULK_HANDLE hDevice, unsigned Mode);
USBBULK_API void            WINAPI USBBULK_SetReadTimeout             (USB_BULK_HANDLE hDevice, int    Timeout);
USBBULK_API void            WINAPI USBBULK_SetWriteTimeout            (USB_BULK_HANDLE hDevice, int    Timeout);
USBBULK_API U32             WINAPI USBBULK_GetEnumTickCount           (USB_BULK_HANDLE hDevice);
USBBULK_API U32             WINAPI USBBULK_GetReadMaxTransferSizeDown (USB_BULK_HANDLE hDevice);
USBBULK_API U32             WINAPI USBBULK_GetWriteMaxTransferSizeDown(USB_BULK_HANDLE hDevice);
USBBULK_API unsigned        WINAPI USBBULK_SetWriteMaxTransferSizeDown(USB_BULK_HANDLE hDevice, U32 TransferSize);
USBBULK_API unsigned        WINAPI USBBULK_SetReadMaxTransferSizeDown (USB_BULK_HANDLE hDevice, U32 TransferSize);
USBBULK_API int             WINAPI USBBULK_GetSN                      (USB_BULK_HANDLE hDevice, U8 * pBuffer, unsigned BufferSize);
USBBULK_API void            WINAPI USBBULK_GetDevInfo                 (USB_BULK_HANDLE hDevice, USBBULK_DEV_INFO * pDevInfo);
USBBULK_API void            WINAPI USBBULK_GetUSBId                   (USB_BULK_HANDLE hDevice, U16 * pVendorIdMask, U16 * pProductIdMask);
USBBULK_API int             WINAPI USBBULK_GetProductName             (USB_BULK_HANDLE hDevice, char * sProductName, unsigned BufferSize);
USBBULK_API int             WINAPI USBBULK_GetVendorName              (USB_BULK_HANDLE hDevice, char * sVendorName, unsigned BufferSize);
USBBULK_API int             WINAPI USBBULK_GetLocationInfo            (USB_BULK_HANDLE hDevice, char * pBuffer, unsigned NumBytesBuffer);
USBBULK_API int             WINAPI USBBULK_GetStringDesc              (USB_BULK_HANDLE hDevice, unsigned StringIndex, U8 * pBuffer, unsigned BufferSize);
USBBULK_API int             WINAPI USBBULK_SetAlternateSetting        (USB_BULK_HANDLE hDevice, U8 AlternateSetting);

/*********************************************************************
*
*       USB-Bulk general GET functions
*/
USBBULK_API unsigned        WINAPI USBBULK_GetDriverCompileDate  (char * s, unsigned Size);
USBBULK_API unsigned        WINAPI USBBULK_GetDriverVersion      (void);
USBBULK_API unsigned        WINAPI USBBULK_GetVersion            (void);
USBBULK_API unsigned        WINAPI USBBULK_GetNumAvailableDevices(U32 * pMask);
USBBULK_API const GUID      WINAPI USBBULK_GetGUID               (void);


/*********************************************************************
*
*       USB-Bulk logging functions
*/
USBBULK_API void            WINAPI USBBULK_EnableLog          (USBBULK_LOG_FUNC * pfLog, USBBULK_WARN_FUNC * pfWarn);
USBBULK_API void            WINAPI USBBULK_SetLogFilter       (U32 FilterMask);
USBBULK_API void            WINAPI USBBULK_SetWarnFilter      (U32 FilterMask);
USBBULK_API void            WINAPI USBBULK_AddLogFilter       (U32 FilterMask);
USBBULK_API void            WINAPI USBBULK_AddWarnFilter      (U32 FilterMask);


/*********************************************************************
*
*       Deprecated functions.
*/
USBBULK_API int             WINAPI USBBULK_WriteRead   (USB_BULK_HANDLE hDevice,             const void * pWrBuffer, int WrNumBytes, void *  pRdBuffer, int RdNumBytes);


#if defined(__cplusplus)
  }     /* Make sure we have C-declarations in C++ programs */
#endif


#endif

/*************************** End of file ****************************/
