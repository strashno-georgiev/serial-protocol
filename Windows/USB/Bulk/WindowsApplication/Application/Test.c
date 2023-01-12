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
Purpose : USB BULK Test Application
          The application will first check whether a DeviceId.txt file
          is available in the current work directory, if so, this file
          will be parsed to check the allowed vendor/device id pairs.
          All valid entries will be added to the internal allowed
          device list.
          The format of the file will be as followed:
          # -> Comment ignore this line.
          xxxx:xxxx -> vendor:device id pair given in hex values
          If the files does not exist the USBBULK_AddAllowedDeviceItem
          entries in the main function are used though.
---------------------------END-OF-HEADER------------------------------
*/

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include "USBBULK.h"

/*********************************************************************
*
*       defines, configurable
*
**********************************************************************
*/
#define PRODUCT_NAME  "Bulk test"

#define SIZEOF_BUFFER  0x4000
#define DEVICE_ID_FILE_NAME   "DeviceIds.txt"

/*********************************************************************
*
*       static data
*
**********************************************************************
*/
static unsigned char _acTxBuffer[SIZEOF_BUFFER];
static unsigned char _acRxBuffer[SIZEOF_BUFFER];

static const char _CmdReadSpeed[]  = "@Test-Read-Speed@";
static const char _CmdWriteSpeed[] = "@Test-Write-Speed@";

/*********************************************************************
*
*       static code
*
**********************************************************************
*/

#ifdef _WIN32

#include <windows.h>

#else

#include <poll.h>
#include <time.h>
#include <sys/time.h>

/*********************************************************************
*
*       Sleep
*
*/
static void Sleep(unsigned ms) {
  poll(NULL,0,ms);
}


/*********************************************************************
*
*       GetTickCount
*/
static unsigned GetTickCount(void) {
#ifdef linux
  struct timespec Time;

  clock_gettime(CLOCK_MONOTONIC, &Time);
  return ((unsigned)Time.tv_sec * 1000) + ((unsigned)Time.tv_nsec / 1000000);
#else
  struct timeval Time;
  gettimeofday(&Time,NULL);
  return ((unsigned)Time.tv_sec * 1000) + ((unsigned)Time.tv_usec / 1000);
#endif
}

#endif

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
*       _GetLine
*/
static const char * _GetLine(const char * pInput, const char ** ppNext) {
  const char * pStart;
  const char * pEnd;
  ptrdiff_t    Len;

  pStart = pInput;
  pEnd = strchr(pStart, '\n');
  if (pEnd == NULL) {
    pEnd = strchr(pStart, '\0');
  }
  Len = pEnd - pStart;
  if (Len <= 0) {
    return NULL;
  }
  *ppNext = ++pEnd;
  return pStart;

}


/*********************************************************************
*
*       _EatWhite
*/
static void _EatWhite(const char ** ps) {
  const char * s;

  s = *ps;
  while ((*s != '\0') && ((*s == ' ') || (*s == '\t') || (*s == '\r') || (*s == '\n'))) {
    s++;
  }
  *ps = s;
}

/*********************************************************************
*
*       _Hex2Dec
*/
static int _Hex2Dec(int c) {
  if ((c >= (int)'0') && (c <= (int)'9')) {
    return c - (int)'0';
  }
  if ((c >= (int)'a') && (c <= (int)'f')) {
    return c - (int)'a' + 10;
  }
  if ((c >= (int)'A') && (c <= (int)'F')) {
    return c - (int)'A' + 10;
  }
  return -1;
}

/*********************************************************************
*
*       _ParseHex
*/
static int _ParseHex(const char ** ps, U32 * pData) {
  U32 Data;
  int NumDigits;
  int v;

  Data = 0;
  NumDigits = 0;
  _EatWhite(ps);
  if (**ps == '0') {
    if (*(*ps + 1) == 'x') {
      (*ps) += 2;
    }
  }
  for (;;) {
    v = (int)**ps;
    v = _Hex2Dec(v);
    if (v >= 0) {
      Data = (Data << 4) | (U32)v;
      (*ps)++;
      NumDigits++;
    } else {
      if (NumDigits == 0) {
        return -1;
      }
      *pData = Data;
      return 0;
    }
  }
}


