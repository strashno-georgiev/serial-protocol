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

File    : USBH_BULK_Start.c
Purpose : Shows emUSB-Host Bulk API usage.

Additional information:
  Preparations:
    This sample requires a device running the emUSB-Device BULK
    sample "USB_BULK_Test.c"

  Expected behavior:
    When a device running "USB_BULK_Test.c" is connected
    the application automatically starts sending transmissions
    of increasing size to the device.

  Sample output:
    <...>
    1:109 MainTask - Vendor  Id = 0x8765
    1:109 MainTask - Product Id = 0x1240
    1:109 MainTask - Serial no. = 1
    1:109 MainTask - Speed      = FullSpeed
    1:109 MainTask - Writing one byte
    1:111 MainTask - Reading one byte
    1:112 MainTask - Operation successful!
    1:112 MainTask - Writing one byte
    1:113 MainTask - Reading one byte
    1:114 MainTask - Operation successful!
    1:114 MainTask - Writing 1 bytes
    1:116 MainTask - Reading 1 bytes
    1:117 MainTask - Writing 2 bytes
    1:119 MainTask - Reading 2 bytes
    1:120 MainTask - Writing 3 bytes
    1:122 MainTask - Reading 3 bytes
    <...>
    15:309 MainTask - Writing 1021 bytes
    15:317 MainTask - Reading 1021 bytes
    15:334 MainTask - Writing 1022 bytes
    15:342 MainTask - Reading 1022 bytes
    15:359 MainTask - Writing 1023 bytes
    15:367 MainTask - Reading 1023 bytes
    15:384 MainTask - Writing 1024 bytes
    15:392 MainTask - Reading 1024 bytes
    15:402 MainTask - Test completed successfully.
*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/
#include <stdio.h>
#include "RTOS.h"
#include "BSP.h"
#include "USBH.h"
#include "USBH_BULK.h"

/*********************************************************************
*
*       Local defines, configurable
*
**********************************************************************
*/
#define DEVICE_VID        0x8765
#define DEVICE_PID        0x1240

#define INC_TEST_START    1
#define INC_TEST_END      1024
#define SIZEOF_BUFFER     INC_TEST_END
#define READ_TIMEOUT      10000
#define WRITE_TIMEOUT     10000

enum {
  TASK_PRIO_APP = 150,
  TASK_PRIO_USBH_MAIN,
  TASK_PRIO_USBH_ISR
};

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static OS_STACKPTR int     _StackMain[2048/sizeof(int)];
static OS_TASK             _TCBMain;

static OS_STACKPTR int     _StackIsr[1536/sizeof(int)];
static OS_TASK             _TCBIsr;
static volatile char       _DevIsReady;
static I8                  _DevIndex;
static U8                  _acData[512];

static U8                  _acTxBuffer[SIZEOF_BUFFER];
static U8                  _acRxBuffer[SIZEOF_BUFFER];

static USBH_NOTIFICATION_HOOK _Hook;

/*********************************************************************
*
*       Static Code
*
**********************************************************************
*/

/*********************************************************************
*
*       _GetPortSpeed
*
*  Function description
*    Returns the given USBH speed as a string.
*/
static const char * _GetPortSpeed(USBH_SPEED Speed) {
  switch (Speed) {
  case USBH_LOW_SPEED:
    return "LowSpeed";
  case USBH_FULL_SPEED:
    return "FullSpeed";
  case USBH_HIGH_SPEED:
    return "HighSpeed";
  default:
    break;
  }
  return "Unknown";
}

/*********************************************************************
*
*       _WriteBuffer
*
*  Function description
*    Sends the length of the data to the device and sends data from
*    the give buffer to the device.
*/
static int _WriteBuffer(USBH_BULK_HANDLE hDevice, U8 * pData, int Len, U8 EPOut) {
  USBH_STATUS Status;
  U32         NumBytesWritten;
  U8          ac[2];

  if (Len) {
    USBH_Logf_Application("Writing %d bytes", Len);
    //
    //  Send 16bit Len, MSB first to be compatible with SampleApp
    //
    ac[0] = (Len >> 8);
    ac[1] = (Len & 0xFF);
    Status = USBH_BULK_Write(hDevice, EPOut, ac, 2, &NumBytesWritten, WRITE_TIMEOUT);
    if (Status != USBH_STATUS_SUCCESS || NumBytesWritten != 2) {
      USBH_Logf_Application("Could not write to device 0x%x", Status);
      return 0;
    }
    Status = USBH_BULK_Write(hDevice, EPOut, pData, Len, &NumBytesWritten, WRITE_TIMEOUT);
    if (Status != USBH_STATUS_SUCCESS) {
      USBH_Logf_Application("Could not write to device 0x%x", Status);
      return 0;
    }
    return NumBytesWritten;
  } else {
    return 0;
  }
}

