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
File    : SYS.h
---------------------------END-OF-HEADER------------------------------
*/

#ifndef SYS_H
#define SYS_H

#include <stdio.h>
#include <string.h>

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

typedef unsigned char U8;
typedef unsigned int  U32;

//
// Serial communication parameters
//
typedef struct {
  unsigned Baudrate;
  unsigned StopBits;
  unsigned Parity;
  unsigned NumBits;
} COM_PARAM;

typedef struct {
  char Desc[50];
  char Device[20];
} CDC_DEVICE;

/*********************************************************************
*
*       Global data
*
**********************************************************************
*/


/*********************************************************************
*
*       Functions
*
**********************************************************************
*/

#ifdef WIN32

#include <windows.h>
#include <io.h>
typedef HANDLE                     SYS_COM;
#define SYS_COM_INVALID            INVALID_HANDLE_VALUE
#define SYS_Sleep(x)               Sleep(x)

#endif
#ifdef linux

#include <poll.h>
typedef int                        SYS_COM;
#define SYS_COM_INVALID            -1
#define SYS_Sleep(x)               poll(NULL, 0, x)

#endif

void     SYS_COMSetTimeout(unsigned Timeout);
int      SYS_COMOpen(SYS_COM *pCOM, const char *pDevice);
void     SYS_COMClose(SYS_COM *pCOM);
int      SYS_COMRead(SYS_COM hCOM, int NumBytes, U8 *pBuff, unsigned Timeout);
int      SYS_COMWrite(SYS_COM hCOM, int NumBytes, const U8 *pData);
int      SYS_COMSetLine(SYS_COM hCOM, int Line, int Value);
int      SYS_COMGetLine(SYS_COM hCOM, int *pCTS, int *pDTR);
int      SYS_COMBreak(SYS_COM hCOM, unsigned Duration);
int      SYS_COMGetParameter(SYS_COM hCOM, COM_PARAM *p);
int      SYS_COMSetParameter(SYS_COM hCOM, COM_PARAM *p);
unsigned SYS_ListComPorts(CDC_DEVICE *pDevices, unsigned MaxDevices);


#endif    /* Guard against multiple inclusion */

/************************** end of file *****************************/
