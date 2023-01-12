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

File    : IP_MQTT_CLIENT_PublisherSubscriber_2Tasks.c
Purpose : Sample program for embOS & emNet demonstrating MQTT.
          This sample application sends and receives messages over a
          single connection to a broker. You can subscribe to one or
          more topics and also publish messages. All QoS levels are
          supported.

          The client connects to the free broker test.mosquitto.org
          and sends a subscribe message for one topic.
          The subscribed topic should be delivered with QoS level 1.
          Furthermore, the client publishes messages (sends messages
          to the broker).

          The application uses two tasks. One to manage initialization
          of the client, connect to the broker and handle the
          reception of the messages and a second task that sends
          periodically a message.

Preparations:
          Works out-of-the-box.

          Change the define USE_MQTT_5 to 0 if you want to test MQTT 3.1.1.

          To keep the setup for this application as simple as
          possible the publisher task sends message for the
          topic which the subscriber task requests from the
          broker.

          In a real-life setup the subscriber will be most likely not
          interested in data that was previously published from itself.
          Therefore, We recommend to subscribe to another topic.

Expected behavior:
          The publisher task sends messages to the broker.
          The subscriber task receives messages from the broker.
          The MQTT client handles all QoS related messages.

Sample output:
          ...
          APP: Connected to 5.196.95.208, port 1883.
          _HandleProperties: IN: Property "IP_MQTT_PROP_TYPE_TOPIC_ALIAS_MAXIMUM" received
          _HandleProperties: IN: Maximum number of topic aliases: 0 (Server reports 10)
          APP: Received Property IP_MQTT_PROP_TYPE_TOPIC_ALIAS_MAXIMUM for CONNACK with PacketID 0.
          _HandleProperties: IN: Property "IP_MQTT_PROP_TYPE_SERVER_KEEP_ALIVE" received
          _HandleProperties: IN: Maximum number of topic aliases: 0 (Server reports 65535)
          APP: Received Property IP_MQTT_PROP_TYPE_SERVER_KEEP_ALIVE for CONNACK with PacketID 0.
          _HandleProperties: IN: Property "IP_MQTT_PROP_TYPE_RECEIVE_MAXIMUM" received
          _HandleProperties: IN: Maximum number of concurrent QoS 1 and QoS 2: 10 (Server reports 10)
          APP: Received Property IP_MQTT_PROP_TYPE_RECEIVE_MAXIMUM for CONNACK with PacketID 0.
          MQTT: Session established.
          MQTT: SUBSCRIBE message (Id: 25248) sent.
          IP_MQTT_CLIENT_Exec: SUBACK received.
          MQTT: SUBACK (Id: 25248) received.
          APP: Message (Type: SUBACK, Id: 0) received. Reason Code: IP_MQTT_REASON_SUCCESS.
          APP: ----
          MQTT: PUBLISH (Id: 26073) sent.
          APP: OUT: Message (Id: 26073) sent. ("Target Test")
          IP_MQTT_CLIENT_Exec: PUBLISH received.
          MQTT: PUBLISH (QoS: 1 | Ret: 0 | Dup: 0) received.
          APP: IN: Message with (Id: 1 | QoS: 1 | Retain: 0 | Duplicate: 0) received for topic "eMQTT"
          APP: IN: Property "IP_MQTT_PROP_TYPE_USER_PROPERTY" received
          APP: IN: User Property key: "eMQTT Publish UserProp Name"
          APP: IN: User Property value: "eMQTT Publish UserProp Value"
          APP: IN: Property "IP_MQTT_PROP_TYPE_USER_PROPERTY" received
          APP: IN: User Property key: "Dynamic user property key 1"
          APP: IN: User Property value: "Dynamic user property value 1"
          APP: IN: Property "IP_MQTT_PROP_TYPE_MESSAGE_EXPIRY_INTERVAL" received
          APP: IN: Message expire interval set to: "60" seconds
          APP: IN: Payload: "Target Test"
          MQTT: PUBACK (Id: 1) sent.

          APP: ----

          MQTT: PUBLISH (Id: 28073) sent.
          APP: OUT: Message (Id: 28073) sent. ("Target Test")
          IP_MQTT_CLIENT_Exec: PUBLISH received.
          MQTT: PUBLISH (QoS: 1 | Ret: 0 | Dup: 0) received.
          APP: IN: Message with (Id: 2 | QoS: 1 | Retain: 0 | Duplicate: 0) received for topic "eMQTT"
          APP: IN: Property "IP_MQTT_PROP_TYPE_USER_PROPERTY" received
          APP: IN: User Property key: "eMQTT Publish UserProp Name"
          APP: IN: User Property value: "eMQTT Publish UserProp Value"
          APP: IN: Property "IP_MQTT_PROP_TYPE_USER_PROPERTY" received
          APP: IN: User Property key: "Dynamic user property key 2"
          APP: IN: User Property value: "Dynamic user property value 2"
          APP: IN: Property "IP_MQTT_PROP_TYPE_MESSAGE_EXPIRY_INTERVAL" received
          APP: IN: Message expire interval set to: "60" seconds
          APP: IN: Payload: "Target Test"
          MQTT: PUBACK (Id: 2) sent.
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