/*********************************************************************
*
*       _ReadBuffer
*
*  Function description
*    Reads a given amount of data from the device as well as
*    the zero-length-packet if the transfer is a multiple of MaxPacketSize.
*/
static int _ReadBuffer(USBH_BULK_HANDLE hDevice, U8 * pDest, int Len, U16 MaxPacketSize, U8 EPIn) {
  USBH_STATUS Status;
  U32         NumBytesRead;
  U32         dummy;

  USBH_Logf_Application("Reading %d bytes", Len);
  Status = USBH_BULK_Read(hDevice, EPIn, pDest, Len, &NumBytesRead, READ_TIMEOUT);
  if (Status != USBH_STATUS_SUCCESS) {
    USBH_Logf_Application("Could not read data 0x%x", Status);
    return 0;
  }
  if (Len % MaxPacketSize == 0) {
    //
    // Read zero-length-packet.
    //
    Status = USBH_BULK_Read(hDevice, EPIn, NULL, 0, &dummy, READ_TIMEOUT);
    if (Status != USBH_STATUS_SUCCESS) {
      USBH_Logf_Application("Could not read ZLP 0x%x", Status);
    }
  }
  return NumBytesRead;
}

/*********************************************************************
*
*       _SendReceive1
*
*  Function description
*    Sends one byte to the device and receives one. The received byte
*    should contain the value of the sent byte incremented by one.
*/
static int _SendReceive1(USBH_BULK_HANDLE hDevice, U8 DataTx, U8 EPIn, U8 EPOut) {
  USBH_STATUS Status;
  U8          DataRx;
  U32         NumBytesRead;
  U32         NumBytesWritten;

  USBH_Logf_Application("Writing one byte");
  Status = USBH_BULK_Write(hDevice, EPOut, &DataTx, 1, &NumBytesWritten, WRITE_TIMEOUT);
  if (Status != USBH_STATUS_SUCCESS || NumBytesWritten != 1) {
    USBH_Logf_Application("Could not write to device 0x%x", Status);
  }
  USBH_Logf_Application("Reading one byte");
  Status = USBH_BULK_Read(hDevice, EPIn, &DataRx, 1, &NumBytesRead, READ_TIMEOUT);
  if (Status != USBH_STATUS_SUCCESS || NumBytesRead != 1) {
    USBH_Logf_Application("Could not read from device 0x%x", Status);
    return 1;
  }
  if (DataRx != (DataTx + 1)) {
    USBH_Logf_Application("Wrong data read");
    return 1;
  }
  USBH_Logf_Application("Operation successful!");
  return 0;
}

/*********************************************************************
*
*       _Test
*
*  Function description
*    Main test function. Sends data to the device, reads it back,
*    checks for correct values.
*/
static int _Test(USBH_BULK_HANDLE hDevice, U16 MaxPacketSize, U8 EPIn, U8 EPOut) {
  int NumBytes;
  int r;
  //
  // Do a simple 1 byte test first
  //
  r = _SendReceive1(hDevice, 0x12, EPIn, EPOut);
  if (r) {
    return r;
  }
  r = _SendReceive1(hDevice, 0x13, EPIn, EPOut);
  if (r) {
    return r;
  }
  //
  // Initially fill buffer
  //
  for (NumBytes = 0; NumBytes < SIZEOF_BUFFER; NumBytes++) {
    _acTxBuffer[NumBytes] = NumBytes % 255;
  }
  //
  // Test different sizes
  //
  for (NumBytes = INC_TEST_START; NumBytes <= INC_TEST_END; NumBytes++) {  // Send and receive various data packets
    r = _WriteBuffer(hDevice, _acTxBuffer, NumBytes, EPOut);
    if (r != NumBytes) {
      USBH_Logf_Application("Incorrect number of bytes sent, expected %d, sent %d", NumBytes, r);
      return 1;
    }
    r = _ReadBuffer(hDevice, _acRxBuffer, NumBytes, MaxPacketSize, EPIn);
    if (r != NumBytes) {
      USBH_Logf_Application("Incorrect number of bytes received, expected %d, received %d", NumBytes, r);
      return 1;
    }
    if (memcmp(_acRxBuffer, _acTxBuffer, NumBytes)) {
      USBH_Logf_Application("Wrong data received");
      return 1;
    }
  }
  return 0;
}

