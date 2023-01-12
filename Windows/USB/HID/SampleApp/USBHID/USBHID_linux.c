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
File    : USBHID_linux.c
Purpose : Implementation of the USBHID functions
---------------------------END-OF-HEADER------------------------------
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <libudev.h>
#include "USBHID.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#define HID_DEBUG   0


/*********************************************************************
*
*       Defines, non configurable
*
**********************************************************************
*/

//
// HID report descriptor defines for the simple parser.
//
#define USBH_HID_REPORT_COUNT             0x95
#define USBH_HID_REPORT_SIZE              0x75
#define USBH_HID_REPORT_INPUT             0x80
#define USBH_HID_REPORT_OUTPUT            0x90
#define USBH_HID_REPORT_INPUT_OUTPUT_MASK 0xFC
#define USBH_HID_REPORT_LONG_ITEM         0xFE

/*********************************************************************
*
*       Types
*
**********************************************************************
*/
typedef struct {
  int        hDevice;           // handle to device
  unsigned   InputReportByteLength;
  unsigned   OutputReportByteLength;
  U16        VendorId;
  U16        ProductId;
  char       acDevicePath[64];
  char       acProductName[64];
  char       acSerial[32];
} CONN_INFO;

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static CONN_INFO * _apConnInfo[USB_MAX_DEVICES];
static unsigned    _NumDevices;
static U16         _VendorPage;

static const char  _sRptDescPath[] = "/device/report_descriptor";

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/


/*********************************************************************
*
*       _ReadReportDescriptor
*
*  Function description
*    Read and parse report descriptor
*
*/
static int _ReadReportDescriptor(const char *pPath, CONN_INFO *pInfo, U16 *pVendorPage) {
  char acBuff[512];
  int  Len;
  int  fd;
  U8   *p;
  U16  RptSize;
  U16  RptCount;
  U32  ReportLenTemp;
  U32  InRptLen;
  U32  OutRptLen;

  pInfo->InputReportByteLength  = 0;
  pInfo->OutputReportByteLength = 0;
  *pVendorPage = 0;
  strncpy(acBuff, pPath, sizeof(acBuff) - sizeof(_sRptDescPath) - 3);
  strcat(acBuff, _sRptDescPath);
  fd = open(acBuff, O_RDONLY);
  if (fd < 0) {
#if HID_DEBUG
    fprintf(stderr, "HID: can't open '%s'\n", acBuff);
#endif
    return -1;
  }
  Len = read(fd, acBuff, sizeof(acBuff));
  close(fd);
#if HID_DEBUG
  {
    int i;
    fprintf(stderr,"HID: report descriptor:\n");
    for (i = 0; i < Len; i++) {
      fprintf(stderr," %02X", acBuff[i] & 0xFF);
    }
    fprintf(stderr,"\n");
  }
#endif
  p = (unsigned char *)acBuff;
  RptSize  = 0;
  RptCount = 0;
  ReportLenTemp = 0;
  InRptLen = 0;
  OutRptLen = 0;
  while (Len > 0) {
    unsigned char cc;
    int ItemLen;

    cc = *p;
    if (cc == 0x06 && Len >= 3) {
     *pVendorPage = p[1] | (p[2] << 8);
    }
    if ((cc & USBH_HID_REPORT_INPUT_OUTPUT_MASK) == USBH_HID_REPORT_INPUT) {    // Input tag
      InRptLen += ReportLenTemp;
      ReportLenTemp = 0;
#if HID_DEBUG
      fprintf(stderr, "HID: INPUT report size = %u bits\n", (unsigned)InRptLen);
#endif
    }
    if ((cc & USBH_HID_REPORT_INPUT_OUTPUT_MASK) == USBH_HID_REPORT_OUTPUT) {    // Output tag
      OutRptLen += ReportLenTemp;
      ReportLenTemp = 0;
#if HID_DEBUG
      fprintf(stderr, "HID: OUTPUT report size = %u bits\n", (unsigned)OutRptLen);
#endif
    }
    if (cc == USBH_HID_REPORT_COUNT && Len >= 2) { // Report count
      RptCount = p[1];
#if HID_DEBUG
      fprintf(stderr, "HID: report count = %u\n", RptCount);
#endif
    }
    if (cc == USBH_HID_REPORT_SIZE && Len >= 2) { // Report size
      RptSize = p[1];
#if HID_DEBUG
      fprintf(stderr, "HID: report size (bits) = %u\n", RptSize);
#endif
    }
    //
    // Multiple report size/count pairs can exist
    //
    if (RptCount != 0 && RptSize != 0) {
      ReportLenTemp += RptCount * RptSize;
      RptCount = 0;
      RptSize  = 0;
    }
    //
    // now move to next item
    //
    if ((cc & USBH_HID_REPORT_LONG_ITEM) == USBH_HID_REPORT_LONG_ITEM) {    // Long item
      if (Len < 3) {
        break;
      }
      ItemLen = p[1] + 3;
    } else {
      //
      // short item
      // 0 = 0 bytes
      // 1 = 1 byte
      // 2 = 2 bytes
      // 3 = 4 bytes
      //
      ItemLen = (cc & 3) + 1;
      if (ItemLen == 4) {
        ItemLen = 5;
      }
    }
    p   += ItemLen;
    Len -= ItemLen;
  }
  //
  // Round up to the next byte.
  //
  pInfo->InputReportByteLength  = (InRptLen + 7) >> 3;
  pInfo->OutputReportByteLength = (OutRptLen + 7) >> 3;
#if HID_DEBUG
  fprintf(stderr, "HID: final INPUT report size = %u\n", pInfo->InputReportByteLength);
  fprintf(stderr, "HID: final OUTPUT report size = %u\n", pInfo->OutputReportByteLength);
#endif
  return 0;
}