#define USE_RX_TASK                0  // 0: Packets are read in ISR, 1: Packets are read in a task of its own.

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

#define NUM_PROPERTIES_CONNECT    2
#define NUM_PROPERTIES_PUBLISH    3
#define NUM_PROPERTIES_LAST_WILL  1
//
// Broker settings
//
#define MQTT_BROKER_ADDR                 "test.mosquitto.org"  // Alternate test broker: broker.emqx.io
#define MQTT_BROKER_PORT                 1883

//
// Client settings
//
#define MQTT_CLIENT_NAME                 "eMQTT client" // An empty client name will allow the server to automatically choose a unqiue client ID. This will be assigned during the connect.
#define MQTT_CLIENT_BUFFER_SIZE          256
#define MQTT_CLIENT_MESSAGE_BUFFER_SIZE  512
#define RCV_TIMEOUT                      500
#define MQTT_CLIENT_MEMORY_POOL_SIZE     1024                         // Pool size should be at least 512 bytes.

//
// Application settings
//
#define TOPIC_FILTER01_TO_SUBSCRIBE      "eMQTT"                      // Topic to subscribe.
#define PUBLISH_TIMEOUT                  5000                         // Send a publish message each x milliseconds.
//
// For "test.mosquitto.org" this must not be 0.
//
#define PING_TIMEOUT                     2                            // Configure MQTT ping to x seconds.


//
// Publish related definitions
//
#define TOPIC_FILTER_TO_PUBLISH          "eMQTT"                              // Topic to publish
#define PAYLOAD                          "Target Test from " MQTT_CLIENT_NAME // Static payload to publish.
#define LAST_WILL_PAYLOAD                "Client " MQTT_CLIENT_NAME " has disconnected ungracefully"

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

//
// Task stacks and Task-Control-Blocks.
//
static OS_STACKPTR int _IPStack[TASK_STACK_SIZE_IP_TASK/sizeof(int)];       // Stack of the IP_Task.
static OS_TASK         _IPTCB;                                              // Task-Control-Block of the IP_Task.

static OS_STACKPTR int _MQTTPubStack[TASK_STACK_SIZE_IP_TASK/sizeof(int)];  // Stack of the MQTT Publisher task.
static OS_TASK         _PublishTCB;                                         // Task-Control-Block of the IP_Task.

#if USE_RX_TASK
static OS_STACKPTR int _IPRxStack[TASK_STACK_SIZE_IP_RX_TASK/sizeof(int)];  // Stack of the IP_RxTask.
static OS_TASK         _IPRxTCB;                                            // Task-Control-Block of the IP_RxTask.
#endif

static char            _acBuffer[MQTT_CLIENT_BUFFER_SIZE];                  // Memory block used by the MQTT client.
static char            _aPayload[MQTT_CLIENT_MESSAGE_BUFFER_SIZE];          // Memory used to store the received payload for printf().


static IP_MQTT_CLIENT_CONTEXT _MQTTClient;
static U32             _aMQTTPool[MQTT_CLIENT_MEMORY_POOL_SIZE / sizeof(int)]; // Memory pool for the MQTT client. Required for the maintenance structures

//
// MQTT API locking resources.
//
static OS_RSEMA _RSema;


#if USE_MQTT_5
static const IP_MQTT_PROPERTY     * apPropertiesConnect[NUM_PROPERTIES_CONNECT];
static const IP_MQTT_PROPERTY     * apPropertiesPublish[NUM_PROPERTIES_PUBLISH];
static const IP_MQTT_PROPERTY     * apPropertiesLastWill[NUM_PROPERTIES_LAST_WILL];

//
// Static connect properties
//
static const IP_MQTT_PROPERTY _PropUserConnect = {
  .PropType = IP_MQTT_PROP_TYPE_USER_PROPERTY,
  .PropData = {
    .Data_StrPair = {
      .Len1 = 27,
      .Len2 = 28,
      .pData1 = "eMQTT Connect UserProp Name",
      .pData2 = "eMQTT Connect UserProp Value"
    }
  }
};

static const IP_MQTT_PROPERTY _PropReceiveMax = {
  .PropType = IP_MQTT_PROP_TYPE_RECEIVE_MAXIMUM,
  .PropData = {
    .Data_U8 = 0x07
  }
};

//
// Static publish properties (See _PUBLISH_and_PING_Task() for a dynamic usage sample)
//
static const IP_MQTT_PROPERTY _PropUserPublish = {
  .PropType = IP_MQTT_PROP_TYPE_USER_PROPERTY,
  .PropData = {
    .Data_StrPair = {
      .Len1 = 27,
      .Len2 = 28,
      .pData1 = "eMQTT Publish UserProp Name",
      .pData2 = "eMQTT Publish UserProp Value"
    }
  }
};

