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

File        : SSH_Shell6.c
Purpose     : SSH server that offers simultaneous input and output.

*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "SSH.h"
#include "IP.h"
#include "RTOS.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#define PROMPT                                                        \
  "emSSH> "

#define SIGNON                                                        \
  "\r\n"                                                              \
  "Welcome to the emSSH command line!  Type Ctrl+D to exit.\r\n"      \
  "\r\n"

#define BANNER \
  "\r\n"                                                              \
  "*************************************************************\r\n" \
  "* This server is powered by SEGGER emSSH.  It simply works! *\r\n" \
  "*************************************************************\r\n" \
  "\r\n"

#define USE_RX_TASK 0

/*********************************************************************
*
*       Local data types
*
**********************************************************************
*/

//
// Task priorities
//
enum {
  TASK_PRIO_IP_APP = 150,
  TASK_PRIO_IP_TASK,         // Priority should be higher than all IP application tasks.
  TASK_PRIO_IP_RX_TASK       // Must be the highest priority of all IP related tasks, comment out to read packets in ISR
};

typedef struct {
  volatile unsigned RdPtr;  // Read pointer
  volatile unsigned WrPtr;  // Write pointer
  volatile unsigned RdCnt;  // Number of bytes read
  volatile unsigned WrCnt;  // Number of bytes written
  unsigned          Capacity;
  U8              * pData;
} RING_BUFFER;

typedef struct {
  SSH_SESSION * pSession;
  unsigned      Cursor;
  int           Ready;
  int           Exit;
  int           Socket;
  RING_BUFFER   TxBuffer;
  RING_BUFFER   RxBuffer;
  OS_TASK       ShellTask;
  OS_TASK       ProtocolTask;
  char          aCommandLine       [70];
  U8            aTxData            [1024];
  U8            aRxData            [128];
  U32           aShellTaskStack    [128];
  U32           aProtocolTaskStack [512];
  U8            aTxBuffer          [4096];
  U8            aRxBuffer          [4096];
} SHELL_CONTEXT;

/*********************************************************************
*
*       Prototypes
*
**********************************************************************
*/

void         MainTask            (void);
static void _ShellMain           (void *pContext);
static void _ProtocolMain        (void *pContext);
static void _TerminalChannelClose(SSH_SESSION *pSession, unsigned Channel);
static int  _TerminalChannelData (      SSH_SESSION * pSession,
                                        unsigned      Channel,
                                  const U8          * pData,
                                        unsigned      DataLen);
static int  _Send                (int Socket, const char *pData, int DataLen, int Flags);
static int  _Recv                (int Socket,       char *pData, int DataLen, int Flags);


/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/

static const SSH_TRANSPORT_API _IP_Transport = {
  _Send,
  _Recv,
  closesocket,
};

static const SSH_CHANNEL_API _TerminalAPI = {
  _TerminalChannelData,
  0,
  0,
  _TerminalChannelClose,
};

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

//
// Assign one shell context per session
//
static SHELL_CONTEXT _aShell[SSH_CONFIG_MAX_SESSIONS];

static OS_STACKPTR int _IPStack[2048/sizeof(int)];
static OS_TASK         _IPTCB;

static OS_STACKPTR int _ServerStack[4096/sizeof(int)];
static OS_TASK         _ServerTCB;

#if USE_RX_TASK
static OS_STACKPTR int _IPRxStack[1024/sizeof(int)];
static OS_TASK         _IPRxTCB;
#endif

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _Bind()
*
*  Function description
*    Bind a TCP port.
*
*  Parameters
*    Port - Bound port.
*
*  Return value
*    >= 0 - Bound socket handle.
*    <  0 - Error.
*/
static int _Bind(int Port) {
  int    Socket;
  int    Status;
  int    Enable;
  struct sockaddr_in local_addr;
  //
  local_addr.sin_family = AF_INET;
  local_addr.sin_port = htons(Port);
  local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  //
  Socket = Status = socket(PF_INET, SOCK_STREAM, 0);
  if (Status >= 0) {
    Enable = 1;
    Status = setsockopt(Socket, SOL_SOCKET, SO_REUSEADDR, (char *)&Enable, sizeof(Enable));
  }
  if (Status >= 0) {
    Status = bind(Socket, (struct sockaddr *)&local_addr, sizeof(local_addr));
  }
  if (Status >= 0) {
    Status = listen(Socket, 10);
  }
  return Status >= 0 ? Socket : Status;
}

