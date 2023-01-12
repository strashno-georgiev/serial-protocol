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
-------------------------- END-OF-HEADER -----------------------------

File    : USB_DFU_Start.c
Purpose : This sample demonstrates a CDC or HID echo server (see MAIN_INTERFACE define)
          that has an additional DFU interface. The device can be switched between runtime
          mode and DFU mode as described in the DFU device class specification.

Additional information:
  Preparations:
    Test can be done using the dfu-util tool.
    Take an arbitrary file and add a dfu-suffix:
      dfu-suffix -v 8765 -d 1011 -a <file>
    Start and connect device. Then download the file:
      dfu-util -d 8765:1010,:1011 -D <file>
    On Linux it may be necessary to execute this command with root rights.
    ATTENTION: if using the CDC interface, replace 1011 by 1021 and 1010 by 1020.

  Expected behavior:
    The device accepts the downloaded file an display it in the terminal window.

  Sample output (PC):
    dfu-util 0.9
    Copyright 2005-2009 Weston Schmidt, Harald Welte and OpenMoko Inc.
    Copyright 2010-2016 Tormod Volden and Stefan Schmidt
    This program is Free Software and has ABSOLUTELY NO WARRANTY
    Please report bugs to http://sourceforge.net/p/dfu-util/tickets/
    Opening DFU capable USB device...
    ID 8765:1020
    Run-time device DFU version 0110
    Claiming USB DFU Runtime Interface...
    Determining device status: state = appIDLE, status = 0
    Device really in Runtime Mode, send DFU detach request...
    Device will detach and reattach...
    Opening DFU USB Device...
    Claiming USB DFU Interface...
    Setting Alternate Setting #0 ...
    Determining device status: state = dfuIDLE, status = 0
    dfuIDLE, continuing
    DFU mode device DFU version 0110
    Device returned transfer size 256
    Copying data from PC to DFU device
    Download    [=========================] 100%         3042 bytes
    Download done.
    state(6) = dfuMANIFEST-SYNC, status(0) = No error condition is present
    state(6) = dfuMANIFEST-SYNC, status(0) = No error condition is present
    state(8) = dfuMANIFEST-WAIT-RESET, status(0) = No error condition is present
    Done!

  Sample output (Target):
    0:005 MainTask - Entering runtime mode...
    ....
    10:942 MainTask - Entering DFU mode...
    10:942 MainTask - USBD_Start
    10:942 MainTask - USB EHCI: 3040 bytes unused EP buffer RAM (USB_ENDPOINT_BUFFER_POOL_SIZE / USBD_AssignMemory())
    13:939 MainTask - Download received, block #0:
    13:939 MainTask - 0000  2F 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A
    13:939 MainTask - 0010  2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A
    13:939 MainTask - 0020  2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A
    13:939 MainTask - 0030  2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A
    13:939 MainTask - 0040  2A 2A 2A 2A 2A 2A 0D 0A 2A 20 20 20 20 20 20 20
    13:939 MainTask - 0050  20 20 20 20 20 20 20 20 20 20 20 20 28 63 29 20
    13:939 MainTask - 0060  53 45 47 47 45 52 20 4D 69 63 72 6F 63 6F 6E 74
    13:939 MainTask - 0070  72 6F 6C 6C 65 72 20 47 6D 62 48 20 20 20 20 20
    13:939 MainTask - 0080  20 20 20 20 20 20 20 20 20 20 20 20 20 2A 0D 0A
    13:939 MainTask - 0090  2A 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20
    13:940 MainTask - 00A0  20 20 20 20 20 20 20 20 20 54 68 65 20 45 6D 62
    13:940 MainTask - 00B0  65 64 64 65 64 20 45 78 70 65 72 74 73 20 20 20
    13:940 MainTask - 00C0  20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20
    13:940 MainTask - 00D0  20 20 20 20 20 2A 0D 0A 2A 2A 2A 2A 2A 2A 2A 2A
    13:941 MainTask - 00E0  2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A
    13:941 MainTask - 00F0  2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A
    14:331 MainTask - Download received, block #1:
    14:331 MainTask - 0000  2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A
    14:331 MainTask - 0010  2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 0D 0A
    ....
    18:306 MainTask - 0090  69 6F 6E 20 2A 2F 0D 0A 0D 0A 2F 2A 2A 2A 2A 2A
    18:306 MainTask - 00A0  2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A
    18:306 MainTask - 00B0  2A 2A 2A 2A 2A 2A 20 45 6E 64 20 6F 66 20 66 69
    18:306 MainTask - 00C0  6C 65 20 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A
    18:306 MainTask - 00D0  2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2A 2F
    18:307 MainTask - 00E0  0D 0A
    18:703 MainTask - Download finished, now manifestation
    20:703 MainTask - Done.
    23:208 MainTask - Entering runtime mode...
