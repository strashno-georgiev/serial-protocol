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

File    : USBH_CCID_Start.c
Purpose : Shows emUSB-Host usage CCID devices.

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
#include "USBH_CCID.h"
#include "USBH_Util.h"

/*********************************************************************
*
*       Local defines, configurable
*
**********************************************************************
*/

#define DEVICE_INDEX            0
#define CARD_SLOT_NO            0

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
static const U8 _APDU_GetUID[] = { 0xFF, 0xCA, 0, 0, 0 };

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

static I8                  _DevValid;

static OS_EVENT            _Event;

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
  (void)pContext;
  if (Event == USBH_DEVICE_EVENT_ADD) {
    USBH_Logf_Application("**** CCID Device added [%d]", DevIndex);
    if (DevIndex == DEVICE_INDEX) {
      _DevValid = 1;
    }
  } else {
    USBH_Logf_Application("**** CCID Device removed [%d]", DevIndex);
    if (DevIndex == DEVICE_INDEX) {
      _DevValid = 0;
    }
  }
  OS_EVENT_Set(&_Event);
}


/*********************************************************************
*
*       _cbOnSlotChange
*
*  Function description
*    Callback, called when the status of a card slot changed.
*/
static void _cbOnSlotChange(void *pContext, U32 SlotState) {
  USBH_USE_PARA(pContext);
  USBH_Logf_Application("New slot state %x", SlotState);
  OS_EVENT_Set(&_Event);
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
  USBH_CCID_HANDLE Handle = USBH_CCID_INVALID_HANDLE;
  U16              SlotStatus;
  I8               CardPresent = 0;
  U32              Len;
  U8               Buff[40];
  USBH_STATUS      Status;
  static USBH_NOTIFICATION_HOOK Hook;

  OS_EVENT_CreateEx(&_Event, OS_EVENT_RESET_MODE_AUTO);
  USBH_Init();
  OS_SetPriority(OS_GetTaskID(), TASK_PRIO_APP);                                       // This task has the lowest prio for real-time application.
                                                                                       // Tasks using emUSB-Host API should always have a lower priority than emUSB-Host main and ISR tasks.
  OS_CREATETASK(&_TCBMain, "USBH_Task", USBH_Task, TASK_PRIO_USBH_MAIN, _StackMain);   // Start USBH main task
  OS_CREATETASK(&_TCBIsr, "USBH_isr", USBH_ISRTask, TASK_PRIO_USBH_ISR, _StackIsr);    // Start USBH ISR task
  USBH_CCID_Init();
  USBH_CCID_AddNotification(&Hook, _cbOnAddRemoveDevice, NULL);
  for (;;) {
    BSP_ToggleLED(1);
    if (_DevValid == 0) {
      if (Handle != USBH_CCID_INVALID_HANDLE) {
        USBH_CCID_Close(Handle);
        Handle = USBH_CCID_INVALID_HANDLE;
        //USBH_Logf_Application("CCID device closed");
      }
      OS_EVENT_WaitTimed(&_Event, 50);
      continue;
    }
    if (Handle == USBH_CCID_INVALID_HANDLE) {
      Handle = USBH_CCID_Open(DEVICE_INDEX);
      if (Handle == USBH_CCID_INVALID_HANDLE) {
        OS_Delay(300);
        continue;
      }
      USBH_CCID_SetTimeout(Handle, 2000);
      USBH_CCID_SetOnSlotChange(Handle, _cbOnSlotChange, NULL);
      //USBH_Logf_Application("CCID device opened successfully");
    }
    Status = USBH_CCID_GetSlotStatus(Handle, CARD_SLOT_NO, &SlotStatus);
    if (Status != USBH_STATUS_SUCCESS) {
      USBH_Logf_Application("Can't get slot status");
      USBH_CCID_Abort(Handle, CARD_SLOT_NO);
      OS_Delay(1000);
      continue;
    }
    if (CardPresent != 0) {
      if (SlotStatus != 0) {
        USBH_Logf_Application("Card removed");
        CardPresent = 0;
      }
    } else {
      if (SlotStatus == 1) {
        Len = sizeof(Buff);
        if (USBH_CCID_PowerOnATR(Handle, CARD_SLOT_NO, &Len, Buff) == USBH_STATUS_SUCCESS) {
          USBH_Logf_Application("ATR:");
          USBH_LogHexDump(USBH_MTYPE_APPLICATION, Len, Buff);
          SlotStatus = 0;
          OS_Delay(50);
        }
      }
      if (SlotStatus == 0) {
#if 0
        U8 Prot;
        U8 TA1;
        TA1 = Buff[2];
        Len = sizeof(Buff);
        if (USBH_CCID_GetParameters(Handle, CARD_SLOT_NO, &Len, Buff, &Prot) == USBH_STATUS_SUCCESS) {
          USBH_Logf_Application("Protocol %u, Parameters:", Prot);
          USBH_LogHexDump(USBH_MTYPE_APPLICATION, Len, Buff);
        }
        Buff[0] = TA1;
        USBH_CCID_SetParameters(Handle, CARD_SLOT_NO, Len, Buff, 0);
#endif
        USBH_Logf_Application("Card inserted");
        CardPresent = 1;
        Len = sizeof(Buff);
        Status = USBH_CCID_APDU(Handle, CARD_SLOT_NO, sizeof(_APDU_GetUID), _APDU_GetUID, &Len, Buff);
        if (Status == USBH_STATUS_SUCCESS && Len == 2 && Buff[0] == 0x61) {
          Len = sizeof(Buff);
          Status = USBH_CCID_GetResponse(Handle, CARD_SLOT_NO, 0, Buff[1], &Len, Buff);
        }
        if (Status == USBH_STATUS_SUCCESS) {
          USBH_Logf_Application("Answer:");
          USBH_LogHexDump(USBH_MTYPE_APPLICATION, Len, Buff);
        } else {
          USBH_CCID_Abort(Handle, CARD_SLOT_NO);
          OS_Delay(100);
        }
      }
    }
    OS_EVENT_WaitTimed(&_Event, 500);
  }
}

/*************************** End of file ****************************/
