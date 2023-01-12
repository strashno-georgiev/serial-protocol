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

File    : USBH_AUDIO_ScanDevices.c
Purpose : emUSB-Host sample to scan all audio devices.
Expected behavior:
  If an audio device is connected, it is enumerated and some information
  about the device is output in the debug terminal.
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
#define MAX_AUDIO_INTERFACES        10

enum {
  TASK_PRIO_APP = 150,
  TASK_PRIO_USBH_MAIN,
  TASK_PRIO_USBH_ISR
};

typedef struct {
  I8                State;        // 0: unused slot, 1: active, -1: removed
  U8                DevIndex;
  USBH_AUDIO_HANDLE Handle;
} AUDIO_INTERFACE;

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

static AUDIO_INTERFACE     _InterfaceList[MAX_AUDIO_INTERFACES];

/*********************************************************************
*
*       Static Code
*
**********************************************************************
*/

/*********************************************************************
*
*       _cbOnAddRemoveDevice
*
*  Function description
*    Callback, called when a device is added or removed.
*    Call in the context of the USBH_Task.
*    The functionality in this routine should not block!
*/
static void _cbOnAddRemoveDevice(void * pContext, U8 DevIndex, USBH_DEVICE_EVENT Event) {
  unsigned i;

  (void)pContext;
  if (Event == USBH_DEVICE_EVENT_ADD) {
    USBH_Logf_Application("**** AUDIO Interface added [%d]", DevIndex);
    for (i = 0; i < MAX_AUDIO_INTERFACES; i++) {
      if (_InterfaceList[i].State == 0) {
        _InterfaceList[i].DevIndex = DevIndex;
        _InterfaceList[i].Handle   = USBH_AUDIO_INVALID_HANDLE;
        _InterfaceList[i].State    = 1;
        break;
      }
    }
  } else {
    USBH_Logf_Application("**** AUDIO Interface removed [%d]", DevIndex);
    for (i = 0; i < MAX_AUDIO_INTERFACES; i++) {
      if (_InterfaceList[i].State > 0 && _InterfaceList[i].DevIndex == DevIndex) {
        _InterfaceList[i].State = -1;
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
  USBH_AUDIO_TERMINAL_INFO      TermInfo;
  USBH_AUDIO_EP_INFO            EPInfo;
  USBH_AUDIO_FU_INFO            FUInfo;
  USBH_AUDIO_SU_INFO            SUInfo;
  USBH_AUDIO_MU_INFO            MUInfo;
  USBH_INTERFACE_HANDLE         IntfHandle;
  char                          Buff[64];
  static USBH_NOTIFICATION_HOOK Hook;

  USBH_Init();
  OS_SetPriority(OS_GetTaskID(), TASK_PRIO_APP);                                       // This task has the lowest prio for real-time application.
                                                                                       // Tasks using emUSB-Host API should always have a lower priority than emUSB-Host main and ISR tasks.
  OS_CREATETASK(&_TCBMain, "USBH_Task", USBH_Task, TASK_PRIO_USBH_MAIN, _StackMain);   // Start USBH main task
  OS_CREATETASK(&_TCBIsr, "USBH_isr", USBH_ISRTask, TASK_PRIO_USBH_ISR, _StackIsr);    // Start USBH ISR task
  USBH_AUDIO_Init();
  USBH_AUDIO_AddNotification(&Hook, _cbOnAddRemoveDevice, NULL);
  for (;;) {
    USBH_OS_Delay(50);
    BSP_ToggleLED(1);
    pIntf = _InterfaceList;
    for (i = 0; i < MAX_AUDIO_INTERFACES; i++) {
      if (pIntf->State < 0) {
        if (pIntf->Handle != USBH_AUDIO_INVALID_HANDLE) {
          USBH_AUDIO_Close(pIntf->Handle);
        }
        pIntf->State = 0;
      }
      if (pIntf->State > 0 && pIntf->Handle == USBH_AUDIO_INVALID_HANDLE) {
        Status = USBH_AUDIO_Open(pIntf->DevIndex, &pIntf->Handle);
        if (Status != USBH_STATUS_SUCCESS) {
          USBH_Logf_Application("Can't open interface (%s)", USBH_GetStatusStr(Status));
          pIntf->State = 0;
          continue;
        }
        Status = USBH_AUDIO_GetInterfaceInfo(pIntf->Handle,  &DevInfo);
        if (Status != USBH_STATUS_SUCCESS) {
          USBH_Logf_Application("Can't get interface info (%x)", Status);
          continue;
        }
        if (DevInfo.SubClass == USBH_AUDIO_SUBCLASS_CONTROL) {
          USBH_Logf_Application("AUDIO control interface found (DevId %u):", DevInfo.DeviceId);
          USBH_Logf_Application("  Vendor %04x, Product %04x, Version %u.0", DevInfo.VendorId, DevInfo.ProductId,
                                                                             (DevInfo.Protocol == 0) ? 1 : (DevInfo.Protocol >> 4));
          IntfHandle = USBH_AUDIO_GetInterfaceHandle(pIntf->Handle);
          if (USBH_GetSerialNumberASCII(IntfHandle, Buff, sizeof(Buff)) == USBH_STATUS_SUCCESS && Buff[0] != 0) {
            USBH_Logf_Application("  Serial: %s", Buff);
          }
          if (USBH_GetStringDescriptorASCII(IntfHandle, USBH_GetDeviceDescriptorPtr(IntfHandle)->iManufacturer, Buff, sizeof(Buff)) == USBH_STATUS_SUCCESS) {
            USBH_Logf_Application("  Manuf.: %s", Buff);
          }
          if (USBH_GetStringDescriptorASCII(IntfHandle, USBH_GetDeviceDescriptorPtr(IntfHandle)->iProduct, Buff, sizeof(Buff)) == USBH_STATUS_SUCCESS) {
            USBH_Logf_Application("  Prod. : %s", Buff);
          }
          for (j = 0;; j++) {
            if (USBH_AUDIO_GetTerminalInfo(pIntf->Handle, j, &TermInfo) != USBH_STATUS_SUCCESS) {
              break;
            }
            if (TermInfo.SourceID == 0) {
              USBH_Logf_Application("  Input terminal %u", TermInfo.UnitID);
            } else {
              USBH_Logf_Application("  Output terminal %u, Source %u", TermInfo.UnitID, TermInfo.SourceID);
            }
            USBH_Logf_Application("    Terminal type %x", TermInfo.TerminalType);
          }
          for (j = 0;; j++) {
            if (USBH_AUDIO_GetFeatureUnitInfo(pIntf->Handle, j, &FUInfo) != USBH_STATUS_SUCCESS) {
              break;
            }
            USBH_Logf_Application("  Feature Unit %u, Source %u", FUInfo.UnitID, FUInfo.SourceID);
            for (k = 0; k < FUInfo.NumControlChannels; k++) {
              USBH_Logf_Application("    Control channel %u, mask = %x", k, FUInfo.bmControls[k]);
              if ((FUInfo.bmControls[k] & USBH_AUDIO_10_FU_CONTROL_MASK_VOLUME) != 0u) {
                USBH_AUDIO_VOLUME_INFO Volume;
                Status = USBH_AUDIO_GetVolume(pIntf->Handle, FUInfo.UnitID, k, &Volume);
                if (Status != USBH_STATUS_SUCCESS) {
                  USBH_Logf_Application("Can't get volume info (%x)", Status);
                  continue;
                }
                USBH_Logf_Application("      Volume: Cur = %x, Min = %x, Max = %x, Res = %x", Volume.Cur, Volume.Min, Volume.Max, Volume.Res);
              }
            }
          }
          for (j = 0;; j++) {
            if (USBH_AUDIO_GetSelectorUnitInfo(pIntf->Handle, j, &SUInfo) != USBH_STATUS_SUCCESS) {
              break;
            }
            USBH_Logf_Application("  Selector Unit %u", SUInfo.UnitID);
            for (k = 0; k < SUInfo.NumInputs; k++) {
              USBH_Logf_Application("    Input %u = source unit %u", k + 1, SUInfo.SourceID[k]);
            }
          }
          for (j = 0;; j++) {
            if (USBH_AUDIO_GetMixerUnitInfo(pIntf->Handle, j, &MUInfo) != USBH_STATUS_SUCCESS) {
              break;
            }
            USBH_Logf_Application("  Mixer Unit %u", MUInfo.UnitID);
            for (k = 0; k < MUInfo.NumInputs; k++) {
              USBH_Logf_Application("    Input %u = source unit %u", k + 1, MUInfo.SourceID[k]);
            }
            USBH_Logf_Application("    Output channels %u, wChannelConfig = %x", MUInfo.NumOutChannels, MUInfo.wChannelConfig);
            for (k = 0; k < MUInfo.LenControls; k++) {
              USBH_Logf_Application("    Control mask[%u] = %02x", k, MUInfo.bmControls[k]);
            }
          }
        }
        if (DevInfo.SubClass == USBH_AUDIO_SUBCLASS_STREAMING) {
          USBH_Logf_Application("AUDIO streaming interface found (DevId %u):", DevInfo.DeviceId);
          for (k = 0;;) {
            Status = USBH_AUDIO_GetEndpointInfo(pIntf->Handle, k, &EPInfo);
            if (Status != USBH_STATUS_SUCCESS) {
              break;
            }
            if (DevInfo.Protocol == 0) {
              USBH_Logf_Application("  Found streaming %s EP on AltSetting %u",
                                     ((EPInfo.EPAddr & 0x80u) == 0u) ? "OUT" : "IN", EPInfo.AlternateSetting);
              USBH_Logf_Application("    Terminal Link %u", EPInfo.TerminalLink);
              if (EPInfo.FormatTag == 1) {
                USBH_Logf_Application("    Channels=%u, Frame=%u, Resolution=%u",
                                      EPInfo.u.Type1.NrChannels, EPInfo.u.Type1.SubFrameSize, EPInfo.u.Type1.BitResolution);
                if (EPInfo.u.Type1.NumSamFreq == 0u) {
                  USBH_Logf_Application("  Sampling Frequency range %u to %u", EPInfo.u.Type1.SamFreq[0], EPInfo.u.Type1.SamFreq[1]);
                } else {
                  USBH_Logf_Application("  Sampling Frequencies:");
                  for (j = 0; j < EPInfo.u.Type1.NumSamFreq; j++) {
                    USBH_Logf_Application("  %8u Hz", EPInfo.u.Type1.SamFreq[j]);
                  }
                }
              }
            }
            k = EPInfo.AlternateSetting + 1;
          }
        }
      }
      pIntf++;
    }
  }
}

/*************************** End of file ****************************/