static const IP_MQTT_PROPERTY _PropExpiry = {
  .PropType = IP_MQTT_PROP_TYPE_MESSAGE_EXPIRY_INTERVAL,
  .PropData = {
    .Data_U32 = 60 // in seconds
  }
};

//
// Static Last Will properties
//
static const IP_MQTT_PROPERTY _PropWillDelay = {
  .PropType = IP_MQTT_PROP_TYPE_WILL_DELAY_INTERVAL,
  .PropData = {
    .Data_U32 = 30 // in seconds
  }
};
#endif

//
// Packet types.
//
static const char* MQTT_PACKET_Types[] = {
  "Unknown",
  "CONNECT",
  "CONNACK",
  "PUBLISH",
  "PUBACK",
  "PUBREC",
  "PUBREL",
  "PUBCOMP",
  "SUBSCRIBE",
  "SUBACK",
  "UNSUBSCRIBE",
  "UNSUBACK",
  "PINGREQ",
  "PINGRESP",
  "DISCONNECT"
};


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
*
*  Return value
*    == 1: O.K.
*    == 0: Error.
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
*
*  Return value
*    >   0: O.K., IP address in host endianness.
*    == -1: Error.
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
*
*  Return value
*    != NULL: O.K.
*    == NULL: Error.
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
    IP_MQTT_CLIENT_APP_LOG(("APP: \nSocket error: %s", IP_Err2Str(SoError)));
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
    IP_MQTT_CLIENT_APP_LOG(("_Disconnect: closing socket %d", (long)pSocket));
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
*
*  Return value
*    >   0: O.K., number of bytes sent.
*    ==  0: Connection has been gracefully closed by the broker.
*    == -1: Error.
*/
static int _Send(void* pSocket, const char* pBuffer, int NumBytes) {
  return send((long)pSocket, pBuffer, NumBytes, 0);
}

/*********************************************************************
*
*       IP_MQTT_CLIENT_TRANSPORT_API
*
*  Description
*    IP stack related function table
*/
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

/*********************************************************************
*
*       _Alloc()
*
*  Function description
*    Wrapper for Alloc(). (emNet: IP_MEM_Alloc())
*/
static void* _Alloc(U32 NumBytesReq) {
  return IP_AllocEx(_aMQTTPool, NumBytesReq);
}

/*********************************************************************
*
*       _Free()
*
*  Function description
*    Wrapper for Free(). (emNet: IP_Free())
*/
static void _Free(void *p) {
  IP_Free(p);
}

/*********************************************************************
*
*       _Lock()
*
*  Function description
*    Acquires the lock to ensure MQTT API is thread safe.
*/
static void _Lock(void) {
  OS_Use(&_RSema);
}

/*********************************************************************
*
*       _Unlock()
*
*  Function description
*    Releases a previously acquired lock for thread safety MQTT API.
*/
static void _Unlock(void) {
  OS_Unuse(&_RSema);
}