/*********************************************************************
*
*       _Accept()
*
*  Function description
*    Accept an incoming connection.
*
*  Parameters
*    Socket - Socket bound to port.
*
*  Return value
*    >= 0 - Socket handle.
*    <  0 - Error.
*/
static int _Accept(int Socket) {
  struct sockaddr_in client_addr;
  int                client_addr_len;
  //
  client_addr_len = sizeof(client_addr);
  Socket = accept(Socket, (struct sockaddr *) &client_addr, &client_addr_len);
  if (Socket < 0) {
    return -1;
  } else {
    return Socket;
  }
}

/*********************************************************************
*
*       _Send()
*
*  Function description
*    Send data to socket.
*
*  Parameters
*    Socket  - Socket to write to.
*    pData   - Pointer to octet string to send.
*    DataLen - Octet length of the octet string to send.
*    Flags   - Socket send flags.
*
*  Return value
*    >= 0 - Number of bytes sent.
*    <  0 - Error.
*
*  Additional information
*    The number of bytes sent can be less than the number
*    of bytes that were requested to be written.
*/
static int _Send(int Socket, const char *pData, int DataLen, int Flags) {
  int Status;
  //
  Status = send(Socket, pData, DataLen, Flags);
  //
  return Status < 0 ? -1 : Status;
}

/*********************************************************************
*
*       _Recv()
*
*  Function description
*    Receive data from socket.
*
*  Parameters
*    Socket  - Socket to read from.
*    pData   - Pointer to object that receives an octet string.
*    DataLen - Octet length of the octet string to receive.
*    Flags   - Socket-based additional flags.
*
*  Return value
*    >= 0 - Number of bytes received.
*    <  0 - Error.
*
*  Additional information
*    The number of bytes received can be less than the number
*    of bytes requested.
*/
static int _Recv(int Socket, char *pData, int DataLen, int Flags) {
  int Status;
  //
  Status = recv(Socket, pData, DataLen, Flags);
  //
  return Status < 0 ? -1 : Status;
}

/*********************************************************************
*
*       _CanRead()
*
*  Function description
*    Check if data can be read from socket.
*
*  Parameters
*    Socket  - Socket to query.
*
*  Return value
*    >  0 - Socket has data to read.
*    <= 0 - Socket has no data.
*/
static int _CanRead(int Socket) {
  IP_fd_set readset;
  int       Status;
  //
  IP_FD_ZERO(&readset);
  IP_FD_SET(Socket, &readset);
  Status = select(&readset, 0, 0, 0);
  //
  return Status > 0;
}

/*********************************************************************
*
*       _RING_BUFFER_Init()
*
*  Function description
*    Initialize rung buffer.
*
*  Parameters
*    pRB      - Pointer to ring buffer.
*    pData    - Pointer to data area to hold ring buffer content.
*    Capacity - Capacity of the ring buffer.
*/
static void _RING_BUFFER_Init(RING_BUFFER *pRB, void *pData, unsigned Capacity) {
  pRB->RdPtr    = 0;
  pRB->WrPtr    = 0;
  pRB->RdCnt    = 0;
  pRB->WrCnt    = 0;
  pRB->Capacity = Capacity;
  pRB->pData    = pData;
}

/*********************************************************************
*
*       _RING_BUFFER_QueryCanRead()
*
*  Function description
*    Query whether ring buffer can be read.
*
*  Parameters
*    pRB - Pointer to ring buffer.
*
*  Return value
*    Number of bytes that can be read from the buffer.
*/
static unsigned _RING_BUFFER_QueryCanRead(const RING_BUFFER *pRB) {
  return pRB->WrCnt - pRB->RdCnt;
}

/*********************************************************************
*
*       _RING_BUFFER_QueryCanWrite()
*
*  Function description
*    Query whether ring buffer can be written.
*
*  Parameters
*    pRB - Pointer to ring buffer.
*
*  Return value
*    Number of bytes that can be written to the buffer.
*/
static unsigned _RING_BUFFER_QueryCanWrite(const RING_BUFFER *pRB) {
  return pRB->Capacity - (pRB->WrCnt - pRB->RdCnt);
}

