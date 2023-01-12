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
File    : Echo.c
Purpose : USB CDC echo sample application
---------------------------END-OF-HEADER------------------------------
*/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "SYS.h"

/*********************************************************************
*
*       defines, configurable
*
**********************************************************************
*/
#define APP_TITLE "USB CDC Sample Echo1"

#define MAX_DEVICES  10

#define MAX_TRANSMISSION_SIZE 4096
#define MAX_NUM_TRANSMISSIONS 32768

/*********************************************************************
*
*       static data
*
**********************************************************************
*/

static CDC_DEVICE _Devices[MAX_DEVICES];

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
*       _Echo1
*
*/
static int _Echo1(unsigned DeviceId) {
  unsigned NumBytes2Send;
  unsigned NumTests2Run;
  unsigned i;
  int r;
  char ac[100];
  SYS_COM hDevice;
  U8 *pTXData;
  U8 *pRXData;

  r = 0;
  if (SYS_COMOpen(&hDevice, _Devices[DeviceId].Device)) {
    printf("Unable to connect to USB CDC device\n");
    return 1;
  }
  printf("Enter the transmission test size in bytes (max. %d): ", MAX_TRANSMISSION_SIZE);
  _ConsoleGetLine(ac, sizeof(ac));
  NumBytes2Send = atoi(&ac[0]);
  if (NumBytes2Send > MAX_TRANSMISSION_SIZE) {
    NumBytes2Send = MAX_TRANSMISSION_SIZE;
  }
  printf("Enter the number of transmissions to run (max. %d): ", MAX_NUM_TRANSMISSIONS);
  _ConsoleGetLine(ac, sizeof(ac));
  NumTests2Run = atoi(&ac[0]);
  if (NumTests2Run > MAX_NUM_TRANSMISSIONS) {
    NumTests2Run = MAX_NUM_TRANSMISSIONS;
  }
  pTXData = (U8 *)malloc(NumBytes2Send);
  pRXData = (U8 *)malloc(NumBytes2Send);
  for (i = 0; i < NumBytes2Send; i++) {
    pTXData[i] = (U8)(i % 255);
  }
  for (i = 1; i <= NumTests2Run; i++) {
    if (SYS_COMWrite(hDevice, NumBytes2Send, pTXData)) {
      r = 1;
      break;
    } else {
      printf("Test transmission %d out of %d: %u bytes transmitted successfully\n",i, NumTests2Run, NumBytes2Send);
    }
    memset(pRXData, 0, NumBytes2Send);
    if (SYS_COMRead(hDevice, NumBytes2Send, pRXData, 1000) != NumBytes2Send) {
      r = 1;
      break;
    }
    if (memcmp(pTXData, pRXData, NumBytes2Send) != 0) {
      printf("Error: bad data received\n");
      r = 1;
      break;
    }
  }
  printf ("\n");
  SYS_COMClose(&hDevice);
  free(pTXData);
  free(pRXData);
  return r;
}

/*********************************************************************
*
*       _GetDeviceId
*
*/
static unsigned _GetDeviceId(void) {
  char     Restart;
  char     Msg = 0;
  unsigned i;
  unsigned DeviceId;
  char     ac[20];
  char   * pEnd = NULL;
  char   * pEndExpected = NULL;
  unsigned  NumDevices;
  do {
    Restart = 'N';
    for (;;) {
      NumDevices = SYS_ListComPorts(_Devices, MAX_DEVICES);
      if (NumDevices) {
        break;
      }
      if (Msg == 0) {
        Msg = 1;
        printf("Waiting for USB CDC devices to connect....\n");
      }
      SYS_Sleep(100);
    }
    printf("\nFound %d %s\n", NumDevices, NumDevices == 1 ? "device" : "devices");
    for (i = 0; i < NumDevices; i++) {
      printf("Found the following device %d:\n", i);
      printf("\t%s (%s)\n", _Devices[i].Desc, _Devices[i].Device);
    }
    printf("To which device do you want to connect?\nPlease type in device number (e.g. '0' for the first device, q/a for abort):");
    _ConsoleGetLine(ac, sizeof(ac));
    pEndExpected = &ac[0] +strlen(ac);
    DeviceId = strtol(&ac[0], &pEnd, 0);
    if ((pEnd != pEndExpected)) {
      printf("Invalid device id was entered!!!!\n");
      if ((toupper(ac[0]) == 'Q') || (toupper(ac[0]) == 'A')) {
        DeviceId = -1;
        break;
      } else {
        Restart = 'Y';
        continue;
      }
    }
    if (DeviceId < NumDevices) {
      break;
    }
  } while (Restart == 'Y');
  return DeviceId;
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
  int        r;
  unsigned   DeviceId;
  char       ac[256];

  //
  // Get a valid device id.
  //
  DeviceId = _GetDeviceId();
  if (DeviceId == -1) {
    r = -1;
    printf("Invalid Id, exiting....");
  } else {
    printf("Starting Echo...\n");
    r = _Echo1(DeviceId);
    if (r == 0) {
      printf("Communication with USB CDC device was successful!\n");
    } else {
      printf("Communication with USB CDC device was not successful!\n");
    }
  }
  printf("Press enter to exit.");
  _ConsoleGetLine(ac, sizeof(ac));
  return r;
}

/******************************* End of file ************************/