/*********************************************************************
*
*       _RecvMessageEx()
*
*  Function description
*    Process received PUBLISH messages.
*
*  Return value
*    >  0: O.K.
*    == 0: Connection has been gracefully closed by the broker.
*    <  0: Error
*/
static int _RecvMessageEx (void* pContext, void* pHandle, int NumBytesRem, U8 * pReasonCode) {
  IP_MQTT_CLIENT_CONTEXT* pMQTTClient;
  IP_MQTT_CLIENT_MESSAGE* pPublish;
  int                     NumBytes;
  int                     r;
  int                     NumBytesTopic;
  int                     NumBytesPayload;
  int                     NumBytesReceived;
  int                     NumBytesProperties;
  char*                   pTopic;
  char*                   pPayload;
  char*                   pProperties;
#if USE_MQTT_5
  U8                      PropType;
  U16                     Data16;
  U32                     Data32;
#endif

  pPublish            = (IP_MQTT_CLIENT_MESSAGE*)pHandle;
  pMQTTClient         = (IP_MQTT_CLIENT_CONTEXT*)pContext;
  pTopic              = NULL;
  pPayload            = NULL;
  pProperties         = NULL;
  NumBytesTopic       = 0;
  NumBytesReceived    = 0;
  NumBytesPayload     = 0;
  NumBytesProperties  = 0;
  //
  // Check if we can store the complete MQTT message in the application buffer.
  //
  if (NumBytesRem > MQTT_CLIENT_MESSAGE_BUFFER_SIZE) {
    NumBytes = MQTT_CLIENT_MESSAGE_BUFFER_SIZE;   // Get as much data as fits into the buffer.
  } else {
    NumBytes = NumBytesRem;                       // Get the complete message.
  }
  //
  // Read and process the MQTT message
  //
  r = IP_MQTT_CLIENT_Recv(pMQTTClient, _aPayload, NumBytes);
  if (r > 0) {
    NumBytesRem      -= r;
    NumBytesReceived += r;
    //
    // Get the information in MQTT message header.
    //
    IP_MQTT_CLIENT_ParsePublishEx(pMQTTClient, pPublish, _aPayload, NumBytes, &pTopic, &NumBytesTopic, &pPayload, &NumBytesPayload, &pProperties, &NumBytesProperties);
    //
    // Process the data
    // To visualize the data transmission, we output the message if it fits into the supplied buffer.
    //
    // Attention: The topic is not zero-terminated (strings in MQTT are generally not zero terminated because they always have a length header).
    //
    IP_MQTT_CLIENT_APP_LOG(("APP: IN: Message with (Id: %d | QoS: %d | Retain: %d | Duplicate: %d) received for topic \"%.*s\"", pPublish->PacketId, pPublish->QoS, pPublish->Retain, pPublish->Duplicate, NumBytesTopic, pTopic));
    if ((r < MQTT_CLIENT_MESSAGE_BUFFER_SIZE) && (NumBytesPayload < MQTT_CLIENT_MESSAGE_BUFFER_SIZE)) {  // If possible finalize the string to output it.
#if USE_MQTT_5
      if (pProperties != NULL) {
        do {
          PropType = *pProperties;
          pProperties++; // First data byte.
          NumBytesProperties--;
          IP_MQTT_CLIENT_APP_LOG(("APP: IN: Property \"%s\" received", IP_MQTT_Property2String(PropType)));
          //
          // PUBLISH properties only.
          //
          switch (PropType) {
            case IP_MQTT_PROP_TYPE_USER_PROPERTY:
              //
              // A User property is a key-value UTF-8 string pair with the format
              // [U16 Len1][String1][U16 Len2][String2]
              // The strings are normally _not_ zero-terminated.
              //
              Data16 = SEGGER_RdU16BE((const U8*)pProperties); // Len1
              pProperties += 2;
              if (Data16 > 0) {
                IP_MQTT_CLIENT_APP_LOG(("APP: IN: User Property key: \"%.*s\" ", Data16, pProperties));
              }
              pProperties += Data16;
              NumBytesProperties -= 2 + Data16;
              Data16 = SEGGER_RdU16BE((const U8*)pProperties); // Len2
              pProperties += 2;
              if (Data16 > 0) {
                IP_MQTT_CLIENT_APP_LOG(("APP: IN: User Property value: \"%.*s\" ", Data16, pProperties));
              }
              pProperties += Data16;
              NumBytesProperties -= 2 + Data16;
              break;
            case IP_MQTT_PROP_TYPE_MESSAGE_EXPIRY_INTERVAL:
              //
              // The Expiry Interval property is a 4 byte integer containing the expiry time in seconds.
              //
              Data32 = SEGGER_RdU32BE((const U8*)pProperties);
              NumBytesProperties -= 4;
              IP_MQTT_CLIENT_APP_LOG(("APP: IN: Message expire interval set to: \"%d\" seconds ", Data32));
              break;
            case IP_MQTT_PROP_TYPE_PAYLOAD_FORMAT_INDICATOR:
            case IP_MQTT_PROP_TYPE_CONTENT_TYPE:
            case IP_MQTT_PROP_TYPE_RESPONSE_TOPIC:
            case IP_MQTT_PROP_TYPE_CORRELATION_DATA:
            case IP_MQTT_PROP_TYPE_SUBSCRIPTION_ID:
            case IP_MQTT_PROP_TYPE_TOPIC_ALIAS_MAXIMUM:
            case IP_MQTT_PROP_TYPE_RETAIN_AVAILABLE:
            default:
              //
              // Property not decoded by this sample application.
              // NumBytesProperties is set to zero to exit the loop.
              //
              NumBytesProperties = 0;
              break;
          }
        } while (NumBytesProperties != 0);
      }
#endif
      if (pPayload != NULL) {
        *(pPayload + NumBytesPayload) = '\0';
        IP_MQTT_CLIENT_APP_LOG(("APP: IN: Payload: \"%s\"", pPayload));
      } else {
        IP_MQTT_CLIENT_APP_LOG(("APP: IN: No payload."));
      }
    } else {
      IP_MQTT_CLIENT_APP_LOG(("APP: IN: Payload: Too large to output."));
    }
    //
    // If message is larger then the available buffer, read and discard it to clean the buffer.
    //
    while (NumBytesRem > 0) {
      if (NumBytesRem < NumBytes) {
        NumBytes = NumBytesRem;
      }
      r = IP_MQTT_CLIENT_Recv(pMQTTClient, _aPayload, NumBytes);
      if (r < 0) {
        NumBytesReceived = r; // Receive failed. Connection closed ?
        break;
      }
      NumBytesRem      -= r;
      NumBytesReceived += r;
    };
  }
  //
  // Set a Reason Code to send back to the server.
  // This is only relevant if QoS of the received message is 1 or 2.
  //
  if (pPublish->QoS > 0) {
    *pReasonCode = IP_MQTT_REASON_SUCCESS;
    //*pReasonCode = IP_MQTT_REASON_IMPL_SPECIFIC_ERR; // Error code for testing.
  }
  return NumBytesReceived;
}

