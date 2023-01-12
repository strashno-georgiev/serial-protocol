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

File    : IP_MQTT_CLIENT_Publisher.c
Purpose : Sample program for embOS & emNet demonstrating an MQTT publisher.
*/

#include "RTOS.h"
#include "BSP.h"
#include "IP.h"
#include "IP_MQTT_CLIENT.h"
#include "SEGGER.h"

/*********************************************************************
*
*       Configuration
*
**********************************************************************
*/

#ifndef USE_MQTT_5
  //
  // This sample is able to communicate using MQTT 5 and MQTT 3.1.1.
  // The default is to use MQTT 5, if you want to use MQTT 3.1.1 set the following define to 0.
  // IP_MQTT_CLIENT_SUPPORT_V5 must be enabled to use MQTT 5.
  //
  #if IP_MQTT_CLIENT_SUPPORT_V5
    #define USE_MQTT_5 1
  #else
    #define USE_MQTT_5 0
  #endif
#endif

#define USE_RX_TASK              0  // 0: Packets are read in ISR, 1: Packets are read in a task of its own.

#define MQTT_CLIENT_BUFFER_SIZE  256
#define RCV_TIMEOUT              5000

#define MQTT_BROKER_ADDR         "test.mosquitto.org" // Alternate test broker: broker.emqx.io
#define MQTT_BROKER_PORT         1883

#define MQTT_CLIENT_NAME         "eMQTT_Pub"
#define TOPIC_FILTER_TO_PUBLISH  "eMQTT"
#define PAYLOAD                  "www.SEGGER.com MQTT_HelloWorld_Sample"
//
// Set keep-alive timeout to 60 seconds.
// For "test.mosquitto.org" this must not be 0, otherwise the server will refuse the connection.
//
#define PING_TIMEOUT               60 // Configure MQTT ping to x seconds.

//
// Task priorities.
//
enum {
   TASK_PRIO_IP_TASK = 150  // Priority should be higher than all IP application tasks.
#if USE_RX_TASK
  ,TASK_PRIO_IP_RX_TASK     // Must be the highest priority of all IP related tasks.
#endif
};

//
// Functions in this sample which are not part of the MQTT add-on.
// If you are not using emNet you can change these macros.
//
#ifndef GET_TIME_32
  #define GET_TIME_32 IP_OS_GetTime32()
#endif

#ifndef CHECK_TIME_EXPIRED
  #define CHECK_TIME_EXPIRED(x) IP_IsExpired(x)
#endif

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static IP_HOOK_ON_STATE_CHANGE _StateChangeHook;
static int                     _IFaceId;

//
// Task stacks and Task-Control-Blocks.
//
static OS_STACKPTR int _IPStack[TASK_STACK_SIZE_IP_TASK/sizeof(int)];       // Stack of the IP_Task.
static OS_TASK         _IPTCB;                                              // Task-Control-Block of the IP_Task.

#if USE_RX_TASK
static OS_STACKPTR int _IPRxStack[TASK_STACK_SIZE_IP_RX_TASK/sizeof(int)];  // Stack of the IP_RxTask.
static OS_TASK         _IPRxTCB;                                            // Task-Control-Block of the IP_RxTask.
#endif

static IP_MQTT_CLIENT_CONTEXT _MQTTClient;

static char            _acBuffer[MQTT_CLIENT_BUFFER_SIZE];                  // Memory block used by the MQTT client.

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
*  Function description
*    Callback that will be notified once the state of an interface
*    changes.
*
*  Parameters
*    IFaceId   : Zero-based interface index.
*    AdminState: Is this interface enabled ?
*    HWState   : Is this interface physically ready ?
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
*       _IsIPAddress()
*
*  Function description
*    Checks if string is a dot-decimal IP address, for example 192.168.1.1
*/
static unsigned _IsIPAddress(const char* sIPAddr) {
  unsigned NumDots;
  unsigned i;
  char c;

  NumDots = 0;
  i       = 0;
  while (1) {
    c = *(sIPAddr + i);
    if ((c >= '0') && (c <= '9')) {
      goto Loop;
    }
    if (c == '.') {
      NumDots++;
      goto Loop;
    }
    if (c == '\0') {
      if ((NumDots < 3) || (i > 15)) { // Error, every dot-decimal IP address includes 3 '.' and is not longer than 15 characters.
Error:
        return 0;
      }
      return 1;
    } else {
      goto Error;
    }
Loop:
    i++;
  }
}