/*********************************************************************
*
*       _RING_BUFFER_Wr()
*
*  Function description
*    Write to ring buffer.
*
*  Parameters
*    pRB     - Pointer to ring buffer.
*    pData   - Pointer to data to write.
*    DataLen - Octet length of the data to write.
*
*  Return value
*    Number of bytes that were be written to the buffer.
*    If the buffer becomes full during writing, the
*    number of bytes written will be less than DataLen.
*/
static unsigned _RING_BUFFER_Wr(      RING_BUFFER * pRB,
                                const void        * pData,
                                      unsigned      DataLen) {
  unsigned N;
  unsigned WrPtr;
  unsigned LChunkLen;
  unsigned RChunkLen;
  //
  // If we don't have enough space to write all of this,
  // write part until the ring buffer is full.
  //
  N = _RING_BUFFER_QueryCanWrite(pRB);
  if (N < DataLen) {
    DataLen = N;
  }
  //
  // Divide into two cases: enough space to write without wrapping,
  // and wrapping required.
  //
  WrPtr = pRB->WrPtr;
  if (WrPtr + DataLen <= pRB->Capacity) {
    memcpy(pRB->pData+WrPtr, pData, DataLen);
    WrPtr += DataLen;
    if (WrPtr == pRB->Capacity) {
      WrPtr = 0;
    }
    pRB->WrPtr = WrPtr;
  } else {
    //
    LChunkLen = pRB->Capacity - WrPtr;
    RChunkLen = DataLen - LChunkLen;
    //
    memcpy(pRB->pData+WrPtr, pData, LChunkLen);
    memcpy(pRB->pData, (U8 *)pData+LChunkLen, RChunkLen);
    //
    pRB->WrPtr = RChunkLen;
  }
  //
  pRB->WrCnt += DataLen;
  //
  return DataLen;
}

/*********************************************************************
*
*       _RING_BUFFER_Rd()
*
*  Function description
*    Read from ring buffer.
*
*  Parameters
*    pRB     - Pointer to ring buffer.
*    pData   - Pointer to object that receives the data.
*    DataLen - Octet length of the data to read.
*
*  Return value
*    Number of bytes that were be read from  the buffer.
*    If the buffer becomes empty during reading, the
*    number of bytes read will be less than DataLen.
*/
static unsigned _RING_BUFFER_Rd(RING_BUFFER * pRB,
                                void        * pData,
                                unsigned      DataLen) {
  unsigned RdPtr;
  unsigned LChunkLen;
  unsigned RChunkLen;
  //
  // If we don't have enough space to write all of this,
  // write part until the ring buffer is full.
  //
  RdPtr = _RING_BUFFER_QueryCanRead(pRB);
  if (RdPtr < DataLen) {
    DataLen = RdPtr;
  }
  //
  // Null read?
  //
  if (DataLen == 0) {
    return DataLen;
  }
  //
  // Divide into two cases: enough space to read without wrapping,
  // and wrapping required.
  //
  RdPtr = pRB->RdPtr;
  if (RdPtr + DataLen <= pRB->Capacity) {
    //
    // Enough space to read, no wrapping required.
    //
    memcpy(pData, pRB->pData+RdPtr, DataLen);
    RdPtr += DataLen;
    if (RdPtr == pRB->Capacity) {
      RdPtr = 0;
    }
    pRB->RdPtr = RdPtr;
  } else {
    //
    // Straddles end of buffer, break into two reads.
    //
    LChunkLen = pRB->Capacity - RdPtr;
    RChunkLen = DataLen - LChunkLen;
    //
    memcpy(pData, pRB->pData+RdPtr, LChunkLen);
    memcpy((char *)pData+LChunkLen, pRB->pData, RChunkLen);
    //
    pRB->RdPtr = RChunkLen;
  }
  //
  pRB->RdCnt += DataLen;
  //
  return DataLen;
}

/*********************************************************************
*
*       _AppExit()
*
*  Function description
*    Exit the application with an error.
*
*  Parameters
*    sReason - Reason for exit, displayed for the user.
*/
static void _AppExit(const char *sReason) {
  SSH_Logf(SSH_LOG_APP, sReason);
  SSH_Panic(-100);
}