/*********************************************************************
*
*       _OnDevReady
*
*  Function description
*    Called by the application task (MainTask) when a USB device with
*    matching DEVICE_VID and DEVICE_PID is enumerated.
*    Information about the connected device is printed and the test is started.
*/
static void _OnDevReady(void) {
  USBH_BULK_HANDLE      hDevice;
  USBH_BULK_DEVICE_INFO DevInfo;
  U32                   NumBytes;
  USBH_STATUS           Status;
  U16                   MaxPacketSize;
  U8                    EPIn;
  U8                    EPOut;
  unsigned              i;
  U8                    r;
  U8                   *p;

  memset(&DevInfo, 0, sizeof(DevInfo));
  //
  // Open the device, the device index is retrieved from the notification callback.
  //
  hDevice = USBH_BULK_Open(_DevIndex);
  if (hDevice) {
    //
    // Print device info.
    //
    Status = USBH_BULK_GetDeviceInfo(hDevice, &DevInfo);
    if (Status != USBH_STATUS_SUCCESS) {
      USBH_Logf_Application("USBH_BULK_GetDeviceInfo failed, 0x%x", Status);
      goto End;
    }
    USBH_Logf_Application("Vendor  Id = 0x%0.4X", DevInfo.VendorId);
    USBH_Logf_Application("Product Id = 0x%0.4X", DevInfo.ProductId);
    USBH_BULK_GetSerialNumber(hDevice, sizeof(_acData), _acData, &NumBytes);
    p = _acData;
    for (i = 0; i < NumBytes; i++) {
      if (_acData[i] != 0) {
        *p++ = _acData[i];
      }
    }
    *p = 0;
    USBH_Logf_Application("Serial no. = %s", _acData);
    USBH_Logf_Application("Speed      = %s", _GetPortSpeed(DevInfo.Speed));
    //
    // Retrieve the endpoint addresses.
    //
    EPIn   = 0;
    EPOut  = 0;
    for (i = 0; i < DevInfo.NumEPs; i++) {
      if (DevInfo.EndpointInfo[i].Direction == USB_IN_DIRECTION) {
        EPIn = DevInfo.EndpointInfo[i].Addr;
      }
      if (DevInfo.EndpointInfo[i].Direction == USB_OUT_DIRECTION) {
        EPOut = DevInfo.EndpointInfo[i].Addr;
      }
    }
    if (EPIn == 0 || EPOut == 0) {
      USBH_Warnf_Application("Endpoint(s) not found.");
      goto End;
    }
    MaxPacketSize = DevInfo.EndpointInfo[0].MaxPacketSize;
    //
    // Execute test procedures.
    //
    r = _Test(hDevice, MaxPacketSize, EPIn, EPOut);
    if (r == 0) {
      USBH_Logf_Application("Test completed successfully.");
    } else {
      USBH_Logf_Application("An error occurred during the test.");
    }
    //
    //  Close the device
    //
    USBH_BULK_Close(hDevice);
  }
  //
  // Wait until BULK is removed.
  //
End:
  while (_DevIsReady) {
    OS_Delay(100);
  }
}

/*********************************************************************
*
*       _cbOnAddRemoveDevice
*
*  Function description
*    Callback, called when a device is added or removed.
*    Called in the context of the USBH_Task.
*    The functionality in this routine should not block!
*/
static void _cbOnAddRemoveDevice(void * pContext, U8 DevIndex, USBH_DEVICE_EVENT Event) {
  (void)pContext;
  switch (Event) {
  case USBH_DEVICE_EVENT_ADD:
    USBH_Logf_Application("**** Device added [%d]\n", DevIndex);
    _DevIndex = DevIndex;
    _DevIsReady = 1;
    break;
  case USBH_DEVICE_EVENT_REMOVE:
    USBH_Logf_Application("**** Device removed [%d]\n", DevIndex);
    _DevIsReady = 0;
    _DevIndex   = -1;
    break;
  default:;   // Should never happen
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
*/
#ifdef __cplusplus
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif
void MainTask(void);
#ifdef __cplusplus
}
#endif
void MainTask(void) {
  USBH_INTERFACE_MASK InterfaceMask;

  USBH_Init();
  OS_SetPriority(OS_GetTaskID(), TASK_PRIO_APP);                                       // This task has the lowest prio for real-time application.
                                                                                       // Tasks using emUSB-Host API should always have a lower priority than emUSB-Host main and ISR tasks.
  OS_CREATETASK(&_TCBMain, "USBH_Task", USBH_Task, TASK_PRIO_USBH_MAIN, _StackMain);   // Start USBH main task
  OS_CREATETASK(&_TCBIsr, "USBH_isr", USBH_ISRTask, TASK_PRIO_USBH_ISR, _StackIsr);    // Start USBH ISR task
  //
  // Set up the interface mask with IDs from "USB_BULK_Test.c".
  //
  InterfaceMask.Mask      = USBH_INFO_MASK_VID | USBH_INFO_MASK_PID;
  InterfaceMask.VendorId  = DEVICE_VID;
  InterfaceMask.ProductId = DEVICE_PID;
  USBH_BULK_Init(NULL);
  USBH_BULK_AddNotification(&_Hook, _cbOnAddRemoveDevice, NULL, &InterfaceMask);
  while (1) {
    BSP_ToggleLED(1);
    OS_Delay(100);
    if (_DevIsReady) {
      _OnDevReady();
    }
  }
}

/*************************** End of file ****************************/
