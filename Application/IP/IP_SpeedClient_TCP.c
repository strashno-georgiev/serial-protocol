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
File    : IP_SpeedClient_TCP.c
Purpose : Speed client for TCP/IP stack using socket interface.
--------- END-OF-HEADER --------------------------------------------*/

#include "RTOS.h"
#include "BSP.h"
#include "IP.h"

/*********************************************************************
*
*       Local defines, configurable
*
**********************************************************************
*/

#ifndef   SPEEDCLIENT_NUM_CHUNKS
  #define SPEEDCLIENT_NUM_CHUNKS  1                                       // Number of chunks (full packets) to try to transfer at once. More could increase speed.
#endif
#define   SERVER_IP_ADDR          IP_BYTES2ADDR(192, 168, 88, 12)         // IP address of server, for example 192.168.88.1 .
#define   SERVER_PORT             1234                                    // Remote destination port.
#define   NUMBER_OF_BYTES         (4uL * 1024uL * 1024uL)                 // Number of bytes to transmit.
#define   BUFFER_SIZE             (SPEEDCLIENT_NUM_CHUNKS * (1500 - 40))  // Maximum number of bytes we can transfer at once; MTU - TCP/IP header.
#define   DIRECTION               3                                       // 1 for receive, 2 for send, 3 for Rx & Tx .
#define   USE_RX_TASK             0                                       // 0: Packets are read in ISR, 1: Packets are read in a task of its own.

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

static char            _aRxTxBuffer[BUFFER_SIZE];
static OS_STACKPTR int _StackIP[TASK_STACK_SIZE_IP_TASK];      // Task stacks
static OS_STACKPTR int _StackClient[768 + APP_TASK_STACK_OVERHEAD];
static OS_TASK         _TCBIP;                                 // Task-control-blocks
static OS_TASK         _TCBClient;
#if USE_RX_TASK
static OS_TASK         _TCBIPRx;
static OS_STACKPTR int _StackIPRx[TASK_STACK_SIZE_IP_RX_TASK];
#endif

//
// Statistics to count all successful transmissions of NUMBER_OF_BYTES
//
static struct {
  U32 RxCnt;
  U32 TxCnt;
  U32 ErrCnt;
} _Statistics;

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
*       _Receive
*
*  Function description
*    Sends a command to server and receives data from server.
*/
static int _Receive(long TCPSockID, unsigned Mtu) {
  U8  acBuffer[20];
  U8  Flag;
  int NumBytesAtOnce;
  U32 ReceiveCnt;
  U32 ReceiveCntOverhead;  // The real number of bytes received including headers (overhead is roughly calculated).
  int r;
  int TimeStart;
  int TimeEnd;

  //
  // Send command and receive data
  //
  acBuffer[0] = 'S';                                                // [0:0]: Command
  IP_StoreU32LE(&acBuffer[1], NUMBER_OF_BYTES);                     // [1:4]: Number of bytes
  IP_StoreU32LE(&acBuffer[5], Mtu);                                 // [5:8]: MTU
  r = send(TCPSockID, (const char*)&acBuffer[0], 9, MSG_DONTWAIT);  // Send command
  if (r == SOCKET_ERROR) {
    return SOCKET_ERROR;
  }
  ReceiveCnt         = 0;
  ReceiveCntOverhead = 0;
  TimeStart          = OS_GetTime();
  do {
    NumBytesAtOnce = recv(TCPSockID, _aRxTxBuffer, sizeof(_aRxTxBuffer), 0);
    if (NumBytesAtOnce <= 0) {
      return SOCKET_ERROR;
    } else {
      ReceiveCnt         += NumBytesAtOnce;
      ReceiveCntOverhead += (SPEEDCLIENT_NUM_CHUNKS * 54) + NumBytesAtOnce;
    }
  } while (ReceiveCnt < NUMBER_OF_BYTES);
  TimeEnd  = OS_GetTime();
  Flag     = 'X';            // Confirmation
  r        = send(TCPSockID, (const char*)&Flag, 1, 0);
  if (r == SOCKET_ERROR) {
    return SOCKET_ERROR;
  }
  //
  // Output performance values
  //
  IP_Logf_Application("%lu Bytes received (without headers) in %d ms.", ReceiveCnt, (TimeEnd - TimeStart));
  IP_Logf_Application("%lu Bytes received (with headers) in %d ms.", ReceiveCntOverhead, (TimeEnd - TimeStart));
  IP_Logf_Application("Average transfer speed (without headers): %lu Bytes/s", (ReceiveCnt / (TimeEnd - TimeStart) * 1000));
  IP_Logf_Application("Average transfer speed (with headers): %lu Bytes/s\n", (ReceiveCntOverhead / (TimeEnd - TimeStart) * 1000));
  BSP_ToggleLED(1);
  return 0;
}

