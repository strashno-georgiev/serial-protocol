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

File    : IP_WEBSOCKET_printf_Server.c
Purpose : Simple WebSocket terminal that can receive text messages
          to printf them to the standard output.
          The WebSocket protocol returned in the response is "debug".
          The protocols suggested by the client are not parsed by this
          sample for simplicity but the protocol returned can be
          configured by the define WEBSOCKET_PROTO .
*/

#include "RTOS.h"
#include "BSP.h"
#include "IP.h"
#include "IP_WEBSOCKET.h"

#include <stdio.h>
#include <string.h>

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#define USE_RX_TASK      0                                // 0: Packets are read in ISR, 1: Packets are read in a task of its own.

//
// WebSocket sample configuration.
//
#define SERVER_PORT      8181
#define WEBSOCKET_PROTO  "debug"  // WebSocket subprotocol sent back.

//
// Task priorities.
//
enum {
   TASK_PRIO_IP_WEBSOCKET = 150
  ,TASK_PRIO_IP_TASK         // Priority should be higher than all IP application tasks.
#if USE_RX_TASK
  ,TASK_PRIO_IP_RX_TASK      // Must be the highest priority of all IP related tasks.
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
*       Defines, fixed
*
**********************************************************************
*/

#define OPEN_RESPONSE  "HTTP/1.1 101 Switching Protocols\r\n" \
                       "Upgrade: websocket\r\n"               \
                       "Connection: Upgrade\r\n"              \
                       "Sec-WebSocket-Protocol: %s\r\n"       \
                       "Sec-WebSocket-Accept: %s\r\n"         \
                       "\r\n"

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

static int _cbWebSocket_Recv(IP_WEBSOCKET_CONTEXT* pContext, IP_WEBSOCKET_CONNECTION* pConnection,       void* pData, unsigned NumBytes);
static int _cbWebSocket_Send(IP_WEBSOCKET_CONTEXT* pContext, IP_WEBSOCKET_CONNECTION* pConnection, const void* pData, unsigned NumBytes);

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static int _IFaceId;

//
// Task stacks and Task-Control-Blocks.
//
static OS_STACKPTR int _IPStack[TASK_STACK_SIZE_IP_TASK/sizeof(int)];                 // Stack of the IP_Task.
static OS_TASK         _IPTCB;                                                        // Task-Control-Block of the IP_Task.

#if USE_RX_TASK
static OS_STACKPTR int _IPRxStack[TASK_STACK_SIZE_IP_RX_TASK/sizeof(int)];            // Stack of the IP_RxTask.
static OS_TASK         _IPRxTCB;                                                      // Task-Control-Block of the IP_RxTask.
#endif

//
// WebSocket
//
static char _acBuffer[1024];

static const IP_WEBSOCKET_TRANSPORT_API _WebSocketTransportAPI = {
  _cbWebSocket_Recv,  // pfReceive
  _cbWebSocket_Send,  // pfSend
  NULL                // pfGenMaskKey, client only.
};

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _cbWebSocket_Send()
*
*  Function description
*    WebSocket callback that for sending data using the underlying
*    network communication API (typically BSD socket API).
*
*  Parameters
*    pContext   : WebSocket context.
*    pConnection: Network connection handle.
*    pData      : Data to send.
*    NumBytes   : Amount of data to send.
*
*  Return value
*    Amount of data sent: >  0
*    Connection closed  : == 0
*    Error              : <  0
*/
static int _cbWebSocket_Send(IP_WEBSOCKET_CONTEXT* pContext, IP_WEBSOCKET_CONNECTION* pConnection, const void* pData, unsigned NumBytes) {
  IP_WEBSOCKET_USE_PARA(pContext);

  return send((long)pConnection, (const char*)pData, NumBytes, 0);
}

/*********************************************************************
*
*       _cbWebSocket_Recv()
*
*  Function description
*    WebSocket callback that for sending data using the underlying
*    network communication API (typically BSD socket API).
*
*  Parameters
*    pContext   : WebSocket context.
*    pConnection: Network connection handle.
*    pData      : Where to store the received data.
*    NumBytes   : Maximum amount of data to receive.
*
*  Return value
*    Amount of data received: >  0
*    Connection closed      : == 0
*    Error                  : <  0
*/
static int _cbWebSocket_Recv(IP_WEBSOCKET_CONTEXT* pContext, IP_WEBSOCKET_CONNECTION* pConnection, void* pData, unsigned NumBytes) {
  IP_WEBSOCKET_USE_PARA(pContext);

  return recv((long)pConnection, (char*)pData, NumBytes, 0);
}

/*********************************************************************
*
*       _Panic()
*
*  Function description
*    Error/panic collection routine.
*
*  Additional information
*    This is an error routine that collects all cases that are dead
*    ends in the program flow. With blocking sockets we should never
*    end in here as the sample will handle the return values that are
*    returned by a blocking socket I/O.
*
*    When using this sample with non-blocking socket I/O and ending up
*    in here, this is an indicator that you are not handling all
*    non-blocking return values correctly. Typically the case missing
*    is to check for a WOULDBLOCK socket error when an API has
*    returned with SOCKET_ERROR (-1).
*/
static void _Panic(void) {
  printf("Error, program halted!\n");
  for (;;);
}

/*********************************************************************
*
*       _WebSocketTask()
*/
static void _WebSocketTask(void) {
  char*                sKey;
  char*                s;
  IP_WEBSOCKET_CONTEXT WebSocketContext;
  struct sockaddr      Addr;
  struct sockaddr_in   InAddr;
  long hSockListen;
  long hSock;
  int  AddrLen;
  int  r;
  int  NumBytesRead;
  U8   MessageType;
  char ac[64];

  //
  // Get a socket into listening state.
  //
  hSockListen = socket(AF_INET, SOCK_STREAM, 0);
  if (hSockListen == SOCKET_ERROR) {
    while(1); // This should never happen!
  }
  memset(&InAddr, 0, sizeof(InAddr));
  InAddr.sin_family      = AF_INET;
  InAddr.sin_port        = htons(SERVER_PORT);
  InAddr.sin_addr.s_addr = INADDR_ANY;
  bind(hSockListen, (struct sockaddr *)&InAddr, sizeof(InAddr));
  listen(hSockListen, 1);
  do {
    //
    // Wait for an incoming connection
    //
    hSock = 0;
    AddrLen = sizeof(Addr);
    if ((hSock = accept(hSockListen, &Addr, &AddrLen)) == SOCKET_ERROR) {
      continue;      // Error.
    }
    //
    // Read HTTP header.
    //
    r = recv(hSock, _acBuffer, sizeof(_acBuffer), 0);
    if (r == sizeof(_acBuffer)) {
      printf("_acBuffer is too small to read the complete HTTP header.\n");
      goto OnError;  // Error.
    }
    if (r <= 0) {
      goto OnError;  // Error.
    }
    _acBuffer[r] = '\0';  // Treat the HTTP header in buffer as one big string.
    //
    // Find Sec-WebSocket-Key field value in HTTP header.
    //
    sKey = strstr(&_acBuffer[0], "Sec-WebSocket-Key: ");
    if (sKey == NULL) {
      printf("Sec-WebSocket-Key field not found.\n");
      goto OnError;  // Error.
    }
    sKey += sizeof("Sec-WebSocket-Key: ") - 1;
    s = strstr(sKey, "\r\n");
    if (s == NULL) {
      printf("End of Sec-WebSocket-Key field not found.\n");
      goto OnError;  // Error.
    }
    //
    // Generate the accept response key.
    //
    r = IP_WEBSOCKET_GenerateAcceptKey(sKey, (unsigned)(s - sKey), &ac[0], sizeof(ac) - 1);
    if (r == 0) {
      printf("Buffer for accept key is not big enough.\n");
      goto OnError;  // Error.
    }
    ac[r] = '\0';
    //
    // Create the accept message and send it to the client.
    //
    r = SEGGER_snprintf(_acBuffer, sizeof(_acBuffer), OPEN_RESPONSE, WEBSOCKET_PROTO, ac);
    if (r == sizeof(_acBuffer)) {
      printf("_acBuffer is too small for the response.\n");
      goto OnError;  // Error.
    }
    send(hSock, (const char*)_acBuffer, strlen(_acBuffer), 0);
    printf("WebSocket client connected.\n");
    //
    // Initialize the WebSocket context for the server.
    //
    IP_WEBSOCKET_InitServer(&WebSocketContext, &_WebSocketTransportAPI, (IP_WEBSOCKET_CONNECTION*)hSock);
    //
    // Process WebSocket messages.
    //
    while (1) {
      //
      // Wait/read the next message type.
      //
      do {
        r = IP_WEBSOCKET_WaitForNextMessage(&WebSocketContext, &MessageType);
        if (r == 0) {  // Connection closed.
          goto OnDisconnect;
        }
        if ((r < 0) && (r != IP_WEBSOCKET_ERR_AGAIN)) {
          _Panic();
        }
      } while (r == IP_WEBSOCKET_ERR_AGAIN);
      //
      // Evaluate the message type.
      //
      if (MessageType == IP_WEBSOCKET_FRAME_TYPE_CLOSE) {
        //
        // Send a close frame with a goodbye message.
        //
        r = SEGGER_snprintf(ac, sizeof(ac), "Bye, bye");
        if (r == sizeof(ac)) {
          printf("ac buffer is too small for close payload.\n");
          goto OnError;  // Error.
        }
        do {
          //
          // In this special case do not check if this is an error
          // other than AGAIN as WebSockets are explicitly allowed
          // to be closed on socket level. The next call might return
          // with a real error as the other side has already closed
          // the socket, this is fine for this single case. Simply
          // close the socket in this case.
          //
          r = IP_WEBSOCKET_Close(&WebSocketContext, &ac[0], IP_WEBSOCKET_CLOSE_CODE_NORMAL_CLOSURE);
        } while (r == IP_WEBSOCKET_ERR_AGAIN);
        break;
      } else {
        if (MessageType == IP_WEBSOCKET_FRAME_TYPE_TEXT) {
          //
          // Receive a text message and printf it.
          //
          NumBytesRead = 0;
          s            = &ac[0];
          for (;;) {
            r = IP_WEBSOCKET_Recv(&WebSocketContext, (void*)s, (sizeof(ac) - 1 - NumBytesRead));
            if        (r > 0) {
              s            += r;
              NumBytesRead += r;
            } else if (r == IP_WEBSOCKET_ERR_ALL_DATA_READ) {
              *s = '\0';
              printf("Client: %s\n", ac);
              break;
            } else if (r == 0) {
              goto OnDisconnect;
            } else {  // r < 0 ?
              if (r != IP_WEBSOCKET_ERR_AGAIN) {
                _Panic();  // Error, any other unhandled return value that should not occur.
              }
            }
          }
        } else {
          //
          // Discard other (binary) messages.
          //
          do {
            r = IP_WEBSOCKET_DiscardMessage(&WebSocketContext);
            if (r == 0) {
              goto OnDisconnect;
            }
            if ((r < 0) && (r != IP_WEBSOCKET_ERR_AGAIN)) {
              _Panic();
            }
          } while (r == IP_WEBSOCKET_ERR_AGAIN);
        }
      }
    }
OnDisconnect:
    printf("WebSocket client disconnected.\n");
OnError:
    closesocket(hSock);
  }  while(1);
}

/*********************************************************************
*
*       Global functions
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
  //
  // Start TCP/IP task
  //
  OS_CREATETASK(&_IPTCB,   "IP_Task",   IP_Task,   TASK_PRIO_IP_TASK,    _IPStack);
#if USE_RX_TASK
  OS_CREATETASK(&_IPRxTCB, "IP_RxTask", IP_RxTask, TASK_PRIO_IP_RX_TASK, _IPRxStack);  // Start the IP_RxTask, optional.
#endif
  IP_Connect(_IFaceId);                                                                // Connect the interface if necessary.
  //
  // IPv4 address configured ?
  //
  while (IP_IFaceIsReadyEx(_IFaceId) == 0) {
    BSP_ToggleLED(0);
    OS_Delay(200);
  }
  BSP_ClrLED(0);
  OS_SetPriority(OS_GetTaskID(), TASK_PRIO_IP_WEBSOCKET);
  OS_SetTaskName(OS_GetTaskID(), "WebSocketServer");
  _WebSocketTask();
}

/*************************** End of file ****************************/