/*********************************************************************
*
*       _EchoTest
*/
static int _EchoTest(USB_BULK_HANDLE hDevice, unsigned Count) {
  unsigned Len;
  int      r;
  unsigned i;

  printf("Echo test\n");
  Len = 0;
  while (Count > 0) {
    if (++Len >= 64) {
      Len = 1;
    }
    for (i = 0; i < Len; i++) {
      _acTxBuffer[i] = (unsigned char)(Count + i);
    }
    r = USBBULK_Write(hDevice, _acTxBuffer, Len);
    if (r == 0) {
      printf("Could not write to device\n");
      return 1;
    }
    r = USBBULK_Read(hDevice, _acRxBuffer, Len);
    if (r == 0) {
      printf("Could not read from device (time out)\n");
      return 1;
    }
    _acTxBuffer[0]++;
    if (memcmp(_acTxBuffer, _acRxBuffer, Len) != 0) {
      printf("Wrong data read\n");
      return 1;
    }
    --Count;
  }
  printf("Operation successful!\n\n");
  return 0;
}

/*********************************************************************
*
*       _Test
*
*/
static int _Test(USB_BULK_HANDLE hDevice) {
  unsigned DataSize;
  unsigned NumBytes;
  unsigned Len;
  int      r;
  unsigned t;
  unsigned Cnt;
  USBBULK_DEV_INFO DevInfo;

  //
  // Do some simple echo tests
  //
  if (_EchoTest(hDevice, 100)) {
    return 1;
  }
  USBBULK_GetDevInfo(hDevice, &DevInfo);
  //
  //
  printf("Read speed test\n");
  strncpy((char *)_acTxBuffer, _CmdReadSpeed, sizeof(_acTxBuffer));
  if (DevInfo.Speed >= USBBULK_SPEED_HIGH) {
    //
    // Set data size for test = 256 MB
    //
    DataSize = 256 * 1024 * 1024;
    _acTxBuffer[sizeof(_CmdReadSpeed) - 1] = '6';
  } else {
    //
    // Set data size for test = 16 MB
    //
    DataSize = 16 * 1024 * 1024;
    _acTxBuffer[sizeof(_CmdReadSpeed) - 1] = '2';
  }
  if (USBBULK_Write(hDevice, _acTxBuffer, sizeof(_CmdReadSpeed)) == 0) {
    printf("Could not write to device\n");
    return 1;
  }
  r = USBBULK_Read(hDevice, _acRxBuffer, 2);
  if (r == 0) {
    printf("Could not read from device (time out)\n");
    return 1;
  }
  if (memcmp(_acRxBuffer, "ok", 2) != 0) {
    printf("Wrong response from device\n");
    return 1;
  }
  memset(_acTxBuffer, 0xAA, sizeof(_acTxBuffer));
  NumBytes = DataSize;
  Cnt = 0;
  t = GetTickCount();
  do {
    Len = NumBytes;
    if (Len > sizeof(_acTxBuffer)) {
      Len = sizeof(_acTxBuffer);
    }
    if (USBBULK_Write(hDevice, _acTxBuffer, Len) == 0) {
      printf("Could not write to device\n");
      return 1;
    }
    NumBytes -= Len;
    if (++Cnt % 64 == 0) {
      putchar('.');
      fflush(stdout);
    }
  } while (NumBytes > 0);
  t = GetTickCount() - t;
  printf("\nPerformance: %u ms for %u MB\n", t, DataSize / (1024 * 1024));
  printf("          =  %llu kB / second\n\n", 1000LL * DataSize / t / 1024);
  //
  //
  printf("Write speed test\n");
  strncpy((char *)_acTxBuffer, _CmdWriteSpeed, sizeof(_acTxBuffer));
  if (DevInfo.Speed >= USBBULK_SPEED_HIGH) {
    //
    // Set data size for test = 256 MB
    //
    DataSize = 256 * 1024 * 1024;
    _acTxBuffer[sizeof(_CmdWriteSpeed) - 1] = '6';
  } else {
    //
    // Set data size for test = 16 MB
    //
    DataSize = 16 * 1024 * 1024;
    _acTxBuffer[sizeof(_CmdWriteSpeed) - 1] = '2';
  }
  if (USBBULK_Write(hDevice, _acTxBuffer, sizeof(_CmdWriteSpeed)) == 0) {
    printf("Could not write to device\n");
    return 1;
  }
  r = USBBULK_Read(hDevice, _acRxBuffer, 2);
  if (r == 0) {
    printf("Could not read from device (time out)\n");
    return 1;
  }
  if (memcmp(_acRxBuffer, "ok", 2) != 0) {
    printf("Wrong response from device\n");
    return 1;
  }
  NumBytes = DataSize;
  Cnt = 0;
  t = GetTickCount();
  do {
    Len = NumBytes;
    if (Len > sizeof(_acRxBuffer)) {
      Len = sizeof(_acRxBuffer);
    }
    if (USBBULK_Read(hDevice, _acRxBuffer, Len) == 0) {
      printf("Could not read from device (time out)\n");
      return 1;
    }
    NumBytes -= Len;
    if (++Cnt % 64 == 0) {
      putchar('.');
      fflush(stdout);
    }
  } while (NumBytes > 0);
  t = GetTickCount() - t;
  printf("\nPerformance: %u ms for %u MB\n", t, DataSize / (1024 * 1024));
  printf("          =  %llu kB / second\n\n", 1000LL * DataSize / t / 1024);
  //
  //
  if (_EchoTest(hDevice, 10)) {
    return 1;
  }
  return 0;
}