/*********************************************************************
*
*       _Enumerate
*
*  Function description
*    Look for HID devices that comply with the Vendor page set.
*
*/
static void _Enumerate(void) {
  const char  *pSysfsPath;
  const char  *pDevPath;
  const char  *pStr;
  CONN_INFO    Info;
  U16          VendorPage;
  struct udev *pUDev;
  struct udev_enumerate  *pEnumerate;
  struct udev_list_entry *pDevices;
  struct udev_list_entry *pDevListEntry;
  struct udev_device     *pHidDev;        // The device's HID udev node.
  struct udev_device     *pDev;           // The actual hardware device.

  pUDev = udev_new();
  if (pUDev == NULL) {
#if HID_DEBUG
    fprintf(stderr,"HID: Can't create udev\n");
#endif
    return;
  }
  //
  // Create a list of the devices in the 'hidraw' subsystem.
  //
  pEnumerate = udev_enumerate_new(pUDev);
  udev_enumerate_add_match_subsystem(pEnumerate, "hidraw");
  udev_enumerate_scan_devices(pEnumerate);
  pDevices = udev_enumerate_get_list_entry(pEnumerate);
  //
  // For each item, see if it matches the VID / PID, and if so
  // create a udev_device record for it
  //
  for (pDevListEntry = pDevices; pDevListEntry != NULL; pDevListEntry = udev_list_entry_get_next(pDevListEntry)) {
    //
    // Get the filename of the /sys entry for the device
    // and create a udev_device object representing it
    //
    pSysfsPath = udev_list_entry_get_name(pDevListEntry);
    pHidDev = udev_device_new_from_syspath(pUDev, pSysfsPath);
    pDevPath = udev_device_get_devnode(pHidDev);
    //
    // The device pointed to by pHidDev contains information about
    // the hidraw device. In order to get information about the
    // USB device, get the parent device with the
    // subsystem/devtype pair of "usb"/"usb_device". This will
    // be several levels up the tree, but the function will find
    // it.
    //
    pDev = udev_device_get_parent_with_subsystem_devtype(pHidDev, "usb", "usb_device");
    if (pDev) {
      memset(&Info, 0, sizeof(Info));
      //
      // Get VID and PID of the device
      //
      pStr = udev_device_get_sysattr_value(pDev,"idVendor");
      Info.VendorId = (pStr != NULL) ? strtol(pStr, NULL, 16) : 0x0;
      pStr = udev_device_get_sysattr_value(pDev, "idProduct");
      Info.ProductId = (pStr != NULL) ? strtol(pStr, NULL, 16) : 0x0;
      pStr = udev_device_get_sysattr_value(pDev, "product");
      if (pStr) {
        strncpy(Info.acProductName, pStr, sizeof(Info.acProductName) - 1);
      }
      pStr = udev_device_get_sysattr_value(pDev, "serial");
      if (pStr) {
        strncpy(Info.acSerial, pStr, sizeof(Info.acSerial) - 1);
      }
#if HID_DEBUG
      fprintf(stderr,"HID: Found device '%s' VID=%04X PID=%04X -> %s\n", Info.acProductName, Info.VendorId, Info.ProductId, pDevPath);
#endif
      if (_ReadReportDescriptor(pSysfsPath, &Info, &VendorPage) == 0 &&
          VendorPage == _VendorPage &&
          _NumDevices < USB_MAX_DEVICES) {
        //
        // Allocate memory for connection info if necessary.
        //
        if (_apConnInfo[_NumDevices] == NULL) {
          _apConnInfo[_NumDevices] = (CONN_INFO *)calloc(sizeof(CONN_INFO), 1);
        }
        Info.hDevice   = -1;
        strncpy(Info.acDevicePath, pDevPath, sizeof(Info.acDevicePath));
        *_apConnInfo[_NumDevices] = Info;
        _NumDevices++;
      }
    }
    udev_device_unref(pHidDev);
  }
  udev_enumerate_unref(pEnumerate);
  udev_unref(pUDev);
}



