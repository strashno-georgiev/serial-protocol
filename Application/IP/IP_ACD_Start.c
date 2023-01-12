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
File    : IP_ACD_Start.c
Purpose : Sample program for emNet with ACD
          Demonstrates use of the IP stack to automatically detect
          and solve IP address collisions in a network.

          The sample is intended to be used with a fixed IP. To fully
          test this sample the following criteria have to be met:
            - A fixed IP addr. has to be configured in the IP_X_Config()
            - Another or the same fixed IP has to be configured for the defines
              IP_ADDR and SUBNET_MASK in this sample. If the fixed IP in IP_X_Config()
              is not available, this sample will test IPs starting at IP_ADDR.
            - A PC configured to the same IP addr. as for this target.

          Test procedure:
            - Make sure the other device with the same IP addr. as configured in IP_X_Config()
              for the target is available in the network.
            - Run the sample, the other host will be detected and the IP addr. IP_ADDR
              will be tested and used if free. If it is not free, IP_ADDR is incremented.
              This is to test collision detection on ACD startup.
            - Reconfigure the other device to use the new IP addr. of the target to test
              ACD while running. On collision detection the IP addr. of the
              target is incremented again.

          Notes:
            - Avoiding a conflict by choosing a new IP addr. is the easiest
              way. Defending the IP addr. can be done as well but a strategy
              to defend has to be implemented by the user itself.

          The following is a sample of the output to the terminal window
          of the test procedure described above:

          MainTask - ACD: IP addr. for interface 0 declined: 192.168.88.88 already in use.
          MainTask - ACD: Restart testing with 192.168.88.89 for interface 0.
          MainTask - ACD: IP addr. checked, no conflicts. Using 192.168.88.89 for interface 0.
          IP_Task - ACD: IP conflict detected for interface 0!
          IP_Task - *** Warning *** ACD: Conflicting host: 00:0C:29:56:3B:77 uses IP: 192.168.88.89
          IP_Task - ACD: Send gratuitous ARP to defend the 192.168.88.89.
          IP_Task - ACD: IP conflict detected for interface 0!
          IP_Task - ACD: IP addr. checked, no conflicts. Using 192.168.88.90 for interface 0.
--------- END-OF-HEADER --------------------------------------------*/

#include "RTOS.h"
#include "BSP.h"
#include "IP.h"

/*********************************************************************
*
*       Configuration
*
**********************************************************************
*/

#define USE_RX_TASK         0  // 0: Packets are read in ISR, 1: Packets are read in a task of its own.

//
// ACD sample.
//
#define IP_ADDR             IP_BYTES2ADDR(192, 168, 88, 88)  // Start IP addr. for first ACD test.
#define SUBNET_MASK         IP_BYTES2ADDR(255, 255,  0,  0)  // Subnet mask.
#define GATEWAY_ADDR        IP_BYTES2ADDR(  0,   0,  0,  0)  // Gateway IP addr. if needed.

#define NUM_PROBES          5           // Number of probes to be sent out to check on startup that the used IP is not in use
#define DEFEND_TIME         5000        // How long [ms] to defend a bound IP addr. against an other host while ACD is active

//
// Task priorities.
//
enum {
   TASK_PRIO_IP_TASK = 150  // Priority should be higher than all IP application tasks.
#if USE_RX_TASK
  ,TASK_PRIO_IP_RX_TASK     // Must be the highest priority of all IP related tasks.
#endif
};

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static IP_HOOK_ON_STATE_CHANGE _StateChangeHook;
static int                     _IFaceId;
static U32                     _IPAddr = IP_ADDR;

//
// Task stacks and Task-Control-Blocks.
//
static OS_STACKPTR int _IPStack[TASK_STACK_SIZE_IP_TASK/sizeof(int)];       // Stack of the IP_Task.
static OS_TASK         _IPTCB;                                              // Task-Control-Block of the IP_Task.