/*********************************************************************
*
*       _MessageHandledEx()
*
*  Function description
*    Called if all QoS related messages are processed and
*      - Sent messages can be freed.
*      - Received messages can be processed by the application.
*
*  Note
*   The application has to store the message with QoS > 0 until it gets/has sent an acknowledgement.
*   Target sends PUBLISH with QoS 0 to broker -> Message can be directly discarded after message has been sent.
*   Target sends PUBLISH with QoS 1 to broker -> Message can be discarded after PUBACK has been received from the broker.
*   Target sends PUBLISH with QoS 2 to broker -> Message can be discarded after PUBCOMP has been received.
*   Broker sends PUBLISH with QoS 0 to target -> Message can be directly processed.
*   Broker sends PUBLISH with QoS 1 to target -> Message can be processed after PUBACK has been sent to the broker.
*   Broker sends PUBLISH with QoS 2 to target -> Message can be processed after PUBCOMP has been received from the broker.
*
*  Return value
*    > 0: O.K.
*    < 0: Error
*/
static int _MessageHandledEx(void* pContext, U8 Type, U16 PacketId, U8 ReasonCode) {
  (void)pContext;
#if USE_MQTT_5 == 0
  (void)ReasonCode;
#endif
  //
  switch (Type) {
    //
    // Tx related packet types
    //
    case IP_MQTT_CLIENT_TYPES_PUBACK:  // PUBACK received. QoS level 1 successfully done. -> Message can be freed.
    case IP_MQTT_CLIENT_TYPES_PUBCOMP: // PUCOMP received. QoS level 2 successfully done. -> Message can be freed.
#if USE_MQTT_5
      IP_MQTT_CLIENT_APP_LOG(("APP: OUT: Message with Id: %d can be freed. Reason Code received from server: %s.", PacketId, IP_MQTT_ReasonCode2String(ReasonCode)));
#else
      IP_MQTT_CLIENT_APP_LOG(("APP: OUT: Message with Id: %d can be freed.", PacketId));
#endif
      break;
    //
    // Rx related packet types
    //
    case IP_MQTT_CLIENT_TYPES_PUBLISH: // PUBACK sent. -> Received packet can be processed.
    case IP_MQTT_CLIENT_TYPES_PUBREL:  // PUBREC sent. PUBREL received. PUBCOMP sent. -> Received packet can be processed.
#if USE_MQTT_5
      IP_MQTT_CLIENT_APP_LOG(("APP: IN: Message with Id: %d can be processed. Reason Code sent to server: %s.", PacketId, IP_MQTT_ReasonCode2String(ReasonCode)));
#else
      IP_MQTT_CLIENT_APP_LOG(("APP: IN: Message with Id: %d can be processed. ", PacketId));
#endif
      break;
    case IP_MQTT_CLIENT_TYPES_PINGRESP:
      IP_MQTT_CLIENT_APP_LOG(("APP: IN: Received PING Response from server."));
      break;
    //
    // Subscription packet types
    //
    case IP_MQTT_CLIENT_TYPES_SUBACK:
    case IP_MQTT_CLIENT_TYPES_UNSUBACK: // UNSUBACK received. The subscribe structure can be freed.
    default:
#if USE_MQTT_5
      IP_MQTT_CLIENT_APP_LOG(("APP: Message (Type: %s, Id: %d) received. Reason Code: %s.", MQTT_PACKET_Types[Type], PacketId, IP_MQTT_ReasonCode2String(ReasonCode)));
#else
      IP_MQTT_CLIENT_APP_LOG(("APP: Message (Type: %s, Id: %d) received.", MQTT_PACKET_Types[Type], PacketId));
#endif
      break;
  }
  IP_MQTT_CLIENT_APP_LOG(("APP: ----"));
  return 1;
}

/*********************************************************************
*
*       _OnProperty()
*
*  Function description
*    Called when a non-PUBLISH (CONNACK, PUBACK, PUBREC, PUBCOMP, SUBACK, UNSUBACK, DISCONNECT, AUTH) message is received with a Property.
*
*  Return value
*    > 0: O.K.
*    < 0: Error
*/
#if USE_MQTT_5
static void _OnProperty (void* pMQTTClient, U16 PacketId, U8 PacketType, IP_MQTT_PROPERTY * pProp) {
  (void)pMQTTClient;
  IP_MQTT_CLIENT_APP_LOG(("APP: Received Property %s for %s with PacketID %d.", IP_MQTT_Property2String(pProp->PropType), MQTT_PACKET_Types[PacketType], PacketId));
  return;
}
#endif

/*********************************************************************
*
*       _HandleError()
*
*  Function description
*    Called in case of an error.
*    Can be used to analyze/handle a connection problem.
*
*  Return value
*    > 0: O.K.
*    < 0: Error
*/
static int _HandleError (void* pContext) {
  IP_MQTT_CLIENT_CONTEXT* pMQTTClient;
  int Error;

  Error       = 1;
  pMQTTClient = (IP_MQTT_CLIENT_CONTEXT*)pContext;
  getsockopt((long)pMQTTClient->Connection.Socket, SOL_SOCKET, SO_ERROR, &Error, sizeof(Error));
  if (Error < 0) {
    IP_MQTT_CLIENT_APP_LOG(("APP: Socket error %d. Disconnect MQTT client.", Error));
    IP_MQTT_CLIENT_Disconnect(pMQTTClient);
  }
  return Error;
}