/*********************************************************************
*
*       _ShellRequest()
*
*  Function description
*    Handle a shell channel request.
*
*  Parameters
*    pSession - Pointer to session.
*    Channel  - Local channel receiving the data.
*    pParas   - Pointer to channel request parameters.
*
*  Return value
*   >= 0 - Success.
*   <  0 - Error.
*/
static int _ShellRequest(SSH_SESSION               * pSession,
                         unsigned                    Channel,
                         SSH_CHANNEL_REQUEST_PARAS * pParas) {
  SHELL_CONTEXT * pShell;
  int             Status;
  //
  Status = 0;
  if (pParas->WantReply) {
    Status = SSH_CHANNEL_SendSuccess(pSession, Channel);
  }
  //
  pShell = &_aShell[SSH_SESSION_QueryIndex(pSession)];
  SSH_CHANNEL_Config(pSession, Channel, 128, &_TerminalAPI, pShell);
  OS_CreateTaskEx(&pShell->ShellTask,
                  "ShellTask",
                  100,
                  _ShellMain,
                  &pShell->aShellTaskStack,
                  sizeof(pShell->aShellTaskStack),
                  10,
                  pSession);
  //
  return Status;
}

/*********************************************************************
*
*       _TerminalChannelData()
*
*  Function description
*    Handle data received from peer.
*
*  Parameters
*    pSession - Pointer to session.
*    Channel  - Local channel receiving the data.
*    pData    - Pointer to object that contains the data.
*    DataLen  - Octet length of the object that contains the data.
*
*  Return value
*   >= 0 - Success.
*   <  0 - Error.
*
*  Additional information
*    Provide in-callback handling of a command line processor.
*    This sample supports only one connection at a time.
*/
static int _TerminalChannelData(      SSH_SESSION * pSession,
                                      unsigned      Channel,
                                const U8          * pData,
                                      unsigned      DataLen) {
  SHELL_CONTEXT *pShell;
  //
  SSH_USE_PARA(pSession);
  SSH_USE_PARA(Channel);
  //
  pShell = &_aShell[SSH_SESSION_QueryIndex(pSession)];
  _RING_BUFFER_Wr(&pShell->RxBuffer, pData, DataLen);
  OS_WakeTask(&pShell->ShellTask);
  //
  return 0;
}

/*********************************************************************
*
*       _TerminalChannelClose()
*
*  Function description
*    Handle channel closure.
*
*  Parameters
*    pSession - Pointer to session.
*    Channel  - Local channel receiving the data.
*
*  Additional information
*    When the channel carrying the shell is closed, terminate
*    the associated shell task.
*/
static void _TerminalChannelClose(SSH_SESSION * pSession, unsigned Channel) {
  SHELL_CONTEXT *pShell;
  //
  SSH_USE_PARA(Channel);
  //
  pShell = &_aShell[SSH_SESSION_QueryIndex(pSession)];
  OS_TerminateTask(&pShell->ShellTask);
}

/*********************************************************************
*
*       _TerminalRequest()
*
*  Function description
*    Request a terminal.
*
*  Parameters
*    pSession - Pointer to session.
*    Channel  - Local channel requesting the terminal.
*    pParas   - Pointer to channel request parameters.
*
*  Return value
*   >= 0 - Success.
*   <  0 - Error.
*/
static int _TerminalRequest(SSH_SESSION               * pSession,
                            unsigned                    Channel,
                            SSH_CHANNEL_REQUEST_PARAS * pParas) {
  int Status;
  //
  if (pParas->WantReply) {
    Status = SSH_CHANNEL_SendSuccess(pSession, Channel);
  } else {
    Status = 0;
  }
  //
  return Status;
}

/*********************************************************************
*
*       _UserauthServiceRequest()
*
*  Function description
*    Request the user authentication service.
*
*  Parameters
*    pSession     - Pointer to session.
*    sServiceName - Service being requested.
*
*  Return value
*   >= 0 - Success.
*   <  0 - Error.
*
*  Additional information
*    Displays a banner before user authentication commences.
*/
static int _UserauthServiceRequest(SSH_SESSION *pSession, const char *sServiceName) {
  int Status;
  //
  Status = SSH_SESSION_SendServiceAccept(pSession, sServiceName);
  if (Status >= 0) {
    Status = SSH_SESSION_SendUserauthBanner(pSession, BANNER, "en");
  }
  //
  return Status;
}

