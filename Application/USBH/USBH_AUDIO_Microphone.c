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

File    : USBH_AUDIO_Microphone.c
Purpose : emUSB-Host sample to access a USB microphone.

Expected behavior:
  If a microphone is connected, audio data is read from the microphone.
  Every 20 ms, a line of '#' characters is output via RTT. The length of that
  line depends on the volume of the audio sound received in the last 20 ms.
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
#include "USBH_AUDIO.h"
#include "USBH_Util.h"

/*********************************************************************
*
*       Local defines, configurable
*
**********************************************************************
*/
#define MAX_AUDIO_INTERFACES        5

enum {
  TASK_PRIO_APP = 150,
  TASK_PRIO_USBH_MAIN,
  TASK_PRIO_USBH_ISR
};

/*********************************************************************
*
*       Static const data
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

typedef struct {
  I8                Valid;
  U8                DevIndex;
} AUDIO_INTERFACE;

static AUDIO_INTERFACE     _InterfaceList[MAX_AUDIO_INTERFACES];
static I8                  _TriggerListen;

/*********************************************************************
*
*       Static Code
*
**********************************************************************
*/

/*********************************************************************
*
*       _cbOnAddRemoveAudio
*
*  Function description
*    Callback, called when a device is added or removed.
*    Call in the context of the USBH_Task.
*    The functionality in this routine should not block!
*/
static void _cbOnAddRemoveAudio(void * pContext, U8 DevIndex, USBH_DEVICE_EVENT Event) {
  unsigned i;

  (void)pContext;
  if (Event == USBH_DEVICE_EVENT_ADD) {
    USBH_Logf_Application("**** AUDIO Interface added [%d]", DevIndex);
    for (i = 0; i < MAX_AUDIO_INTERFACES; i++) {
      if (_InterfaceList[i].Valid == 0) {
        _InterfaceList[i].DevIndex = DevIndex;
        _InterfaceList[i].Valid    = 1;
        _TriggerListen             = 1;
        break;
      }
    }
  } else {
    USBH_Logf_Application("**** AUDIO Interface removed [%d]", DevIndex);
    for (i = 0; i < MAX_AUDIO_INTERFACES; i++) {
      if (_InterfaceList[i].Valid != 0 && _InterfaceList[i].DevIndex == DevIndex) {
        _InterfaceList[i].Valid = 0;
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
*/
#ifdef __cplusplus
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif
void MainTask(void);
#ifdef __cplusplus
}
#endif
void MainTask(void) {
  unsigned                      i;
  unsigned                      j;
  unsigned                      k;
  USBH_STATUS                   Status;
  AUDIO_INTERFACE             * pIntf;
  USBH_AUDIO_INTERFACE_INFO     DevInfo;
  USBH_AUDIO_EP_INFO            EPInfo;
  USBH_AUDIO_EP_INFO            EPInInfo;
  USBH_AUDIO_FU_INFO            FUInfo;
  U32                           SampleFrequency;
  USBH_INTERFACE_HANDLE         IntfHandle;
  char                          sBuff[62];
  USBH_AUDIO_HANDLE             AHdl;
  USBH_AUDIO_HANDLE             AudioCtrl;
  USBH_AUDIO_HANDLE             AudioIn;
  USBH_AUDIO_CHANNEL            Channel;
  U32                           Len;
  const U8                    * pData;
  U32                           AvgVal;
  U32                           AvgCnt;
  static USBH_NOTIFICATION_HOOK Hook;

  USBH_Init();
  OS_SetPriority(OS_GetTaskID(), TASK_PRIO_APP);                                       // This task has the lowest prio for real-time application.
                                                                                       // Tasks using emUSB-Host API should always have a lower priority than emUSB-Host main and ISR tasks.
  OS_CREATETASK(&_TCBMain, "USBH_Task", USBH_Task, TASK_PRIO_USBH_MAIN, _StackMain);   // Start USBH main task
  OS_CREATETASK(&_TCBIsr, "USBH_isr", USBH_ISRTask, TASK_PRIO_USBH_ISR, _StackIsr);    // Start USBH ISR task
  USBH_AUDIO_Init();
  USBH_AUDIO_AddNotification(&Hook, _cbOnAddRemoveAudio, NULL);
  for (;;) {
    USBH_OS_Delay(50);
    BSP_ToggleLED(0);
    if (_TriggerListen == 0) {
      continue;
    }
    _TriggerListen = 0;
    //
    // Try to find a audio device to read sound data from.
    //
    AudioCtrl = USBH_AUDIO_INVALID_HANDLE;
    AudioIn   = USBH_AUDIO_INVALID_HANDLE;
    memset(&EPInInfo, 0, sizeof(EPInInfo));
    SampleFrequency = 0;
    pIntf = _InterfaceList;
    for (i = 0; i < MAX_AUDIO_INTERFACES; i++) {
      if (pIntf->Valid != 0) {
        Status = USBH_AUDIO_Open(pIntf->DevIndex, &AHdl);
        if (Status != USBH_STATUS_SUCCESS) {
          USBH_Logf_Application("Can't open interface (%s)", USBH_GetStatusStr(Status));
          AHdl = USBH_AUDIO_INVALID_HANDLE;
          goto IntfDone;
        }
        Status = USBH_AUDIO_GetInterfaceInfo(AHdl, &DevInfo);
        if (Status != USBH_STATUS_SUCCESS) {
          USBH_Logf_Application("Can't get interface info (%s)", USBH_GetStatusStr(Status));
          goto IntfDone;
        }
        if (DevInfo.SubClass == USBH_AUDIO_SUBCLASS_CONTROL) {
          USBH_Logf_Application("AUDIO device:");
          USBH_Logf_Application("  Vendor %04x, Product %04x, Version %u.0", DevInfo.VendorId, DevInfo.ProductId,
                                                                           (DevInfo.Protocol == 0) ? 1 : (DevInfo.Protocol >> 4));
          IntfHandle = USBH_AUDIO_GetInterfaceHandle(AHdl);
          if (USBH_GetStringDescriptorASCII(IntfHandle, USBH_GetDeviceDescriptorPtr(IntfHandle)->iManufacturer, sBuff, sizeof(sBuff)) == USBH_STATUS_SUCCESS) {
            USBH_Logf_Application("  Manuf.: %s", sBuff);
          }
          if (USBH_GetStringDescriptorASCII(IntfHandle, USBH_GetDeviceDescriptorPtr(IntfHandle)->iProduct, sBuff, sizeof(sBuff)) == USBH_STATUS_SUCCESS) {
            USBH_Logf_Application("  Prod. : %s", sBuff);
          }
          if (DevInfo.Protocol == 0) {
            AudioCtrl = AHdl;
            AHdl = USBH_AUDIO_INVALID_HANDLE;
            goto IntfDone;
          }
        }
        if (DevInfo.SubClass == USBH_AUDIO_SUBCLASS_STREAMING) {
          Status = USBH_AUDIO_GetEndpointInfo(AHdl, 0, &EPInfo);
          if (Status != USBH_STATUS_SUCCESS) {
            USBH_Logf_Application("Can't get endpoint info (%s)", USBH_GetStatusStr(Status));
            goto IntfDone;
          }
          if ((EPInfo.EPAddr & 0x80u) == 0u) {
            goto IntfDone;
          }
          USBH_Logf_Application("  Found streaming IN, Channels=%u, Frame=%u, Resolution=%u",
                                 EPInfo.u.Type1.NrChannels, EPInfo.u.Type1.SubFrameSize, EPInfo.u.Type1.BitResolution);
          if (EPInfo.u.Type1.NumSamFreq == 0u) {
            USBH_Logf_Application("  Sampling Frequency range %u to %u", EPInfo.u.Type1.SamFreq[0], EPInfo.u.Type1.SamFreq[1]);
            SampleFrequency = EPInfo.u.Type1.SamFreq[1];
          } else {
            USBH_Logf_Application("  Sampling Frequencies:");
            for (j = 0; j < EPInfo.u.Type1.NumSamFreq; j++) {
              SampleFrequency = EPInfo.u.Type1.SamFreq[j];
              USBH_Logf_Application("  %8u Hz", SampleFrequency);
            }
          }
          if (EPInfo.u.Type1.SubFrameSize == 2 && EPInfo.u.Type1.BitResolution == 16) {
            AudioIn = AHdl;
            AHdl = USBH_AUDIO_INVALID_HANDLE;
            EPInInfo = EPInfo;
            goto IntfDone;
          }
          USBH_Logf_Application("  Audio parameters do not match");
        }
IntfDone:
        if (AHdl != USBH_AUDIO_INVALID_HANDLE) {
          USBH_AUDIO_Close(AHdl);
        }
      }
      pIntf++;
    }
    if (AudioCtrl == USBH_AUDIO_INVALID_HANDLE || AudioIn == USBH_AUDIO_INVALID_HANDLE) {
      goto AudioDone;
    }
    USBH_Logf_Application("Configure Audio device...");
    if (EPInInfo.AlternateSetting != 0) {
      USBH_Logf_Application("SetAltInterface to %u", EPInInfo.AlternateSetting);
      Status = USBH_AUDIO_SetAlternateInterface(AudioIn, EPInInfo.AlternateSetting);
      if (Status != USBH_STATUS_SUCCESS) {
        USBH_Logf_Application("  failed: %s", USBH_GetStatusStr(Status));
        goto AudioDone;
      }
    }
    if ((EPInInfo.bmAttributes & 1u) != 0u) {
      U32 Tmp;

      USBH_Logf_Application("SetSampleFreq to %u Hz", SampleFrequency);
      Tmp = SampleFrequency;
      Status = USBH_AUDIO_SetSampleFrequency(AudioIn, EPInInfo.EPAddr, &Tmp);
      if (Status != USBH_STATUS_SUCCESS) {
        USBH_Logf_Application("  failed: %s", USBH_GetStatusStr(Status));
        goto AudioDone;
      }
      if (SampleFrequency != Tmp) {
        USBH_Logf_Application("  can't set SetSampleFreq, device reports %u Hz", Tmp);
        goto AudioDone;
      }
    }
    for (j = 0;; j++) {
      if (USBH_AUDIO_GetFeatureUnitInfo(AudioCtrl, j, &FUInfo) != USBH_STATUS_SUCCESS) {
        break;
      }
      USBH_Logf_Application("Feature unit %u, Source %u", FUInfo.UnitID, FUInfo.SourceID);
      for (k = 0; k < FUInfo.NumControlChannels; k++) {
        if ((FUInfo.bmControls[k] & USBH_AUDIO_10_FU_CONTROL_MASK_MUTE) != 0u) {
          USBH_Logf_Application("  Unmute Unit %u, control %u", FUInfo.UnitID, k);
          Status = USBH_AUDIO_SetMute(AudioCtrl, FUInfo.UnitID, k, 0);
          if (Status != USBH_STATUS_SUCCESS) {
            USBH_Logf_Application("    failed: %s", USBH_GetStatusStr(Status));
            goto AudioDone;
          }
        }
        if ((FUInfo.bmControls[k] & USBH_AUDIO_10_FU_CONTROL_MASK_VOLUME) != 0u) {
          USBH_AUDIO_VOLUME_INFO Volume;

          USBH_Logf_Application("  Get volume unit %u, control %u", FUInfo.UnitID, k);
          Status = USBH_AUDIO_GetVolume(AudioCtrl, FUInfo.UnitID, k, &Volume);
          if (Status != USBH_STATUS_SUCCESS) {
            USBH_Logf_Application("    failed: %s", USBH_GetStatusStr(Status));
            goto AudioDone;
          }
          USBH_Logf_Application("    Volume[%u] = %x, %x, %x, %x", k, Volume.Cur, Volume.Min, Volume.Max, Volume.Res);
          USBH_Logf_Application("  Set volume to maximum");
          Status = USBH_AUDIO_SetVolume(AudioCtrl, FUInfo.UnitID, k, Volume.Max);
          if (Status != USBH_STATUS_SUCCESS) {
            if (Status != USBH_STATUS_NOT_SUPPORTED) {
              USBH_Logf_Application("    failed: %s", USBH_GetStatusStr(Status));
              goto AudioDone;
            }
            USBH_Logf_Application("    Volume control not supported");
          }
        }
      }
    }
    USBH_Logf_Application("Open streaming channel");
    Status = USBH_AUDIO_OpenInChannel(AudioIn, EPInInfo.EPAddr, &Channel);
    if (Status != USBH_STATUS_SUCCESS) {
      USBH_Logf_Application("  failed: %s", USBH_GetStatusStr(Status));
      goto AudioDone;
    }
    USBH_Logf_Application("Listening ....");
    BSP_SetLED(0);
    memset(sBuff, '#', sizeof(sBuff));
    sBuff[sizeof(sBuff) - 2] = '\n';
    sBuff[sizeof(sBuff) - 1] = '\0';
    AvgVal = 0;
    AvgCnt = 0;
    j = 0;
    for (i = 0;;) {
      Status = USBH_AUDIO_Receive(Channel, &Len, &pData, 0);
      if (Status != USBH_STATUS_SUCCESS) {
        USBH_Logf_Application("  Receive audio failed: %s", USBH_GetStatusStr(Status));
        break;
      }
      //
      // Calculate average volume of the audio data.
      //
      while (Len >= 2) {
        k = *pData++;
        k += (*pData++ << 8);
        if (k >= 0x8000) {
          k ^= 0xFFFF;
        }
        AvgVal += k >> 2;
        AvgCnt++;
        Len -= 2;
      }
      USBH_AUDIO_Ack(Channel);
      if (++i > 20) {
        //
        // Output volume bar
        //
        AvgVal /= AvgCnt;
        AvgVal *= sizeof(sBuff) - 2;
        k = (AvgVal + (1 << 12)) >> 13;
        if (k != 0 || j != 0) {
          k++;
          if (k > sizeof(sBuff) - 1) {
            k = sizeof(sBuff) - 1;
          }
          USBH_Puts(sBuff + sizeof(sBuff) - 1 - k);
          j = 1;
        }
        AvgVal = 0;
        AvgCnt = 0;
        i = 0;
      }
    }
    USBH_AUDIO_CloseInChannel(Channel);
    USBH_Logf_Application("End");
AudioDone:
    if (AudioCtrl != USBH_AUDIO_INVALID_HANDLE) {
      USBH_AUDIO_Close(AudioCtrl);
    }
    if (AudioIn != USBH_AUDIO_INVALID_HANDLE) {
      USBH_AUDIO_Close(AudioIn);
    }
  }
}

/*************************** End of file ****************************/
