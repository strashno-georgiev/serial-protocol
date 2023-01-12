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

File    : IP_SimpleServer.c
Purpose : Sample program for embOS & emNet
          Demonstrates a simple Telnet server on a port
          (by default: 23) that outputs
          the current system time for each character received.
          To connect to the target, use the command line:
          > telnet <target-ip>
          Where <target-ip> represents the IP address of the target,
          which depends on the configuration used in IP_X_Config() .
          The recv() function waits for a given timeout for further
          characters to receive before it disconnects the client due
          to idle timeout.

          The following is a sample of the output to the terminal window:

          0:000 MainTask - INIT: Init started. Version 2.13.11
          0:002 MainTask - DRIVER: Found PHY with Id 0x181 at addr 0x1F
          0:003 MainTask - INIT: Link is down
          0:003 MainTask - INIT: Init completed
          0:003 IP_Task - INIT: IP_Task started
          3:000 IP_Task - LINK: Link state changed: Full duplex, 100 MHz
          4:000 IP_Task - DHCPc: Sending discover!
          4:516 IP_Task - DHCPc: IFace 0: Offer: IP: 192.168.11.64, Mask: 255.255.0.0, GW: 192.168.11.1.
          5:000 IP_Task - DHCPc: IP addr. checked, no conflicts
          5:000 IP_Task - DHCPc: Sending Request.
          5:001 IP_Task - DHCPc: IFace 0: Using IP: 192.168.11.64, Mask: 255.255.0.0, GW: 192.168.11.1.
          10:442 Telnet - New IPv4 client accepted.
          19:034 Telnet - recv() timeout after 5 seconds of inactivity!
          19:034 Telnet - Disconnecting client.
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

/*********************************************************************
*
*       Configuration
*
**********************************************************************
*/

#define USE_RX_TASK  0  // 0: Packets are read in ISR, 1: Packets are read in a task of its own.

//
// Telnet server sample.
//
#define TIMEOUT      5000  // Timeout for recv() [ms].
#define SERVER_PORT  23