#if USE_RX_TASK
static OS_STACKPTR int _IPRxStack[TASK_STACK_SIZE_IP_RX_TASK/sizeof(int)];  // Stack of the IP_RxTask.
static OS_TASK         _IPRxTCB;                                            // Task-Control-Block of the IP_RxTask.
#endif

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
*       Static code
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
*       _RenewIPAddr
*
*  Function description
*    Called from the ACD module if a address conflict has been detected
*    during start-up. Returns a new IP address which should be used
*    instead.
*/
static U32 _RenewIPAddr(unsigned IFace) {
  IP_USE_PARA(IFace);

  return _IPAddr++;  // Return next IP addr.
}

/*********************************************************************
*
*       _Restart
*
*  Function description
*    Called from the ACD module if a address conflict has been detected during
*    run-time. The function can be used to set a new IP address or to give the
*    stack the command to defend the IP address. Defending the IP address means
*    that each conflicting ARP request will be responded with a gratuitous ARP
*    packet. Note that this can produce high traffic load if both conflicting
*    targets do not stop sending gratuitous ARP packets.
*
*  Return value
*    0: Send no further gratuitous ARP packets.
*    1: Target keeps IP address and sends further gratuitous ARP packets.
*/
static int _Restart(unsigned IFace) {
  IP_USE_PARA(IFace);

  //
  // In case of a conflict set a new IP address.
  //
  IP_SetAddrMask(_IPAddr++, SUBNET_MASK);  // Assign next IP addr. and subnet mask
  IP_ACD_Activate(0);                      // Re-activate ACD with new IP addr.
  return 0;
}

/*********************************************************************
*
*       ACD functions
*/
static const ACD_FUNC _ACD_Func = {
  _RenewIPAddr,  // pfRenewIPAddr, used to renew the IP address if a conflict has been detected during startup
  NULL,          // pfDefend     , used to defend the IP address against a conflicting host on the network. It is the users responsibility to defend his IP addr.
  _Restart       // pfRestart    , used to restart the address conflict detection in case of a conflict with a host on the network
};

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
void MainTask(void) {
  IP_Init();
  _IFaceId = IP_INFO_GetNumInterfaces() - 1;                                           // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
  OS_SetPriority(OS_GetTaskID(), TASK_PRIO_IP_TASK);                                   // For now, this task has highest prio except IP management tasks.
  OS_CREATETASK(&_IPTCB  , "IP_Task"  , IP_Task  , TASK_PRIO_IP_TASK   , _IPStack);    // Start the IP_Task.
#if USE_RX_TASK
  OS_CREATETASK(&_IPRxTCB, "IP_RxTask", IP_RxTask, TASK_PRIO_IP_RX_TASK, _IPRxStack);  // Start the IP_RxTask, optional.
#endif
  IP_AddStateChangeHook(&_StateChangeHook, _OnStateChange);                            // Register hook to be notified on disconnects.
  IP_Connect(_IFaceId);                                                                // Connect the interface if necessary.
  OS_SetPriority(OS_GetTaskID(), 255);                                                 // Now this task has highest prio for real-time application. This is only allowed when this task does not use blocking IP API after this point.
  //
  // Enhance log filter to output ACD related status messages in debug builds
  //
  IP_AddLogFilter(IP_MTYPE_ACD);
  //
  // Wait for link to become ready
  //
  while (IP_GetCurrentLinkSpeedEx(_IFaceId) == 0) {
    OS_Delay(10);
  }
  //
  // Enable ACD configuration
  //
  IP_ACD_Config(_IFaceId, NUM_PROBES, DEFEND_TIME, &_ACD_Func);  // IFace: 0, Number of probes, Defend interval [ms], callback function pointers
  IP_ACD_Activate(_IFaceId);
  //
  // Application
  //
  while (1) {
    BSP_ToggleLED(1);
    OS_Delay (200);
  }
}

/*************************** End of file ****************************/
