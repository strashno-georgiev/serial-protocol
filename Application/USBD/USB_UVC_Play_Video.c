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
File    : USB_UVC_Play_Video.c
Purpose : The sample plays a video from a storage medium (normally a SD card).
          The video must be in YUV422 format and can be viewed by
          any web cam viewer application which can display the YUV422 format.

Additional information:
  Preparations:
    To run this sample the info buffer size should be increased,
    this can be done by adding #define USB_DESC_BUFFER_SIZE 512
    to the project's USB_Conf.h file.
    #define USB_SUPPORT_TRANSFER_ISO 1 also must be added.

    A video must be converted into the YUV422 format.
    This can be done using ffmpeg:
    ffmpeg -i InputVideo.mp4 -pix_fmt yuyv422 -c:v rawvideo -an -s 320x240 -r 30 video.yuv
    You can add "-t 5" before "-i" to reduce the duration to 5 seconds.
    This sample is meant to be used with USB high-speed controllers.
    An example file named video.yuv is provided with this shipment (.../Application/USBD/video.yuv)

    On full-speed USB controllers high-resolution videos will be played significantly slower.
    When running this sample with USB full-speed it is possible
    that you might have to decrease the ISO_EP_SIZE to make sure
    the sample works at all. The total bandwidth on the
    USB full-speed bus for periodic endpoints (interrupt and
    isochronous is 1500). When you connect the sample it will
    require 1023 bytes (+9 bytes protocol overhead) from that
    bandwidth in it's default configuration. If your other
    devices already have a large portion of that bandwidth
    reserved the sample won't work.

  Expected behavior:
    Video is seen in the webcam viewing program.

  Sample output:
    The target side does not produce terminal output.
    On the PC the video should be seen in the webcam viewing program.
*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/
#include <string.h>
#include <stdio.h>
#include "USB.h"
#include "FS.h"
#include "USB_UVC.h"
#include "RTOS.h"
#include "BSP.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
#define VIDEO_FPS       30
#define VIDEO_WIDTH     320
#define VIDEO_HEIGHT    240
#define VIDEO_FILENAME  "video.yuv"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/
#define FRAME_SIZE      (VIDEO_WIDTH * VIDEO_HEIGHT * 2)
#define PACKET_SIZE     ((FRAME_SIZE * VIDEO_FPS) / 8000)  // 8000 microframes in a second (USB high-speed)
#define MAX_PACKET_SIZE (PACKET_SIZE + USBD_UVC_PAYLOAD_HEADER_SIZE)

#if MAX_PACKET_SIZE > (1024 * 3)
  #error "Video resolution or FPS is too large."
#endif

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
  0x1088,         // ProductId
  "SEGGER",       // VendorName
  "SEGGER video", // ProductName
  "13245678"      // SerialNumber
};

/*********************************************************************
*
*       Const data
*
**********************************************************************
*/
/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static U32                         _acUVCBuffer[USBD_UVC_NUM_BUFFERS][USBD_UVC_DATA_BUFFER_SIZE / 4];
static U32                         _acBuffer[2][PACKET_SIZE * 64 / 4];
static const USBD_UVC_RESOLUTION  _aRes[] = {{VIDEO_WIDTH, VIDEO_HEIGHT}};
static unsigned                   _FrameIndex;
static USBD_UVC_BUFFER            _Buf;

static OS_STACKPTR int            _aFSStack[600]; /* Task stacks */
static OS_TASK                    _FSTCB;         /* Task-control-blocks */
static OS_MAILBOX                 _MB;


typedef struct _MB_DATA {
  unsigned  NumBytes;
  U8        EndOfFrame;
  U8        BufferIndex;
} MB_DATA;
static MB_DATA _MB_Data[2];

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _ResChange
*
*  Function description
*    Called when the PC requests a change in the current image resolution.
*/
static void _ResChange(unsigned FrameIndex) {
  _FrameIndex = FrameIndex - 1;
}