/*********************************************************************
*
*       _ParseIPAddr()
*
*  Function description
*    Parses a string for a dot-decimal IP address and returns the
*    IP as 32-bit number in host endianness.
*/
static long _ParseIPAddr(const char* sIPAddr) {
  long     IPAddr;
  unsigned Value;
  unsigned NumDots;
  unsigned i;
  unsigned j;
  char     acDigits[4];
  char     c;

  IPAddr = 0;
  //
  // Check if string is a valid IP address.
  //
  Value = _IsIPAddress(sIPAddr);
  if (Value) {
    //
    // Parse the IP address.
    //
    NumDots = 3;
    i       = 0;
    j       = 0;
    while (1) {
      c = *(sIPAddr + i);
      if (c == '\0') {
        //
        // Add the last byte of the IP address.
        //
        acDigits[j] = '\0';
        Value = SEGGER_atoi(acDigits);
        if (Value < 255) {
          IPAddr |= Value;
        }
        return IPAddr; // O.K., string completely parsed. Returning IP address.
      }
      //
      // Parse the first three byte of the IP address.
      //
      if (c != '.') {
        acDigits[j] = c;
        j++;
      } else {
        acDigits[j] = '\0';
        Value = SEGGER_atoi(acDigits);
        if (Value <= 255) {
          IPAddr |= (Value << (NumDots * 8));
          NumDots--;
          j = 0;
        } else {
          return -1;  // Error, illegal number in IP address.
        }
      }
      i++;
    }
  }
  return -1;
}

/*********************************************************************
*
*       _Connect()
*
*  Function description
*    Creates a socket and opens a TCP connection to the MQTT broker.
*/
static IP_MQTT_CLIENT_SOCKET _Connect(const char* sBrokerAddr, unsigned BrokerPort) {
  struct hostent*    pHostEntry;
  struct sockaddr_in sin;
  long               Ip;
  long               hSock;
  int                SoError;
  int                r;
  U32                Timeout;

  if (_IsIPAddress(sBrokerAddr)) {
    Ip = _ParseIPAddr(sBrokerAddr);
    Ip = htonl(Ip);
  } else {
    //
    // Convert host into IP address.
    //
    pHostEntry = gethostbyname((char*)sBrokerAddr);
    if (pHostEntry == NULL) {
      IP_MQTT_CLIENT_APP_LOG(("APP: gethostbyname failed: %s", sBrokerAddr));
      return NULL;
    }
    Ip = *(unsigned*)(*pHostEntry->h_addr_list);
  }
  //
  // Create socket and connect to the MQTT broker.
  //
  hSock = socket(AF_INET, SOCK_STREAM, 0);
  if(hSock  == -1) {
    IP_MQTT_CLIENT_APP_LOG(("APP: Could not create socket!"));
    return NULL;
  }
  //
  // Set receive timeout.
  //
  Timeout = RCV_TIMEOUT;
  setsockopt(hSock , SOL_SOCKET, SO_RCVTIMEO, (char*)&Timeout, sizeof(Timeout));
  //
  // Connect.
  //
  memset(&sin, 0, sizeof(sin));
  sin.sin_family      = AF_INET;
  sin.sin_port        = htons((U16)BrokerPort);
  sin.sin_addr.s_addr = Ip;
  r = connect(hSock, (struct sockaddr*)&sin, sizeof(sin));
  if(r == SOCKET_ERROR) {
    getsockopt(hSock, SOL_SOCKET, SO_ERROR, &SoError, sizeof(SoError));
    closesocket(hSock);
    IP_MQTT_CLIENT_APP_LOG(("APP: Connect error: %s", IP_Err2Str(SoError)));
    return NULL;
  }
  IP_MQTT_CLIENT_APP_LOG(("APP: Connected to %i, port %d.", Ip, BrokerPort));
  return (IP_MQTT_CLIENT_SOCKET)hSock;
}