/*********************************************************************
*
*       _HandleDisconnect()
*
*  Function description
*    Called in case of a disconnect from the server.
*    Only available if MQTT 5 is used.
*    With MQTT 3.1.1 there is no option for the server to send a disconnect.
*
*  Return value
*    > 0: O.K.
*    < 0: Error
*/
#if USE_MQTT_5
static void _HandleDisconnect (void* pContext, U8 ReasonCode) {
  IP_MQTT_CLIENT_CONTEXT* pMQTTClient;

  pMQTTClient = (IP_MQTT_CLIENT_CONTEXT*)pContext;

  IP_MQTT_CLIENT_APP_LOG(("APP: _HandleDisconnect: %d %s", ReasonCode, IP_MQTT_ReasonCode2String(ReasonCode)));
  IP_MQTT_CLIENT_Disconnect(pMQTTClient);
  return;
}
#endif

/*********************************************************************
*
*       IP_MQTT_CLIENT_APP_API
*
*  Description
*    Application related function table
*/
static const IP_MQTT_CLIENT_APP_API _APP_Api = {
  _GenRandom,       // (*pfGenRandom)
  _Alloc,           // (*pfAlloc)
  _Free,            // (*pfFree)
  _Lock,            // (*pfLock)
  _Unlock,          // (*pfUnlock)
  NULL,             // (*pfRecvMessage)
  NULL,             // (*pfOnMessageConfirm)    // Used with MQTT v3.1.1 only
  _HandleError,     // (*pfHandleError)
#if USE_MQTT_5
  _HandleDisconnect,// (*pfHandleDisconnect)    // Used with MQTT v5 only
#else
  NULL,
#endif
  _MessageHandledEx,// (*pfOnMessageConfirmEx)  // Used with MQTT v5 or 3.1.1 if "pfOnMessageConfirm" is not set
  _RecvMessageEx,   // (*pfRecvMessageEx)
#if USE_MQTT_5
  _OnProperty       // (*pfOnProperty)          // Used with MQTT v5 only
#else
  NULL
#endif
};

