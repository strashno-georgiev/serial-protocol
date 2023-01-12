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

File    : IP_SendMail_Secure.c
Purpose : Sample program for embOS & emNet & emSSL if enabled.
          Demonstrates sending a mail via a SMTP server using a secure
          connection and resolving the servers host name via DNS.
          For full functionality the sample needs to be configured to
          the configuration of your mail server and your sender and
          recipient addresses.

Notes:
  (1) For compatibility with interfaces that need to connect in
      any way this sample calls connect and disconnect routines
      that might not be needed in all cases.

      This sample can be used for Ethernet and dial-up interfaces
      and is configured to use the last registered interface as
      its main interface.
*/

#include "RTOS.h"
#include "BSP.h"
#include "IP.h"
#include "IP_SMTPC.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#define USE_SSL           1                                // Use SSL to secure the connection if supported by the server.
#define SECURITY_POLICY   SMTPC_SEC_POLICY_ALLOW_INSECURE  // Default: Try to secure the connection but allow fallback to insecure.
#define USE_RX_TASK       0                                // 0: Packets are read in ISR, 1: Packets are read in a task of its own.

#if USE_SSL
#include "SSL.h"

#define SMTPC_STACK_SIZE  3584
#else
#define SMTPC_STACK_SIZE  2048
#endif

//
// SMTP client sample: Sender configuration.
//
#define SENDER_NAME  ""  // Your name that will be displayed as senders name in a mail client. Example "John Doe".
#define SENDER_ADDR  ""  // Your mail address. Example "john.doe@foobar.com".
#define SERVER_ADDR  ""  // Host name or IP addr. of your mail server. Example "mail.foobar.com".
#define SERVER_USER  ""  // Your user name for your mail account. Example "john.doe@foobar.com".
#define SERVER_PASS  ""  // Your password for your mail account. Example "password".

//
// SMTP client sample: Recipient configuration.
//
#define RECIPIENT_0_NAME  ""                 // Recipient name that will be displayed by a mail client. Example "Kirk Doe".
#define RECIPIENT_0_ADDR  ""                 // Recipient mail addr. Example "kirk.doe@foobar.com".
#define RECIPIENT_0_TYPE  SMTPC_REC_TYPE_TO  // SMTPC_REC_TYPE_TO, SMTPC_REC_TYPE_CC or SMTPC_REC_TYPE_BCC .

#define RECIPIENT_1_NAME  ""                 // Recipient name that will be displayed by a mail client. Example "Kirk Doe".
#define RECIPIENT_1_ADDR  ""                 // Recipient mail addr. Example "kirk.doe@foobar.com".
#define RECIPIENT_1_TYPE  0                  // SMTPC_REC_TYPE_TO, SMTPC_REC_TYPE_CC or SMTPC_REC_TYPE_BCC . 0 disables this recipient for the sample.

#define RECIPIENT_2_NAME  ""                 // Recipient name that will be displayed by a mail client. Example "Kirk Doe".
#define RECIPIENT_2_ADDR  ""                 // Recipient mail addr. Example "kirk.doe@foobar.com".
#define RECIPIENT_2_TYPE  0                  // SMTPC_REC_TYPE_TO, SMTPC_REC_TYPE_CC or SMTPC_REC_TYPE_BCC . 0 disables this recipient for the sample.

//
// SMTP client sample: Message configuration.
//
#define MESSAGE_SUBJECT  "SMTP message sent via emNet SMTP client"
#define MESSAGE_BODY     "emNet SMTP client - www.segger.com"

//
// SMTP client sample: Generic configuration.
//
#define OWN_DOMAIN  "sample.com"  // Your domain. According to RFC 821 the maximum total length of a domain name or number is 64 characters. Can be NULL.
#define UTC_OFFSET  "+0100"       // The offset from Coordinated Universal Time (UTC). Can be NULL.
#define TIMEOUT     10000         // Timeout for DNS request [ms].

//
// Task priorities.
//
enum {
   TASK_PRIO_IP_SMTPC = 150
  ,TASK_PRIO_IP_TASK         // Priority should be higher than all IP application tasks.
#if USE_RX_TASK
  ,TASK_PRIO_IP_RX_TASK      // Must be the highest priority of all IP related tasks.
#endif
};