/*********************************************************************
*
*       _AddUVC
*
*  Function description
*    Add UVC mouse class to USB stack
*/
static int _AddUVC(void) {
  USBD_UVC_INIT_DATA InitData;
  USB_ADD_EP_INFO EP_Info;
  U8 i;

  memset(&InitData, 0, sizeof(InitData));
  memset(&EP_Info, 0, sizeof(EP_Info));
  EP_Info.InDir = USB_DIR_IN;
  EP_Info.TransferType = USB_TRANSFER_TYPE_ISO;
  EP_Info.Interval = 1;
  EP_Info.MaxPacketSize = MAX_PACKET_SIZE;
  EP_Info.Flags = USB_ADD_EP_FLAG_USE_ISO_SYNC_TYPES;
#if USB_SUPPORT_TRANSFER_ISO
  EP_Info.ISO_Type = USB_ISO_SYNC_TYPE_ASYNCHRONOUS;
#endif
  InitData.EPIn    = USBD_AddEPEx(&EP_Info, NULL, 0);
  for (i = 0; i < USBD_UVC_NUM_BUFFERS; i++) {
    _Buf.Buf[i].pData = (U8 *)_acUVCBuffer[i];
  }
  InitData.pBuf    = &_Buf;
  InitData.aResolutions = _aRes;
  InitData.NumResolutions = SEGGER_COUNTOF(_aRes);
  USBD_SetDeviceInfo(&_DeviceInfo);
  return USBD_UVC_Add(&InitData);
}

/*********************************************************************
*
*       _FSTask
*
*  Function description
*/
static void _FSTask(void* pContext) {
  FS_FILE * pFile;
  U32       NumBytesFrame;
  U32       NumBytes;
  int       r;
  MB_DATA   MBData;
  U8        BufferIndex;

  (void)pContext;
  FS_Init();
  FS_FAT_SupportLFN();

  while (1) {
    while (FS_GetVolumeStatus("") == FS_MEDIA_NOT_PRESENT) {
      USBD_Logf_Application("Volume not present.");
      USB_OS_Delay(500);
    }
    pFile = FS_FOpen(VIDEO_FILENAME, "r");
    if (pFile != NULL) {
      NumBytes = 0;
      BufferIndex = 1;
      NumBytesFrame = _aRes[_FrameIndex].Width * _aRes[_FrameIndex].Height * 2; // 2 bytes per pixel
      do {
        //
        // Mailbox is full, all buffers still contain unprocessed data.
        //
        while (OS_MAILBOX_GetMessageCnt(&_MB) == 2) {
          USB_OS_Delay(1);
        }
        r = FS_Read(pFile, _acBuffer[BufferIndex], SEGGER_MIN(sizeof(_acBuffer[0]), NumBytesFrame - NumBytes));
        if (r >= 0) {
          NumBytes += r;
          if (NumBytesFrame == NumBytes) {
            NumBytes = 0;
            MBData.EndOfFrame = 1;
          } else {
            MBData.EndOfFrame = 0;
          }
          MBData.NumBytes = r;
          MBData.BufferIndex = BufferIndex;
          OS_MAILBOX_PutBlocked(&_MB, &MBData);
          BufferIndex ^= 1;
        } else {
          USBD_Logf_Application("FS_Read returned %d (%s)", r, FS_ErrorNo2Text(FS_FError(pFile)));
        }
      } while (r > 0);
      FS_FClose(pFile);
    } else {
      USBD_Logf_Application("Could not open video file.");
      USB_OS_Delay(500);
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
  MB_DATA MBData;

  OS_MAILBOX_Create(&_MB, sizeof(_MB_Data[0]), 2, _MB_Data);
  USBD_Init();
  USBD_EnableIAD();
  _AddUVC();
  USBD_UVC_SetOnResolutionChange(_ResChange);
  OS_CREATETASK_EX(&_FSTCB, "FS Task", _FSTask, 100, _aFSStack, NULL);
  USBD_Start();

  while (1) {
    //
    // Wait for configuration
    //
    while ((USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)) != USB_STAT_CONFIGURED) {
      BSP_ToggleLED(0);
      USB_OS_Delay(50);
    }
    if (OS_MAILBOX_WaitTimed(&_MB, 100) == 0) {
      OS_MAILBOX_Peek(&_MB, (char *)&MBData);
      //USBD_Logf_Application("Transfer start BufferIndex %d NumBytes %d EndOfFrame %d", MBData.BufferIndex, MBData.NumBytes, MBData.EndOfFrame);
      if (MBData.EndOfFrame != 0u) {
        USBD_UVC_Write((U8 *)&_acBuffer[MBData.BufferIndex][0], MBData.NumBytes, USBD_UVC_END_OF_FRAME);
      } else {
        USBD_UVC_Write((U8 *)&_acBuffer[MBData.BufferIndex][0], MBData.NumBytes, 0);
      }
      OS_MAILBOX_GetTimed(&_MB, (char *)&MBData, 100);
    }
  }
}

/**************************** end of file ***************************/