/*********************************************************************
*
*       _Send
*
*  Function description
*    Sends a command to server and sends data to server.
*/
static int _Send(long TCPSockID, unsigned Mtu) {
  U8  acBuffer[20];
  int NumBytesAtOnce;
  U32 SendCnt;
  U32 SendCntOverhead;  // The real number of bytes sent including headers (overhead is roughly calculated).
  U8  Flag;
  int r;
  int TimeStart;
  int TimeEnd;
  int SizeToSend;

  //
  // Send command
  //
  acBuffer[0] = 'R';                                                 // [0:0]: Command
  IP_StoreU32LE(&acBuffer[1], NUMBER_OF_BYTES);                      // [1:4]: Number of bytes
  IP_StoreU32LE(&acBuffer[5], Mtu);                                  // [5:8]: MTU
  r = send(TCPSockID, (const char*) &acBuffer[0], 9, MSG_DONTWAIT);  // Send command
  if (r == SOCKET_ERROR) {
    return SOCKET_ERROR;
  }
  //
  // Send data
  //
  SendCnt         = 0;
  SendCntOverhead = 0;
  TimeStart       = OS_GetTime();
  do {
    if ((NUMBER_OF_BYTES - SendCnt) < Mtu) {
      SizeToSend = NUMBER_OF_BYTES - SendCnt;
    } else {
      SizeToSend = Mtu;
    }
    NumBytesAtOnce = send(TCPSockID, (const char*)&_aRxTxBuffer[0], SizeToSend, 0);
    if (NumBytesAtOnce == SOCKET_ERROR) {
      return NumBytesAtOnce;
    } else {
      SendCnt         += NumBytesAtOnce;
      SendCntOverhead += (SPEEDCLIENT_NUM_CHUNKS * 54) + NumBytesAtOnce;
    }
  } while (SendCnt < NUMBER_OF_BYTES);
  TimeEnd = OS_GetTime();
  Flag    = 0;
  //
  // Wait for response to make sure data has been sent completly
  //
  r = recv(TCPSockID, (char*)&Flag, 1, 0);
  if (r <= 0) {
    return SOCKET_ERROR;
  }
  //
  // Output performance values
  //
  IP_Logf_Application("%lu Bytes sent (without headers) in %d ms.", SendCnt, (TimeEnd - TimeStart));
  IP_Logf_Application("%lu Bytes sent (with headers) in %d ms.", SendCntOverhead, (TimeEnd - TimeStart));
  IP_Logf_Application("Average transfer speed (without headers): %lu Bytes/s", ((SendCnt / (TimeEnd - TimeStart)) * 1000));
  IP_Logf_Application("Average transfer speed (with headers): %lu Bytes/s\n", ((SendCntOverhead / (TimeEnd - TimeStart)) * 1000));
  BSP_ToggleLED(1);
  return 0;
}

/*********************************************************************
*
*       _Client
*/
static void _Client(void) {
  long               TCPSockID;
  struct sockaddr_in ServerAddr;
  int                ConnectStatus;
  int                r;
  int                Opt;
  int                Mtu;

  //
  // Wait until link is up and network interface is configured.
  //
  while (IP_IFaceIsReadyEx(_IFaceId) == 0) {
    OS_Delay(50);
  }
  Mtu = IP_TCP_GetMTU(_IFaceId) - 40;             // MTU - TCP/IP header
  while(1) {
    TCPSockID = socket(AF_INET, SOCK_STREAM, 0);  // Open socket
    if (TCPSockID == 0) {                         // Error, Could not get socket
      while (1) {
        BSP_ToggleLED(0);
        OS_Delay(20);
      }
    } else {
      //
      // Set keep alive option
      //
      Opt = 1;
      setsockopt(TCPSockID, SOL_SOCKET, SO_KEEPALIVE, &Opt, sizeof(Opt));
      //
      // Connect to server
      //
      BSP_SetLED(0);
      ServerAddr.sin_family      = AF_INET;
      ServerAddr.sin_port        = htons(SERVER_PORT);
      ServerAddr.sin_addr.s_addr = htonl(SERVER_IP_ADDR);
      ConnectStatus              = connect(TCPSockID, (struct sockaddr *)&ServerAddr, sizeof(struct sockaddr_in));
      if (ConnectStatus != SOCKET_ERROR) {
        while(1) {
          if (DIRECTION & 1) {
            r = _Receive(TCPSockID, Mtu);
            if (r == -1) {
              break;
            }
            _Statistics.RxCnt++;
          }
          if (DIRECTION & 2) {
            r = _Send(TCPSockID, Mtu);
            if (r == -1) {
              break;
            }
            _Statistics.TxCnt++;
          }
          OS_Delay(50);
        }
      }
    }
    _Statistics.ErrCnt++;
    closesocket(TCPSockID);
    OS_Delay(1000);
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
  _IFaceId = IP_INFO_GetNumInterfaces() - 1;                              // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
  OS_SetPriority(OS_GetTaskID(), 150);                                    // For now, this task has highest prio except IP management tasks.
  OS_CREATETASK(&_TCBIP    , "IP_Task"  , IP_Task  , 150, _StackIP);      // Start the IP_Task.
#if USE_RX_TASK
  OS_CREATETASK(&_TCBIPRx  , "IP_RxTask", IP_RxTask, 250, _StackIPRx);    // Start the IP_RxTask, optional.
#endif
  OS_CREATETASK(&_TCBClient, "Client"   , _Client  , 100, _StackClient);  // Start the speed client.
  IP_AddStateChangeHook(&_StateChangeHook, _OnStateChange);               // Register hook to be notified on disconnects.
  IP_Connect(_IFaceId);                                                   // Connect the interface if necessary.
  OS_SetPriority(OS_GetTaskID(), 255);                                    // Now this task has highest prio for real-time application. This is only allowed when this task does not use blocking IP API after this point.
  while (1) {
    OS_Delay(200);
  }
}

/****** End Of File *************************************************/
