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
File    : SYS_Windows.c
Purpose : Operating system specific functions for Windows
---------------------------END-OF-HEADER------------------------------
*/

#include "SYS.h"
#include <conio.h>
#include <windows.h>
#include <iphlpapi.h>
#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>
#include <time.h>

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/


/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/


/*********************************************************************
*
*       Types
*
**********************************************************************
*/


/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static unsigned _COMTimeout = 1000;

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/


/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       SYS_ListComPorts
*
*  Function description
*    Find all USB CDC devices and list COM Port 
*/
unsigned SYS_ListComPorts(CDC_DEVICE *pDevices, unsigned MaxDevices) {
  unsigned i;
  int   Port;
  char  *p;
  char  acBuff[512];
  DWORD DataT;
  HDEVINFO hDevInfo;
  SP_DEVINFO_DATA DeviceInfoData;
  unsigned NumDevices;

  NumDevices = 0;
  // Create a HDEVINFO with all present devices.
  hDevInfo = SetupDiGetClassDevs(NULL, 0, 0, DIGCF_PRESENT | DIGCF_ALLCLASSES );
  if (hDevInfo == INVALID_HANDLE_VALUE) {
      return 0;
  }
  // Enumerate through all devices in Set.
  DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
  for (i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData); i++) {
    if (SetupDiGetDeviceRegistryProperty(
        hDevInfo,
        &DeviceInfoData,
        SPDRP_HARDWAREID,
        &DataT,
        (PBYTE)acBuff,
        sizeof(acBuff),
        NULL)) {
      if(strstr(acBuff, "USB") != NULL &&
         strstr(acBuff, "PID_") != NULL &&
         strstr(acBuff, "VID_") != NULL) {
        strncpy(pDevices[NumDevices].Desc, acBuff, sizeof(pDevices[NumDevices].Desc));
        pDevices[NumDevices].Desc[sizeof(pDevices[NumDevices].Desc) - 1] = 0;
        if (SetupDiGetDeviceRegistryProperty(
            hDevInfo,
            &DeviceInfoData,
            SPDRP_FRIENDLYNAME,
            &DataT,
            (PBYTE)acBuff,
            sizeof(acBuff),
            NULL)) {
          p = strstr(acBuff, "(COM");
          if (p != NULL) {
            Port = atoi(p + 4);
            if (Port > 0) {
              _snprintf(pDevices[NumDevices].Device, sizeof(pDevices[NumDevices].Device), "\\\\.\\COM%d", Port);                  
              pDevices[NumDevices].Device[sizeof(pDevices[NumDevices].Device) - 1] = 0;
              if (++NumDevices >= MaxDevices) {
                goto Done;
              }
            }
          }
        }
      }
    } 
  }
Done:
  //  Cleanup
  SetupDiDestroyDeviceInfoList(hDevInfo);
  return NumDevices;
}

/*********************************************************************
*
*       SYS_COMSetTimeout
*/
void SYS_COMSetTimeout(unsigned Timeout) {
  _COMTimeout = Timeout;
}

/*********************************************************************
*
*       SYS_COMWrite
*
*  Function description
*    Write bytes to COM
*/
int SYS_COMWrite(SYS_COM hCOM, int NumBytes, const U8 *pData) {
  int   r;
  DWORD NumBytesWritten;
  DWORD CallTime;
  DWORD Err;

  CallTime = GetTickCount();
  r = WriteFile(hCOM, pData, NumBytes, &NumBytesWritten, NULL);
  CallTime = GetTickCount() - CallTime;
  if ((int)CallTime > _COMTimeout + _COMTimeout / 5) {
    printf("Warning: WriteFile took %u ms\n", CallTime);
  }
  if (r == 0) {
    Err = GetLastError();
    printf("Write Error %d\n", Err);
    return -1;
  }
  if (NumBytesWritten != NumBytes) {
    printf("Error data size on write: %d of %d.\n", NumBytesWritten, NumBytes);
    return -1;
  }
  return 0;
}


/*********************************************************************
*
*       SYS_COMRead
*
*  Function description
*    Read bytes from COM
*/
int SYS_COMRead(SYS_COM hCOM, int NumBytes, U8 *pBuff, unsigned Timeout) {
  int   Cnt;
  int   r;
  DWORD NumBytesRead;
  DWORD Err;
  DWORD StartTime;

  Cnt = 0;
  StartTime = GetTickCount();
  do
  {
    r = ReadFile(hCOM, pBuff+Cnt, 1, &NumBytesRead, NULL);
    if (r == 0) {
      Err = GetLastError();
      printf("Read Error, %d\n", Err);
      return -1;
    }
    if (NumBytesRead == 0) {
      if (GetTickCount() - StartTime > (DWORD)Timeout) {
        printf("Read timeout\n");
        return Cnt;
      }
      Sleep(1);
      continue;
    }
    Cnt++;
  } while(Cnt < NumBytes);
  return Cnt;
}