//
// Task priorities.
//
enum {
   TASK_PRIO_IP_TELNET = 150
  ,TASK_PRIO_IP_TASK          // Priority should be higher than all IP application tasks.
#if USE_RX_TASK
  ,TASK_PRIO_IP_RX_TASK       // Must be the highest priority of all IP related tasks.
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

static char _acOut[512];

//
// Task stacks and Task-Control-Blocks.
//
static OS_STACKPTR int _IPStack[TASK_STACK_SIZE_IP_TASK/sizeof(int)];               // Stack of the IP_Task.
static OS_TASK         _IPTCB;                                                      // Task-Control-Block of the IP_Task.

#if USE_RX_TASK
static OS_STACKPTR int _IPRxStack[TASK_STACK_SIZE_IP_RX_TASK/sizeof(int)];          // Stack of the IP_RxTask.
static OS_TASK         _IPRxTCB;                                                    // Task-Control-Block of the IP_RxTask.
#endif

static OS_STACKPTR int _TelnetStack[(1024 + APP_TASK_STACK_OVERHEAD)/sizeof(int)];  // Stack of the Telnet server.
static OS_TASK         _TelnetTCB;                                                  // Task-Control-Block of the Telnet server.

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
*       _CreateSocket()
*
* Function description
*   Creates a socket for the requested protocol family.
*
* Parameter
*   IPProtVer - Protocol family which should be used (PF_INET or PF_INET6).
*
* Return value
*   O.K. : Socket handle.
*   Error: SOCKET_ERROR .
*/
static int _CreateSocket(unsigned IPProtVer) {
  int hSock;

  hSock = SOCKET_ERROR;
  //
  // Create IPv4 socket
  //
  if (IPProtVer == AF_INET) {
    hSock = socket(AF_INET, SOCK_STREAM, 0);
  }
  return hSock;
}

/*********************************************************************
*
*       _BindAtTcpPort()
*
* Function description
*   Binds a socket to a port.
*
* Parameter
*   IPProtVer - Protocol family which should be used (AF_INET).
*   hSock     - Socket handle
*   Port      - Port which should be to wait for connections.
*
* Return value
*   O.K. : == 0
*   Error: != 0
*/
static int _BindAtTcpPort(unsigned IPProtVer, int hSock, U16 LPort) {
  int r;

  r = -1;
  //
  // Bind it to the port
  //
  if (IPProtVer == AF_INET) {
    struct sockaddr_in Addr;

    IP_MEMSET(&Addr, 0, sizeof(Addr));
    Addr.sin_family      = AF_INET;
    Addr.sin_port        = htons(LPort);
    Addr.sin_addr.s_addr = INADDR_ANY;
    r = bind(hSock, (struct sockaddr*)&Addr, sizeof(Addr));
  }
  return r;
}

/*********************************************************************
*
*       _ListenAtTcpPort()
*
* Function description
*   Creates a socket, binds it to a port and sets the socket into
*   listening state.
*
* Parameter
*   IPProtVer - Protocol family which should be used (PF_INET or PF_INET6).
*   Port      - Port which should be to wait for connections.
*
* Return value
*   O.K. : Socket handle.
*   Error: SOCKET_ERROR .
*/
static int _ListenAtTcpPort(unsigned IPProtVer, U16 Port) {
  int hSock;
  int r;

  //
  // Create socket
  //
  hSock = _CreateSocket(IPProtVer);
  if (hSock != SOCKET_ERROR) {
    //
    // Bind it to the port
    //
    r = _BindAtTcpPort(IPProtVer, hSock, Port);
    //
    // Start listening on the socket.
    //
    if (r != 0) {
      hSock = SOCKET_ERROR;
    } else {
      r = listen(hSock, 1);
      if (r != 0) {
        hSock = SOCKET_ERROR;
      }
    }
  }
  return hSock;
}

/*********************************************************************
*
*       _Process()
*
* Function description
*   This is the actual (very simple) Telnet server. This function
*   processes one client connection.
*
* Parameters
*   hSock: Client socket to serve.
*/
static void _Process(int hSock) {
  const char *sHello = "Hello ... Press any key.\n\r";
        U32   Timeout;
        int   Error;
        int   r;
        char  Dummy;  // Used to receive one character, key press.

  send(hSock, (char*)sHello, strlen(sHello), 0);
  //
  // recv() by default is a blocking function. this means it blocks
  // until it receives data. We set a timeout to guarantee that the
  // function returns after the given delay.
  //
  Timeout = TIMEOUT;
  setsockopt(hSock, SOL_SOCKET, SO_RCVTIMEO, &Timeout, sizeof(Timeout));
  do {
    r = recv(hSock, &Dummy, 1, 0);
    if (r <= 0) {
      getsockopt(hSock, SOL_SOCKET, SO_ERROR, &Error, sizeof(Error));
      if (Error == IP_ERR_TIMEDOUT) {
        IP_Logf_Application("recv() timeout after %lu seconds of inactivity!", (TIMEOUT / 1000));
        IP_Logf_Application("Disconnecting client.");
      }
      return;  // Error, receive timeout => disconnect client.
    }
    SEGGER_snprintf(_acOut, sizeof(_acOut), "OS_Time = %lu\n\r", OS_Time);
    send(hSock, (char*)_acOut, strlen(_acOut), 0);
  } while (1);
}

/*********************************************************************
*
*       _TelnetTask()
*
* Function description
*   Creates a parent socket and handles clients that connect to the
*   server. This sample can handle one client at the same time. Each
*   client that connects creates a child socket that is then passed
*   to the process routine to detach client handling from the server
*   functionality.
*/
static void _TelnetTask(void) {
  int hSockParent4;
  int hSockChild;

  //
  // Try until we get a valid IPv4 parent socket.
  //
  while (1) {
    hSockParent4 = _ListenAtTcpPort(PF_INET, SERVER_PORT);
    if (hSockParent4 == SOCKET_ERROR) {
      OS_Delay(2000);
      continue;  // Error, try again.
    }
    break;
  }
  //
  // Wait for a connection and process the data request after accepting the connection.
  //
  while (1) {
    //
    // Check for a new IPv4 connection.
    //
    hSockChild = accept(hSockParent4, NULL, NULL);
    if (hSockChild == SOCKET_ERROR) {
      continue;               // Error, try again.
    }
    IP_Logf_Application("New IPv4 client accepted.");
    _Process(hSockChild);     // Process the client.
    closesocket(hSockChild);  // Close connection to client from our side (too).
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
  IP_AddLogFilter(IP_MTYPE_APPLICATION);
  _IFaceId = IP_INFO_GetNumInterfaces() - 1;                                                 // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
  OS_SetPriority(OS_GetTaskID(), TASK_PRIO_IP_TASK);                                         // For now, this task has highest prio except IP management tasks.
  OS_CREATETASK(&_IPTCB    , "IP_Task"  , IP_Task    , TASK_PRIO_IP_TASK   , _IPStack);      // Start the IP_Task.
#if USE_RX_TASK
  OS_CREATETASK(&_IPRxTCB  , "IP_RxTask", IP_RxTask  , TASK_PRIO_IP_RX_TASK, _IPRxStack);    // Start the IP_RxTask, optional.
#endif
  OS_CREATETASK(&_TelnetTCB, "Telnet"   , _TelnetTask, TASK_PRIO_IP_TELNET , _TelnetStack);  // Start the Telnet server.
  IP_AddStateChangeHook(&_StateChangeHook, _OnStateChange);                                  // Register hook to be notified on disconnects.
  IP_Connect(_IFaceId);                                                                      // Connect the interface if necessary.
  OS_SetPriority(OS_GetTaskID(), 255);                                                       // Now this task has highest prio for real-time application. This is only allowed when this task does not use blocking IP API after this point.
  while (1) {
    BSP_ToggleLED(1);
    OS_Delay(200);
  }
}

/****** End Of File *************************************************/