/*********************************************************************
*
*       _Disconnect()
*
*  Function description
*    Closes a socket.
*/
static void _Disconnect(void* pSocket) {
  if (pSocket) {
    closesocket((long)pSocket);
  }
}

/*********************************************************************
*
*       _Recv()
*
*  Function description
*    Receives data via socket interface.
*
*  Return value
*    >   0: O.K., number of bytes received.
*    ==  0: Connection has been gracefully closed by the broker.
*    == -1: Error.
*/
static int _Recv(void* pSocket, char* pBuffer, int NumBytes) {
  return recv((long)pSocket, pBuffer, NumBytes, 0);
}

/*********************************************************************
*
*       _Send()
*
*  Function description
*    Sends data via socket interface.
*/
static int _Send(void* pSocket, const char* pBuffer, int NumBytes) {
  return send((long)pSocket, pBuffer, NumBytes, 0);
}

static const IP_MQTT_CLIENT_TRANSPORT_API _IP_Api = {
  _Connect,
  _Disconnect,
  _Recv,
  _Send
};

/*********************************************************************
*
*       _GenRandom()
*
*  Function description
*    Generates a random number.
*/
static U16 _GenRandom(void) {
  U32 TimeStamp;

  TimeStamp = OS_GetTime32();
  return (U16)TimeStamp;  // Return a random value, which can be used for packet Id, ...
}