/*********************************************************************
*
*       _GetDeviceId
*
*/
static unsigned _GetDeviceId(void) {
  U32      DeviceMask;
  char     Restart;
  char     Msg = 0;
  unsigned i;
  unsigned NumDevices = 0;
  unsigned DeviceId;
  USB_BULK_HANDLE hDevice;
  char            acName[256];
  char            ac[20];
  char          * pEnd = NULL;
  char          * pEndExpected = NULL;
  do {
    Restart = 'N';
    for (;;) {
      NumDevices = USBBULK_GetNumAvailableDevices(&DeviceMask);
      if (NumDevices) {
        break;
      }
      if (Msg == 0) {
        Msg = 1;
        printf("Waiting for USB BULK devices to connect....\n");
      }
      Sleep(100);
    }
    printf("\nFound %d %s\n", NumDevices, NumDevices == 1 ? "device" : "devices");
    for (i = 0; i < NumDevices; i++) {
      if (DeviceMask & (1 << i)) {
        hDevice = USBBULK_Open(i);
        if (hDevice == 0) {
          printf("Can't open device %d:\n", i);
          continue;
        }
        printf("Found the following device %d:\n", i);
        acName[0] = 0;
        USBBULK_GetVendorName(hDevice, acName, sizeof(acName));
        printf("  Vendor Name : %s\n", acName);
        acName[0] = 0;
        USBBULK_GetProductName(hDevice, acName, sizeof(acName));
        printf("  Product Name: %s\n", acName);
        if (strcmp(acName, PRODUCT_NAME) != 0) {
          printf("\n  WARNING: Expected Product Name is: \"%s\". Did you use the correct sample application on the target side? \n\n", PRODUCT_NAME);
        }
        acName[0] = 0;
        USBBULK_GetSN(hDevice, (unsigned char *)acName, sizeof(acName));
        printf("  Serial no.  : %s\n", acName);
        USBBULK_Close(hDevice);
      }
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
    if (DeviceId < USBBULK_MAX_DEVICES) {
      break;
    }
  } while (Restart == 'Y');
  printf("\n");
  return DeviceId;
}


/*********************************************************************
*
*       _ShowDriverInfo
*
*/
static void _ShowDriverInfo(void) {
  char ac[200];
  unsigned Ver;
  char Build[2];

  Ver = USBBULK_GetDriverVersion();
  if (Ver) {
    Build[0] = Build[1] = 0;
    if (Ver % 100) {
      Build[0] = (int)(Ver % 100) + 'a' - 1;
    }
    USBBULK_GetDriverCompileDate(ac, sizeof(ac));
    printf("USB BULK driver version: %d.%.2d%s, compiled: %s\n", (int)(Ver / 10000), (int)(Ver / 100) % 100, Build, ac);
  }
}

/*********************************************************************
*
*       main
*
* Function description
*/
int main(int argc, char* argv[]) {
  int               r;
  unsigned          DeviceId;
  USB_BULK_HANDLE   hDevice;
  char              ac[256];
  FILE            * pFile;


  //
  //  Init the USBBULK module
  //
  USBBULK_Init(NULL, NULL);
  //
  // Check whether we can open the device id list.
  //
  pFile = fopen(DEVICE_ID_FILE_NAME, "rt");
  if (pFile == NULL) {
    //
    //  Add all allowed devices via (VendorId, ProductId)
    //
    USBBULK_AddAllowedDeviceItem(0x8765, 0x1234);
    USBBULK_AddAllowedDeviceItem(0x8765, 0x1240);
    USBBULK_AddAllowedDeviceItem(0x8765, 0x2000);
  } else {
    char * pBuffer;
    const char * pNext;
    const char * pBegin;
    unsigned VendorId;
    unsigned ProductId;

    //
    // Get file size
    //
    fseek(pFile, 0, SEEK_END);
    unsigned FileSize = ftell(pFile);
    //
    //  Go back to the beginning
    //
    fseek(pFile, 0, SEEK_SET);
    //
    // Read back the whole file into an allocated buffer
    //
    pBuffer = (char *)malloc(FileSize);
    assert(pBuffer != NULL);
    fread(pBuffer, sizeof(char), FileSize, pFile);
    fclose(pFile);
    pBegin = (const char *)pBuffer;
    do {
      //
      // Get one line out of our buffer
      //
      pBegin =  _GetLine(pBegin, &pNext);
      if (pBegin == NULL) {
        break;
      }
      _EatWhite(&pBegin);
      //
      // Comment found ignore line
      //
      if (*pBegin == '#') {
        pBegin = pNext;
        continue;
      }
      if (_ParseHex(&pBegin, &VendorId) != 0) {
        VendorId = 0;
      }
      pBegin++;
      if (_ParseHex(&pBegin, &ProductId) != 0) {
        ProductId = 0;
      }
      if (VendorId != 0 && ProductId != 0) {
        USBBULK_AddAllowedDeviceItem(VendorId, ProductId);
      }
      pBegin = pNext;
    } while (pBegin != NULL);
    free(pBuffer);
  }
  //
  // Retrieve some version information from the USBBULK module
  //
  _ShowDriverInfo();
  //
  // Choose USB device
  //
  DeviceId = _GetDeviceId();
  hDevice = USBBULK_Open(DeviceId);
  if (hDevice == 0) {
    printf("Unable to connect to USB BULK device\n");
    return 1;
  }
  USBBULK_SetReadTimeout(hDevice, 2 * 1000);
  USBBULK_SetWriteTimeout(hDevice, 2 * 1000);
  //
  // Perform tests
  //
  r = _Test(hDevice);
  USBBULK_Close(hDevice);
  if (r == 0) {
    printf("Communication with USB BULK device was successful!\n");
  } else {
    printf("Communication with USB BULK device was not successful!\n");
  }
  USBBULK_Exit();
  printf("Press enter to exit.");
  _ConsoleGetLine(ac, sizeof(ac));
  return r;
}

/******************************* End of file ************************/