/*********************************************************************
*
*       SYS_COMOpen
*
*  Function description
*    Opens and initializes a COM port
*/
int SYS_COMOpen(SYS_COM *pCOM, const char *pDevice) {
  HANDLE hCOM;
  DWORD Err;
  DCB dcb;
  COMMTIMEOUTS ct;
  int r;
  int Ret = -1;

  hCOM = CreateFile(
                    pDevice,
                    GENERIC_READ | GENERIC_WRITE,
                    0,                                    // share mode
                    NULL,                                 // security attr.
                    OPEN_EXISTING,
                    0,                                    // Flags & attributes
                    0                                     // hTemplateFile
                    );  
  if (hCOM == INVALID_HANDLE_VALUE) {
    Err = GetLastError();
    printf("Can not open %s, err = %u\n", pDevice, Err);
    goto Err;
  }
  //
  // Setup COM port
  //
  r = SetupComm(hCOM, 0, 0);
  if (r == 0) {
    printf("failed to setup buffer size.\n");
    goto Err;
  }
  GetCommState(hCOM, &dcb);
  dcb.BaudRate     = 9600;
  dcb.fBinary      = 1;           // Binary transfers
  dcb.fParity      = 0;           // Disable parity check
  dcb.fOutxCtsFlow = 0;           // Ignore CTS
  dcb.fOutxDsrFlow = 0;           // Ignore DSR
  dcb.ByteSize     = 8;           // Our default is 8 data bits
  dcb.Parity       = NOPARITY;    // No parity
  dcb.StopBits     = 0;           // 1 stop bit
  dcb.fOutX        = 0;           // Ignore incoming XOFFs
  dcb.fInX         = 0;           // Do not send XOFFs
  dcb.fAbortOnError = 0;
  dcb.fDtrControl = DTR_CONTROL_ENABLE;
  dcb.fRtsControl = RTS_CONTROL_ENABLE;
  r = SetCommState(hCOM, &dcb);
  if (r == 0) {
    printf("failed to setup baudrate.\n");
    goto Err;
  }
  //
  // When ReadIntervalTimeout is set to MAXDWORD
  // and both ReadTotalTimeoutMultiplier and ReadTotalTimeoutConstant
  // set to zero. The ReadFile will always return regardless.
  // if there any data available or not.
  // Setting WriteTotalTimeoutMultiplier and WriteTotalTimeoutConstant
  // to zero means no timeout is used for write operation
  //
  ct.ReadIntervalTimeout         = MAXDWORD;   
  ct.ReadTotalTimeoutMultiplier  = 0;
  ct.ReadTotalTimeoutConstant    = 0;
  ct.WriteTotalTimeoutMultiplier = 0;
  ct.WriteTotalTimeoutConstant   = _COMTimeout;
  r = SetCommTimeouts(hCOM, &ct); 
  if (r == 0) {
    printf("failed to setup timeouts for read & write.\n");
    goto Err;
  }
  Ret = 0;
Err:
  *pCOM = hCOM;
  return Ret;
}

/*********************************************************************
*
*       SYS_COMClose
*
*  Function description
*    Close COM port
*/
void SYS_COMClose(SYS_COM *pCOM) {
  CloseHandle(*pCOM);
  *pCOM = INVALID_HANDLE_VALUE;
}

/*********************************************************************
*
*       SYS_COMSetLine
*
*  Function description
*    Set/Clear modem line
*/
int SYS_COMSetLine(SYS_COM hCOM, int Line, int Value) {
  if (Line == 0) {
    //
    // Handle RTS
    //
    if (Value) {
      if (EscapeCommFunction(hCOM, SETRTS) == 0) {
        return -1;
        }
      } else {
      if (EscapeCommFunction(hCOM, CLRRTS) == 0) {
        return -1;
      }
    }
  } else {
    if (Value) {
      if (EscapeCommFunction(hCOM, SETDTR) == 0) {
        return -1;
      }
    } else {
      if (EscapeCommFunction(hCOM, CLRDTR) == 0) {
        return -1;
      }
    }
  }
  return 0;
}

/*********************************************************************
*
*       SYS_COMGetLine
*
*  Function description
*    Get modem line values
*/
int SYS_COMGetLine(SYS_COM hCOM, int *pCTS, int *pDTR) {
  DWORD ModemCtrl;

  if (GetCommModemStatus(hCOM, &ModemCtrl) == 0) {
    return -1;
  }
  if (ModemCtrl & MS_CTS_ON) {
    *pCTS = 1;
  } else {
    *pCTS = 0;
  }
  if (ModemCtrl & MS_DSR_ON) {
    *pDTR = 1;
  } else {
    *pDTR = 0;
  }
  return 0;
}

/*********************************************************************
*
*       SYS_COMBreak
*
*  Function description
*    Get modem line values
*/
int SYS_COMBreak(SYS_COM hCOM, unsigned Duration) {
  if (SetCommBreak(hCOM) == 0) {
    return -1;
  }
  Sleep(Duration);
  if (ClearCommBreak(hCOM) == 0) {
    return -1;
  }
  return 0;
}

/*********************************************************************
*
*       SYS_COMGetParameter
*
*  Function description
*    Get communication parameters
*/
int SYS_COMGetParameter(SYS_COM hCOM, COM_PARAM *p) {
  DCB dcb;

  if (GetCommState(hCOM, &dcb) == 0) {
    return -1;
  }
  p->Baudrate = dcb.BaudRate;
  p->NumBits  = dcb.ByteSize;
  p->Parity   = dcb.Parity;
  p->StopBits = dcb.StopBits;
  return 0;
}

/*********************************************************************
*
*       SYS_COMSetParameter
*
*  Function description
*    Set communication parameters
*/
int SYS_COMSetParameter(SYS_COM hCOM, COM_PARAM *p) {
  DCB dcb;

  if (GetCommState(hCOM, &dcb) == 0) {
    return -1;
  }
  dcb.BaudRate = p->Baudrate;
  dcb.ByteSize = p->NumBits;
  dcb.Parity   = p->Parity;
  dcb.StopBits = p->StopBits;
  if (SetCommState(hCOM, &dcb) == 0) {
    return -1;
  }
  return 0;
}

/*************************** end of file ****************************/
