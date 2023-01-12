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

File    : USB_CompositeDevice_CDC_MSD.c
Purpose : Sample showing a USB device with multiple interfaces (CDC+MSD).
          This sample combines the functionality of USB_CDC_Echo.c
          and USB_MSD_FS_Start.c samples.

Additional information:
  Preparations:
    For CDC:
    On Windows 8.1 and below the "usbser" driver is not automatically
    assigned to the CDC-ACM device. To install the "usbser" driver
    see \Windows\USB\CDC . The device can be accessed via COM port
    emulation programs e.g. PuTTY.

    On Linux no drivers are needed, the device should show up as
    /dev/ttyACM0 or similar. "sudo screen /dev/ttyACM0 115200"
    can be used to access the device.

    On macOS no drivers are needed, the device should show up as
    /dev/tty.usbmodem13245678 or similar. The "screen" terminal
    program can be used to access the device.

    For MSD:
    The correct emFile configuration file has
    to be included in the project. Depending on the hardware
    it can be one of the following:
    * FS_ConfigRAMDisk_23k.c
    * FS_ConfigNAND_*.c
    * FS_ConfigMMC_CardMode_*.c
    * FS_ConfigNAND_*.c

  Expected behavior:
    For CDC:
    After connecting the USB cable the PC registers a new COM port appears.
    Terminal programs are able to open the COM port.
    Any data sent should be received back from the target.

    For MSD:
    A new MSD volume is recognized by the PC.

  Sample output:
    The target side does not produce terminal output.
*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/
#include "USB.h"
#include "USB_CDC.h"
#include "BSP.h"
#include "USB_MSD.h"
#include "FS.h"
#include "RTOS.h"

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/
//
//  Information that is used during enumeration.
//
static const USB_DEVICE_INFO _DeviceInfo = {
  0x8765,         // VendorId
  0x1256,         // ProductId
  "Vendor",       // VendorName
  "MSD/CDC Composite device",  // ProductName
  "1234567890ABCDEF"           // SerialNumber
};
//
// String information used when inquiring the volume 0.
//
static const USB_MSD_LUN_INFO _Lun0Info = {
  "Vendor",     // MSD VendorName
  "MSD Volume", // MSD ProductName
  "1.00",       // MSD ProductVer
  "134657890"   // MSD SerialNo
};

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
// Data for MSD Task
static OS_STACKPTR int _aMSDStack[512]; /* Task stacks */
static OS_TASK _MSDTCB;               /* Task-control-blocks */

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _AddMSD
*
*  Function description
*    Add mass storage device to USB stack
*/
static void _AddMSD(void) {
  static U8 _abOutBuffer[USB_HS_BULK_MAX_PACKET_SIZE];
  USB_MSD_INIT_DATA     InitData;
  USB_MSD_INST_DATA     InstData;
  USB_ADD_EP_INFO       EPBulkIn;
  USB_ADD_EP_INFO       EPBulkOut;

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
  USBD_MSD_Add(&InitData);
  //
  // Add logical unit 0: RAM drive, using SDRAM
  //
  memset(&InstData, 0,  sizeof(InstData));
  InstData.pAPI                    = &USB_MSD_StorageByName;
  InstData.DriverData.pStart       = (void *)"";
  InstData.pLunInfo = &_Lun0Info;
  USBD_MSD_AddUnit(&InstData);
}
/*********************************************************************
*
*       _MSDTask
*
*  Function description
*    Add mass storage device to USB stack
*/
static void _MSDTask(void) {
  while (1) {
    while ((USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)) != USB_STAT_CONFIGURED) {
      USB_OS_Delay(50);
    }
    USBD_MSD_Task();
  }
}

/*********************************************************************
*
*       _OnLineCoding
*
*  Function description
*    Called whenever a "SetLineCoding" Packet has been received
*
*  Notes
*    (1) Context
*        This function is called directly from an ISR in most cases.
*/
static void _OnLineCoding(USB_CDC_LINE_CODING * pLineCoding) {
#if 0
  USBD_Logf_Application("DTERate=%u, CharFormat=%u, ParityType=%u, DataBits=%u\n",
          pLineCoding->DTERate,
          pLineCoding->CharFormat,
          pLineCoding->ParityType,
          pLineCoding->DataBits);
#else
  BSP_USE_PARA(pLineCoding);
#endif
}

/*********************************************************************
*
*       _AddCDC
*
*  Function description
*    Add communication device class to USB stack
*/
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
  USBD_CDC_SetOnLineCoding(hInst, _OnLineCoding);
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
* USB handling task.
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
  USB_CDC_HANDLE hInstCDC;

  USBD_Init();
  USBD_EnableIAD();
  USBD_SetDeviceInfo(&_DeviceInfo);
  hInstCDC = _AddCDC();
  _AddMSD();
  USBD_Start();
  OS_CREATETASK(&_MSDTCB,  "MSDTask",  _MSDTask, 200, _aMSDStack);

  while (1) {
    char ac[64];
    int  NumBytesReceived;

    //
    // Wait for configuration
    //
    while ((USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)) != USB_STAT_CONFIGURED) {
      BSP_ToggleLED(0);
      USB_OS_Delay(50);
    }
    BSP_SetLED(0);
    NumBytesReceived = USBD_CDC_Receive(hInstCDC, &ac[0], sizeof(ac), 0);
    if (NumBytesReceived > 0) {
      USBD_CDC_Write(hInstCDC, &ac[0], NumBytesReceived, 0);
    }
  }
}

/**************************** end of file ***************************/