//
// Task stack sizes that might not fit for some interfaces (multiples of sizeof(int)).
//
#ifndef   APP_TASK_STACK_OVERHEAD
  #define APP_TASK_STACK_OVERHEAD  0
#endif

/*********************************************************************
*
*       Types, local
*
**********************************************************************
*/

typedef struct {
  int hSock;            // Server socket.
#if USE_SSL
  int IsSecure;         // Nonzero indicates secure state.
  SSL_SESSION Session;  // emSSL TLS session.
#endif
} SMTPC_APP_CONTEXT;

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

#if USE_SSL
static int          _cbUpgrade       (SMTPC_SOCKET hSock, const char* sServer);
static void         _cbDowngrade     (SMTPC_SOCKET hSock);
#endif
static SMTPC_SOCKET _cbConnect       (const char* sServer);
static void         _cbDisconnect    (SMTPC_SOCKET hSock);
static int          _cbSend          (const char *pData, int NumBytes, SMTPC_SOCKET hSock);
static int          _cbRecv          (      char *pData, int NumBytes, SMTPC_SOCKET hSock);
static U32          _cbGetTimeDate   (void);
static int          _cbSendAttachment(IP_SMTPC_CONTEXT* pContext, const IP_SMTPC_MULTIPART_ITEM* pItem, const IP_SMTPC_MULTIPART_API* pAPI);

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static SMTPC_APP_CONTEXT _Context;

#if USE_SSL
//
// SSL transport API.
//
static const SSL_TRANSPORT_API _IP_Transport = {
  send,
  recv,
  NULL, // Don't verify the time. Otherwise a function that returns the unix time is needed.
  NULL
};
#endif

//
// SMTP client configuration data.
//
static const IP_SMTPC_MAIL_ADDR _aMailAddr[] = {
   { SENDER_NAME     , SENDER_ADDR     , SMTPC_REC_TYPE_FROM }
  ,{ RECIPIENT_0_NAME, RECIPIENT_0_ADDR, RECIPIENT_0_TYPE }
#if (RECIPIENT_1_TYPE != 0)
  ,{ RECIPIENT_1_NAME, RECIPIENT_1_ADDR, RECIPIENT_1_TYPE }
#endif
#if (RECIPIENT_2_TYPE != 0)
  ,{ RECIPIENT_2_NAME, RECIPIENT_2_ADDR, RECIPIENT_2_TYPE }
#endif
};

static const IP_SMTPC_MTA _MTAConfig = {
  SERVER_ADDR,
  SERVER_USER,
  SERVER_PASS,
  SECURITY_POLICY
};

//
// Table of attachments (multipart items) to add
// to the message. A custom context (pContext) can
// be passed to the callback sending the content of
// the item. For this sample it is the index to an
// array of strings. For a real application pContext
// could be used to pass the local file system path
// of a file to attach to the callback.
//

static const char* _aAttachmentContent[] = {
  "Hello world!",
  "Another text file attachment."
};

static const IP_SMTPC_MULTIPART_ITEM _aAttachment[] = {
  // sFilename  , pUserContext  , sContentType, pfSend
  // ---------------------------------------------------------------
  {  "Test0.txt", (const void*)0, "text/plain", _cbSendAttachment },
  {  "Test1.txt", (const void*)1, "text/plain", _cbSendAttachment }
};

static const IP_SMTPC_MESSAGE _Message = {
  MESSAGE_SUBJECT,
  MESSAGE_BODY,
  "------------030007030004040809060800",
  &_aAttachment[0],
  SEGGER_COUNTOF(_aAttachment)
};

static const IP_SMTPC_API _IP_Api = {
   _cbConnect
  ,_cbDisconnect
  ,_cbSend
  ,_cbRecv
#if USE_SSL
  ,_cbUpgrade
  ,_cbDowngrade
#else
  ,NULL
  ,NULL
#endif
};

