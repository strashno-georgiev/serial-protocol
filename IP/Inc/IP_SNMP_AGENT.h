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

File    : IP_SNMP_AGENT.h
Purpose : Header file for SNMP agent.
*/

#include "SEGGER.h"
#include "IP_SNMP_AGENT_Conf.h"

#ifndef IP_SNMP_AGENT_H       // Avoid multiple inclusion.
#define IP_SNMP_AGENT_H

#if defined(__cplusplus)
  extern "C" {                // Make sure we have C-declarations in C++ programs.
#endif

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#ifndef   IP_SNMP_AGENT_SUPPORT_64_BIT_TYPES
  #define IP_SNMP_AGENT_SUPPORT_64_BIT_TYPES  1  // Can be disabled to support compiling on older toolchains (not C99 compliant).
#endif

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

//
// SNMP versions.
//
#define IP_SNMP_VERSION_1  0
#define IP_SNMP_VERSION_2  1

//
// Access permissions.
//
#define IP_SNMP_AGENT_PERM_READ_MASK   (1 << 0)
#define IP_SNMP_AGENT_PERM_WRITE_MASK  (1 << 1)

//
// SNMP PDU types.
//
#define IP_SNMP_PDU_TYPE_GET_REQUEST       0xA0
#define IP_SNMP_PDU_TYPE_GET_NEXT_REQUEST  0xA1
#define IP_SNMP_PDU_TYPE_GET_RESPONSE      0xA2
#define IP_SNMP_PDU_TYPE_SET_REQUEST       0xA3
#define IP_SNMP_PDU_TYPE_TRAPV1            0xA4
#define IP_SNMP_PDU_TYPE_GET_BULK_REQUEST  0xA5
#define IP_SNMP_PDU_TYPE_INFORMV2          0xA6
#define IP_SNMP_PDU_TYPE_TRAPV2            0xA7

//
// SNMP generic-trap OID values.
//
#define IP_SNMP_GENERIC_TRAP_OID_COLD_START              { 0x2B, 0x06, 0x01, 0x06, 0x03, 0x01, 0x01, 0x05, 0x01 }  // Generic SMIv1 TRAP #0 OID value.
#define IP_SNMP_GENERIC_TRAP_OID_WARM_START              { 0x2B, 0x06, 0x01, 0x06, 0x03, 0x01, 0x01, 0x05, 0x02 }  // Generic SMIv1 TRAP #1 OID value.
#define IP_SNMP_GENERIC_TRAP_OID_LINK_DOWN               { 0x2B, 0x06, 0x01, 0x06, 0x03, 0x01, 0x01, 0x05, 0x03 }  // Generic SMIv1 TRAP #2 OID value.
#define IP_SNMP_GENERIC_TRAP_OID_LINK_UP                 { 0x2B, 0x06, 0x01, 0x06, 0x03, 0x01, 0x01, 0x05, 0x04 }  // Generic SMIv1 TRAP #3 OID value.
#define IP_SNMP_GENERIC_TRAP_OID_AUTHENTICATION_FAILURE  { 0x2B, 0x06, 0x01, 0x06, 0x03, 0x01, 0x01, 0x05, 0x05 }  // Generic SMIv1 TRAP #4 OID value.
#define IP_SNMP_GENERIC_TRAP_OID_EGP_NEIGHBOR_LOSS       { 0x2B, 0x06, 0x01, 0x06, 0x03, 0x01, 0x01, 0x05, 0x06 }  // Generic SMIv1 TRAP #5 OID value.
#define IP_SNMP_GENERIC_TRAP_OID_ENTERPRISE_SPECIFIC     { 0x2B, 0x06, 0x01, 0x06, 0x03, 0x01, 0x01, 0x05, 0x07 }  // Generic SMIv1 TRAP #6 OID value.

//
// SNMP field types.
//
#define IP_SNMP_TYPE_INTEGER       0x02
#define IP_SNMP_TYPE_OCTET_STRING  0x04
#define IP_SNMP_TYPE_NULL          0x05
#define IP_SNMP_TYPE_OID           0x06
#define IP_SNMP_TYPE_IP_ADDRESS    0x40  // Bytes can not be truncated, always 4 bytes. This is IPv4 only. There is no type for IPv6.
#define IP_SNMP_TYPE_COUNTER32     0x41
#define IP_SNMP_TYPE_UNSIGNED32    0x42
#define IP_SNMP_TYPE_TIME_TICKS    0x43
#define IP_SNMP_TYPE_OPAQUE        0x44  // Used to encapsulate non standard types such as float(120). The real tag to use is BER encoded.

//
// Non-standard SNMP field types.
// This are typically SNMP types with a value above 32
// and typically come wrapped in an Opaque type.
// Although not specified in the main RFCs of SNMP
// they are agreed to be implemented in a specific
// way and commonly used by various tools.
// When testing with tools like Net-SNMP a response
// of an Opaque wrapped type might only display the
// bytes of the Opaque instead of a readable result.
//
// float and double are special cases as well as
// typically the value of a type wrapped inside an
// Opaque needs to be BER serialized. As float and double
// are already IEEE 754 encoded they will not be encoded
// again.
//
#if IP_SNMP_AGENT_SUPPORT_64_BIT_TYPES
#define IP_SNMP_TYPE_COUNTER64   0x46
#define IP_SNMP_TYPE_DOUBLE      0x79
#define IP_SNMP_TYPE_INTEGER64   0x7A
#define IP_SNMP_TYPE_UNSIGNED64  0x7B
#endif

#define IP_SNMP_TYPE_FLOAT       0x78

//
// SNMP field types that use the same tag ID as other types.
// These types are compatible and are therefore only remapped.
//
#define IP_SNMP_TYPE_INTEGER32  IP_SNMP_TYPE_INTEGER
#define IP_SNMP_TYPE_BITS       IP_SNMP_TYPE_OCTET_STRING
#define IP_SNMP_TYPE_COUNTER    IP_SNMP_TYPE_COUNTER32
#define IP_SNMP_TYPE_GAUGE      IP_SNMP_TYPE_UNSIGNED32
#define IP_SNMP_TYPE_GAUGE32    IP_SNMP_TYPE_UNSIGNED32

//
// SNMP return/error codes. To be used as return values in MIB callbacks.
//
#define IP_SNMP_OK                 0

#define IP_SNMP_ERR_TOO_BIG        1
#define IP_SNMP_ERR_NO_SUCH_NAME   2
#define IP_SNMP_ERR_BAD_VALUE      3
#define IP_SNMP_ERR_GENERIC        5
#define IP_SNMP_ERR_NO_ACCESS      6
#define IP_SNMP_ERR_WRONG_TYPE     7
#define IP_SNMP_ERR_NO_CREATION   11
#define IP_SNMP_ERR_AUTH          16

//
// IP_SNMP_AGENT return/error codes. Returned by the SNMP agent API.
//
#define IP_SNMP_AGENT_OK                        0

#define IP_SNMP_AGENT_ERR_MISC                 -1
#define IP_SNMP_AGENT_ERR_UNSUPPORTED_VERSION  -2
#define IP_SNMP_AGENT_ERR_AUTH                 -3
#define IP_SNMP_AGENT_ERR_MALFORMED_MESSAGE    -4

//
// IP_SNMP_AGENT INFORM status codes.
//
#define IP_SNMP_AGENT_INFORM_STATUS_CANCELED         -2
#define IP_SNMP_AGENT_INFORM_STATUS_TIMEOUT          -1
#define IP_SNMP_AGENT_INFORM_STATUS_WAITING_FOR_ACK   0
#define IP_SNMP_AGENT_INFORM_STATUS_ACK_RECEIVED      1
#define IP_SNMP_AGENT_INFORM_STATUS_NACK_RECEIVED     2

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

typedef struct {
  void  (*pfInit)             (void);
  void  (*pfDeInit)           (void);
  void  (*pfLock)             (void);
  void  (*pfUnlock)           (void);
  void* (*pfAllocSendBuffer)  (void* pUserContext, U8** ppBuffer, U32 NumBytes, U8 IPAddrLen);
  void  (*pfFreeSendBuffer)   (void* pUserContext, void* p, char SendCalled, int r);
  int   (*pfSendTrapInform)   (void* pContext, void* pUserContext, void* hBuffer, const U8* pData, U32 NumBytes, U8* pIPAddr, U16 Port, U8 IPAddrLen);
  U32   (*pfGetTime)          (void);
  U32   (*pfSysTicks2SnmpTime)(U32 SysTicks);
  U32   (*pfSnmpTime2SysTicks)(U32 SnmpTime);
} IP_SNMP_AGENT_API;

typedef struct {
  const U8* pOID;
        U16 Len;
        U8  Perm;
} IP_SNMP_AGENT_PERM;

typedef struct IP_SNMP_AGENT_COMMUNITY {
  struct IP_SNMP_AGENT_COMMUNITY* pNext;
  const  char*                    sCommunity;
         U32                      Len;
  const  IP_SNMP_AGENT_PERM*      pPerm;
} IP_SNMP_AGENT_COMMUNITY;

typedef struct {
  U8* pBuffer;
  U8* pData;
  U32 BufferSize;
  U32 NumBytesLeft;
} IP_SNMP_AGENT_BUFFER_DESC;

typedef struct {
        U8*                        pData;             // Data pointer that can be passed through multiple functions. Typically used for open/close Varbind actions.
        void*                      pUserContext;
        IP_SNMP_AGENT_COMMUNITY*   pCommunity;
        IP_SNMP_AGENT_BUFFER_DESC* pInBufferDesc;
        void*                      pVarbindFlags;     // Only when processing incoming messages. Contains flags that are set during process of a Varbind
                                                      // for other functions to know a specific state. Should be reset before processing each new Varbind.
  const U8*                        pEnterpriseOID;
  const U8*                        pTrapOID;
        U32                        EnterpriseOIDLen;
        U32                        TrapOIDLen;
        IP_SNMP_AGENT_BUFFER_DESC  OutBufferDesc;
        U32                        AgentAddr;
        U8                         Error;             // SNMP error code.
        U8                         AuthError;         // In case we have not enough permissions for an access we might avoid sending back to prevent brute forcing the community string.
} IP_SNMP_AGENT_CONTEXT;

typedef struct IP_SNMP_AGENT_TRAP_INFORM_CONTEXT {
  struct IP_SNMP_AGENT_TRAP_INFORM_CONTEXT* pNext;                // Pointer to next TRAP/INFORM context for INFORM wait for ACK list.
         IP_SNMP_AGENT_CONTEXT*             pVarbindListContext;  // Pointer to context holding the custom Varbinds to send.
         IP_SNMP_AGENT_COMMUNITY*           pCommunity;           // Community (string) to use.
         void*                              pSendContext;         // Send context, typically a socket.
         U8                                 IPAddr[16];           // Enough space to handle IPv4 and IPv6 manager addresses. Address has to be stored in network order.
         U32                                Timeout;              // INFORM timeout [ms] of each message sent.
         U32                                NextTimeout;          // Next timestamp when to check for a necessary resend.
         U32                                TickCnt;              // The request timestamp (tick count) needs to be reused in case we have a resend of an INFORM.
         I32                                RequestId;            // The request ID needs to be reused in case we have a resend of an INFORM.
         U16                                Port;                 // TRAP/INFORM port in network order.
         U8                                 IPAddrLen;            // Length of IP address to determine if it is an IPv4(4 bytes) or IPv6(16 bytes) address.
         U8                                 Type;                 // IP_SNMP_PDU_TYPE_TRAPV1 or IP_SNMP_PDU_TYPE_TRAPV2 or IP_SNMP_PDU_TYPE_INFORMV2.
         U8                                 Retries;              // Number of INFORM retries to send.
         I8                                 Status;               // Only used for v2 INFORM messages.
} IP_SNMP_AGENT_TRAP_INFORM_CONTEXT;

typedef int  (*IP_SNMP_AGENT_pfMIB)             (IP_SNMP_AGENT_CONTEXT* pContext, void* pUserContext, const U8* pMIB, U32 MIBLen, const U8* pIndex, U32 IndexLen, U8 RequestType, U8 VarType);
typedef void (*IP_SNMP_AGENT_pfOnInformResponse)(void* pUserContext, IP_SNMP_AGENT_CONTEXT* pVarbindContext, IP_SNMP_AGENT_TRAP_INFORM_CONTEXT* pTrapInformContext, int Status);

typedef struct IP_SNMP_AGENT_HOOK_ON_INFORM_RESPONSE {
  struct IP_SNMP_AGENT_HOOK_ON_INFORM_RESPONSE* pNext;
         IP_SNMP_AGENT_pfOnInformResponse       pf;  // Callback upon a change of the wait for INFORM ACK element.
} IP_SNMP_AGENT_HOOK_ON_INFORM_RESPONSE;

typedef struct IP_SNMP_AGENT_MIB {
  struct IP_SNMP_AGENT_MIB*  pParent;      // Parent MIB of the current one.
  struct IP_SNMP_AGENT_MIB*  pFirstChild;  // First child MIB of the current one.
  struct IP_SNMP_AGENT_MIB*  pNext;        // Next one with the same parent.
         U32                 Id;           // OID part of the current MIB.
         IP_SNMP_AGENT_pfMIB pf;           // Callback for the current MIB.
         void*               pContext;     // Context specific pointer to store information like an additional API structure for internal purposes.
} IP_SNMP_AGENT_MIB;

//
// System description represented at MIB-II at oid value 1.3.6.1.2.1.1 .
// For more details please refer to http://www.alvestrand.no/objectid/1.3.6.1.2.1.1.html .
//
typedef struct {
  const char* sSysDescr;                                                    // String including full name and version of the target and other information. Up to 255 characters + termination.
  const U8*   pSysObjectID;                                                 // The vendor's authoritative identification of the network management subsystem contained in the entity.
        U32   SysObjectIDLen;                                               // Length of the oid value at pSysObjectID.
  U32 (*pfGetSysUpTime)     (void);                                         // Time in in hundredths of a second since the network management portion of the system was last re-initialized.
  int (*pfGetSetSysContact) (char* pBuffer, U32* pNumBytes, char IsWrite);  // String including information regarding the contact person for this managed node and how to contact this person. Up to 255 characters + termination.
  int (*pfGetSetSysName)    (char* pBuffer, U32* pNumBytes, char IsWrite);  // String including an administratively-assigned name for this managed node e.g. FQDN. Up to 255 characters + termination.
  int (*pfGetSetSysLocation)(char* pBuffer, U32* pNumBytes, char IsWrite);  // String including the physical location of this node. Up to 255 characters + termination.
  U8          SysServices;                                                  // Value representing the services offered.
} IP_SNMP_AGENT_MIB2_SYSTEM_API;

//
// Interfaces description represented at MIB-II at oid value 1.3.6.1.2.1.1 .
// For more details please refer to http://www.alvestrand.no/objectid/1.3.6.1.2.1.1.html .
//
typedef struct {
  int  (*pfGetIfNumber)         (void);
  int  (*pfGetIfIndex)          (unsigned IfEntry);
  void (*pfGetIfDescr)          (unsigned IfEntry, char* pBuffer, U32* pNumBytes);
  int  (*pfGetIfType)           (unsigned IfEntry);
  int  (*pfGetIfMtu)            (unsigned IfEntry);
  U32  (*pfGetIfSpeed)          (unsigned IfEntry);
  void (*pfGetIfPhysAddress)    (unsigned IfEntry, U8* pBuffer, U32* pNumBytes);
  int  (*pfGetIfAdminStatus)    (unsigned IfEntry);
  int  (*pfSetIfAdminStatus)    (unsigned IfEntry, unsigned Status);
  int  (*pfGetIfOperStatus)     (unsigned IfEntry);
  U32  (*pfGetIfLastChange)     (unsigned IfEntry);
  U32  (*pfGetIfInOctets)       (unsigned IfEntry);
  U32  (*pfGetIfInUcastPkts)    (unsigned IfEntry);
  U32  (*pfGetIfInNUcastPkts)   (unsigned IfEntry);
  U32  (*pfGetIfInDiscards)     (unsigned IfEntry);
  U32  (*pfGetIfInErrors)       (unsigned IfEntry);
  U32  (*pfGetIfInUnknownProtos)(unsigned IfEntry);
  U32  (*pfGetIfOutOctets)      (unsigned IfEntry);
  U32  (*pfGetIfOutUcastPkts)   (unsigned IfEntry);
  U32  (*pfGetIfOutNUcastPkts)  (unsigned IfEntry);
  U32  (*pfGetIfOutDiscards)    (unsigned IfEntry);
  U32  (*pfGetIfOutErrors)      (unsigned IfEntry);
  U32  (*pfGetIfOutQLen)        (unsigned IfEntry);
  void (*pfGetIfSpecific)       (unsigned IfEntry, U8* pBuffer, U32* pNumBytes);
} IP_SNMP_AGENT_MIB2_INTERFACES_API;

/*********************************************************************
*
*       API functions, internal
*
*  Internal API that is made public so glue layers like the OS layer
*  can be shipped in source without the need for internal header
*  files when using object shipments.
*
**********************************************************************
*/

const IP_SNMP_AGENT_API* IP_SNMP_AGENT_GetAPI(void);

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

void IP_SNMP_AGENT_AddCommunity         (IP_SNMP_AGENT_COMMUNITY* pCommunity, const char* sCommunity, U32 Len);
int  IP_SNMP_AGENT_AddMIB               (const U8* pParentOID, U32 Len, IP_SNMP_AGENT_MIB* pMIB, IP_SNMP_AGENT_pfMIB pf, U32 Id);
void IP_SNMP_AGENT_AddInformReponseHook (IP_SNMP_AGENT_HOOK_ON_INFORM_RESPONSE* pHook, IP_SNMP_AGENT_pfOnInformResponse pf);
void IP_SNMP_AGENT_CancelInform         (IP_SNMP_AGENT_TRAP_INFORM_CONTEXT* pTrapInformContext);
int  IP_SNMP_AGENT_CheckInformStatus    (IP_SNMP_AGENT_TRAP_INFORM_CONTEXT* pContext);
void IP_SNMP_AGENT_DeInit               (void);
U32  IP_SNMP_AGENT_Exec                 (void);
int  IP_SNMP_AGENT_GetMessageType       (const U8* pIn, U32 NumBytesIn, U8* pType);
void IP_SNMP_AGENT_Init                 (const IP_SNMP_AGENT_API* pAPI);
void IP_SNMP_AGENT_PrepareTrapInform    (IP_SNMP_AGENT_CONTEXT* pContext, void* pUserContext, const U8* pEnterpriseOID, U32 EnterpriseOIDLen, const U8* pTrapOID, U32 TrapOIDLen, U8* pBuffer, U32 BufferSize, U32 AgentAddr);
int  IP_SNMP_AGENT_ProcessInformResponse(const U8* pIn, U32 NumBytesIn);
int  IP_SNMP_AGENT_ProcessRequest       (const U8* pMessage, U32 MessageLen, U8* pBuffer, U32 BufferSize, void* pUserContext);
int  IP_SNMP_AGENT_SendTrapInform       (void* pContext, IP_SNMP_AGENT_CONTEXT* pVarbindContext, IP_SNMP_AGENT_TRAP_INFORM_CONTEXT* pTrapInformContext);
void IP_SNMP_AGENT_SetCommunityPerm     (IP_SNMP_AGENT_COMMUNITY* pCommunity, const IP_SNMP_AGENT_PERM* pPerm);

//
// Functions to hook in standard MIBs into the tree.
//
int IP_SNMP_AGENT_AddMIB_IsoOrgDodInternetPrivateEnterprise (void);
int IP_SNMP_AGENT_AddMIB_IsoOrgDodInternetIetfMib2Interfaces(const IP_SNMP_AGENT_MIB2_INTERFACES_API* pAPI);
int IP_SNMP_AGENT_AddMIB_IsoOrgDodInternetIetfMib2System    (const IP_SNMP_AGENT_MIB2_SYSTEM_API* pAPI);

//
// MIB-II Interface API layer for emNet.
//
#define IP_SNMP_AGENT_MIB2_INTERFACES_embOSIP  IP_SNMP_AGENT_MIB2_INTERFACES_emNet
extern const IP_SNMP_AGENT_MIB2_INTERFACES_API IP_SNMP_AGENT_MIB2_INTERFACES_emNet;

//
// Message construct functions.
//
int IP_SNMP_AGENT_CloseVarbind              (IP_SNMP_AGENT_CONTEXT* pContext);
int IP_SNMP_AGENT_OpenVarbind               (IP_SNMP_AGENT_CONTEXT* pContext);
int IP_SNMP_AGENT_StoreCounter32            (IP_SNMP_AGENT_CONTEXT* pContext, U32 v);
int IP_SNMP_AGENT_StoreCurrentMibOidAndIndex(IP_SNMP_AGENT_CONTEXT* pDstContext, IP_SNMP_AGENT_CONTEXT* pSrcContext, U32 NumIndexes, ...);
int IP_SNMP_AGENT_StoreInstanceNA           (IP_SNMP_AGENT_CONTEXT* pContext);
int IP_SNMP_AGENT_StoreInteger              (IP_SNMP_AGENT_CONTEXT* pContext, I32 v);
int IP_SNMP_AGENT_StoreIpAddress            (IP_SNMP_AGENT_CONTEXT* pContext, U32 IpAddress);
int IP_SNMP_AGENT_StoreOctetString          (IP_SNMP_AGENT_CONTEXT* pContext, const U8* pData, U32 NumBytes);
int IP_SNMP_AGENT_StoreOID                  (IP_SNMP_AGENT_CONTEXT* pContext, const U8* pOIDBytes, U32 OIDLen, U32 MIBLen, U8 IsValue);
int IP_SNMP_AGENT_StoreOpaque               (IP_SNMP_AGENT_CONTEXT* pContext, const U8* pData, U32 NumBytes);
int IP_SNMP_AGENT_StoreTimeTicks            (IP_SNMP_AGENT_CONTEXT* pContext, U32 v);
int IP_SNMP_AGENT_StoreUnsigned32           (IP_SNMP_AGENT_CONTEXT* pContext, U32 v);

//
// Message parsing functions.
//
int IP_SNMP_AGENT_ParseCounter32  (IP_SNMP_AGENT_CONTEXT* pContext, U32* pCounter32);
int IP_SNMP_AGENT_ParseInteger    (IP_SNMP_AGENT_CONTEXT* pContext, I32* pInteger);
int IP_SNMP_AGENT_ParseIpAddress  (IP_SNMP_AGENT_CONTEXT* pContext, U32* pIpAddress);
int IP_SNMP_AGENT_ParseOctetString(IP_SNMP_AGENT_CONTEXT* pContext, const U8** ppData, U32* pLen);
int IP_SNMP_AGENT_ParseOID        (IP_SNMP_AGENT_CONTEXT* pContext, const U8** ppData, U32* pLen);
int IP_SNMP_AGENT_ParseOpaque     (IP_SNMP_AGENT_CONTEXT* pContext, const U8** ppData, U32* pLen);
int IP_SNMP_AGENT_ParseTimeTicks  (IP_SNMP_AGENT_CONTEXT* pContext, U32* pTimeTicks);
int IP_SNMP_AGENT_ParseUnsigned32 (IP_SNMP_AGENT_CONTEXT* pContext, U32* pUnsigned32);

//
// Opaque message construct/parsing functions. These might not be
// supported by every SNMP solution and are typically based on
// drafts that have been agreed by several people to use it in
// this way. However they are not officially supported and might
// be reported simply as Opaque fields with an octet string in there.
//
#if IP_SNMP_AGENT_SUPPORT_64_BIT_TYPES
int IP_SNMP_AGENT_StoreCounter64 (IP_SNMP_AGENT_CONTEXT* pContext, U64 v);
int IP_SNMP_AGENT_StoreDouble    (IP_SNMP_AGENT_CONTEXT* pContext, double v);
int IP_SNMP_AGENT_StoreInteger64 (IP_SNMP_AGENT_CONTEXT* pContext, I64 v);
int IP_SNMP_AGENT_StoreUnsigned64(IP_SNMP_AGENT_CONTEXT* pContext, U64 v);

int IP_SNMP_AGENT_ParseCounter64 (IP_SNMP_AGENT_CONTEXT* pContext, U64* pCounter64);
int IP_SNMP_AGENT_ParseDouble    (IP_SNMP_AGENT_CONTEXT* pContext, double* pDouble);
int IP_SNMP_AGENT_ParseInteger64 (IP_SNMP_AGENT_CONTEXT* pContext, I64* pInteger64);
int IP_SNMP_AGENT_ParseUnsigned64(IP_SNMP_AGENT_CONTEXT* pContext, U64* pUnsigned64);
#endif

int IP_SNMP_AGENT_StoreFloat     (IP_SNMP_AGENT_CONTEXT* pContext, float v);
int IP_SNMP_AGENT_ParseFloat     (IP_SNMP_AGENT_CONTEXT* pContext, float* pFloat);

//
// Function macros for implementing types that are already handled by other real functions.
// The SNMP standard defines several types that use the same ID tag. Therefore an application
// could as well use one of the real functions that use the same type instead of the macro.
// This works as even for an Unsigned32 type only the bytes are used that are required to
// handle the information to transport.
// Examples:
//   Unsigned32 with value         1 (0x01)       requires 1 byte to store the value.
//   Unsigned32 with value      1000 (0x03E8)     requires 2 byte to store the value.
//   Unsigned32 with value    100000 (0x0186A0)   requires 3 byte to store the value.
//   Unsigned32 with value 100000000 (0x05F5E100) requires 4 byte to store the value.
//
#define IP_SNMP_AGENT_StoreBits       IP_SNMP_AGENT_StoreOctetString
#define IP_SNMP_AGENT_StoreCounter    IP_SNMP_AGENT_StoreCounter32
#define IP_SNMP_AGENT_StoreGauge      IP_SNMP_AGENT_StoreUnsigned32
#define IP_SNMP_AGENT_StoreGauge32    IP_SNMP_AGENT_StoreUnsigned32
#define IP_SNMP_AGENT_StoreInteger32  IP_SNMP_AGENT_StoreInteger

#define IP_SNMP_AGENT_ParseBits       IP_SNMP_AGENT_ParseOctetString
#define IP_SNMP_AGENT_ParseCounter    IP_SNMP_AGENT_ParseCounter32
#define IP_SNMP_AGENT_ParseGauge      IP_SNMP_AGENT_ParseUnsigned32
#define IP_SNMP_AGENT_ParseGauge32    IP_SNMP_AGENT_ParseUnsigned32
#define IP_SNMP_AGENT_ParseInteger32  IP_SNMP_AGENT_ParseInteger

//
// Helper functions.
//
int IP_SNMP_AGENT_DecodeOIDValue(const U8* pOID, U32* pLen, U32* pValue, const U8** ppNext);
int IP_SNMP_AGENT_EncodeOIDValue(U32 Value, U8* pBuffer, U32 BufferSize, U8** ppNext, U8* pNumEncodedBytes);


#if defined(__cplusplus)
}                             // Make sure we have C-declarations in C++ programs.
#endif

#endif                        // Avoid multiple inclusion.

/*************************** End of file ****************************/