/*********************************************************************
*
*       _UserauthRequestNone()
*
*  Function description
*    Request authentication of user with method "none".
*
*  Parameters
*    pSession  - Pointer to session.
*    pReqParas - Pointer to user authentication request parameters.
*
*  Return value
*   >= 0 - Success.
*   <  0 - Error.
*/
static int _UserauthRequestNone(SSH_SESSION                * pSession,
                                SSH_USERAUTH_REQUEST_PARAS * pReqParas) {
  SSH_USERAUTH_NONE_PARAS NoneParas;
  int                     Status;
  //
  SSH_USE_PARA(pSession);
  //
  Status = SSH_USERAUTH_NONE_ParseParas(pReqParas, &NoneParas);
  if (Status < 0) {
    Status = SSH_ERROR_USERAUTH_FAIL;
  } else if (pReqParas->UserNameLen == 4 &&
             SSH_MEMCMP(pReqParas->pUserName, "anon", 4) == 0) {
    Status = 0;
  } else {
    Status = SSH_ERROR_USERAUTH_FAIL;
  }
  //
  return Status;
}

/*********************************************************************
*
*       _UserauthRequestPassword()
*
*  Function description
*    Request authentication of user with method "password".
*
*  Parameters
*    pSession  - Pointer to session.
*    pReqParas - Pointer to user authentication request parameters.
*
*  Return value
*   >= 0 - Success.
*   <  0 - Error.
*/
static int _UserauthRequestPassword(SSH_SESSION                * pSession,
                                    SSH_USERAUTH_REQUEST_PARAS * pReqParas) {
  SSH_USERAUTH_PASSWORD_PARAS PasswordParas;
  int                         Status;
  //
  SSH_USE_PARA(pSession);
  //
  Status = SSH_USERAUTH_PASSWORD_ParseParas(pReqParas, &PasswordParas);
  if (Status < 0) {
    Status = SSH_ERROR_USERAUTH_FAIL;
  } else if (pReqParas->UserNameLen == 5 &&
             memcmp(pReqParas->pUserName, "admin", 5) == 0) {
    if (PasswordParas.PasswordLen == 6 &&
        memcmp(PasswordParas.pPassword, "secret", 6) == 0) {
      Status = 0;
    } else {
      Status = SSH_ERROR_USERAUTH_FAIL;
    }
  } else {
    Status = SSH_ERROR_USERAUTH_FAIL;
  }
  //
  return Status;
}

/*********************************************************************
*
*       _Puts()
*
*  Function description
*    Send string to peer.
*
*  Parameters
*    pShell - Pointer to shell context.
*    sText  - String to send.
*/
static void _Puts(SHELL_CONTEXT *pShell, const char *sText) {
  unsigned Len;
  unsigned ChunkLen;
  //
  Len = strlen(sText);
  //
  while (Len > 0) {
    ChunkLen = _RING_BUFFER_Wr(&pShell->TxBuffer, sText, Len);
    sText += ChunkLen;
    Len   -= ChunkLen;
    if (Len != 0) {
      OS_Delay(10000);
    }
  }
}

/*********************************************************************
*
*       _Putc()
*
*  Function description
*    Send character peer.
*
*  Parameters
*    pShell - Pointer to shell context.
*    Ch     - Character to send.
*/
static void _Putc(SHELL_CONTEXT *pShell, U8 Ch) {
  char aBuf[2];
  //
  aBuf[0] = (char)Ch;
  aBuf[1] = 0;
  //
  _Puts(pShell, aBuf);
}

/*********************************************************************
*
*       _Getc()
*
*  Parameters
*    pShell - Pointer to shell context.
*
*  Function description
*    Read character from peer.
*
*  Return value
*    Character read.
*/
static U8 _Getc(SHELL_CONTEXT *pShell) {
  U8 Ch;
  //
  while (_RING_BUFFER_QueryCanRead(&pShell->RxBuffer) == 0) {
    OS_Delay(10000);
  }
  _RING_BUFFER_Rd(&pShell->RxBuffer, &Ch, 1);
  return Ch;
}