static const IP_SMTPC_APPLICATION _SMTPC_App = {
  _cbGetTimeDate,  // Time and Date routine, adapt _GetTimeDate() .
  NULL,          // Pointer to status callback function. Can be NULL.
  OWN_DOMAIN,    // Your domain. According to RFC 821 the maximum total length of a domain name or number is 64 characters. Can be NULL.
  UTC_OFFSET     // The offset from Coordinated Universal Time (UTC). Can be NULL.
};

static IP_HOOK_ON_STATE_CHANGE _StateChangeHook;
static int                     _IFaceId;

//
// Task stacks and Task-Control-Blocks.
//
static OS_STACKPTR int _IPStack[TASK_STACK_SIZE_IP_TASK/sizeof(int)];                          // Stack of the IP_Task.
static OS_TASK         _IPTCB;                                                                 // Task-Control-Block of the IP_Task.

#if USE_RX_TASK
static OS_STACKPTR int _IPRxStack[TASK_STACK_SIZE_IP_RX_TASK/sizeof(int)];                     // Stack of the IP_RxTask.
static OS_TASK         _IPRxTCB;                                                               // Task-Control-Block of the IP_RxTask.
#endif

static OS_STACKPTR int _SMTPcStack[(SMTPC_STACK_SIZE + APP_TASK_STACK_OVERHEAD)/sizeof(int)];  // Stack of the SMTP client.
static OS_TASK         _SMTPcTCB;                                                              // Task-Control-Block of the SMTP client.

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

#if USE_SSL
/*********************************************************************
*
*       _cbUpgrade()
*
*  Function description
*    Upgrades a connection from insecure to secure.
*
*  Parameters
*    hSock  : SMTPC insecure socket (or context).
*    sServer: Server we are connecting to for authentication.
*
*  Return value
*    O.K. : >   0
*    Error: <=  0
*/
static int _cbUpgrade(SMTPC_SOCKET hSock, const char* sServer) {
  SMTPC_APP_CONTEXT* pContext;
  int                Status;

  pContext = (SMTPC_APP_CONTEXT*)hSock;
  SSL_SESSION_Prepare(&pContext->Session, pContext->hSock, &_IP_Transport);
  Status = SSL_SESSION_Connect(&pContext->Session, sServer);
  if (Status < 0) {
    return Status;
  } else {
    pContext->IsSecure = 1;
    return 0;
  }
}

/*********************************************************************
*
*       _cbDowngrade()
*
*  Function description
*    Downgrades a connection from insecure to secure.
*
*  Parameters
*    hSock: SMTPC secure socket (or context).
*/
static void _cbDowngrade(SMTPC_SOCKET hSock) {
  SMTPC_APP_CONTEXT* pContext;

  pContext = (SMTPC_APP_CONTEXT*)hSock;
  SSL_SESSION_Disconnect(&pContext->Session);
  pContext->IsSecure = 0;
}
#endif

/*********************************************************************
*
*       _cbConnect()
*
*  Function description
*    Creates a socket and opens a TCP connection to the mail server.
*
*  Parameters
*    sServer: Host name of the mail server.
*
*  Return value
*    O.K. : Socket handle (or context).
*    Error: NULL.
*/
static SMTPC_SOCKET _cbConnect(const char* sServer) {
  SMTPC_APP_CONTEXT* pContext;
  struct sockaddr_in Addr;
  U32 IPAddr;
  int hSock;
  int r;

  //
  // Resolve server host name to IP addr.
  //
  r = IP_ResolveHost(sServer, &IPAddr, TIMEOUT);  // Resolve host name.
  if (r) {
    SMTPC_LOG(("DNS resolve for \"%s\" failed.\n", sServer));
    return NULL;
  }
  //
  // Create socket and connect to mail server.
  //
  hSock = socket(AF_INET, SOCK_STREAM, 0);
  if (hSock == SOCKET_ERROR) {
    SMTPC_LOG(("Could not create socket!\n"));
    return NULL;
  }
  IP_MEMSET(&Addr, 0, sizeof(Addr));
  Addr.sin_family = AF_INET;
  Addr.sin_port = htons(SMTPC_SERVER_PORT);
  Addr.sin_addr.s_addr = IPAddr;
  r = connect(hSock, (struct sockaddr*)&Addr, sizeof(Addr));
  if (r == SOCKET_ERROR) {
    SMTPC_LOG(("\nSocket error :"));
    closesocket(hSock);
    return NULL;
  }
  //
  // Alloc memory for context. In this sample the context
  // is used as static data which is fine if only a single
  // task is sending mails. For multiple tasks being able
  // to send mails independent the memory for the context
  // should be allocated.
  // If memory gets allocated this might fail and in this
  // case should be taken into account for the return value.
  //
  pContext = &_Context;
  memset(pContext, 0, sizeof(*pContext));
  pContext->hSock = hSock;
  SMTPC_LOG(("APP: Connected.\n"));
  return (SMTPC_SOCKET)pContext;
}

