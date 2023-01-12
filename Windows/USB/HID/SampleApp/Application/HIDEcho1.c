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
File    : HIDEcho1.c
Purpose : USB HID echo sample.
---------------------------END-OF-HEADER------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "USBHID.h"
#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#endif

/*********************************************************************
*
*       defines, configurable
*
**********************************************************************
*/
#define USB_HID_DEFAULT_VENDOR_PAGE  0x12   // Must be identical to target hardware, Allowed values 8bit value (0x00-0xff).

/*********************************************************************
*
*       static data
*
**********************************************************************
*/

/*********************************************************************
*
*       static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _ConsoleGetLine
*
*/
static char * _ConsoleGetLine(char * pBuffer, U32 BufferSize) {
  fgets(pBuffer, BufferSize, stdin);
  pBuffer[strlen(pBuffer) - 1] = 0;
  return pBuffer;
}

/*********************************************************************
*
*       _SendReceive1
*
*/
static int _SendReceive(unsigned DeviceId, U8 * pDataTx, unsigned NumBytes2Send, U8 * pDataRx, unsigned NumBytes2Receive) {
  int r;

  r = USBHID_Write(DeviceId, pDataTx, NumBytes2Send);
  if (r < 1) {
    printf("Could not write to device\n");
    return 1;                      // Error
  }
  r = USBHID_Read(DeviceId, pDataRx, NumBytes2Receive);
  if (r < 1) {
    printf("Could not read from device\n");
    return 1;                      // Error
  }
  if (*pDataRx != (*pDataTx + 1)) {
    printf("Wrong data read\n");
    return 1;                      // Error
  }
  printf(".");
  fflush(stdout);
  return 0;
}

/*********************************************************************
*
*       _Echo1
*
*/
static int _Echo1(unsigned DeviceId) {
  int      i;
  int      r;
  int      NumEchoes;
  char     ac[100];
  unsigned InputReportLength;
  unsigned OutputReportLength;
  U8     * pDataRx;
  U8     * pDataTx;

  r = 0;
  if (USBHID_Open(DeviceId) != 0) {
    printf("Unable to connect to USB HID device\n");
    return 1;
  }
  //
  //  Retrieve report sizes
  //
  InputReportLength  = USBHID_GetInputReportSize(DeviceId);
  OutputReportLength = USBHID_GetOutputReportSize(DeviceId);
  //
  // Alloc memeory for in/out report
  pDataTx = (U8 *)calloc(OutputReportLength, sizeof(U8));
  pDataRx = (U8 *)calloc(InputReportLength,  sizeof(U8));
  if ((pDataRx == NULL) || (pDataTx == NULL)) {
    printf("No memory available to create buffer for sending/receiving data\n");
    r = 1;
    goto End;
  }
  printf("Enter the number of echoes to be sent to the echo client: ");
  _ConsoleGetLine(ac, sizeof(ac));
  NumEchoes = atoi(&ac[0]);
  for (i = 0; i < NumEchoes; i++) {
    *pDataTx = i % 255;
    if (_SendReceive(DeviceId, pDataTx, OutputReportLength, pDataRx, InputReportLength)) {
      r = 1;
      break;
    }
  }
  if (r == 0) {
    printf ("\n%d echoes successfully transferred.\n", NumEchoes);
  }
End:
  free(pDataTx);
  free(pDataRx);
  USBHID_Close(DeviceId);
  return r;
}

/*********************************************************************
*
*       _ShowInfo
*
*/
static void _ShowInfo(unsigned DeviceId) {
  char     ac[255];
  char     sn[33];
  U16      VendorId;
  U16      ProductId;
  unsigned InputReportLength;
  unsigned OutputReportLength;

  USBHID_GetProductName(DeviceId, ac, sizeof(ac));
  VendorId  = USBHID_GetVendorId(DeviceId);
  ProductId = USBHID_GetProductId(DeviceId);
  memset(sn, 0, sizeof(sn));
  USBHID_GetSerialNumber(DeviceId, sn, sizeof(sn) - 1);
  InputReportLength = USBHID_GetInputReportSize(DeviceId);
  OutputReportLength = USBHID_GetOutputReportSize(DeviceId);
  printf("Device %d:\n"
         "  Productname: %s\n"
         "  VID        : 0x%.4x\n"
         "  PID        : 0x%.4x\n"
         "  SerialNo   : %s\n"
         "  ReportSizes:\n"
         "   Input     : %d bytes\n"
         "   Output    : %d bytes\n", DeviceId, ac, VendorId, ProductId, sn, InputReportLength, OutputReportLength);
}


/*********************************************************************
*
*       public code
*
**********************************************************************
*/

/*********************************************************************
*
*       main
*
*/
int main(int argc, char* argv[]) {
  int      r;
  U32      DeviceMask;
  unsigned NumDevices;
  unsigned DeviceId;
  char     ac[10];

  DeviceId   = 0;
  //
  //  Initialize first
  //
  USBHID_Init(USB_HID_DEFAULT_VENDOR_PAGE);
  //
  //  Check first, how many devices are available.
  //
  r       = 1;  // Set error so far
  NumDevices = USBHID_GetNumAvailableDevices(&DeviceMask);
  if (NumDevices == 1) {
    _ShowInfo(0);
  } else if (NumDevices) {
    printf("AvailableDevices = %d\n", NumDevices);
    for (r = 0; r < 32; r++) {
      if ((1 << r) & DeviceMask) {
        _ShowInfo(r);
      }
    }
    printf("To which device do you want to connect? ");
    _ConsoleGetLine(ac, sizeof(ac));
    DeviceId = atoi(ac);
  } else {
    printf("No devices available\n");
    DeviceId = USB_MAX_DEVICES;
  }
  if (DeviceId < USB_MAX_DEVICES) {
    if ((1 << DeviceId) & DeviceMask) {
      printf("Starting Echo...\n");
      r = _Echo1(DeviceId);
    }
  }
  if (r == 0) {
    printf("Communication with USB HID device was successful!\n");
  } else {
    printf("Communication with USB HID device was not successful!\n");
  }
  USBHID_Exit();
  printf("Press enter to exit.");
  _ConsoleGetLine(ac, sizeof(ac));
  return r;
}

/******************************* End of file ************************/
