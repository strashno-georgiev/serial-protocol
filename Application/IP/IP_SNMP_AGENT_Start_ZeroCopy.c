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

File    : IP_SNMP_AGENT_Start_ZeroCopy.c
Purpose : Sample program for embOS & TCP/IP & SNMP agent & zero-copy API.

Additional information:
  For compatibility with interfaces that need to connect in any way this
  sample calls connect and disconnect routines that might not be needed
  in all cases.
  This sample can be used for Ethernet and dial-up interfaces and is
  configured to use the last registered interface as its main interface.

  This sample demonstrates use of the IP stack together with the SNMP
  Agent add-on. The SNMP Agent can be tested with the Net-SNMP toolset
  or with any other SNMP manager. A sample MIB has been added to the
  following OID, being able to control 8 LEDs:
  iso(1).org(3).dod(6).internet(1).private(4).enterprise(1).PRIVATE_ENTERPRISE_NUMBER .

  The following indexes Are available for the sample MIB:
    .0  : (get-request only) Get the status of all 8 LEDs in one byte.
          Bit 0 set means LED0 is lit.
    .1-8: Get/Set the status of one LED. .1 indexes LED1. Writing 0 as
          value clears the LED, any other value sets the LED.
*/

#include "RTOS.h"
#include "BSP.h"
#include "IP.h"
#include "IP_SNMP_AGENT.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#define USE_RX_TASK  0  // 0: Packets are read in ISR, 1: Packets are read in a task of its own.

//
// The Private Enterprise Number used by default in this sample is the PEN of SEGEGR Microcontroller GmbH & Co. KG.
// Your own PEN can be acquired from the IANA free of charge. It is not allowed to use the SEGGER PEN in your own product.
//
#define PRIVATE_ENTERPRISE_NUMBER  46410

//
// SNMP TRAP/INFORM receivers list. Configuration for manager #0.
//
#ifndef   TRAP_INFORM_0_HOST
  #define TRAP_INFORM_0_HOST       htonl(IP_BYTES2ADDR(192, 168,  11, 129))  // IP addr. to send to.
#endif

#ifndef   TRAP_INFORM_0_PORT
  #define TRAP_INFORM_0_PORT       162                                       // Port that listens on the manager for messages.
#endif

#ifndef   TRAP_INFORM_0_TYPE
  #define TRAP_INFORM_0_TYPE       IP_SNMP_PDU_TYPE_TRAPV1                   // Type of message to send.
#endif

#ifndef   TRAP_INFORM_0_TIMEOUT
  #define TRAP_INFORM_0_TIMEOUT    1000                                      // Timeout [ms].
#endif

#ifndef   TRAP_INFORM_0_RETRIES
  #define TRAP_INFORM_0_RETRIES    5                                         // Retries for INFORM messages, igonered for other types.
#endif

#ifndef   TRAP_INFORM_0_COMMUNITY
  #define TRAP_INFORM_0_COMMUNITY  &_SNMP_Community_Public                   // Community handle to use.
#endif

//
// SNMP TRAP/INFORM receivers list. Configuration for manager #1.
// Comment out "#define TRAP_INFORM_1_HOST" to remove this manager
// from the recivers list.
//
#ifndef   TRAP_INFORM_1_HOST
  #define TRAP_INFORM_1_HOST       htonl(IP_BYTES2ADDR(192, 168,  11, 129))  // IP addr. to send to.
#endif

#ifndef   TRAP_INFORM_1_PORT
  #define TRAP_INFORM_1_PORT       162                                       // Port that listens on the manager for messages.
#endif

#ifndef   TRAP_INFORM_1_TYPE
  #define TRAP_INFORM_1_TYPE       IP_SNMP_PDU_TYPE_TRAPV2                   // Type of message to send.
#endif

#ifndef   TRAP_INFORM_1_TIMEOUT
  #define TRAP_INFORM_1_TIMEOUT    1000                                      // Timeout [ms].
#endif

#ifndef   TRAP_INFORM_1_RETRIES
  #define TRAP_INFORM_1_RETRIES    5                                         // Retries for INFORM messages, igonered for other types.
#endif

#ifndef   TRAP_INFORM_1_COMMUNITY
  #define TRAP_INFORM_1_COMMUNITY  &_SNMP_Community_Public                   // Community handle to use.
#endif


#ifdef TRAP_INFORM_1_HOST
  #define NUM_TRAP_INFORM_HOSTS  2
#else
  #define NUM_TRAP_INFORM_HOSTS  1
#endif

//
// SNMP agent configuration.
//
#ifndef   MESSAGE_PORT
  #define MESSAGE_PORT              161
#endif

#ifndef   TRAP_INFORM_PORT
  #define TRAP_INFORM_PORT          162
#endif

#ifndef   MAX_SNMP_MESSAGE_SIZE
  #define MAX_SNMP_MESSAGE_SIZE     1472  // MTU(1500) - IPHeader(20) - UDPHeader(8) .
#endif

#ifndef   PUBLIC_COMMUNITY_STRING
  #define PUBLIC_COMMUNITY_STRING   "public"
#endif

#ifndef   PRIVATE_COMMUNITY_STRING
  #define PRIVATE_COMMUNITY_STRING  "Segger"
#endif

