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

File    : IP_TFTPServerSample.c
Purpose : Sample program for embOS & emNet
          Demonstrates use of the emNet TFTP server that can be
          used to access files on the file system of the target.
Notes   : For compatibility with interfaces that need to connect in
          any way this sample calls connect and disconnect routines
          that might not be needed in all cases.

          This sample can be used for Ethernet and dial-up interfaces
          and is configured to use the last registered interface as
          its main interface.
*/

#include "RTOS.h"
#include "BSP.h"
#include "IP.h"
#include "IP_FS.h"
#include "IP_TFTP.h"

/*********************************************************************
*
*       Configuration
*
**********************************************************************
*/

#define USE_RX_TASK   0  // 0: Packets are read in ISR, 1: Packets are read in a task of its own.

//
// TFTP server sample.
//
#define SERVER_PORT  69

//
// Task priorities.
//
enum {
   TASK_PRIO_IP_TFTP = 150
  ,TASK_PRIO_IP_TASK        // Priority should be higher than all IP application tasks.
#if USE_RX_TASK
  ,TASK_PRIO_IP_RX_TASK     // Must be the highest priority of all IP related tasks.
#endif
};

//
// Task stack sizes that might not fit for some interfaces (multiples of sizeof(int)).
//
#ifndef   APP_TASK_STACK_OVERHEAD
  #define APP_TASK_STACK_OVERHEAD     0
#endif

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static IP_HOOK_ON_STATE_CHANGE _StateChangeHook;
static int                     _IFaceId;

static TFTP_CONTEXT            _Context;        // TFTP context holding settings and states.
static char                    _acBuffer[516];  // Largest possible TFTP packet size is 516 bytes. 512 bytes payload and 4 bytes TFTP header.

//
// Task stacks and Task-Control-Blocks.
//
static OS_STACKPTR int _IPStack[TASK_STACK_SIZE_IP_TASK/sizeof(int)];       // Stack of the IP_Task.
static OS_TASK         _IPTCB;                                              // Task-Control-Block of the IP_Task.

#if USE_RX_TASK
static OS_STACKPTR int _IPRxStack[TASK_STACK_SIZE_IP_RX_TASK/sizeof(int)];  // Stack of the IP_RxTask.
static OS_TASK         _IPRxTCB;                                            // Task-Control-Block of the IP_RxTask.
#endif

static OS_STACKPTR int _TFTPStack[2048/sizeof(int)];                        // Stack of the TFTP task.
static OS_TASK         _TFTPTCB;                                            // Task-Control-Block of the TFTP task.

/*********************************************************************
*
*       Prototypes
*
**********************************************************************
*/
#ifdef __cplusplus
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif
void MainTask(void);
#ifdef __cplusplus
}
#endif

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _OnStateChange()
*
* Function description
*   Callback that will be notified once the state of an interface
*   changes.
*
* Parameters
*   IFaceId   : Zero-based interface index.
*   AdminState: Is this interface enabled ?
*   HWState   : Is this interface physically ready ?
*/
static void _OnStateChange(unsigned IFaceId, U8 AdminState, U8 HWState) {
  //
  // Check if this is a disconnect from the peer or a link down.
  // In this case call IP_Disconnect() to get into a known state.
  //
  if (((AdminState == IP_ADMIN_STATE_DOWN) && (HWState == 1)) ||  // Typical for dial-up connection e.g. PPP when closed from peer. Link up but app. closed.
      ((AdminState == IP_ADMIN_STATE_UP)   && (HWState == 0))) {  // Typical for any Ethernet connection e.g. PPPoE. App. opened but link down.
    IP_Disconnect(IFaceId);                                       // Disconnect the interface to a clean state.
  }
}

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       MainTask()
*
* Function description
*   Main task executed by the RTOS to create further resources and
*   running the main application.
*/
void MainTask(void) {
  IP_Init();
  _IFaceId = IP_INFO_GetNumInterfaces() - 1;                                                                           // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
  OS_SetPriority(OS_GetTaskID(), TASK_PRIO_IP_TASK);                                                                   // For now, this task has highest prio except IP management tasks.
  OS_CREATETASK   (&_IPTCB  , "IP_Task"    , IP_Task           , TASK_PRIO_IP_TASK   , _IPStack);                      // Start the IP_Task.
#if USE_RX_TASK
  OS_CREATETASK   (&_IPRxTCB, "IP_RxTask"  , IP_RxTask         , TASK_PRIO_IP_RX_TASK, _IPRxStack);                    // Start the IP_RxTask, optional.
#endif
  IP_TFTP_InitContext(&_Context, _IFaceId, &IP_FS_FS, _acBuffer, sizeof(_acBuffer), SERVER_PORT);                      // Init the TFTP server.
  OS_CREATETASK_EX(&_TFTPTCB, "TFTP server", IP_TFTP_ServerTask, TASK_PRIO_IP_TFTP   , _TFTPStack, (void*)&_Context);  // Start the TFTP server task.
  IP_AddStateChangeHook(&_StateChangeHook, _OnStateChange);                                                            // Register hook to be notified on disconnects.
  IP_Connect(_IFaceId);                                                                                                // Connect the interface if necessary.
  OS_SetPriority(OS_GetTaskID(), 255);                                                                                 // Now this task has highest prio for real-time application. This is only allowed when this task does not use blocking IP API after this point.
  while (IP_IFaceIsReadyEx(_IFaceId) == 0) {
    OS_Delay(50);
  }
  while (1) {
    BSP_ToggleLED(1);
    OS_Delay(200);
  }
}

/****** End Of File *************************************************/