/*********************************************************************
*
*       _PUBLISH_and_PING_Task()
*
*  Function description
*    Sends publish and PINGREQ messages.
*/
static void _PUBLISH_and_PING_Task(void) {
  IP_MQTT_CLIENT_MESSAGE* pPublish;
  int                     r;
  U32                     TimeExpirePublish;
  U32                     TimeExpirePing;
#if USE_MQTT_5
  IP_MQTT_PROPERTY        DynamicUserProp;
  char                    acKey[40];
  char                    acValue[45];
  unsigned                Counter;

  Counter = 0;
#endif

  TimeExpirePublish = 0;
  TimeExpirePing    = 0;

  do {
    r = IP_MQTT_CLIENT_IsClientConnected(&_MQTTClient);
    if (r > 0) {
      //
      // Initially set timeouts. This is done after the connect check because
      // the KeepAlive timeout value can change depending on the CONNACK from the server.
      // The server's KeepAlive timeout takes precedence.
      // KeepAlive time is in seconds.
      // As per MQTT 3.1.1 or 5 spec the server allows for one and a half times the
      // KeepAlive time period before disconnecting the client.
      //
      if (TimeExpirePublish == 0 && TimeExpirePing == 0) {
        TimeExpirePublish = GET_TIME_32 + PUBLISH_TIMEOUT;
        TimeExpirePing    = GET_TIME_32 + (_MQTTClient.KeepAlive * 1000);
      }
      //
      // Send KeepAlive (PINGREQ) if configured.
      //
      if (_MQTTClient.KeepAlive != 0 && CHECK_TIME_EXPIRED(TimeExpirePing)) {
        TimeExpirePing = GET_TIME_32 + (_MQTTClient.KeepAlive * 1000); // Set new expire time.
        IP_MQTT_CLIENT_Timer(); // Send PINGREQ.
        IP_MQTT_CLIENT_APP_LOG(("APP: PINGREQ sent."));
      }
      //
      // Build and send Publish
      //
      //
      if (CHECK_TIME_EXPIRED(TimeExpirePublish)) {
        TimeExpirePublish = GET_TIME_32 + PUBLISH_TIMEOUT; // Set new expire time.
        //
        // Allocate message structure.
        //
        pPublish = (IP_MQTT_CLIENT_MESSAGE*)_Alloc(sizeof(IP_MQTT_CLIENT_MESSAGE));
        if (pPublish != NULL) {
          //
          // Initialize the MQTT message, which should be published.
          //
          memset(pPublish, 0, sizeof(IP_MQTT_CLIENT_MESSAGE));
          pPublish->sTopic  = TOPIC_FILTER_TO_PUBLISH;
          pPublish->pData   = PAYLOAD;
          pPublish->DataLen = strlen(PAYLOAD);
          pPublish->QoS     = IP_MQTT_CLIENT_FLAGS_PUBLISH_QOS1;
#if USE_MQTT_5
          //
          // Add static const publish properties.
          //
          apPropertiesPublish[0] = &_PropUserPublish;
          apPropertiesPublish[1] = &_PropExpiry;
          //
          // Fill and add a dynamic property.
          //
          Counter++;
          DynamicUserProp.PropType = IP_MQTT_PROP_TYPE_USER_PROPERTY;
          SEGGER_snprintf(acKey, sizeof(acKey), "Dynamic user property key %d", Counter);
          SEGGER_snprintf(acValue, sizeof(acValue), "Dynamic user property value %d", Counter);
          DynamicUserProp.PropData.Data_StrPair.pData1 = acKey;
          DynamicUserProp.PropData.Data_StrPair.Len1   = strlen(acKey);
          DynamicUserProp.PropData.Data_StrPair.pData2 = acValue;
          DynamicUserProp.PropData.Data_StrPair.Len2   = strlen(acValue);
          apPropertiesPublish[2] = &DynamicUserProp;

          pPublish->paProperties = apPropertiesPublish;
          pPublish->NumProperties = SEGGER_COUNTOF(apPropertiesPublish);
#endif
          //
          // Publish a message.
          //
          r = IP_MQTT_CLIENT_Publish(&_MQTTClient, pPublish);
          if (r >= 0) {                                                     // Message successfully sent.
            if (pPublish->QoS > IP_MQTT_CLIENT_FLAGS_PUBLISH_QOS0) {        // If QoS level > QoS level 0: Output the Message Id to visualize the message flow.
              IP_MQTT_CLIENT_APP_LOG(("APP: OUT: Message (Id: %d) sent. (\"%s\")", pPublish->PacketId, pPublish->pData));
            } else {
              IP_MQTT_CLIENT_APP_LOG(("APP: OUT: Message sent."));
            }
          } else {
            IP_MQTT_CLIENT_APP_LOG(("APP: Could not send message."));
            //
            // If we were unable to send the PUBLISH message (e.g. we just lost our network connection)
            // we have to free the message here.
            // Alternatively the application can re-use the allocated structure and try again.
            //
            _Free(pPublish);
          }
          if (pPublish->QoS == 0) {
            //
            // Free message structure, if no QoS handling is required
            // If QoS is used the structure will be freed from the MQTT module, if it is no longer required.
            //
            _Free(pPublish);
          }
        } else {
          IP_MQTT_CLIENT_APP_LOG(("APP: Could not allocate memory to send message."));
        }
      }
    }
    OS_Delay(10);
  } while (1);
}