static const IP_MQTT_CLIENT_APP_API _APP_Api = {
  _GenRandom,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

/*********************************************************************
*
*       _MQTT_Client()
*
*  Function description
*    MQTT client application.
*/
static void _MQTT_Client(void) {
  IP_MQTT_CLIENT_MESSAGE Publish;
  IP_MQTT_CONNECT_PARAM  ConnectPara;
  U32                    TimeExpirePing;
  U8                     ReasonCode;
  int                    Cnt;
  int                    r;
  int                    Error;

  //
  // Initialize MQTT client context.
  //
  IP_MQTT_CLIENT_Init(&_MQTTClient, _acBuffer, MQTT_CLIENT_BUFFER_SIZE, &_IP_Api, &_APP_Api, MQTT_CLIENT_NAME);
  //
  // Main application loop.
  //
  do {
    //
    // Configure MQTT keep alive time (for sending PINGREQ messages)
    //
    if (PING_TIMEOUT != 0) {
      r = IP_MQTT_CLIENT_SetKeepAlive(&_MQTTClient, PING_TIMEOUT);
      if (r != 0) {
        IP_Logf_Application("APP: Could not set timeout (currently connected ?)");
      }
    }
    TimeExpirePing = 0;
    //
    // Connect to the MQTT broker.
    //
    memset(&ConnectPara, 0, sizeof(IP_MQTT_CONNECT_PARAM));
    ConnectPara.CleanSession = 1;
    ConnectPara.Port = MQTT_BROKER_PORT;
    ConnectPara.sAddr = MQTT_BROKER_ADDR;
#if USE_MQTT_5
    ConnectPara.Version = 5;
#else
    ConnectPara.Version = 4; // MQTT 3.1.1
#endif
    r = IP_MQTT_CLIENT_ConnectEx(&_MQTTClient, &ConnectPara, &ReasonCode);
    if (r != 0) {
#if USE_MQTT_5
      IP_Logf_Application("APP: MQTT connect error: %d, ReasonCode %s.", r, IP_MQTT_ReasonCode2String(ReasonCode));
#else
      IP_Logf_Application("APP: MQTT connect error: %d", r);
#endif
      goto Disconnect;
    }
    BSP_SetLED(0);
    //
    // Publish message loop.
    //
    Cnt = 0;
    while (1) {
      //
      // Initially set timeouts. This is done after the connect check because
      // the KeepAlive timeout value can change depending on the CONNACK from the server.
      // The server's KeepAlive timeout takes precedence.
      // KeepAlive time is in seconds.
      // As per MQTT 3.1.1 or 5 spec the server allows for one and a half times the
      // KeepAlive time period before disconnecting the client.
      //
      if (TimeExpirePing == 0) {
        TimeExpirePing    = GET_TIME_32 + (_MQTTClient.KeepAlive * 1000);
      }
      //
      // Send KeepAlive (PINGREQ) if configured.
      //
      if (_MQTTClient.KeepAlive != 0 && CHECK_TIME_EXPIRED(TimeExpirePing)) {
        TimeExpirePing = GET_TIME_32 + (_MQTTClient.KeepAlive * 1000); // Set new expire time.
        IP_MQTT_CLIENT_Timer(); // Send PINGREQ.
        IP_Logf_Application("APP: PINGREQ sent.");
      }
      //
      // Initialize the MQTT message.
      //
      memset(&Publish, 0, sizeof(Publish));
      Publish.sTopic  = TOPIC_FILTER_TO_PUBLISH;
      Publish.pData   = PAYLOAD;
      Publish.DataLen = strlen(PAYLOAD);
      Publish.QoS     = IP_MQTT_CLIENT_FLAGS_PUBLISH_QOS2;
      //
      // Publish a message.
      //
      r = IP_MQTT_CLIENT_Publish(&_MQTTClient, &Publish);
      if (r < 0) {
        Error = 0;
        getsockopt((long)_MQTTClient.Connection.Socket, SOL_SOCKET, SO_ERROR, &Error, sizeof(Error));
        if (Error == IP_ERR_TIMEDOUT) {
          //
          // Retry.
          //
          OS_Delay(100);
          continue;
        }
        break;
      }
      Cnt++;
      IP_MQTT_CLIENT_APP_LOG(("APP: --------"));
      IP_MQTT_CLIENT_APP_LOG(("APP: Message No. %d:", Cnt));
      IP_MQTT_CLIENT_APP_LOG(("APP:   Topic  : \"%s\"", Publish.sTopic));
      IP_MQTT_CLIENT_APP_LOG(("APP:   Payload: \"%s\"", Publish.pData));
      OS_Delay(1000);
    }
    //
    // Disconnect.
    //
Disconnect:
    IP_MQTT_CLIENT_Disconnect(&_MQTTClient);
    IP_MQTT_CLIENT_APP_LOG(("APP: Done."));
    BSP_ClrLED(0);
    OS_Delay(10000);
  } while (1);
}

/*********************************************************************
*
*       MainTask()
*
*  Function description
*    Main task executed by the RTOS to create further resources and
*    running the main application.
*/
void MainTask(void) {
  IP_Init();
  _IFaceId = IP_INFO_GetNumInterfaces() - 1;                                           // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
  OS_SetPriority(OS_GetTaskID(), TASK_PRIO_IP_TASK - 1);                               // For now, this task has highest prio except IP management tasks.
  OS_CREATETASK(&_IPTCB  , "IP_Task"  , IP_Task  , TASK_PRIO_IP_TASK   , _IPStack);    // Start the IP_Task.
#if USE_RX_TASK
  OS_CREATETASK(&_IPRxTCB, "IP_RxTask", IP_RxTask, TASK_PRIO_IP_RX_TASK, _IPRxStack);  // Start the IP_RxTask, optional.
#endif
  IP_AddStateChangeHook(&_StateChangeHook, _OnStateChange);                            // Register hook to be notified on disconnects.
  IP_Connect(_IFaceId);                                                                // Connect the interface if necessary.
  while (IP_IFaceIsReadyEx(_IFaceId) == 0) {
    BSP_ToggleLED(1);
    OS_Delay(50);
  }
  BSP_ClrLED(0);
  _MQTT_Client();
}

/*************************** End of file ****************************/