//
// Task priorities.
//
enum {
   TASK_PRIO_IP_SNMP_AGENT_MESSAGE = 150
  ,TASK_PRIO_IP_SNMP_AGENT  // Should have highest SNMP agent priority as this is the management task.
  ,TASK_PRIO_IP_TASK        // Priority should be higher than all IP application tasks.
#if USE_RX_TASK
  ,TASK_PRIO_IP_RX_TASK     // Must be the highest priority of all IP related tasks.
#endif
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

static void  _SNMP_Init                        (void);
static void  _SNMP_DeInit                      (void);
static void  _SNMP_Lock                        (void);
static void  _SNMP_Unlock                      (void);
static void* _SNMP_AllocSendBuffer             (void* pUserContext, U8** ppBuffer, U32 NumBytes, U8 IPAddrLen);
static void  _SNMP_FreeSendBuffer              (void* pUserContext, void* p, char SendCalled, int r);
static int   _SNMP_SendTrapInform              (void* pContext, void* pUserContext, void* hBuffer, const U8* pData, U32 NumBytes, U8* pIPAddr, U16 Port, U8 IPAddrLen);
static U32   _SNMP_GetTime                     (void);
static U32   _SNMP_SysTicks2SnmpTime           (U32 SysTicks);
static U32   _SNMP_SnmpTime2SysTicks           (U32 SnmpTime);

static U32   _SNMP_Mib2System_GetSysUpTime     (void);
static int   _SNMP_Mib2System_GetSetSysContact (char* pBuffer, U32* pNumBytes, char IsWrite);
static int   _SNMP_Mib2System_GetSetSysName    (char* pBuffer, U32* pNumBytes, char IsWrite);
static int   _SNMP_Mib2System_GetSetSysLocation(char* pBuffer, U32* pNumBytes, char IsWrite);

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static const U8 _aOID_IsoOrgDodInternetPrivateEnterprise[] = { 0x2B, 0x06, 0x01, 0x04, 0x01 };                    // iso(1).org(3).dod(6).internet(1).private(4).enterprise(1) .
static const U8 _SnmpTrapColdStartOID[]                    = IP_SNMP_GENERIC_TRAP_OID_COLD_START;

static const U8 _abEnterpriseOID[]                         = { 0x2B, 0x06, 0x01, 0x04, 0x01, 0x82, 0xEA, 0x4A };  // iso(1).org(3).dod(6).internet(1).private(4).enterprise(1).ENCODED(PRIVATE_ENTERPRISE_NUMBER) .
#if (PRIVATE_ENTERPRISE_NUMBER != 46410)
  //
  // The byte representation in BER encoding for an OID can easily be converted
  // from its decimal number using the IP_SNMP_AGENT_EncodeOIDValue() API.
  //
  #error Please change the enterprise ID of _abEnterpriseOID manually
#endif

//
// API callbacks to feed the SNMP Agent with user information.
//
static const IP_SNMP_AGENT_API _SNMPAgentApi = {
  _SNMP_Init,               // pfInit, typically used to initialize a lock API.
  _SNMP_DeInit,             // pfDeInit, typically used to deinitialize a lock API.
  _SNMP_Lock,               // pfLock .
  _SNMP_Unlock,             // pfUnlock .
  _SNMP_AllocSendBuffer,    // pfAllocSendBuffer .
  _SNMP_FreeSendBuffer,     // pfFreeSendBuffer .
  _SNMP_SendTrapInform,     // pfSendTrapInform .
  _SNMP_GetTime,            // pfGetTime .
  _SNMP_SysTicks2SnmpTime,  // pfSysTicks2SnmpTime .
  _SNMP_SnmpTime2SysTicks   // pfSnmpTime2SysTicks .
};

static const IP_SNMP_AGENT_MIB2_SYSTEM_API _SNMPAgentMib2SystemApi = {
  "emNet SNMP Agent with emNet and embOS",  // system.sysDescr .
  _abEnterpriseOID,                         // system.sysObjectID, pointer to oid value .
  SEGGER_COUNTOF(_abEnterpriseOID),         // system.sysObjectID, length of oid value .
  _SNMP_Mib2System_GetSysUpTime,            // system.sysUpTime .
  _SNMP_Mib2System_GetSetSysContact,        // system.sysContact .
  _SNMP_Mib2System_GetSetSysName,           // system.sysName .
  _SNMP_Mib2System_GetSetSysLocation,       // system.sysLocation .
  72                                        // sysServices .
};

//
// Permissions that are assigned to a registered community granting
// access rights. Specific OIDs not entered here inherit permissions
// from higher levels.
// The last line has to be present. The permissions of this line will
// be the default permissions if there are no parent OIDs with permissions.
//

#ifdef __GNUC__
  #pragma GCC diagnostic ignored "-Wmissing-braces"  // Avoid warning for missing braces with GCC as the GCC seems to think there is a problem with assigning values.
#endif

static const IP_SNMP_AGENT_PERM _aSNMP_Perm_Public[] = {
  // Array containing OID                     , size of array                                   , Permissions to grant.
  &_aOID_IsoOrgDodInternetPrivateEnterprise[0], sizeof(_aOID_IsoOrgDodInternetPrivateEnterprise), IP_SNMP_AGENT_PERM_READ_MASK,
  NULL                                        , 0                                               , IP_SNMP_AGENT_PERM_READ_MASK
};

static const IP_SNMP_AGENT_PERM _aSNMP_Perm_Private[] = {
  // Array containing OID, size of array           , Permissions to grant.
  &_abEnterpriseOID[0]   , sizeof(_abEnterpriseOID), IP_SNMP_AGENT_PERM_READ_MASK | IP_SNMP_AGENT_PERM_WRITE_MASK,
  NULL                   , 0                       , IP_SNMP_AGENT_PERM_READ_MASK | IP_SNMP_AGENT_PERM_WRITE_MASK
};

//
// Generic IP variables.
//
static IP_HOOK_ON_STATE_CHANGE _StateChangeHook;
static int                     _IFaceId;

//
// Task stacks and Task-Control-Blocks.
//
static OS_STACKPTR int _IPStack[(TASK_STACK_SIZE_IP_TASK + 256)/sizeof(int)];  // Stack of the IP_Task.
static OS_TASK         _IPTCB;                                                 // Task-Control-Block of the IP_Task.

#if USE_RX_TASK
static OS_STACKPTR int _IPRxStack[TASK_STACK_SIZE_IP_RX_TASK/sizeof(int)];     // Stack of the IP_RxTask.
static OS_TASK         _IPRxTCB;                                               // Task-Control-Block of the IP_RxTask.
#endif

//
// SNMP API locking resources.
//
static OS_RSEMA _Lock;

//
// Memory blocks for SNMP community.
//
static IP_SNMP_AGENT_COMMUNITY _SNMP_Community_Public;
static IP_SNMP_AGENT_COMMUNITY _SNMP_Community_Private;

//
// Memory blocks for custom MIBs.
//
static IP_SNMP_AGENT_MIB _SNMP_SampleMIB;

//
// SNMP sample variables.
//
static U8 _LEDState;

//
// TRAP/INFORM receivers list that will receive a boot TRAP/INFORM
// message once the SNMP agent has been initialized and is ready to
// handle incoming requests.
//
static IP_SNMP_AGENT_TRAP_INFORM_CONTEXT _aTrapInformConfig[NUM_TRAP_INFORM_HOSTS];

//
// TRAP/INFORM memory to use.
//
static IP_SNMP_AGENT_HOOK_ON_INFORM_RESPONSE _OnInformResponseHook;
static IP_SNMP_AGENT_CONTEXT                 _TrapInformVarbindContext;
static U8                                    _abTrapInformVarbindBuffer[1472];  // Buffer where the TRAP/INFORM Varbinds are temporarily stored.

//
// MIB-II System memory for holding information strings.
//
static char _Mib2SystemSysContact[256]  = "support@segger.com";    // For more information please refer to http://www.alvestrand.no/objectid/1.3.6.1.2.1.1.4.html .
static char _Mib2SystemSysName[256]     = "snmpagent.segger.com";  // For more information please refer to http://www.alvestrand.no/objectid/1.3.6.1.2.1.1.5.html .
static char _Mib2SystemSysLocation[256] = "Your desktop";          // For more information please refer to http://www.alvestrand.no/objectid/1.3.6.1.2.1.1.6.html .

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
*       _SNMP_Init()
*
*  Function description
*    Init locking mechanism used by the SNMP agent API.
*/
static void _SNMP_Init(void) {
  OS_CREATERSEMA(&_Lock);
}

/*********************************************************************
*
*       _SNMP_DeInit()
*
*  Function description
*    De-init locking mechanism used by the SNMP agent API.
*/
static void _SNMP_DeInit(void) {
  OS_DeleteRSema(&_Lock);
}

/*********************************************************************
*
*       _SNMP_Lock()
*
*  Function description
*    Acquires the lock to ensure SNMP API is thread safe.
*/
static void _SNMP_Lock(void) {
  OS_Use(&_Lock);
}

/*********************************************************************
*
*       _SNMP_Unlock()
*
*  Function description
*    Releases a previously acquired lock for thread safety SNMP API.
*/
static void _SNMP_Unlock(void) {
  OS_Unuse(&_Lock);
}

/*********************************************************************
*
*       _SNMP_AllocSendBuffer()
*
*  Function description
*    Allocates a buffer that is used to store the complete message
*    to send.
*
*  Parameters
*    pUserContext: User specific context set when creating the context.
*    ppBuffer    : Pointer where to store the data pointer to the buffer.
*    NumBytes    : Size of the buffer to allocate for our message.
*    IPAddrLen   : Length of the IP address used to send to the next host.
*                  A length of 4 means an IPv4 host to send to, a length
*                  of 16 means sending to an IPv6 host.
*
*  Return value
*    Error: NULL
*    O.K. : Handle to allocated buffer. Might be the same as stored in ppBuffer.
*
*  Additional information
*    The buffer can be a static buffer that can then be sent using the
*    socket interface. Another option would be to allocate a zero-copy
*    packet to avoid unnecessary copy operations.
*/
static void* _SNMP_AllocSendBuffer(void* pUserContext, U8** ppBuffer, U32 NumBytes, U8 IPAddrLen) {
  IP_PACKET* pPacket;

  IP_USE_PARA(pUserContext);
  IP_USE_PARA(IPAddrLen);

  pPacket = IP_UDP_AllocEx(_IFaceId, NumBytes);
  if (pPacket == NULL) {
    IP_SNMP_AGENT_WARN(("Unable to alloc packet for TRAP/INFORM."));
  } else {
    *ppBuffer = pPacket->pData;
  }
  return pPacket;
}

/*********************************************************************
*
*       _SNMP_FreeSendBuffer()
*
*  Function description
*    Frees a send buffer that has been previously allocated.
*
*  Parameters
*    pUserContext: User specific context set when creating the context.
*    p           : Handle to free.
*    SendCalled  : Was the send callback used ?
*    r           : Return value of the send callback if it has been used.
*
*  Additional information
*    If the allocated buffer was a zero-copy buffer, the free operation
*    is typically handled by the stack on sending and this routine does
*    not have to do anything at all. The same applies to a static buffer.
*/
static void _SNMP_FreeSendBuffer(void* pUserContext, void* p, char SendCalled, int r) {
  IP_USE_PARA(pUserContext);
  IP_USE_PARA(r);

  //
  // Only free the buffer if it has not been done by the send routine.
  //
  if (SendCalled == 0) {
    IP_UDP_Free((IP_PACKET*)p);
  }
}

/*********************************************************************
*
*       _SNMP_SendTrapInform()
*
*  Function description
*    Sends a TRAP/INFORM message.
*
*  Parameters
*    pContext    : Send context, typically a socket.
*    pUserContext: User specific context set when creating the context.
*    hBuffer     : Pointer to the buffer handle. Might be an IP_PACKET pointer.
*    pData       : Pointer to the data to send.
*    NumBytes    : Length of message to send.
*    pIPAddr     : Pointer to the IP address to send to in network order.
*    Port        : Foreign port to send to in network order.
*    IPAddrLen   : Length of the IP address at pIPAddr. A length of 4
*                  means an IPv4 host to send to, a length of 16 means
*                  sending to an IPv6 host.
*
*  Return value
*    O.K. : >= 0
*    Error: <  0
*/
static int _SNMP_SendTrapInform(void* pContext, void* pUserContext, void* hBuffer, const U8* pData, U32 NumBytes, U8* pIPAddr, U16 Port, U8 IPAddrLen) {
  IP_PACKET* pPacket;
  int        r;

  IP_USE_PARA(pContext);
  IP_USE_PARA(pUserContext);
  IP_USE_PARA(IPAddrLen);

  pPacket = (IP_PACKET*)hBuffer;
  IP_MEMCPY(pPacket->pData, pData, NumBytes);
  pPacket->NumBytes = NumBytes;
  r = IP_UDP_SendAndFree(_IFaceId, *(U32*)pIPAddr, Port, TRAP_INFORM_PORT, pPacket);
  return r;
}

/*********************************************************************
*
*       _SNMP_GetTime()
*
*  Function description
*    Retrieves the current system time in milliseconds.
*
*  Return value
*    System time [ms].
*/
static U32 _SNMP_GetTime(void) {
  return OS_GetTime32();
}

/*********************************************************************
*
*       _SNMP_SysTicks2SnmpTime()
*
*  Function description
*    Converts a timestamp in system ticks (typically 1ms) into a
*    1/100 seconds SNMP timestamp.
*
*  Return value
*    Tick counts in 1/100 seconds.
*/
static U32 _SNMP_SysTicks2SnmpTime(U32 SysTicks) {
  return (SysTicks / 10);
}

/*********************************************************************
*
*       _SNMP_SnmpTime2SysTicks()
*
*  Function description
*    Converts 1/100 seconds SNMP timestamp into a system tick
*    timestamp (typically 1ms).
*
*  Return value
*    System ticks.
*/
static U32 _SNMP_SnmpTime2SysTicks(U32 SnmpTime) {
  return (SnmpTime * 10);
}

/*********************************************************************
*
*       _SNMP_Mib2System_GetSysUpTime()
*
*  Function description
*    Retrieves the tick count of the system in 1/100 seconds since
*    the network management portion of the system was last
*    re-initialized.
*
*  Return value
*    Tick counts in 1/100 seconds since re-init.
*/
static U32 _SNMP_Mib2System_GetSysUpTime(void) {
  //
  // For simplicity this is the time since boot.
  //
  return _SNMP_SysTicks2SnmpTime(_SNMP_GetTime());
}

/*********************************************************************
*
*       _SNMP_Mib2System_GetSetSysContact()
*
*  Function description
*    Retrieves/sets the system.sysContact information.
*
*  Parameters
*    pBuffer  : IsWrite == 0: Pointer to buffer where to store the string.
*               IsWrite == 1: Pointer to buffer where to read a string.
*    pNumBytes: IsWrite == 0: Pointer to size of the buffer at pBuffer and where to store the length of the string (without termination).
*               IsWrite == 1: Pointer where to find the length of the string (without termination).
*    IsWrite  : 0: Variable read, other parameters are used for output.
*               1: Variable write, other parameters are used for input.
*
*  Return value
*    O.K. : 0
*    Error: Other
*
*  Additional information
*    - On IsWrite==1 values that shall be used later on need to be preserved manually.
*    - Max. allowed string length is up to 255 printable characters.
*    - http://www.alvestrand.no/objectid/1.3.6.1.2.1.1.4.html
*/
static int _SNMP_Mib2System_GetSetSysContact(char* pBuffer, U32* pNumBytes, char IsWrite) {
  U32 NumBytes;

  if (IsWrite == 0) {
    //
    // Read value for sending.
    //
    NumBytes   = strlen(_Mib2SystemSysContact);            // Count how long our string is.
    NumBytes   = SEGGER_MIN(NumBytes, *pNumBytes);         // Limit to the buffer size.
    memcpy(pBuffer, &_Mib2SystemSysContact[0], NumBytes);  // Copy string.
    *pNumBytes = NumBytes;                                 // Store the length of the string.
  } else {
    //
    // Save value received.
    //
    NumBytes = SEGGER_MIN((sizeof(_Mib2SystemSysContact) - 1), *pNumBytes);  // Calc. how many bytes shall be stored/can be stored.
    memcpy(&_Mib2SystemSysContact[0], pBuffer, NumBytes);                    // Copy string.
    _Mib2SystemSysContact[NumBytes] = '\0';                                  // Terminate string.
  }
  return 0;  // O.K.
}

/*********************************************************************
*
*       _SNMP_Mib2System_GetSetSysName()
*
*  Function description
*    Retrieves/sets the system.sysName information.
*
*  Parameters
*    pBuffer  : IsWrite == 0: Pointer to buffer where to store the string.
*               IsWrite == 1: Pointer to buffer where to read a string.
*    pNumBytes: IsWrite == 0: Pointer to size of the buffer at pBuffer and where to store the length of the string (without termination).
*               IsWrite == 1: Pointer where to find the length of the string (without termination).
*    IsWrite  : 0: Variable read, other parameters are used for output.
*               1: Variable write, other parameters are used for input.
*
*  Return value
*    O.K. : 0
*    Error: Other
*
*  Additional information
*    - On IsWrite==1 values that shall be used later on need to be preserved manually.
*    - Max. allowed string length is up to 255 printable characters.
*    - http://www.alvestrand.no/objectid/1.3.6.1.2.1.1.5.html
*/
static int _SNMP_Mib2System_GetSetSysName(char* pBuffer, U32* pNumBytes, char IsWrite) {
  U32 NumBytes;

  if (IsWrite == 0) {
    //
    // Read value for sending.
    //
    NumBytes   = strlen(_Mib2SystemSysName);            // Count how long our string is.
    NumBytes   = SEGGER_MIN(NumBytes, *pNumBytes);      // Limit to the buffer size.
    memcpy(pBuffer, &_Mib2SystemSysName[0], NumBytes);  // Copy string.
    *pNumBytes = NumBytes;                              // Store the length of the string.
  } else {
    //
    // Save value received.
    //
    NumBytes = SEGGER_MIN((sizeof(_Mib2SystemSysName) - 1), *pNumBytes);  // Calc. how many bytes shall be stored/can be stored.
    memcpy(&_Mib2SystemSysName[0], pBuffer, NumBytes);                    // Copy string.
    _Mib2SystemSysName[NumBytes] = '\0';                                  // Terminate string.
  }
  return 0;  // O.K.
}

/*********************************************************************
*
*       _SNMP_Mib2System_GetSetSysLocation()
*
*  Function description
*    Retrieves/sets the system.sysLocation information.
*
*  Parameters
*    pBuffer  : IsWrite == 0: Pointer to buffer where to store the string.
*               IsWrite == 1: Pointer to buffer where to read a string.
*    pNumBytes: IsWrite == 0: Pointer to size of the buffer at pBuffer and where to store the length of the string (without termination).
*               IsWrite == 1: Pointer where to find the length of the string (without termination).
*    IsWrite  : 0: Variable read, other parameters are used for output.
*               1: Variable write, other parameters are used for input.
*
*  Return value
*    O.K. : 0
*    Error: Other
*
*  Additional information
*    - On IsWrite==1 values that shall be used later on need to be preserved manually.
*    - Max. allowed string length is up to 255 printable characters.
*    - http://www.alvestrand.no/objectid/1.3.6.1.2.1.1.6.html
*/
static int _SNMP_Mib2System_GetSetSysLocation(char* pBuffer, U32* pNumBytes, char IsWrite) {
  U32 NumBytes;

  if (IsWrite == 0) {
    //
    // Read value for sending.
    //
    NumBytes   = strlen(_Mib2SystemSysLocation);            // Count how long our string is.
    NumBytes   = SEGGER_MIN(NumBytes, *pNumBytes);          // Limit to the buffer size.
    memcpy(pBuffer, &_Mib2SystemSysLocation[0], NumBytes);  // Copy string.
    *pNumBytes = NumBytes;                                  // Store the length of the string.
  } else {
    //
    // Save value received.
    //
    NumBytes = SEGGER_MIN((sizeof(_Mib2SystemSysLocation) - 1), *pNumBytes);  // Calc. how many bytes shall be stored/can be stored.
    memcpy(&_Mib2SystemSysLocation[0], pBuffer, NumBytes);                    // Copy string.
    _Mib2SystemSysLocation[NumBytes] = '\0';                                  // Terminate string.
  }
  return 0;  // O.K.
}

/*********************************************************************
*
*       _SNMP_Sample_cb()
*
*  Function description
*    Callback handler that can be assign to one or more MIBs.
*
*  Parameters
*    pContext    : Context for the current message and response.
*    pUserContext: User specific context passed to the process message API.
*    pMIB        : Pointer to the MIB that is currently accessed.
*    MIBLen      : Length of the MIB that is currently accessed.
*    pIndex      : Pointer to the encoded index of the OID. Might be NULL
*                  in case of a getnext-request.
*    IndexLen    : Length of the encoded index in bytes.
*    RequestType : IP_SNMP_PDU_SET_REQUEST or
*                  IP_SNMP_PDU_GET_REQUEST or
*                  IP_SNMP_PDU_TYPE_GET_NEXT_REQUEST .
*    VarType     : Type of variable that waits to be parsed for input data.
*                  Only valid if RequestType is IP_SNMP_PDU_SET_REQUEST .
*
*  Return value
*    Everything O.K.    : IP_SNMP_OK
*    In case of an error: IP_SNMP_ERR_*
*
*  Additional information
*    - pIndex might point to more than one index OID value e.g. when
*      using multi dimensional arrays. In any case the index should
*      be decoded before it is used to make sure values above 127
*      are correctly used.
*    - Parameters of a set-request need to be stored back with their new value.
*    - The memory that pMIB and pIndex point to might be reused by store
*      functions. If the data stored at their location is important you
*      have to save them locally on your own. It is advised to do all
*      checks at the beginning of the callback so you do not rely on these
*      parameters while or after you use store functions.
*/
static int _SNMP_Sample_cb(IP_SNMP_AGENT_CONTEXT* pContext, void* pUserContext, const U8* pMIB, U32 MIBLen, const U8* pIndex, U32 IndexLen, U8 RequestType, U8 VarType) {
  I32 OnOff;
  U32 Index;
  U32 NumBytesDecoded;
  U8  LEDMask;

  (void)pUserContext;  // Context passed through the whole SNMP API by the application.
  (void)pMIB;          // OID part with the address of the MIB found.
  (void)MIBLen;        // Length of the MIB OID.

  Index           = 0u;
  LEDMask         = 0u;
  NumBytesDecoded = IndexLen;

  if (pIndex != NULL) {
    //
    // Decode index if available. Typically the only case where it is not available
    // is when the first index available is requested for a getnext-request.
    //
    if (IP_SNMP_AGENT_DecodeOIDValue(pIndex, &NumBytesDecoded, &Index, &pIndex) != 0) {
      return IP_SNMP_ERR_GENERIC;
    }
  }
  if (RequestType == IP_SNMP_PDU_TYPE_GET_NEXT_REQUEST) {  // Handle getnext-request.
    //
    // A getnext-request has to provide the next indexed item after the one addressed.
    // As for this sample we expect to have only a one dimensional index this is simply
    // incrementing the read index by one.
    // For a getnext-request the callback has to store the OID value of the new item
    // as well.
    // In general the callback has the following options how to react:
    // 1) The MIB callback is not provided with an index                           : The callback has to store the first available OID and its value.
    // 2) The MIB callback is     able to store the next item after the given index: The callback has to store the OID and the value of the next item.
    // 3) The MIB callback is NOT able to store the next item after the given index: The callback has to store an NA exception or to report an IP_SNMP_ERR_NO_CREATION error.
    //
    if (pIndex == NULL) {  // Case 1).
      Index = 0;           // Return the first index available.
    } else {
      Index++;             // Return the next index available after the one provided.
    }
  }
  //
  // Do some checks.
  //
  if ((Index > 8) ||                   // This sample is designed for an index of 0..8 where 0 is the status of all LEDs and 1..8 is a LED index.
      (IndexLen > NumBytesDecoded)) {  // We expect to have only one index, if there are more index bytes to parse this means an error (trying to access .x.y where only .x is available).
    if (IP_SNMP_AGENT_StoreInstanceNA(pContext) != 0) {
      return IP_SNMP_ERR_TOO_BIG;
    }
    return IP_SNMP_OK;
    //
    // As alternate IP_SNMP_ERR_NO_CREATION can be returned but
    // will abort processing of the VarbindList.
    //
    // return IP_SNMP_ERR_NO_CREATION;  // This resource does not exist.
  }
  if (Index != 0) {
    LEDMask = (1 << (Index - 1));
  }
  //
  // Process get-, getnext- or set-request.
  //
  if (RequestType == IP_SNMP_PDU_TYPE_SET_REQUEST) {  // Handle set-request.
    //
    // Check that the parameter type of the variable is what we expect.
    //
    if (VarType != IP_SNMP_TYPE_INTEGER) {
      return IP_SNMP_ERR_WRONG_TYPE;
    }
    if (IP_SNMP_AGENT_ParseInteger(pContext, &OnOff) != 0) {
      return IP_SNMP_ERR_GENERIC;
    }
    //
    // Set LED state based on index:
    //   Index 0: Invalid.
    //   Other  : Set LED state for one specific LED by one byte.
    //
    if (Index == 0) {
      return IP_SNMP_ERR_NO_ACCESS;
    } else {
      //
      // Set status of one LED.
      //
      if (OnOff != 0) {  // On ?
        _LEDState |= LEDMask;
        BSP_SetLED(Index - 1);
      } else {
        _LEDState &= (LEDMask ^ 255);
        BSP_ClrLED(Index - 1);
      }
      if (IP_SNMP_AGENT_StoreInteger(pContext, OnOff) != 0) {
        return IP_SNMP_ERR_TOO_BIG;
      }
    }
  } else {                                            // Handle get-request.
    if (RequestType == IP_SNMP_PDU_TYPE_GET_NEXT_REQUEST) {
      //
      // Store OID of "next" item returned if this is for a getnext-request.
      // The value will be stored by the following code in case of a getnext-
      // and a get-request.
      //
      if (IP_SNMP_AGENT_StoreCurrentMibOidAndIndex(pContext, pContext, 1, Index) != 0) {
        return IP_SNMP_ERR_TOO_BIG;
      }
    }
    //
    // Get LED state based on index:
    //   Index 0: Get LED state for all 8 possible LEDs in one byte.
    //   Other  : Get LED state for one specific LED in one byte.
    //
    if (Index == 0) {
      if (IP_SNMP_AGENT_StoreInteger(pContext, _LEDState) != 0) {
        return IP_SNMP_ERR_TOO_BIG;
      }
    } else {
      //
      // Return status of one LED.
      //
      if ((_LEDState & LEDMask) != 0) {
        OnOff = 1;
      } else {
        OnOff = 0;
      }
      if (IP_SNMP_AGENT_StoreInteger(pContext, OnOff) != 0) {
        return IP_SNMP_ERR_TOO_BIG;
      }
    }
  }
  return IP_SNMP_OK;
}

/*********************************************************************
*
*       _SNMP_AGENT_SendBootTrap()
*
*  Function description
*    Sends a boot trap to a list of managers. This shall be done once
*    the SNMP agent is up and running to inform managers about it.
*
*  Additional information
*    This can be done to inform SNMP managers that our agent is now
*    up and running. This is a sample implementation that by default
*    configuration of this sample sends a coldBoot trap with one
*    additional custom Varbind to the managers.
*/
static void _SNMP_AGENT_SendBootTrap(void) {
  U32      IPAddr;
  unsigned u;
  int      r;
  U8       ab[sizeof(_abEnterpriseOID) + 1];

  //
  // Prepare TRAP/INFORM receivers list.
  //
  IPAddr = TRAP_INFORM_0_HOST;
  memcpy(&_aTrapInformConfig[0].IPAddr[0], &IPAddr, sizeof(TRAP_INFORM_0_HOST));
  _aTrapInformConfig[0].IPAddrLen    = sizeof(TRAP_INFORM_0_HOST);
  _aTrapInformConfig[0].Port         = TRAP_INFORM_0_PORT;
  _aTrapInformConfig[0].Type         = TRAP_INFORM_0_TYPE;
  _aTrapInformConfig[0].pCommunity   = TRAP_INFORM_0_COMMUNITY;
  _aTrapInformConfig[0].Timeout      = TRAP_INFORM_0_TIMEOUT;
  _aTrapInformConfig[0].Retries      = TRAP_INFORM_0_RETRIES;
#ifdef TRAP_INFORM_1_HOST
  IPAddr = TRAP_INFORM_1_HOST;
  memcpy(&_aTrapInformConfig[1].IPAddr[0], &IPAddr, sizeof(TRAP_INFORM_1_HOST));
  _aTrapInformConfig[1].IPAddrLen    = sizeof(TRAP_INFORM_1_HOST);
  _aTrapInformConfig[1].Port         = TRAP_INFORM_1_PORT;
  _aTrapInformConfig[1].Type         = TRAP_INFORM_1_TYPE;
  _aTrapInformConfig[1].pCommunity   = TRAP_INFORM_1_COMMUNITY;
  _aTrapInformConfig[1].Timeout      = TRAP_INFORM_1_TIMEOUT;
  _aTrapInformConfig[1].Retries      = TRAP_INFORM_1_RETRIES;
#endif
  //
  // Prepare TRAP/INFORM context and custom Varbinds to send.
  // The TRAP/INFORM context can be reused to send the same Varbinds to multiple managers.
  //
  IP_SNMP_AGENT_PrepareTrapInform(&_TrapInformVarbindContext, NULL, &_abEnterpriseOID[0], sizeof(_abEnterpriseOID), &_SnmpTrapColdStartOID[0], sizeof(_SnmpTrapColdStartOID), &_abTrapInformVarbindBuffer[0], sizeof(_abTrapInformVarbindBuffer), IP_GetIPAddr(_IFaceId));
  //
  // Store one custom Varbind (OID & value).
  //
  memcpy(&ab[0], &_abEnterpriseOID[0], sizeof(_abEnterpriseOID));
  IP_SNMP_AGENT_OpenVarbind(&_TrapInformVarbindContext);                                                           // Step 1: Open Varbind (store Varbind header).
  ab[sizeof(_abEnterpriseOID)] = 0;                                                                                // Set index of OID to store to .0 .
  IP_SNMP_AGENT_StoreOID(&_TrapInformVarbindContext, (const U8*)ab, sizeof(_abEnterpriseOID) + 1, sizeof(ab), 0);  // Step 2: Store the OID value into the Varbind.
  IP_SNMP_AGENT_StoreInteger(&_TrapInformVarbindContext, 12345);                                                   // Step 3: Store the value sent in the Varbind.
  IP_SNMP_AGENT_CloseVarbind(&_TrapInformVarbindContext);                                                          // Step 4: Close the Varbind (calculate Varbind length and enter it into the Varbind header). Repeat step 1 - 4 to store further Varbinds.
  //
  // Send TRAP/INFORM messages to agents.
  //
  for (u = 0; u < SEGGER_COUNTOF(_aTrapInformConfig); u++) {
    r = IP_SNMP_AGENT_SendTrapInform(NULL, &_TrapInformVarbindContext, &_aTrapInformConfig[u]);
    if        (r <  0) {
      //
      // Error.
      //
      IP_SNMP_AGENT_WARN(("Failed to send TRAP/INFORM."));
      //
      // Sending a message can fail at several stages.
      // As the message might still be queued for sending
      // IP_SNMP_AGENT_CancelInform() should be called to
      // make sure resending is stopped and the message
      // really is unlinked from all queues.
      //
      IP_SNMP_AGENT_CancelInform(&_aTrapInformConfig[u]);
    } else if (r == 0) {
      //
      // Message successfully sent. If the receiver settings have
      // been allocated they can now be freed.
      //
      IP_SNMP_AGENT_LOG(("TRAP sent. Varbind context and config can be freed."));
    } else if (r == 1) {
      //
      // Message successfully sent. The receiver settings need to
      // be preserved until IP_SNMP_AGENT_CheckInformStatus() returns
      // that work on the receiver has been done or a hook registered
      // with IP_SNMP_AGENT_AddInformReponseHook() signals this item,
      // either with success or with an error.
      //
      IP_SNMP_AGENT_LOG(("INFORM sent. Varbind context and config need to be preserved."));
    }
  }
}

/*********************************************************************
*
*       _SNMP_AGENT_OnRx()
*
*  Function description
*    OnRx callback that receives incoming INFORM responses and handles
*    SNMP request messages and sending back to them.
*
*  Parameters
*    pInPacket: Pointer to packet.
*    pContext : Pointer to context if any (typically NULL).
*
*  Return value
*    Packet is valid                  : IP_OK
*    Packet is invalid for some reason: IP_RX_ERROR
*/
static int _SNMP_AGENT_OnRx(IP_PACKET* pInPacket, void* pContext) {
  const  U8*        pInData;
         U8*        pOutData;
         IP_PACKET* pOutPacket;
         U32        IPAddr;
         U32        MaxPacketSize;
         int        ResponseLen;
         U16        LPort;
         U16        FPort;
         U16        NumBytes;

  IP_USE_PARA(pContext);

  //
  // Get information about the data received. One of them is the port on which
  // the packet came in which can be used to check if it is a request or an INFORM ACK.
  //
  IP_UDP_GetSrcAddr(pInPacket, &IPAddr, sizeof(IPAddr));
  LPort    = IP_UDP_GetLPort(pInPacket);
  FPort    = IP_UDP_GetFPort(pInPacket);
  pInData  = (const U8*)IP_UDP_GetDataPtr(pInPacket);
  NumBytes = IP_UDP_GetDataSize(pInPacket);
  if        (LPort == TRAP_INFORM_PORT) {
    //
    // Handle INFORM response.
    //
    IP_SNMP_AGENT_ProcessInformResponse(pInData, NumBytes);
  } else if (LPort == MESSAGE_PORT) {
    //
    // Handle SNMP request.
    //
    MaxPacketSize = IP_GetMaxAvailPacketSize(_IFaceId);
    if (MaxPacketSize == 0) {
      IP_SNMP_AGENT_WARN(("No free packet for response."));
    } else {
      MaxPacketSize -= 28;  // Minus IPv4(20 bytes) and UDP(8 bytes) header length.
      pOutPacket = IP_UDP_AllocEx(_IFaceId, MaxPacketSize);
      if (pOutPacket == NULL) {
        IP_SNMP_AGENT_WARN(("Unable to alloc packet for response."));
      } else {
        pOutData    = (U8*)IP_UDP_GetDataPtr(pOutPacket);
        ResponseLen = IP_SNMP_AGENT_ProcessRequest(pInData, NumBytes, pOutData, MaxPacketSize, NULL);
        //
        // Send back a response if we have to.
        //
        if (ResponseLen > 0) {
          pOutPacket->NumBytes = ResponseLen;
          IP_UDP_SendAndFree(_IFaceId, IPAddr, FPort, LPort, pOutPacket);
        } else {
          IP_UDP_Free(pOutPacket);
        }
      }
    }
  } else {
    return IP_RX_ERROR;  // Error, unkown packet received.
  }
  return IP_OK;
}

/*********************************************************************
*
*       _SNMP_AGENT_OnInformResponse()
*
*  Function description
*    Callback that is activated upon a change in an INFORM message
*    waiting for an ACK to be received.
*
*  Parameters
*    pUserContext      : User specific context set when creating the context.
*    pVarbindContext   : Pointer to context that holds the custom Varbinds to
*                        send in the INFORM.
*    pTrapInformContext: Pointer to the context of the INFORM message.
*    Status            : IP_SNMP_AGENT_INFORM_STATUS_TIMEOUT
*                        IP_SNMP_AGENT_INFORM_STATUS_ACK_RECEIVED
*/
static void _SNMP_AGENT_OnInformResponse(void* pUserContext, IP_SNMP_AGENT_CONTEXT* pVarbindContext, IP_SNMP_AGENT_TRAP_INFORM_CONTEXT* pTrapInformContext, int Status) {
  IP_USE_PARA(pUserContext);
  IP_USE_PARA(pVarbindContext);
  IP_USE_PARA(pTrapInformContext);

  if        (Status == IP_SNMP_AGENT_INFORM_STATUS_TIMEOUT) {
    IP_SNMP_AGENT_LOG(("INFORM timeout."));
  } else if (Status == IP_SNMP_AGENT_INFORM_STATUS_ACK_RECEIVED) {
    IP_SNMP_AGENT_LOG(("INFORM ACK received."));
  } else if (Status == IP_SNMP_AGENT_INFORM_STATUS_NACK_RECEIVED) {
    IP_SNMP_AGENT_LOG(("INFORM NACK received."));
  }
  IP_SNMP_AGENT_LOG(("Varbind context and config can be freed."));
}

/*********************************************************************
*
*       _SNMP_AGENT_InitConfig()
*
*  Function description
*    Initialzes and configures the SNMP agent.
*/
static void _SNMP_AGENT_InitConfig(void) {
  int r;

  //
  // Init the SNMP agent.
  //
  IP_SNMP_AGENT_Init(&_SNMPAgentApi);
  //
  // Add the MIB-II tree iso(1).org(3).dod(6).internet(1).ietf(2).mib2(1).system(1) .
  //
  IP_SNMP_AGENT_AddMIB_IsoOrgDodInternetIetfMib2System(&_SNMPAgentMib2SystemApi);
  //
  // Add the MIB-II tree iso(1).org(3).dod(6).internet(1).ietf(2).mib2(1).interfaces(2) .
  //
  IP_SNMP_AGENT_AddMIB_IsoOrgDodInternetIetfMib2Interfaces(&IP_SNMP_AGENT_MIB2_INTERFACES_emNet);
  //
  // Add the default MIB tree iso(1).org(3).dod(6).internet(1).private(4).enterprise(1) .
  //
  IP_SNMP_AGENT_AddMIB_IsoOrgDodInternetPrivateEnterprise();
  //
  // Add a private MIB with OID iso(1).org(3).dod(6).internet(1).private(4).enterprise(1).PRIVATE_ENTERPRISE_NUMBER .
  //
  r = IP_SNMP_AGENT_AddMIB(&_aOID_IsoOrgDodInternetPrivateEnterprise[0], sizeof(_aOID_IsoOrgDodInternetPrivateEnterprise), &_SNMP_SampleMIB, _SNMP_Sample_cb, PRIVATE_ENTERPRISE_NUMBER);
  if (r != 0) {
    IP_SNMP_AGENT_LOG(("Failed to add sample private MIB."));
    while (1) {
      OS_Delay(100);
    }
  }
  //
  // Add a community string (without termination) and set permissons for it.
  //
  IP_SNMP_AGENT_AddCommunity    (&_SNMP_Community_Public, PUBLIC_COMMUNITY_STRING, sizeof(PUBLIC_COMMUNITY_STRING) - 1);
  IP_SNMP_AGENT_SetCommunityPerm(&_SNMP_Community_Public, _aSNMP_Perm_Public);  // Set permissions for public community (typically read permissions).
  //
  // Add a second community string (without termination) and set permissons for it.
  //
  IP_SNMP_AGENT_AddCommunity    (&_SNMP_Community_Private, PRIVATE_COMMUNITY_STRING, sizeof(PRIVATE_COMMUNITY_STRING) - 1);
  IP_SNMP_AGENT_SetCommunityPerm(&_SNMP_Community_Private, _aSNMP_Perm_Private);  // Set permissions for private community (typically read and write permissions).
  //
  // Register on INFORM response hook that signals timeouts and
  // received INFORM responses for waiting items.
  //
  IP_SNMP_AGENT_AddInformReponseHook(&_OnInformResponseHook, &_SNMP_AGENT_OnInformResponse);
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
  I32 Timeout;

  IP_Init();
  _IFaceId = IP_INFO_GetNumInterfaces() - 1;                                           // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
  OS_SetPriority(OS_GetTaskID(), TASK_PRIO_IP_TASK - 1);                               // For now, this task has highest prio except IP management tasks.
  OS_CREATETASK(&_IPTCB  , "IP_Task"  , IP_Task  , TASK_PRIO_IP_TASK   , _IPStack);    // Start the IP_Task.
#if USE_RX_TASK
  OS_CREATETASK(&_IPRxTCB, "IP_RxTask", IP_RxTask, TASK_PRIO_IP_RX_TASK, _IPRxStack);  // Start the IP_RxTask, optional.
#endif
  IP_AddStateChangeHook(&_StateChangeHook, _OnStateChange);                            // Register hook to be notified on disconnects.
  IP_Connect(_IFaceId);                                                                // Connect the interface if necessary.
  _SNMP_AGENT_InitConfig();                                                            // Initialize and configure the SNMP agent.
  while (IP_IFaceIsReadyEx(_IFaceId) == 0) {
    OS_Delay(50);
  }
  OS_SetPriority(OS_GetTaskID(), TASK_PRIO_IP_SNMP_AGENT);                             // Now this task is used for management and sending/resending TRAP/INFORM messages.
  OS_SetTaskName(OS_GetTaskID(), "SNMPa");
  //
  // Register SNMP agent callback for message handling.
  //
  IP_UDP_Open(0uL /* any foreign host */, 0 /* any foreign port */, MESSAGE_PORT    , _SNMP_AGENT_OnRx, NULL);
  IP_UDP_Open(0uL /* any foreign host */, 0 /* any foreign port */, TRAP_INFORM_PORT, _SNMP_AGENT_OnRx, NULL);
  //
  // Send boot trap to a list of managers to let them know we are alive.
  //
  _SNMP_AGENT_SendBootTrap();
  //
  // Handle management tasks of the SNMP agent like resends.
  //
  while (1) {
    Timeout = IP_SNMP_AGENT_Exec();
    OS_Delay(Timeout);
  }
}

/*************************** End of file ****************************/