/*********************************************************************
*
*       _Gets()
*
*  Function description
*    Read string from peer.
*
*  Parameters
*    pShell - Pointer to shell context.
*
*  Return value
*    Character read.
*/
static int _Gets(SHELL_CONTEXT *pShell) {
  U8 Ch;
  //
  pShell->Cursor = 0;
  //
  for (;;) {
    Ch = _Getc(pShell);
    if (0x20 <= Ch && Ch <= 0x7E) {
      if (pShell->Cursor < sizeof(pShell->aCommandLine)-1) {
        pShell->aCommandLine[pShell->Cursor++] = Ch;
        _Putc(pShell, Ch);
      }
    } else if (Ch == 0x08 || Ch == 0x7F) {
      if (pShell->Cursor > 0) {
        --pShell->Cursor;
        _Puts(pShell, "\b \b");
      }
    } else if (Ch == '\r') {
      pShell->aCommandLine[pShell->Cursor++] = 0;
      return 0;
    } else if (Ch == 0x04) {
      return -1;
    }
  }
}

/*********************************************************************
*
*       _ShellMain()
*
*  Function description
*    Main command line processor acting as a shell.
*
*  Parameters
*    pContext - Pointer to SSH session.
*/
static void _ShellMain(void *pContext) {
  SSH_SESSION   * pSession;
  SHELL_CONTEXT * pShell;
  //
  pSession = pContext;
  pShell = &_aShell[SSH_SESSION_QueryIndex(pSession)];
  //
  _Puts(pShell, SIGNON);
  for (;;) {
    _Puts(pShell, PROMPT);
    if (_Gets(pShell) == 0) {
      _Puts(pShell, "\r\n...");
      _Puts(pShell, pShell->aCommandLine);
      _Puts(pShell, "\r\n");
    } else {
      pShell->Exit = 1;
    }
  }
}

/*********************************************************************
*
*       _ServiceChannel()
*
*  Function description
*    Service a shell channel.
*
*  Parameters
*    pSession - Pointer to SSH session.
*    Channel  - Local channel.
*
*  Return value
*    >  0 - Processed channel.
*    == 0 - Channel required no processing.
*    <  0 - Error.
*/
static int _ServiceChannel(SSH_SESSION *pSession, unsigned Channel) {
  SHELL_CONTEXT * pShell;
  unsigned        Len;
  U8              aData[64];
  int             Status;
  //
  pShell = &_aShell[SSH_SESSION_QueryIndex(pSession)];
  if (pShell == 0) {
    Status = 0;
  } else if (pShell->Exit) {
    if (SSH_CHANNEL_QueryCanWrite(pSession, Channel) >= 12) {
      Status = SSH_CHANNEL_SendData(pShell->pSession, Channel, "\r\n\r\nBye!\r\n\r\n", 12);
    }
    SSH_SESSION_Disconnect(pShell->pSession, SSH_DISCONNECT_BY_APPLICATION);
    Status = -1;
  } else if (_RING_BUFFER_QueryCanRead(&pShell->TxBuffer) > 0) {
    Len = _RING_BUFFER_QueryCanRead(&pShell->TxBuffer);
    Len = SEGGER_MIN(Len, SSH_CHANNEL_QueryCanWrite(pShell->pSession, Channel));
    Len = SEGGER_MIN(Len, sizeof(aData));
    if (Len > 0) {
      _RING_BUFFER_Rd(&pShell->TxBuffer, aData, Len);
      Status = SSH_CHANNEL_SendData(pShell->pSession, Channel, aData, Len);
      OS_WakeTask(&pShell->ShellTask);
    }
    Status = 1;
  } else {
    Status = 0;
  }
  //
  return Status;
}

/*********************************************************************
*
*       _ProtocolMain()
*
*  Function description
*    Handle SSH protocol.
*
*  Parameters
*    pContext - Pointer to SSH session.
*
*  Additional information
*    Acts rather like sshd.
*/
static void _ProtocolMain(void *pContext) {
  SSH_SESSION   * pSession;
  int             Socket;
  int             Status;
  //
  pSession = pContext;
  Socket = SSH_SESSION_QuerySocket(pSession);
  for (;;) {
    Status = 0;
    if (_CanRead(Socket)) {
      Status = SSH_SESSION_Process(pSession);
    } else {
      Status = SSH_SESSION_IterateChannels(pSession, _ServiceChannel);
    }
    if (Status < 0) {
      OS_TerminateTask(0);
    } else if (Status == 0) {
      OS_Delay(10);
    }
  }
}

