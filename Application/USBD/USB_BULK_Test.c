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

File    : USB_BULK_Test.c
Purpose : The sample sends/receives different packet sizes and
          measures the transfer speed.

Additional information:
  Preparations:
    The sample should be used together with it's PC counterpart
    found under \Windows\USB\BULK\SampleApplication\

    On Windows this sample requires the WinUSB driver.
    This driver, if not already installed, is retrieved via
    Windows Update automatically when a device running this
    sample is connected. If Windows Update is disabled you can
    install the driver manually, see \Windows\USB\BULK\WinUSBInstall .

    On Linux either root or udev rules are required to access
    the bulk device, see \Windows\USB\BULK\USBBULK_API_Linux .

    On macOS bulk devices can be accessed without additional
    changes, see \Windows\USB\BULK\USBBULK_API_MacOSX .

  Expected behavior:
    After running the PC counterpart and connecting the USB cable
    the PC counterpart should start the test automatically.


  Sample output:
    The target side does not produce terminal output.
    PC counterpart output:
    Found 1 device
    Found the following device 0:
    Vendor Name : Vendor
    Product Name: Bulk test
    Serial no.  : 13245678
    To which device do you want to connect?
    Please type in device number (e.g. '0' for the first device, q/a for abort):0
    Writing one byte
    Reading one byte
    Operation successful!

    Writing 1 bytes
    Reading 1 bytes
    Writing 2 bytes
    Reading 2 bytes

    <...>

    Writing 1024 bytes
    Reading 1024 bytes
    Testing speed:..................................................
    Performance: 1063 ms for 4MB
    Communication with USB BULK device was successful!
*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/
#include <stdio.h>
#include "USB_Bulk.h"
#include "BSP.h"

/*********************************************************************
*
*       Information that are used during enumeration
*/
static const USB_DEVICE_INFO _DeviceInfo = {
  0x8765,         // VendorId
  0x1240,         // ProductId
  "Vendor",       // VendorName
  "Bulk test",    // ProductName
  "13245678"      // SerialNumber
};

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static U32 _ac[4100 / 4];

/*********************************************************************
*
*       _AddBULK
*
*  Function description
*    Add generic USB BULK interface to USB stack
*/
static USB_BULK_HANDLE _AddBULK(void) {
  static U8             _abOutBuffer[USB_HS_BULK_MAX_PACKET_SIZE * 2];
  USB_BULK_INIT_DATA    InitData;
  USB_ADD_EP_INFO       EPIn;
  USB_ADD_EP_INFO       EPOut;
  USB_BULK_HANDLE       hInst;

  memset(&InitData, 0, sizeof(InitData));
  EPIn.Flags          = 0;                             // Flags not used.
  EPIn.InDir          = USB_DIR_IN;                    // IN direction (Device to Host)
  EPIn.Interval       = 0;                             // Interval not used for Bulk endpoints.
  EPIn.MaxPacketSize  = USB_HS_BULK_MAX_PACKET_SIZE;   // Maximum packet size (512 for Bulk in high-speed).
  EPIn.TransferType   = USB_TRANSFER_TYPE_BULK;        // Endpoint type - Bulk.
  InitData.EPIn  = USBD_AddEPEx(&EPIn, NULL, 0);

  EPOut.Flags         = 0;                             // Flags not used.
  EPOut.InDir         = USB_DIR_OUT;                   // OUT direction (Host to Device)
  EPOut.Interval      = 0;                             // Interval not used for Bulk endpoints.
  EPOut.MaxPacketSize = USB_HS_BULK_MAX_PACKET_SIZE;   // Maximum packet size (512 for Bulk in high-speed).
  EPOut.TransferType  = USB_TRANSFER_TYPE_BULK;        // Endpoint type - Bulk.
  InitData.EPOut = USBD_AddEPEx(&EPOut, _abOutBuffer, sizeof(_abOutBuffer));

  hInst = USBD_BULK_Add(&InitData);
  USBD_BULK_SetMSDescInfo(hInst);
  return hInst;
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
  USB_BULK_HANDLE hInst;

  USBD_Init();
  hInst = _AddBULK();
  USBD_SetDeviceInfo(&_DeviceInfo);
  USBD_Start();
  while (1) {
    unsigned char c;

    //
    // Wait for configuration
    //
    while ((USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)) != USB_STAT_CONFIGURED) {
      BSP_ToggleLED(0); // Toggle LED to indicate configuration
      USB_OS_Delay(50);
    }
    BSP_SetLED(0);

    USBD_BULK_Read(hInst, &c, 1, 0);
    if (c > 0x10) {
      c++;
      USBD_BULK_Write(hInst, &c, 1, 0);
    } else {
      int NumBytes = c << 8;
      USBD_BULK_Read(hInst, &c, 1, 0);
      NumBytes |= c;
      if (NumBytes <= (int)sizeof(_ac)) {
        USBD_BULK_Read(hInst, _ac, NumBytes, 0);
        USBD_BULK_Write(hInst, _ac, NumBytes, 0);
      } else {
        int i;
        int NumBytesAtOnce;
        for (i = 0; i < NumBytes; i += NumBytesAtOnce) {
          NumBytesAtOnce = NumBytes - i;
          if (NumBytesAtOnce > (int)sizeof(_ac)) {
            NumBytesAtOnce = sizeof(_ac);
          }
          USBD_BULK_Read(hInst, _ac, NumBytesAtOnce, 0);
          USBD_BULK_Write(hInst, _ac, NumBytesAtOnce, 0);
        }
      }
    }
  }
}