*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/
#define MAIN_INTERFACE    0       // 0 = HID, 1 = CDC

#include <string.h>
#include "USB.h"
#if MAIN_INTERFACE == 0
#include "USB_HID.h"
#endif
#if MAIN_INTERFACE == 1
#include "USB_CDC.h"
#endif
#include "USB_DFU.h"
#include "BSP.h"

/*********************************************************************
*
*       Defines configurable
*
**********************************************************************
*/
//
// Definitions for the HID interface
//
#define INPUT_REPORT_SIZE   64    // Defines the input (device -> host) report size
#define OUTPUT_REPORT_SIZE  64    // Defines the output (Host -> device) report size
#define VENDOR_PAGE_ID      0x12  // Defines the vendor specific page that
                                  // shall be used, allowed values 0x00 - 0xff
                                  // This value must be identical to HOST application

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/
//
//  Information that is used during enumeration.
//
static const USB_DEVICE_INFO _DeviceInfoRT = {
  0x8765,                             // VendorId
  0x1010 + 0x10 * MAIN_INTERFACE,     // ProductId. Should be unique for this sample
  "Vendor",                           // VendorName
  "DFU sample",                       // ProductName
  "12345678"                          // SerialNumber
};

static const USB_DEVICE_INFO _DeviceInfo = {
  0x8765,                             // VendorId
  0x1011 + 0x10 * MAIN_INTERFACE,     // ProductId. Should be unique for this sample
  "Vendor",                           // VendorName
  "DFU bootloader",                   // ProductName
  "12345678"                          // SerialNumber
};

static U8                      _DownloadBuff[256];
static USBD_DFU_DOWNLOAD       _Download;
static USBD_DFU_DETACH_REQUEST _Detach;

//
// DFU runtime configuration
//
static const USB_DFU_INIT_DATA _DFUInfoRT = {
  USB_DFU_MODE_RUNTIME,                                     // Mode
  USB_DFU_ATTR_WILL_DETACH,                                 // Attributes
  5000,                                                     // Detach timeout
  0,                                                        // Transfer size
  0,                                                        // Flags
  "DFU interface",                                          // Interface name
  _Detach,                                                  // Callback for detach request
  NULL,                                                     // Callback for download data
  NULL,                                                     // Download buffer
  NULL                                                      // Callback for upload data
};

//
// DFU mode configuration
//
static const USB_DFU_INIT_DATA _DFUInfo = {
  USB_DFU_MODE_DFU,                                         // Mode
  USB_DFU_ATTR_DOWNLOAD,                                    // Attributes
  0,                                                        // Detach timeout
  sizeof(_DownloadBuff),                                    // Transfer size
  0,                                                        // Flags
  "DFU interface",                                          // Interface name
  NULL,                                                     // Callback for detach request
  _Download,                                                // Callback for download data
  _DownloadBuff,                                            // Download buffer
  NULL                                                      // Callback for upload data
};


#if MAIN_INTERFACE == 0
static const U8 _aHIDReport[] = {
    0x06, VENDOR_PAGE_ID, 0xFF,    // USAGE_PAGE (Vendor Defined Page 1)
    0x09, 0x01,                    // USAGE (Vendor Usage 1)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x05, 0x06,                    //   USAGE_PAGE (Generic Device)
    0x09, 0x00,                    //   USAGE (Undefined)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x95, INPUT_REPORT_SIZE,       //   REPORT_COUNT (64)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x05, 0x06,                    //   USAGE_PAGE (Generic Device)
    0x09, 0x00,                    //   USAGE (Undefined)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x95, OUTPUT_REPORT_SIZE,      //   REPORT_COUNT (64)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
    0xc0                           // END_COLLECTION
};
#endif

/*********************************************************************
*
*       Static
*
**********************************************************************
*/