/*********************************************************************
*
*       _MQTT_Client()
*
*  Function description
*    MQTT client application.
*    Initializes the MQTT client and creates a task to publish
*    messages.
*
*    The main application loop establishes a connection to a broker,
*    send a subscribe message and handles incoming packets.
*/
static void _MQTT_Client(void) {
  IP_MQTT_CLIENT_MESSAGE      LastWill;
  IP_MQTT_CLIENT_TOPIC_FILTER TopicFilter[1];
  IP_MQTT_CLIENT_SUBSCRIBE*   pSubscribe;
  IP_MQTT_CONNECT_PARAM       ConnectPara;
  U8                          ReasonCode;
  int                         r;
  IP_fd_set                   ReadFds;

  IP_AddMemory(_aMQTTPool, sizeof(_aMQTTPool));    // Memory pool used for allocation of maintenance structures.
  //
  // Initialize MQTT client context.
  //
  IP_MQTT_CLIENT_Init(&_MQTTClient, _acBuffer, MQTT_CLIENT_BUFFER_SIZE, &_IP_Api, &_APP_Api, MQTT_CLIENT_NAME);
  OS_CREATETASK(&_PublishTCB , "MQTTSend" , _PUBLISH_and_PING_Task , TASK_PRIO_IP_TASK - 1 , _MQTTPubStack);    // Create the Publish task
  OS_CREATERSEMA(&_RSema);                         // Create semaphore for locking.
  //
  // Main application loop.
  //
  do {
    //
    // Set the Last Will message for our publish topic.
    // This message will be sent to all other clients subscribed to this topic
    // if this client disconnects ungracefully, and the Last Will Timeout has passed.
    //
    LastWill.sTopic  = TOPIC_FILTER_TO_PUBLISH;
    LastWill.pData   = LAST_WILL_PAYLOAD;
    LastWill.DataLen = strlen(LAST_WILL_PAYLOAD);
    LastWill.QoS     = IP_MQTT_CLIENT_FLAGS_PUBLISH_QOS0;
#if USE_MQTT_5
    apPropertiesLastWill[0] = &_PropWillDelay;
    LastWill.paProperties   = apPropertiesLastWill;
    LastWill.NumProperties  = SEGGER_COUNTOF(apPropertiesConnect);;
#endif
    IP_MQTT_CLIENT_SetLastWill(&_MQTTClient, &LastWill);
    //
    // Connect to the MQTT broker.
    //
    memset(&ConnectPara, 0, sizeof(IP_MQTT_CONNECT_PARAM));
    ConnectPara.CleanSession = 1;
    ConnectPara.Port = MQTT_BROKER_PORT;
    ConnectPara.sAddr = MQTT_BROKER_ADDR;
#if USE_MQTT_5
    ConnectPara.Version = 5;
    memset(apPropertiesConnect, 0, sizeof(apPropertiesConnect));
    apPropertiesConnect[0] = &_PropUserConnect;
    apPropertiesConnect[1] = &_PropReceiveMax;
    ConnectPara.paProperties = apPropertiesConnect;
    ConnectPara.NumProperties = SEGGER_COUNTOF(apPropertiesConnect);
#else
    ConnectPara.Version = 4; // MQTT 3.1.1
#endif
    //
    // Configure MQTT keep alive time (for sending PINGREQ messages)
    //
    if (PING_TIMEOUT != 0) {
      r = IP_MQTT_CLIENT_SetKeepAlive(&_MQTTClient, PING_TIMEOUT);
      if (r != 0) {
        IP_MQTT_CLIENT_APP_LOG(("APP: Could not set timeout (currently connected ?)"));
      }
    }
    //
    // Execute connect.
    //
    r = IP_MQTT_CLIENT_ConnectEx(&_MQTTClient, &ConnectPara, &ReasonCode);
    if (r != 0) {
#if USE_MQTT_5
      IP_MQTT_CLIENT_APP_LOG(("APP: MQTT connect error: %d, ReasonCode %s.", r, IP_MQTT_ReasonCode2String(ReasonCode)));
#else
      IP_MQTT_CLIENT_APP_LOG(("APP: MQTT connect error: %d", r));
#endif
      goto Disconnect;
    }
    //
    // Initialize the topic filter and subscribe structures.
    //
    pSubscribe = (IP_MQTT_CLIENT_SUBSCRIBE*)_Alloc(sizeof(IP_MQTT_CLIENT_SUBSCRIBE));
    if (pSubscribe == NULL) {
      IP_MQTT_CLIENT_APP_LOG(("APP: Could not allocate IP_MQTT_CLIENT_SUBSCRIBE structure."));
      goto Disconnect;
    }
    memset(TopicFilter, 0, sizeof(TopicFilter));     // Topic 1
    TopicFilter[0].sTopicFilter = TOPIC_FILTER01_TO_SUBSCRIBE;
    TopicFilter[0].QoS          = IP_MQTT_CLIENT_FLAGS_PUBLISH_QOS1;
    memset(pSubscribe, 0, sizeof(IP_MQTT_CLIENT_SUBSCRIBE));             // Subscribe structure.
    pSubscribe->pTopicFilter    = TopicFilter;
    pSubscribe->TopicCnt        = SEGGER_COUNTOF(TopicFilter);
    //
    // Subscribe to topic.
    //
    r = IP_MQTT_CLIENT_Subscribe(&_MQTTClient, pSubscribe);
    if (r < 0) {
      IP_MQTT_CLIENT_APP_LOG(("APP: Subscribe failed. Check topic filter structure."));
    } else {
      //
      // Process incoming messages and send messages
      //
      while (1) {
        r = IP_MQTT_CLIENT_IsClientConnected(&_MQTTClient);
        if (r > 0) {
          IP_FD_ZERO(&ReadFds);                                       // Clear the set
          IP_FD_SET((long)_MQTTClient.Connection.Socket, &ReadFds);   // Add socket to the set
          r = select(&ReadFds, NULL, NULL, 500);                      // Check for activity. Wait 500ms.
          if (r > 0) {
            r = IP_MQTT_CLIENT_Exec(&_MQTTClient);                    // Get messages for subscribed topics, process QoS messages, ...
            if (r == 0) {
              IP_MQTT_CLIENT_APP_LOG(("IP_MQTT_CLIENT_Exec: Connection gracefully closed by peer."));
              break; // Exit loop and try to re-connect.
            } else if (r < 0) {
              IP_MQTT_CLIENT_APP_LOG(("IP_MQTT_CLIENT_Exec: Error: %d.", r));
              break; // Exit loop and try to re-connect.
            }
          }
        } else {
          //
          // If we lost our connection - exit the message processing loop.
          //
          break;
        }
      }
    }
    //
    // Disconnect.
    //
Disconnect:
    IP_MQTT_CLIENT_Disconnect(&_MQTTClient);
    IP_MQTT_CLIENT_APP_LOG(("APP: Disconnect done."));
    OS_Delay(1000);
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
  OS_CREATETASK(&_IPTCB      , "IP_Task"           , IP_Task       , TASK_PRIO_IP_TASK     , _IPStack);       // Start the IP_Task.
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