/*********************************************************************
*
*       _cbDisconnect()
*
*  Function description
*    Closes a socket (or context).
*
*  Parameters
*    hSock: Socket handle (or context) to close.
*/
static void _cbDisconnect(SMTPC_SOCKET hSock) {
  SMTPC_APP_CONTEXT* pContext;

  pContext = (SMTPC_APP_CONTEXT*)hSock;
  closesocket(pContext->hSock);
}

/*********************************************************************
*
*       _cbSend()
*
*  Function description
*    Sends data via socket interface.
*
*  Parameters
*    pData   : Pointer to data to send.
*    NumBytes: Number of bytes to send.
*    hSock   : Socket handle (or context).
*
*  Return value
*    O.K. : >= 0
*    Error: <  0
*/
static int _cbSend(const char* pData, int NumBytes, SMTPC_SOCKET hSock) {
  SMTPC_APP_CONTEXT* pContext;

  pContext = (SMTPC_APP_CONTEXT*)hSock;
#if USE_SSL
  if (pContext->IsSecure != 0) {
    return SSL_SESSION_Send(&pContext->Session, pData, NumBytes);
  }
  else
#endif
  {
    return send(pContext->hSock, pData, NumBytes, 0);
  }
}

/*********************************************************************
*
*       _cbRecv()
*
*  Function description
*    Receives data via socket interface.
*
*  Parameters
*    pData   : Pointer to data buffer to store received data.
*    NumBytes: Max. number of bytes to receive.
*    hSock   : Socket handle (or context).
*
*  Return value
*    O.K. : >   0
*    Error: <=  0
*/
static int _cbRecv(char* pData, int NumBytes, SMTPC_SOCKET hSock) {
  SMTPC_APP_CONTEXT* pContext;
  int                Status;

  pContext = (SMTPC_APP_CONTEXT*)hSock;
#if USE_SSL
  if (pContext->IsSecure != 0) {
    do {
      Status = SSL_SESSION_Receive(&pContext->Session, pData, NumBytes);
    } while (Status == 0);  // Receiving 0 bytes means something different on a plain socket.
  }
  else
#endif
  {
    Status = recv(pContext->hSock, pData, NumBytes, 0);
  }
  return Status;
}

/*********************************************************************
*
*       _cbGetTimeDate()
*
*  Function description
*    Returns time and date of the system.
*
*  Return value
*    Date and time of the system as U32.
*/
static U32 _cbGetTimeDate(void) {
  U32 r;
  U16 Sec;
  U16 Min;
  U16 Hour;
  U16 Day;
  U16 Month;
  U16 Year;

  Sec    = 0;   //    0 based. Valid range: 0..59 .
  Min    = 0;   //    0 based. Valid range: 0..59 .
  Hour   = 9;   //    0 based. Valid range: 0..23 .
  Day    = 9;   //    1 based. Means that 1 is 1. Valid range is 1..31 (depending on month).
  Month  = 1;   //    1 based. Means that January is 1. Valid range is 1..12 .
  Year   = 29;  // 1980 based. Means that 2008 would be 28.
  r      = (Sec / 2) + (Min << 5) + (Hour  << 11);
  r     |= ((U32)(Day + (Month << 5) + (Year  << 9))) << 16;
  return r;
}