static int   _NumBytesDownload;
static U16   _BlockNo;
static I8    _DownloadEvent;
static I8    _DetachEvent;
static char  _ac[USB_HS_INT_MAX_PACKET_SIZE];

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _AddHID
*
*  Function description
*    Add HID class to USB stack
*/
#if MAIN_INTERFACE == 0
static USB_HID_HANDLE _AddHID(void) {
  static U8 _abOutBuffer[USB_HS_INT_MAX_PACKET_SIZE];

  USB_HID_INIT_DATA     InitData;
  USB_ADD_EP_INFO       EPIntIn;
  USB_ADD_EP_INFO       EPIntOut;
  USB_HID_HANDLE        hInst;

  memset(&InitData, 0, sizeof(InitData));
  EPIntIn.Flags           = 0;                             // Flags not used.
  EPIntIn.InDir           = USB_DIR_IN;                    // IN direction (Device to Host)
  EPIntIn.Interval        = 8;                             // Interval of 1 ms (125 us * 8)
  EPIntIn.MaxPacketSize   = USB_HS_INT_MAX_PACKET_SIZE;    // Maximum packet size (64 for Interrupt).
  EPIntIn.TransferType    = USB_TRANSFER_TYPE_INT;         // Endpoint type - Interrupt.
  InitData.EPIn = USBD_AddEPEx(&EPIntIn, NULL, 0);

  EPIntOut.Flags           = 0;                             // Flags not used.
  EPIntOut.InDir           = USB_DIR_OUT;                   // OUT direction (Host to Device)
  EPIntOut.Interval        = 8;                             // Interval of 1 ms (125 us * 8)
  EPIntOut.MaxPacketSize   = USB_HS_INT_MAX_PACKET_SIZE;    // Maximum packet size (64 for Interrupt).
  EPIntOut.TransferType    = USB_TRANSFER_TYPE_INT;         // Endpoint type - Interrupt.
  InitData.EPOut = USBD_AddEPEx(&EPIntOut, _abOutBuffer, sizeof(_abOutBuffer));

  InitData.pReport = _aHIDReport;
  InitData.NumBytesReport  = sizeof(_aHIDReport);
  hInst = USBD_HID_Add(&InitData);
  return hInst;
}
#endif

/*********************************************************************
*
*       _AddCDC
*
*  Function description
*    Add communication device class to USB stack
*/
#if MAIN_INTERFACE == 1
static USB_CDC_HANDLE _AddCDC(void) {
  static U8             _abOutBuffer[USB_HS_BULK_MAX_PACKET_SIZE];
  USB_CDC_INIT_DATA     InitData;
  USB_ADD_EP_INFO       EPBulkIn;
  USB_ADD_EP_INFO       EPBulkOut;
  USB_ADD_EP_INFO       EPIntIn;
  USB_CDC_HANDLE        hInst;

  memset(&InitData, 0, sizeof(InitData));
  EPBulkIn.Flags          = 0;                             // Flags not used.
  EPBulkIn.InDir          = USB_DIR_IN;                    // IN direction (Device to Host)
  EPBulkIn.Interval       = 0;                             // Interval not used for Bulk endpoints.
  EPBulkIn.MaxPacketSize  = USB_HS_BULK_MAX_PACKET_SIZE;   // Maximum packet size (512 for Bulk in high-speed).
  EPBulkIn.TransferType   = USB_TRANSFER_TYPE_BULK;        // Endpoint type - Bulk.
  InitData.EPIn  = USBD_AddEPEx(&EPBulkIn, NULL, 0);

  EPBulkOut.Flags         = 0;                             // Flags not used.
  EPBulkOut.InDir         = USB_DIR_OUT;                   // OUT direction (Host to Device)
  EPBulkOut.Interval      = 0;                             // Interval not used for Bulk endpoints.
  EPBulkOut.MaxPacketSize = USB_HS_BULK_MAX_PACKET_SIZE;   // Maximum packet size (512 for Bulk in high-speed).
  EPBulkOut.TransferType  = USB_TRANSFER_TYPE_BULK;        // Endpoint type - Bulk.
  InitData.EPOut = USBD_AddEPEx(&EPBulkOut, _abOutBuffer, sizeof(_abOutBuffer));

  EPIntIn.Flags           = 0;                             // Flags not used.
  EPIntIn.InDir           = USB_DIR_IN;                    // IN direction (Device to Host)
  EPIntIn.Interval        = 64;                            // Interval of 8 ms (64 * 125us)
  EPIntIn.MaxPacketSize   = USB_HS_INT_MAX_PACKET_SIZE;    // Maximum packet size (64 for Interrupt).
  EPIntIn.TransferType    = USB_TRANSFER_TYPE_INT;         // Endpoint type - Interrupt.
  InitData.EPInt = USBD_AddEPEx(&EPIntIn, NULL, 0);

  hInst = USBD_CDC_Add(&InitData);
  return hInst;
}
#endif

/*********************************************************************
*
*       _Download
*
*  Function description
*    Called, if the host has send a download packet.
*/
static void _Download(int NumBytes, U16 BlockNum) {
  _BlockNo          = BlockNum;
  _NumBytesDownload = NumBytes;
  _DownloadEvent    = 1;
}