/*********************************************************************
*
*       _SSHTask()
*
*  Function description
*    Listen for incoming connections and start SSH handler tasks.
*
*  Additional information
*    Acts rather like sshd.
*/
static void _SSHTask(void) {
  int             BoundSocket;
  int             Socket;
  unsigned        SessionIndex;
  SSH_SESSION   * pSession;
  SHELL_CONTEXT * pShell;
  //
  // Hook user authentication to display a banner.  Allow "none"
  // and "password" user authentication.
  //
  SSH_SERVICE_Add(&SSH_SERVICE_USERAUTH, _UserauthServiceRequest);
  SSH_USERAUTH_METHOD_Add(&SSH_USERAUTH_METHOD_NONE,     _UserauthRequestNone);
  SSH_USERAUTH_METHOD_Add(&SSH_USERAUTH_METHOD_PASSWORD, _UserauthRequestPassword);
  //
  // Add support for interactive shells.
  //
  SSH_CHANNEL_REQUEST_Add(&SSH_CHANNEL_REQUEST_SHELL,         _ShellRequest);
  SSH_CHANNEL_REQUEST_Add(&SSH_CHANNEL_REQUEST_ENV,           NULL);
  SSH_CHANNEL_REQUEST_Add(&SSH_CHANNEL_REQUEST_PTYREQ,        _TerminalRequest);
  SSH_CHANNEL_REQUEST_Add(&SSH_CHANNEL_REQUEST_WINDOW_CHANGE, NULL);
  //
  // Bind SSH port.
  //
  BoundSocket = _Bind(22);
  if (BoundSocket < 0) {
    _AppExit("Cannot bind port 22!");
  }
  //
  for (;;) {
    //
    // Try to allocate a session for the next incoming connection.
    //
    SSH_SESSION_Alloc(&pSession);
    if (pSession == NULL) {
      OS_Delay(100);
      continue;
    }
    //
    do {
      Socket = _Accept(BoundSocket);
    } while (Socket < 0);
    //
    SessionIndex = SSH_SESSION_QueryIndex(pSession);
    pShell = &_aShell[SessionIndex];
    memset(pShell, 0, sizeof(*pShell));
    pShell->pSession = pSession;
    SSH_SESSION_Init(pShell->pSession, Socket, &_IP_Transport);
    SSH_SESSION_ConfBuffers(pShell->pSession,
                            pShell->aRxBuffer, sizeof(pShell->aRxBuffer),
                            pShell->aTxBuffer, sizeof(pShell->aTxBuffer));
    _RING_BUFFER_Init(&pShell->RxBuffer, pShell->aRxData, sizeof(pShell->aRxData));
    _RING_BUFFER_Init(&pShell->TxBuffer, pShell->aTxData, sizeof(pShell->aTxData));
    //
    OS_CreateTaskEx(&pShell->ProtocolTask,
                    "ProtocolTask",
                    100,
                    _ProtocolMain,
                    &pShell->aProtocolTaskStack,
                    sizeof(pShell->aProtocolTaskStack),
                    10,
                    pSession);
  }
}

/*********************************************************************
*
*             Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       MainTask()
*
*  Function description
*    Application entry point.
*/
void MainTask(void) {
  //
  int IFaceId;
  IP_Init();
  IFaceId = IP_INFO_GetNumInterfaces() - 1;  // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
  SSH_Init();
  //
  OS_CREATETASK(&_IPTCB,   "IP_Task",   IP_Task,    TASK_PRIO_IP_TASK,    _IPStack);    // Start the IP task
#if USE_RX_TASK
  OS_CREATETASK(&_IPRxTCB, "IP_RxTask", IP_RxTask , TASK_PRIO_IP_RX_TASK, _IPRxStack);  // Start the IP_RxTask, optional.
#endif
  IP_Connect(IFaceId);
  while (IP_IFaceIsReady() == 0) {
    OS_Delay(50);
  }
  //
  // SSH connections may require more stack than MainTask()
  // is given by the canned startup code.  Therefore we start the
  // SSH-based task with more stack so that it runs correctly and
  // kill off MainTask.
  //
  OS_CREATETASK(&_ServerTCB, "SSHTask", _SSHTask, TASK_PRIO_IP_APP, _ServerStack);
  OS_TerminateTask(0);
}

/*************************** End of file ****************************/