/*********************************************************************
*
*       _Id2Info
*
*  Function description
*    returns CONN_INFO pointer for a device with given id.
*/
static CONN_INFO *_Id2Info(unsigned Id) {
  if (Id < USB_MAX_DEVICES) {
    return(_apConnInfo[Id]);
  }
  return NULL;
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
*    Opens a handle to the device that shall be opened.
*
*  Return value:
*    0       -  O.K. Opening was successful or already opened.
*    1       -  Error. Handle to the device could not opened.
*/
int USBHID_Open(unsigned Id) {
  CONN_INFO * pConnInfo;
  int         hDevice;

  pConnInfo = _Id2Info(Id);
  if (pConnInfo == NULL)  {
    return 1;   // Error device Id does not exist.
  }
  if (pConnInfo->hDevice < 0) {
    hDevice = open(pConnInfo->acDevicePath, O_RDWR);
    if (hDevice < 0) {
      return 1;
    }
    pConnInfo->hDevice = hDevice;
  }
  return 0;
}

/*********************************************************************
*
*       USBHID_Close
*
*  Function description
*    Close the connection to an open device.
*
*/
void USBHID_Close(unsigned Id) {
  CONN_INFO * pConnInfo;

  pConnInfo = _Id2Info(Id);
  if (pConnInfo) {
    if (pConnInfo->hDevice >= 0) {
      close(pConnInfo->hDevice);
    }
    pConnInfo->hDevice = -1;
  }
}

/*********************************************************************
*
*       USBHID_Init
*
*  Function description
*   Initializes the USB HID User API.
*/
void USBHID_Init(U8 VendorPage) {
  USBHID_SetVendorPage(VendorPage);
  _Enumerate();
}

/*********************************************************************
*
*       USBHID_Exit
*
*  Function description
*    Close the connection an open device.
*/
void USBHID_Exit(void) {
  CONN_INFO * pConnInfo;
  unsigned    i;

  for (i = 0; i < _NumDevices; i++) {
    pConnInfo = _apConnInfo[i];
    if (pConnInfo) {
      if (pConnInfo->hDevice >= 0) {
        close(pConnInfo->hDevice);
      }
      free(pConnInfo);
      _apConnInfo[i] = NULL;
    }
  }
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
  int         r;

  pConnInfo = _Id2Info(Id);
  if (pConnInfo == NULL || pConnInfo->hDevice < 0)  {
    return -1;   // Error device Id does not exist or not open.
  }
  if (NumBytes != (pConnInfo->InputReportByteLength)) {
    return -1;   // Error report size does not match
  }
#if HID_DEBUG
  fprintf(stderr,"HID: wait for %d bytes to read\n", NumBytes);
#endif
  r = read(pConnInfo->hDevice, pBuffer, NumBytes);
#if HID_DEBUG
  fprintf(stderr,"HID: %d bytes read\n", r);
#endif
  return r;
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
//U32  NumBytesRead = 0;

  pConnInfo = _Id2Info(Id);
  if (pConnInfo == NULL || pConnInfo->hDevice < 0)  {
    return -1;   // Error device Id does not exist or not open.
  }
  if (NumBytes != (pConnInfo->InputReportByteLength)) {
    return -1;   // Error report size does not match
  }

  return -1;        // not implemented yet

  //return NumBytesRead;
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
  int res;
  char *pTmpBuff;

  pConnInfo = _Id2Info(Id);
  if (pConnInfo == NULL || pConnInfo->hDevice < 0)  {
    return -1;   // Error device Id does not exist or not open.
  }
  if (NumBytes != (pConnInfo->OutputReportByteLength)) {
    return -1;   // Error report size does not match
  }
  pTmpBuff = alloca(pConnInfo->OutputReportByteLength + 1);
  memcpy(pTmpBuff + 1, pBuffer, NumBytes);
  *pTmpBuff = 0;
  res = write(pConnInfo->hDevice, pTmpBuff, NumBytes + 1);
#if HID_DEBUG
  fprintf(stderr,"HID: %d bytes written\n", res);
  if (res < 0) {
    fprintf(stderr, "errno=%d\n",errno);
    perror("HID: write");
  }
#endif
  if (res > 0) {
    res--;
  }
  return res;
}


/*********************************************************************
*
*       USBHID_GetNumAvailableDevices
*
*  Function description
*    Returns the number of available devices.
*    pMask will be filled by this routine.
*    pMask shall be interpreted as bit mask
*    where a bit set means this device is available.
*    eg. *pMask = 0x00000005 -> Device 0 and device 2 are available.
*
*  Return value:
*    Number of available device(s).
*
*/
unsigned USBHID_GetNumAvailableDevices(U32 * pMask) {
  unsigned      i;
  U32           Mask;
  CONN_INFO    * pConnInfo;

  if (_NumDevices == 0) {
    _Enumerate();
  }
  Mask  = 0;
  for (i = 0; i < _NumDevices; i++) {
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
*  Return value:
*    0    - Error
*    1    - O.K.
*
*/
int USBHID_GetProductName(unsigned Id, char * pBuffer, unsigned NumBytes) {
  CONN_INFO    * pConnInfo;

  pConnInfo = _Id2Info(Id);
  if (pConnInfo) {
    strncpy(pBuffer, pConnInfo->acProductName, NumBytes);
    return 1;
  }
  return 0;
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
  CONN_INFO    * pConnInfo;

  pConnInfo = _Id2Info(Id);
  if (pConnInfo) {
    strncpy(pBuffer, pConnInfo->acSerial, NumBytes);
    return 1;
  }
  return 0;
}

/*********************************************************************
*
*       USBHID_GetInputReportSize
*
*  Function description
*    Returns the input report size of the device.
*
*  Return value:
*    == 0    - Error
*    <> 0    - report size in bytes.
*/
int USBHID_GetInputReportSize(unsigned Id) {
  CONN_INFO    * pConnInfo;

  pConnInfo = _Id2Info(Id);
  if (pConnInfo) {
    return pConnInfo->InputReportByteLength;
  }
  return 0;
}

/*********************************************************************
*
*       USBHID_GetOutputReportSize
*
*  Function description
*    Returns the outpur report size of the device.
*
*  Return value:
*    == 0    - Error
*    <> 0    - report size in bytes.
*/
int USBHID_GetOutputReportSize(unsigned Id) {
  CONN_INFO    * pConnInfo;

  pConnInfo = _Id2Info(Id);
  if (pConnInfo) {
    return pConnInfo->OutputReportByteLength;
  }
  return 0;
}

/*********************************************************************
*
*       USBHID_GetProductId
*
*  Function description
*    Returns the USB product id of the device.
*
*  Return value:
*    == 0    - Error
*    <> 0    - Product Id
*/
U16 USBHID_GetProductId(unsigned Id) {
  CONN_INFO    * pConnInfo;

  pConnInfo = _Id2Info(Id);
  if (pConnInfo) {
    return pConnInfo->ProductId;
  }
  return 0;
}

/*********************************************************************
*
*       USBHID_GetVendorId
*
*  Function description
*    Returns the USB vendor id of the device.
*
*  Return value:
*    == 0    - Error
*    <> 0    - Vendor Id
*/
U16 USBHID_GetVendorId(unsigned Id) {
  CONN_INFO    * pConnInfo;

  pConnInfo = _Id2Info(Id);
  if (pConnInfo) {
    return pConnInfo->VendorId;
  }
  return 0;
}

/*********************************************************************
*
*       USBHID_SetVendorPage
*
*  Function description
*    Sets the vendor page so that all HID devices with the specified
*    page will be found.
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
*    Notes:
*      (1)   - Be aware. Any open handles to the device will be closed.
*
*/
void USBHID_RefreshList(void) {
  USBHID_Exit();
  _Enumerate();
}


/*************************** End of file ****************************/