/*********************************************************************
*
*       _Detach
*
*  Function description
*    Called, if the host has send a detach request.
*/
static void _Detach(U16 Timeout) {
  USB_USE_PARA(Timeout);
  _DetachEvent = 1;
}

/*********************************************************************
*
*       _RuntimeTask
*
*  Function description
*    Executed when running in runtime mode
*/
static void _RuntimeTask(void) {
#if MAIN_INTERFACE == 0
  USB_HID_HANDLE hInst;
#endif
#if MAIN_INTERFACE == 1
  USB_CDC_HANDLE hInst;
  int            NumBytesReceived;
#endif

  USBD_Init();
  USBD_Logf_Application("Entering runtime mode...");
  USBD_SetDeviceInfo(&_DeviceInfoRT);
  USBD_DFU_Add(&_DFUInfoRT);
  USBD_DFU_SetMSDescInfo();
#if MAIN_INTERFACE == 0
  hInst = _AddHID();
#endif
#if MAIN_INTERFACE == 1
  USBD_EnableIAD();
  hInst = _AddCDC();
#endif
  USBD_Start();
  _DetachEvent = 0;
  while (_DetachEvent == 0) {
    //
    // Wait for configuration
    //
    while ((USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)) != USB_STAT_CONFIGURED) {
      BSP_ToggleLED(0);
      USB_OS_Delay(50);
    }
    BSP_SetLED(0);
#if MAIN_INTERFACE == 0
    if (USBD_HID_Read(hInst, &_ac[0], OUTPUT_REPORT_SIZE, 50) > 0) {
      _ac[0]++;
      USBD_HID_Write(hInst, &_ac[0], INPUT_REPORT_SIZE, 500);
    }
#endif
#if MAIN_INTERFACE == 1
    NumBytesReceived = USBD_CDC_Receive(hInst, &_ac[0], sizeof(_ac), 50);
    if (NumBytesReceived > 0) {
      USBD_CDC_Write(hInst, &_ac[0], NumBytesReceived, 0);
    }
#endif
  }
  USB_OS_Delay(50);
}

/*********************************************************************
*
*       _DFUTask
*
*  Function description
*    Executed when running in DFU mode (e.g. bootloader)
*/
static void _DFUTask(void) {
  int Cnt;

  USBD_Init();
  USBD_Logf_Application("Entering DFU mode...");
  USBD_SetDeviceInfo(&_DeviceInfo);
  USBD_DFU_Add(&_DFUInfo);
  USBD_DFU_SetMSDescInfo();
  USBD_Start();
  _DownloadEvent = 0;
  //
  // Wait for configuration
  //
  while ((USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)) != USB_STAT_CONFIGURED) {
    BSP_ToggleLED(0);
    USB_OS_Delay(50);
  }
  BSP_SetLED(0);
  while ((USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)) == USB_STAT_CONFIGURED) {
    if (_DownloadEvent != 0) {
      if (_NumBytesDownload < 0) {
        USBD_Logf_Application("Download aborted by host");
        break;
      }
      if (_NumBytesDownload > 0) {
        USBD_Logf_Application("Download received, block #%u:", _BlockNo);
        USBD_LogHexDump(USB_MTYPE_INFO, _NumBytesDownload, _DownloadBuff);
        USB_OS_Delay(300);
        _DownloadEvent = 0;
        USBD_DFU_Ack();
      }
      if (_NumBytesDownload == 0) {
        USBD_Logf_Application("Download finished, now manifestation");
        USBD_DFU_SetPollTimeout(500);
        USBD_DFU_Ack();
        USB_OS_Delay(2000);
        USBD_Logf_Application("Done.");
        USBD_DFU_ManifestComplt();
        //
        // Wait for host to receive status
        //
        Cnt = 2000;
        while (--Cnt > 0 && USBD_DFU_GetStatusReqCnt() == 0 &&
               (USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)) == USB_STAT_CONFIGURED) {
          USB_OS_Delay(5);
        }
        USB_OS_Delay(5);
        break;
      }
    }
  }
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       MainTask
*
* Function description
*   USB handling task.
*   Modify to implement the desired protocol
*/
#ifdef __cplusplus
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif
void MainTask(void);
#ifdef __cplusplus
}
#endif
void MainTask(void) {
  for (;;) {
    //
    // Start-up in runtime mode
    //
    _RuntimeTask();
    //
    // If the function returns, we got a detach request.
    // Now configure for DFU mode.
    //
    USBD_DeInit();
    USB_OS_Delay(2000);
    _DFUTask();
    //
    // Back to runtime mode
    //
    USBD_DeInit();
    USB_OS_Delay(2000);
  }
}