/*********************************************************************
*
*       _cbSendAttachment()
*
*  Function description
*    Sends the content of an attachment.
*
*  Parameters
*    pContext: SMTPc context.
*    pItem   : Item to send.
*    pAPI    : API used to send the content.
*
*  Return value
*    == 0: O.K., attachment sent.
*    <  0: Error.
*/
static int _cbSendAttachment(IP_SMTPC_CONTEXT* pContext, const IP_SMTPC_MULTIPART_ITEM* pItem, const IP_SMTPC_MULTIPART_API* pAPI) {
  unsigned Index;

  //
  // Send the content of the attachment. Sending can be done in chunks
  // for generated content or e.g. when sending content of a file that
  // is read in chunks.
  //
  Index = (unsigned)pItem->pUserContext;
  pAPI->pfSend(pContext, _aAttachmentContent[Index], strlen(_aAttachmentContent[Index]));
  return 0;
}

/*********************************************************************
*
*       _SMTPcTask()
*
*  Function description
*    SMTP client task that tries to resolve the server name via DNS
*    to send a message as configured.
*/
static void _SMTPcTask(void) {
  int  r;
  int  i;
  int  NumIFaces;
  U8   IFaceReady;

  //
  // Check if sample is correctly configured!
  //
  if(*_MTAConfig.sServer == 0) {
    SMTPC_WARN(("Configuration not valid. Enter valid SMTP server, sender and recipient(s).\n"));
    while (1) {
      OS_Delay(100);
    }
  }
  //
  // Make sure that at least one interface is ready before starting the sample.
  //
  IFaceReady = 0;
  NumIFaces  = IP_INFO_GetNumInterfaces();
  do {
    for (i = 0; i < NumIFaces; i++) {
      if (IP_IFaceIsReadyEx(_IFaceId) != 0) {
        IFaceReady = 1;
        break;
      }
      OS_Delay(100);
    }
  } while (IFaceReady == 0);
  //
  // Send mail.
  //
  r = IP_SMTPC_Send(&_IP_Api, &_aMailAddr[0], SEGGER_COUNTOF(_aMailAddr), &_Message, &_MTAConfig, &_SMTPC_App);
  if (r != 0) {
    SMTPC_WARN(("Mail could not be sent.\n"));
  } else {
    SMTPC_LOG(("Mail sent.\n"));
  }
  while (1) {
    OS_Delay(100);
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
*  Function description
*    Main task executed by the RTOS to create further resources and
*    running the main application.
*/
void MainTask(void) {
  IP_Init();
#if USE_SSL
  SSL_Init();
#endif
  _IFaceId = IP_INFO_GetNumInterfaces() - 1;                                               // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
  OS_SetPriority(OS_GetTaskID(), TASK_PRIO_IP_TASK);                                       // For now, this task has highest prio except IP management tasks.
  OS_CREATETASK(&_IPTCB   , "IP_Task"   , IP_Task   , TASK_PRIO_IP_TASK   , _IPStack);     // Start the IP_Task.
#if USE_RX_TASK
  OS_CREATETASK(&_IPRxTCB , "IP_RxTask" , IP_RxTask , TASK_PRIO_IP_RX_TASK, _IPRxStack);   // Start the IP_RxTask, optional.
#endif
  OS_CREATETASK(&_SMTPcTCB, "SMTPClient", _SMTPcTask, TASK_PRIO_IP_SMTPC  , _SMTPcStack);  // Start the SMTP client.
  IP_AddStateChangeHook(&_StateChangeHook, _OnStateChange);                                // Register hook to be notified on disconnects.
  IP_Connect(_IFaceId);                                                                    // Connect the interface if necessary.
  OS_SetPriority(OS_GetTaskID(), 255);                                                     // Now this task has highest prio for real-time application. This is only allowed when this task does not use blocking IP API after this point.
  while (IP_IFaceIsReadyEx(_IFaceId) == 0) {
    OS_Delay(50);
  }
  while (1) {
    BSP_ToggleLED(1);
    OS_Delay(200);
  }
}

/*************************** End of file ****************************/
