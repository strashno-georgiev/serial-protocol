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
File    : USBHID.c
Purpose : Implementation of the USBHID functions
---------------------------END-OF-HEADER------------------------------
*/

#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <setupapi.h>
#include <hidsdi.h>
#include "USBHID.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

/*********************************************************************
*
*       Defines, non configurable
*
**********************************************************************
*/

/*********************************************************************
*
*       Types
*
**********************************************************************
*/
typedef struct {
  HANDLE     hDevice;           // handle to device
  unsigned   InputReportByteLength;
  unsigned   OutputReportByteLength;
  U16        VendorId;
  U16        ProductId;
  char       acDevicePath[512];
} CONN_INFO;

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static CONN_INFO * _apConnInfo[USB_MAX_DEVICES];
static int         _IsInited;
static unsigned    _NumDevices;
static U16         _VendorPage;

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/


/*********************************************************************
*
*       _AddConnection
*
*  Function description
*    Checks the information about the queried device, if complying with
*    out vendor page information, add the device to the connection list.
*
*
*  Return value:
*   1    - O.K., HID device added.
*   0    - HID device not compatible.
*
*/
static int _AddConnection(HANDLE hDevList, SP_DEVICE_INTERFACE_DATA * pDevData, CONN_INFO * pConnInfo) {
  SP_INTERFACE_DEVICE_DETAIL_DATA * pDevDetail;
  DWORD                             ReqLen;
  BOOL                              succ;
  HANDLE                            hDevice;
  int                               r;

  r          = 0;
  ReqLen     = 0;
  pDevDetail = NULL;
  // Get length of INTERFACE_DEVICE_DETAIL_DATA, allocate buffer
  // This function call returns the system error "buffer too small" (code 122).
  // We ignore this return code.
  SetupDiGetDeviceInterfaceDetail(hDevList, pDevData, NULL, ReqLen, &ReqLen, NULL);
  pDevDetail = (SP_INTERFACE_DEVICE_DETAIL_DATA*)malloc(ReqLen);
  if (pDevDetail == NULL ) {
    // Memory allocation failed
    return 0;
  }
  // Now get the INTERFACE_DEVICE_DETAIL_DATA struct
  ZeroMemory(pDevDetail,ReqLen);
  pDevDetail->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
  succ = SetupDiGetDeviceInterfaceDetail(hDevList, pDevData, pDevDetail, ReqLen, &ReqLen, NULL);
  if (succ == FALSE) {
    //
    // Could not retrieve information about the device
    //
    free(pDevDetail);
    return 0;
  }
  //
  //  Open a device handle to the HID device and retriece HID information
  //
  hDevice = CreateFile(pDevDetail->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
  if (hDevice != INVALID_HANDLE_VALUE) {
    PHIDP_PREPARSED_DATA  pPreparsedData;
    HIDP_CAPS             Capabilities;

    //
    //  Parse the information from HID device
    //
    HidD_GetPreparsedData(hDevice, &pPreparsedData);
    HidP_GetCaps(pPreparsedData, &Capabilities);
    //
    //  Does the device match the Vendor specific page implementation?
    //
    HidD_FreePreparsedData(pPreparsedData);
    if (Capabilities.UsagePage == _VendorPage) {
      HIDD_ATTRIBUTES  Attributes;

      memset(&Attributes, 0, sizeof(Attributes));
      Attributes.Size = sizeof(Attributes);
      HidD_GetAttributes(hDevice, &Attributes);
      strncpy(pConnInfo->acDevicePath, pDevDetail->DevicePath, sizeof(pConnInfo->acDevicePath));
      pConnInfo->InputReportByteLength  = Capabilities.InputReportByteLength;
      pConnInfo->OutputReportByteLength = Capabilities.OutputReportByteLength;
      pConnInfo->ProductId              = Attributes.ProductID;
      pConnInfo->VendorId               = Attributes.VendorID;
      r = 1;
    }
    CloseHandle(hDevice);
  }
  free(pDevDetail);
  return r;
}


/*********************************************************************
*
*       _Init
*
*  Function description
*    Initialise the USBHID API and look for new HID devices that
*    comply with the Vendor page set.
*
*  Return value:
*   1    - O.K., devices have been found.
*   0    - Error, no devices found.
*/
static int _Init(void) {
  HDEVINFO                 hDevList;
  SP_DEVICE_INTERFACE_DATA DevData;
  BOOL                     r;
  DWORD                    i;
  GUID                     HidGuid;

  //
  // Get the GUID for HID class from Windows
  //
  HidD_GetHidGuid(&HidGuid);
  //
  // Create a list of attached devices
  //
  hDevList = SetupDiGetClassDevs(&HidGuid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
  if (hDevList == INVALID_HANDLE_VALUE) {
    printf("SetupDiGetClassDevs failed, err=%d\n",GetLastError());
    return 0;
  }
  ZeroMemory(&DevData,sizeof(DevData));
  DevData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
  _NumDevices = 0;
  //
  // Enumerate thru all HID devices
  //
  for (i = 0; i < 127; i++) {
    DWORD LastError;
    r = SetupDiEnumDeviceInterfaces(hDevList, NULL, &HidGuid, i, &DevData);
    if (r == 0) {
      LastError = GetLastError();
      if (LastError == ERROR_NO_MORE_ITEMS) {
        break;
      } else {
        //
        // There are no devices attached using the USB HID class
        //
        SetupDiDestroyDeviceInfoList(hDevList);
        return 0;              // Error: No HID devices found.
      }
    }
    //
    // Allocate memory for connection info if necessary.
    //
    if (_apConnInfo[_NumDevices] == NULL) {
      _apConnInfo[_NumDevices] = (CONN_INFO *)calloc(sizeof(CONN_INFO), 1);
    }
    //
    // Check and store information required into pConnInfo
    //
    if (_AddConnection(hDevList, &DevData, _apConnInfo[_NumDevices])) {
      if (++_NumDevices == USB_MAX_DEVICES) {
        break;
      }
    }
  }
  SetupDiDestroyDeviceInfoList(hDevList);
  if (_NumDevices  == 0) {
    return 0;        // Error: No HID device found matching our requirements.
  }
  return 1;          // O.K found devices.
}

/*********************************************************************
*
*       _Write
*
*  Function description
*    Writes an output report to the device.
*    This function creates a temporary buffer, since the HID stack of
*    windows needs an additional byte preceding the report for storing the
*    report ID.
*
*  Return value:
*    Number of bytes that have be written.
*
*/
static int _Write(CONN_INFO * pConnInfo, const void * pBuffer, unsigned NumBytes) {
  unsigned long NumBytesWritten = 0;
  U8 * pTempBuffer;

  pTempBuffer = (U8 *)calloc(pConnInfo->OutputReportByteLength, sizeof(U8));
  if (pTempBuffer == NULL) {
    return 0;
  }
  memcpy(pTempBuffer + 1, pBuffer, NumBytes);
  if (WriteFile(pConnInfo->hDevice, pTempBuffer, pConnInfo->OutputReportByteLength, &NumBytesWritten, NULL)) {
    NumBytesWritten  = NumBytes;
  } else {
    int r;
    r = GetLastError();
    free(pTempBuffer);
    return -r;
  }
  free(pTempBuffer);
  return NumBytesWritten;
}

/*********************************************************************
*
*       _Read
*
*  Function description
*    Reads an input report from the device.
*    This function creates a temporary buffer, since the HID stack of
*    windows needs an additional byte preceding the report for storing the
*    report ID.
*
*  Return value:
*    Number of bytes that have be read.
*
*/
static int _Read(CONN_INFO * pConnInfo, void * pBuffer, unsigned NumBytesReq) {
  U32  NumBytesRead = 0;
  U32  nBytesRead;

  if (pConnInfo) {
    U8 * pTempBuffer;

    if (NumBytesReq == 0) {
      return 0;
    }
    pTempBuffer = (U8 *)calloc(pConnInfo->InputReportByteLength, sizeof(U8));
    if (pTempBuffer == NULL) {
      return 0;
    }
    if (ReadFile(pConnInfo->hDevice, pTempBuffer, pConnInfo->InputReportByteLength, &nBytesRead, NULL)) {
      memcpy(pBuffer, pTempBuffer + 1, NumBytesReq);
      NumBytesRead = NumBytesReq;
    } else {
      int r;
      r = GetLastError();
      free(pTempBuffer);
      return -r;
    }
    free(pTempBuffer);
  }
  return NumBytesRead;

}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       USBHID_Open
*
*  Function description
*    Opens a handle to a device.
*
*  Parameters
*    Id - Index of the HID device. This is the bit number of the mask
*         returned by USBHID_GetNumAvailableDevices().
*
*  Return value
*    == 0 : O.K. Opening was successful or already opened.
*    == 1 : Error. Handle to the device could not opened.
*/
int USBHID_Open(unsigned Id) {
  CONN_INFO * pConnInfo;
  HANDLE    * hDevice;
  int         r;

  r = 1;
  pConnInfo = _apConnInfo[Id];
  if (pConnInfo == NULL)  {
    return 1;   // Error device Id does not exist.
  }
  if ((pConnInfo->hDevice == INVALID_HANDLE_VALUE) || (pConnInfo->hDevice == 0)) {
    hDevice = CreateFile(pConnInfo->acDevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice != INVALID_HANDLE_VALUE) {
      //
      // We increase the size of the ring input buffer to the maximum possible.
      // This is from Windows XP onwards 512. Otherwise the packages will be received
      // by the Host but are lost.
      //
      HidD_SetNumInputBuffers(hDevice, 512);
      r = 0;
    }
    pConnInfo->hDevice = hDevice;
  } else {
    r = 0;
  }
  return r;
}

/*********************************************************************
*
*       USBHID_Close
*
*  Function description
*    Close the connection to an open device.
*
*  Parameters
*    Id - Index of the HID device. This is the bit number of the mask
*         returned by USBHID_GetNumAvailableDevices().
*/
void USBHID_Close(unsigned Id) {
  CONN_INFO * pConnInfo;

  pConnInfo = _apConnInfo[Id];
  if (pConnInfo) {
    CloseHandle(pConnInfo->hDevice);
    pConnInfo->hDevice = 0;
  }
}

/*********************************************************************
*
*       USBHID_Init
*
*  Function description
*    Sets the specific vendor page, initializes the USB HID User API
*    and retrieves the information of the HID device.
*
*  Parameters
*    VendorPage - This parameter specifies the lower 8 bits of
*                 the vendor-specific usage page number.
*                 It must be identical on both device and host.
*/
void USBHID_Init(U8 VendorPage) {
  USBHID_SetVendorPage(VendorPage);
  _IsInited = _Init();
}

/*********************************************************************
*
*       USBHID_Exit
*
*  Function description
*    Closes the connection to all open devices and de-initializes
*    the HID module.
*/
void USBHID_Exit(void) {
  CONN_INFO * pConnInfo;
  unsigned    i;

  for (i = 0; i < _NumDevices; i++) {
    pConnInfo = _apConnInfo[i];
    if (pConnInfo) {
      CloseHandle(pConnInfo->hDevice);
      free(pConnInfo);
      pConnInfo = NULL;
    }
    _apConnInfo[i] = pConnInfo;
  }
  _IsInited   = 0;
  _NumDevices = 0;
}

/*********************************************************************
*
*       USBHID_Read
*
*  Function description
*    Reads an input report from device via the interrupt endpoint.
*
*  Return value:
*    On Error:   -1, No valid device Id used or the report size does not match with device.
*    On success: Number of bytes that have be written.
*
*/
int USBHID_Read(unsigned Id, void * pBuffer, unsigned NumBytes) {
  CONN_INFO * pConnInfo;

  pConnInfo = _apConnInfo[Id];
  if (pConnInfo == NULL)  {
    return -1;   // Error device Id does not exist.
  }
  if (NumBytes != (pConnInfo->InputReportByteLength - 1)) {
    return -1;   // Error report size does not match
  }
  return _Read(pConnInfo, pBuffer, NumBytes);
}

/*********************************************************************
*
*       USBHID_GetReport
*
*  Function description
*    Reads an input report the from device via a setup request.
*
*  Return value:
*    On Error:   -1, No valid device Id used or the report size does not match with device.
*    On success: Number of bytes that have be read.
*
*/
int USBHID_GetReport(unsigned Id, void * pBuffer, unsigned NumBytes, unsigned ReportId) {
  CONN_INFO * pConnInfo;
  U32  NumBytesRead = 0;

  pConnInfo = _apConnInfo[Id];
  if (pConnInfo == NULL)  {
    return -1;   // Error device Id does not exist.
  }
  if (NumBytes != (pConnInfo->InputReportByteLength - 1)) {
    return -1;   // Error report size does not match
  }
  if (pConnInfo) {
    U8 * pTempBuffer;

    if (NumBytes == 0) {
      return 0;
    }
    pTempBuffer = (U8 *)calloc(pConnInfo->InputReportByteLength, sizeof(U8));
    if (pTempBuffer == NULL) {
      return 0;
    }
    *pTempBuffer = ReportId;
    if (HidD_GetInputReport(pConnInfo->hDevice, pTempBuffer, pConnInfo->InputReportByteLength)) {
      memcpy(pBuffer, pTempBuffer + 1, NumBytes);
      NumBytesRead = NumBytes;
    } else {
      int r;
      r = GetLastError();
      free(pTempBuffer);
      return -r;
    }
    free(pTempBuffer);
  }
  return NumBytesRead;
}


/*********************************************************************
*
*       USBHID_Write
*
*  Function description
*    Writes an output report to device.
*
*  Return value:
*    On Error:   -1, No valid device Id used or the report size does not match with device.
*    On success: Number of bytes that have be written.
*
*/
int USBHID_Write(unsigned Id, const void * pBuffer, unsigned NumBytes) {
  CONN_INFO * pConnInfo;

  pConnInfo = _apConnInfo[Id];
  if (pConnInfo == NULL)  {
    return -1;   // Error device Id does not exist.
  }
  if (NumBytes != (pConnInfo->OutputReportByteLength - 1)) {
    return -1;   // Error report size does not match
  }
  return _Write(pConnInfo, pBuffer, NumBytes);
}



/*********************************************************************
*
*       USBHID_GetNumAvailableDevices
*
*  Function description
*    Returns the number of available devices.
*
*  Parameters
*    pMask - Pointer to unsigned integer value which is used to store
*            the bit mask of available devices. This parameter may be NULL.
*
*  Return value
*    Number of available devices.
*
*  Additional information
*    pMask will be filled by this routine. It shall be interpreted as
*    a bit mask where a bit set means this device is available.
*    For example, device 0 and device 2 are available, if pMask has
*    the value 0x00000005.
*/
unsigned USBHID_GetNumAvailableDevices(U32 * pMask) {
  unsigned      i;
  U32           Mask;
  CONN_INFO    * pConnInfo;

  if (_IsInited == 0 || _NumDevices == 0) {
    _IsInited = _Init();
  }
  Mask  = 0;
  for (i = 0; i < USB_MAX_DEVICES; i++) {
    pConnInfo = _apConnInfo[i];
    if (pConnInfo != NULL) {
      Mask |= (1 << i);
    }
  }
  if (pMask) {
    *pMask = Mask;
  }
  return _NumDevices;
}

/*********************************************************************
*
*       USBHID_GetProductName
*
*  Function description
*    Stores the name of the device into pBuffer.
*
*  Parameters
*    Id       - Index of the HID device. This is the bit number of
*               the mask returned by USBHID_GetNumAvailableDevices().
*    pBuffer  - Pointer to a buffer for the product name.
*    NumBytes - Size of the buffer in bytes.
*
*  Return value
*    == 0: An error occurred.
*    == 1: Success.
*/
int USBHID_GetProductName(unsigned Id, char * pBuffer, unsigned NumBytes) {
  int r;
  CONN_INFO * pConnInfo;
  U16       * pUniCodeString;
  HANDLE      hDevice;

  r = 0;
  pConnInfo = _apConnInfo[Id];
  if (pConnInfo) {
    unsigned i;

    pUniCodeString = (U16 *)malloc(NumBytes * 2);
    if (pUniCodeString == NULL) {
      return 0;
    }
    hDevice = CreateFile(pConnInfo->acDevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    r = HidD_GetProductString(hDevice, pUniCodeString , NumBytes * 2);
    for (i = 0; i < NumBytes; i++) {
      *pBuffer++ = (U8)pUniCodeString[i];
    }
    free(pUniCodeString);
    CloseHandle(hDevice);
  }
  return r;
}

/*********************************************************************
*
*       USBHID_GetSerialNumber
*
*  Function description
*    Stores the serial number string of the device into pBuffer.
*
*  Return value:
*    0    - Error
*    1    - O.K.
*
*/
int USBHID_GetSerialNumber(unsigned Id, char * pBuffer, unsigned NumBytes) {
  int r;
  CONN_INFO * pConnInfo;
  U16       * pUniCodeString;
  HANDLE      hDevice;

  r = 0;
  pConnInfo = _apConnInfo[Id];
  if (pConnInfo) {
    unsigned i;

    pUniCodeString = (U16 *)malloc(NumBytes * 2);
    if (pUniCodeString == NULL) {
      return 0;
    }
    hDevice = CreateFile(pConnInfo->acDevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    r = HidD_GetSerialNumberString(hDevice, pUniCodeString , NumBytes * 2);
    for (i = 0; i < NumBytes; i++) {
      *pBuffer++ = (U8)pUniCodeString[i];
    }
    if (i < NumBytes) {
      *pBuffer = 0;
    }
    free(pUniCodeString);
    CloseHandle(hDevice);
  }
  return r;
}

/*********************************************************************
*
*       USBHID_GetInputReportSize
*
*  Function description
*    Returns the input report size of the device.
*
*  Parameters
*    Id       - Index of the HID device. This is the bit number of
*               the mask returned by USBHID_GetNumAvailableDevices().
*
*  Return value:
*    == 0: An error occurred.
*    != 0: Size of the report in bytes.
*/
int USBHID_GetInputReportSize(unsigned Id) {
  if (_apConnInfo[Id] == NULL) {
    return 0;
  }
  return _apConnInfo[Id]->InputReportByteLength - 1;
}

/*********************************************************************
*
*       USBHID_GetOutputReportSize
*
*  Function description
*    Returns the output report size of the device.
*
*  Parameters
*    Id       - Index of the HID device. This is the bit number of
*               the mask returned by USBHID_GetNumAvailableDevices().
*
*  Return value:
*    == 0: An error occurred.
*    != 0: Size of the report in bytes.
*/
int USBHID_GetOutputReportSize(unsigned Id) {
  if (_apConnInfo[Id] == NULL) {
    return 0;
  }
  return _apConnInfo[Id]->OutputReportByteLength - 1;
}

/*********************************************************************
*
*       USBHID_GetProductId
*
*  Function description
*    Returns the USB product ID of the device.
*
*  Parameters
*    Id       - Index of the HID device. This is the bit number of
*               the mask returned by USBHID_GetNumAvailableDevices().
*
*  Return value:
*    == 0: An error occurred.
*    != 0: Product ID.
*/
U16 USBHID_GetProductId(unsigned Id) {
  if (_apConnInfo[Id] == NULL) {
    return 0;
  }
  return _apConnInfo[Id]->ProductId;
}

/*********************************************************************
*
*       USBHID_GetVendorId
*
*  Function description
*    Returns the USB vendor ID of the device.
*
*  Parameters
*    Id       - Index of the HID device. This is the bit number of
*               the mask returned by USBHID_GetNumAvailableDevices().
*
*  Return value:
*    == 0: An error occurred.
*    != 0: Vendor ID.
*/
U16 USBHID_GetVendorId(unsigned Id) {
  if (_apConnInfo[Id] == NULL) {
    return 0;
  }
  return _apConnInfo[Id]->VendorId;
}

/*********************************************************************
*
*       USBHID_SetVendorPage
*
*  Function description
*    Sets the vendor page so that all HID devices with the specified
*    page will be found.
*
*  Parameters
*    Page - This parameter specifies the lower 8 bits of
*           the vendor-specific usage page number.
*           It must be identical on both device and host.
*/
void USBHID_SetVendorPage(U8 Page) {
  _VendorPage = (0xff << 8) | Page;
}

/*********************************************************************
*
*       USBHID_RefreshList
*
*  Function description
*    Refreshes the connection info list.
*
*  Additional information
*    Note that any open handles will be closed while refreshing
*    the connection list.
*/
void USBHID_RefreshList(void) {
  USBHID_Exit();
  _Init();
}


/*************************** End of file ****************************/
