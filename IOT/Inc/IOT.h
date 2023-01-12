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

File        : IOT.h
Purpose     : API functions for IoT toolkit.

*/

#ifndef IOT_H
#define IOT_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "IOT_ConfDefaults.h"
#include "SEGGER_MEM.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

/*********************************************************************
*
*       IoT toolkit version number
*/
#define IOT_VERSION               23200       // Format: Mmmrr. Example: 12305 is 1.23e


/*********************************************************************
*
*       HTTP client errors
*
*  Description
*    Errors that the HTTP client can generate.
*/
#define IOT_HTTP_ERROR_FIELD_TOO_LONG          (-700)  // Field is too long when parsing HTTP responses
#define IOT_HTTP_ERROR_SOCKET_TERMINATED       (-701)  // Socket closed when reading response
#define IOT_HTTP_ERROR_BAD_SYNTAX              (-702)  // Value doesn't conform to valid syntax
#define IOT_HTTP_ERROR_BUFFER_OVERFLOW         (-703)  // Internal buffer overflowed
#define IOT_HTTP_ERROR_BUFFER_UNDERFLOW        (-704)  // Internal buffer underflowed

/*********************************************************************
*
*       JSON parser errors
*
*  Description
*    Errors that the JSON parser can generate.
*/
#define IOT_JSON_ERROR_OUT_OF_MEMORY           (-800)  // JSON tree parser ran out of memory
#define IOT_JSON_ERROR_BUFFER_TOO_SMALL        (-801)  // Lexical buffer is to small
#define IOT_JSON_ERROR_ILLEGAL_CODE_POINT      (-802)  // Code point outside basic multilingual plane
#define IOT_JSON_ERROR_ILLEGAL_ESCAPE          (-803)  // Illegal escape sequence
#define IOT_JSON_ERROR_BAD_STRUCTURE           (-804)  // Value is structured incorrectly
#define IOT_JSON_ERROR_EXPECTED_VALUE          (-805)  // Syntax error: expected a value
#define IOT_JSON_ERROR_EXPECTED_QUOTATION      (-806)  // Syntax error: expected a quotation mark
#define IOT_JSON_ERROR_EXPECTED_COLON          (-807)  // Syntax error: expected a colon
#define IOT_JSON_ERROR_EXPECTED_RBRACK         (-808)  // Syntax error: expected a comma or close bracket
#define IOT_JSON_ERROR_EXPECTED_RBRACE         (-809)  // Syntax error: expected a comma or close brace
#define IOT_JSON_ERROR_INCOMPLETE              (-810)  // End of input but expected more data
#define IOT_JSON_ERROR_UNKNOWN_ID              (-811)  // Literal ID is not "true", "false", or "null"

/*********************************************************************
*
*       Types required for API
*
**********************************************************************
*/

// Forward-declared types
typedef struct IOT_HTTP_VERSION_API_s IOT_HTTP_VERSION_API;
typedef struct IOT_HTTP_CONTEXT_s     IOT_HTTP_CONTEXT;

/*********************************************************************
*
*       IOT_IO_CONNECT_FUNC
*
*  Description
*    Connect transport.
*
*  Parameters
*    pSession - Pointer to user-provided session context.
*    sHost    - Zero-terminates string describing host to connect to.
*    Port     - Port to connect to.
*
*  Return value
*    >= 0 - Success, connected.
*    <  0 - Failure.
*/
typedef int (IOT_IO_CONNECT_FUNC)(void *pSession, const char *sHost, unsigned Port);

/*********************************************************************
*
*       IOT_IO_DISCONNECT_FUNC
*
*  Description
*    Disconnect transport.
*
*  Parameters
*    pSession - Pointer to user-provided session context.
*
*  Return value
*    >= 0 - Success, disconnected.
*    <  0 - Failure.
*/
typedef int (IOT_IO_DISCONNECT_FUNC)(void *pSession);

/*********************************************************************
*
*       IOT_IO_SEND_FUNC
*
*  Description
*    Send data to transport.
*
*  Parameters
*    pSession - Pointer to user-provided session context.
*    pData    - Pointer to octet string to write to transport.
*    DataLen  - Octet length of the octet string.
*
*  Return value
*    >= 0 - Success, written.
*    <  0 - Failure.
*/
typedef int (IOT_IO_SEND_FUNC)(void *pSession, const void *pData, unsigned DataLen);

/*********************************************************************
*
*       IOT_IO_RECV_FUNC
*
*  Description
*    Receive data from transport.
*
*  Parameters
*    pSession - Pointer to user-provided session context.
*    pData    - Pointer to octet string that receives data from transport.
*    DataLen  - Octet length of the octet string.
*
*  Return value
*    >  0 - Success, number of bytes read from transport.
*    == 0 - Underlying transport closed gracefully.
*    <  0 - Failure.
*/
typedef int (IOT_IO_RECV_FUNC)(void *pSession, void *pData, unsigned DataLen);

/*********************************************************************
*
*       IOT_IO_API
*
*  Description
*    I/O API for HTTP client.
*/
typedef struct {
  IOT_IO_CONNECT_FUNC    *pfConnect;     // Connect transport method.
  IOT_IO_DISCONNECT_FUNC *pfDisconnect;  // Disconnect transport method.
  IOT_IO_SEND_FUNC       *pfSend;        // Send data to transport method.
  IOT_IO_RECV_FUNC       *pfRecv;        // Receive data from transport method.
} IOT_IO_API;

/*********************************************************************
*
*       IOT_HTTP_PART_TYPE
*
*  Description
*    Individual parts of an HTTP (or other) request.
*/
typedef enum {
  IOT_HTTP_PART_TYPE_SCHEME,         // Scheme type (http, https...)
  IOT_HTTP_PART_TYPE_USER,           // User part (e.g. for ftp scheme)
  IOT_HTTP_PART_TYPE_PASSWORD,       // Password part (e.g. for ftp scheme)
  IOT_HTTP_PART_TYPE_HOST,           // Host part
  IOT_HTTP_PART_TYPE_PATH,           // Path fragment
  IOT_HTTP_PART_TYPE_QUERY,          // Query parameter (introduced by "?")
  IOT_HTTP_PART_TYPE_FRAGMENT,       // Fragment part of URI (introduced by "#")
  IOT_HTTP_PART_TYPE_METHOD,         // Method part of request (GET, POST...)
  IOT_HTTP_PART_TYPE_HEADER,         // Request key-value header
  IOT_HTTP_PART_TYPE_SIGNED_HEADER,  // Request header takes part in signature
  IOT_HTTP_PART_TYPE_CONTENT         // Entity body content
} IOT_HTTP_PART_TYPE;

/*********************************************************************
*
*       IOT_HTTP_PART_TYPE
*
*  Description
*    Parameters for an HTTP request.
*
*  Additional information
*    All fields in this structure are private.
*/
typedef struct {
  const char         * sKey;    // Internal: Pointer to key name or NULL
  const char         * sValue;  // Internal: Pointer to part value or NULL
  IOT_HTTP_PART_TYPE   Type;    // Internal: Part type
} IOT_HTTP_PARA;

typedef enum {
  IOT_HTTP_TRANSFER_ENCODING_NONE,
  IOT_HTTP_TRANSFER_ENCODING_IDENTITY,
  IOT_HTTP_TRANSFER_ENCODING_CHUNKED
} IOT_HTTP_TRANSFER_ENCODING;

typedef struct {
  const IOT_IO_API * pAPI;
  void             * pContext;
} IOT_IO_CONTEXT;

/*********************************************************************
*
*       IOT_HTTP_AUTH_FUNC
*
*  Description
*    Write authorization header.
*
*  Parameters
*    pContext - Pointer to HTTP context.
*
*  Additional information
*    This function is registered by IOT_HTTP_SetAuth() and is
*    invoked when the request is executed in order to construct
*    and send any authorization header computed over the HTTP
*    parameters.
*/
typedef void (IOT_HTTP_AUTH_FUNC)(IOT_HTTP_CONTEXT *pContext);

typedef struct {
  IOT_HTTP_AUTH_FUNC * pfWrHeader;
  void               * pContext;
} IOT_HTTP_AUTH;

/*********************************************************************
*
*       IOT entities that replicate CRYPTO capabilities.
*/
typedef struct {
  char     * pCursor;         // Current input or output pointer, the cursor on the stream
  unsigned   OutputByteCnt;   // Number of bytes left to be written
  unsigned   OverflowCnt;     // Number of bytes overwritten
  unsigned   Capacity;        // Total number of bytes in the buffer
  char     * pBuffer;         // Start of the write buffer.
} IOT_BUFFER;

struct IOT_HTTP_CONTEXT_s {
  unsigned                     Flags;
  unsigned                     Port;
  const IOT_HTTP_VERSION_API * pAPI;               // HTTP/1.0, HTTP/1.1, HTTP/2
  IOT_HTTP_TRANSFER_ENCODING   TransferEncoding;   // Response transfer encoding
  unsigned                     ContentLen;         // Content length decoded from header
  unsigned                     ContentRemain;      // Remaining content chunk size
  IOT_IO_CONTEXT               IO;
  IOT_HTTP_AUTH                Auth;
  IOT_BUFFER                   Buffer;
  unsigned                     PutHaveContent;     // AddContent() called
  unsigned                     PutContentLen;      // Total content length
  unsigned                     ParaCnt;
  IOT_HTTP_PARA              * pPara;
  unsigned                     ParaLen;
};

/*********************************************************************
*
*       IOT_HTTP_HEADER_CALLBACK_FUNC
*
*  Description
*    HTTP response header callback.
*
*  Parameters
*    pContext - Pointer to HTTP context.
*    sKey     - Zero-terminated header key.
*    sValue   - Zero-terminated header value.
*
*  Additional information
*    A header callback is invoked through IOT_HTTP_ProcessHeaders(),
*    with each header in the HTTP response invoking a header callback.
*/
typedef void IOT_HTTP_HEADER_CALLBACK_FUNC(IOT_HTTP_CONTEXT *pContext, const char *sKey, const char *sValue);

/*********************************************************************
*
*       IOT_HTTP_HEADER_EX_CALLBACK_FUNC
*
*  Description
*    HTTP response header callback, extended.
*
*  Parameters
*    pHttpCtx - Pointer to HTTP context.
*    sKey     - Zero-terminated header key.
*    sValue   - Zero-terminated header value.
*    pUserCtx - Pointer to user-provided context.
*
*  Additional information
*    A header callback is invoked through IOT_HTTP_ProcessHeadersEx(),
*    with each header in the HTTP response invoking a header callback
*    and passing through a user-provided context.
*/
typedef void (IOT_HTTP_HEADER_EX_CALLBACK_FUNC)(IOT_HTTP_CONTEXT *pContext, const char *sKey, const char *sValue, void *pUserCtx);

/*********************************************************************
*
*       IOT_HTTP_ENUMERATE_CALLBACK_FUNC
*
*  Description
*    HTTP request header enumeration callback.
*
*  Parameters
*    pContext - Pointer to HTTP context.
*    sKey     - Zero-terminated header key.
*    sValue   - Zero-terminated header value.
*    Index    - Zero-based index of header parameter.
*
*  Additional information
*    A header enumeration callback is invoked through IOT_HTTP_EnumerateParas(),
*    with each parameter in the HTTP request invoking a header callback.
*/
typedef void (IOT_HTTP_ENUMERATE_CALLBACK_FUNC)(IOT_HTTP_CONTEXT *pContext, const char *sKey, const char *sValue, unsigned Index);

typedef struct IOT_JSON_CONTEXT_tag IOT_JSON_CONTEXT;

/*********************************************************************
*
*       IOT_JSON_VOID_EVENT_FUNC
*
*  Description
*    JSON event, no parameter.
*
*  Parameters
*    pContext - Pointer to user-supplied context.
*
*  Additional information
*    Function prototype for an event without a parameter.
*/
typedef void (IOT_JSON_VOID_EVENT_FUNC)(IOT_JSON_CONTEXT *pContext);

/*********************************************************************
*
*       IOT_JSON_ARG_EVENT_FUNC
*
*  Description
*    JSON event, string parameter.
*
*  Parameters
*    pContext - Pointer to user-supplied context.
*    sText    - Pointer to zero-terminated string argument.
*
*  Additional information
*    Function prototype for an event with a zero-terminated string
*    parameter.
*/
typedef void (IOT_JSON_ARG_EVENT_FUNC) (IOT_JSON_CONTEXT *pContext, const char *sText);

/*********************************************************************
*
*       IOT_JSON_EVENTS
*
*  Description
*    JSON event handlers.
*/
typedef struct {
  IOT_JSON_VOID_EVENT_FUNC *pfBeginObject;  // Start of object event.
  IOT_JSON_VOID_EVENT_FUNC *pfEndObject;    // End of object event.
  IOT_JSON_VOID_EVENT_FUNC *pfBeginArray;   // Start of array event.
  IOT_JSON_VOID_EVENT_FUNC *pfEndArray;     // End of array event.
  IOT_JSON_ARG_EVENT_FUNC  *pfKey;          // Key id event.
  IOT_JSON_ARG_EVENT_FUNC  *pfString;       // String value event.
  IOT_JSON_ARG_EVENT_FUNC  *pfNumber;       // Number value event.
  IOT_JSON_ARG_EVENT_FUNC  *pfLiteral;      // Literal name event.
} IOT_JSON_EVENTS;

/*********************************************************************
*
*       IOT_JSON_TYPE
*
*  Description
*    JSON value type.
*/
typedef enum {
  IOT_JSON_TYPE_OBJECT,  // Node is an object
  IOT_JSON_TYPE_ARRAY,   // Node is an array
  IOT_JSON_TYPE_PAIR,    // Node is a mapping pair
  IOT_JSON_TYPE_STRING,  // Node is a string
  IOT_JSON_TYPE_NUMBER,  // Node is a number
  IOT_JSON_TYPE_TRUE,    // Node is 'true'
  IOT_JSON_TYPE_FALSE,   // Node is 'false'
  IOT_JSON_TYPE_NULL,    // Node is 'null'
} IOT_JSON_TYPE;

// Internal JSON state machine states.
typedef enum {
  IOT_JSON_STATE_START_VALUE,
  IOT_JSON_STATE_STRING,
  IOT_JSON_STATE_NUMBER,
  IOT_JSON_STATE_LITERAL,
  IOT_JSON_STATE_REQUIRE_STRING,
  IOT_JSON_STATE_REQUIRE_COLON,
  IOT_JSON_STATE_REQUIRE_CLOSE_BRACE_OR_COMMA,
  IOT_JSON_STATE_REQUIRE_CLOSE_BRACE_OR_STRING,
  IOT_JSON_STATE_REQUIRE_CLOSE_BRACKET_OR_COMMA,
  IOT_JSON_STATE_REQUIRE_CLOSE_BRACKET_OR_VALUE,
  IOT_JSON_STATE_PARSE_COMPLETE,
  IOT_JSON_STATE_ESCAPE,
  IOT_JSON_STATE_U1,
  IOT_JSON_STATE_U2,
  IOT_JSON_STATE_U3,
  IOT_JSON_STATE_U4,
  IOT_JSON_STATE_ERROR
} IOT_JSON_STATE;

// Opaque JSON state
struct IOT_JSON_CONTEXT_tag {
  IOT_JSON_STATE          State;            // current parser state
  const IOT_JSON_EVENTS * pEvents;          // event handlers; can be null
  char                  * pBuffer;          // buffer we are parsing
  unsigned                BufferLen;        // capacity of the parse buffer
  unsigned                Index;            // index to the current parse byte
  U8                      aParseStack[32];  // internal parser state stack
  unsigned                aParseSP;         // internal parser state stack pointer
  unsigned                Unicode;          // Unicode character being developed
  U8                      aPath[32];        // client path context
  U8                      PathLength;       // number of valid items in path
  const char            * sKeys;            // keys to match, null if none
  void *                  pUserContext;     // context maintained on behalf of client
  int                     Error;            // when in error state, qualifies the error
};

typedef struct IOT_JSON_NODE IOT_JSON_NODE;
struct IOT_JSON_NODE {
  IOT_JSON_TYPE   Type;    // Type of this node
  IOT_JSON_NODE * pNext;   // Next node in list (linking arrays and object keys)
  IOT_JSON_NODE * pParent; // Pointer to node's parent
  IOT_JSON_NODE * pValue;  // Pointer to value (JSON_TYPE_KEY, JSON_TYPE_ARRAY, JSON_TYPE_OBJECT)
  char          * sStr;    // Only valid for JSON_TYPE_PAIR and JSON_TYPE_STRING
  double          Number;  // Only valid for JSON_TYPE_NUMBER
};

/*********************************************************************
*
*       IOT_HTTP_AUTH_DIGEST_PARAS
*
*  Description
*    Parameters required for digest authentication.
*/
typedef struct {
  U32  NonceCount;             // Outgoing request nonce count (set by client)
  char aUserName    [33];      // Plaintext user name          (set by client)
  char aPassword    [32];      // Plaintext password           (set by client)
  char aAlgorithm   [32];      // Digest algorithm             (set by server)
  char aRealm       [32];      // Authentication realm         (set by server)
  char aDomain      [128];     // Domain                       (set by server)
  char aNonce       [64];      // Server nonce                 (set by server)
  char aCnonce      [32];      // Client nonce                 (set by client)
  char aOpaque      [64];      // Opaque value                 (set by server)
  char aQop         [16];      // Quality of protection        (set by server and client)
  char aStale       [16];      // Flag indicating stale nonce  (set by server)
  char aCharset     [16];      // Character set in use         (set by server)
  char aUserhash    [16];      // Hashed user names supported  (set by server and client)
  U8   aHashUserName[32];      // Internal: Hashed user name   (computed by IoT Toolkit)
} IOT_HTTP_AUTH_DIGEST_PARAS;

/*********************************************************************
*
*       API interfaces
*
**********************************************************************
*/

extern const IOT_HTTP_VERSION_API IOT_HTTP_VERSION_HTTP_1v0;
extern const IOT_HTTP_VERSION_API IOT_HTTP_VERSION_HTTP_1v1;

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

/*********************************************************************
*
*       HTTP client functions
*/
void            IOT_HTTP_Init                (IOT_HTTP_CONTEXT *pSelf, void *pBuf, unsigned BufLen, IOT_HTTP_PARA *pPara, unsigned ParaLen);
void            IOT_HTTP_SetIO               (IOT_HTTP_CONTEXT *pSelf, const IOT_IO_API *pAPI, void *pContext);
void            IOT_HTTP_SetPort             (IOT_HTTP_CONTEXT *pSelf, unsigned Port);
void            IOT_HTTP_SetVersion          (IOT_HTTP_CONTEXT *pSelf, const IOT_HTTP_VERSION_API *pAPI);
void            IOT_HTTP_SetAuth             (IOT_HTTP_CONTEXT *pSelf, IOT_HTTP_AUTH_FUNC *pfWrAuth, void *pContext);
void            IOT_HTTP_Reset               (IOT_HTTP_CONTEXT *pSelf);
void            IOT_HTTP_AddScheme           (IOT_HTTP_CONTEXT *pSelf, const char *sScheme);
void            IOT_HTTP_AddHost             (IOT_HTTP_CONTEXT *pSelf, const char *sHost);
void            IOT_HTTP_AddUser             (IOT_HTTP_CONTEXT *pSelf, const char *sUser);
void            IOT_HTTP_AddPassword         (IOT_HTTP_CONTEXT *pSelf, const char *sPass);
void            IOT_HTTP_AddPath             (IOT_HTTP_CONTEXT *pSelf, const char *sPath);
void            IOT_HTTP_AddQuery            (IOT_HTTP_CONTEXT *pSelf, const char *sKey, const char *sValue);
void            IOT_HTTP_AddFragment         (IOT_HTTP_CONTEXT *pSelf, const char *sValue);
void            IOT_HTTP_AddMethod           (IOT_HTTP_CONTEXT *pSelf, const char *sMethod);
void            IOT_HTTP_AddHeader           (IOT_HTTP_CONTEXT *pSelf, const char *sKey, const char *sValue);
void            IOT_HTTP_AddSignedHeader     (IOT_HTTP_CONTEXT *pSelf, const char *sKey, const char *sValue);
void            IOT_HTTP_AddContent          (IOT_HTTP_CONTEXT *pSelf, const char *sContent);
unsigned        IOT_HTTP_GetPort             (IOT_HTTP_CONTEXT *pSelf);
const char *    IOT_HTTP_GetScheme           (IOT_HTTP_CONTEXT *pSelf);
const char *    IOT_HTTP_GetHost             (IOT_HTTP_CONTEXT *pSelf);
const char *    IOT_HTTP_GetUser             (IOT_HTTP_CONTEXT *pSelf);
const char *    IOT_HTTP_GetPassword         (IOT_HTTP_CONTEXT *pSelf);
const char *    IOT_HTTP_GetQuery            (IOT_HTTP_CONTEXT *pSelf, const char *sKey);
const char *    IOT_HTTP_GetMethod           (IOT_HTTP_CONTEXT *pSelf);
const char *    IOT_HTTP_GetHeader           (IOT_HTTP_CONTEXT *pSelf, const char *sKey);
void            IOT_HTTP_GetPath             (IOT_HTTP_CONTEXT *pSelf, char *sText, unsigned TextLen);
int             IOT_HTTP_QueryURL            (IOT_HTTP_CONTEXT *pSelf, char *pURL, unsigned URLLen);
int             IOT_HTTP_Connect             (IOT_HTTP_CONTEXT *pSelf);
void            IOT_HTTP_Disconnect          (IOT_HTTP_CONTEXT *pSelf);
int             IOT_HTTP_Exec                (IOT_HTTP_CONTEXT *pSelf);
int             IOT_HTTP_ExecWithAuth        (IOT_HTTP_CONTEXT *pSelf);
int             IOT_HTTP_ExecWithBearer      (IOT_HTTP_CONTEXT *pSelf, const char *sToken);
int             IOT_HTTP_ProcessHeaders      (IOT_HTTP_CONTEXT *pSelf, IOT_HTTP_HEADER_CALLBACK_FUNC    *pfParser);
int             IOT_HTTP_ProcessHeadersEx    (IOT_HTTP_CONTEXT *pSelf, IOT_HTTP_HEADER_EX_CALLBACK_FUNC *pfParser, void *pUserContext);
int             IOT_HTTP_ProcessStatusLine   (IOT_HTTP_CONTEXT *pSelf, unsigned *pStatusCode);
unsigned        IOT_HTTP_GetContentLen       (IOT_HTTP_CONTEXT *pSelf);
void            IOT_HTTP_GetBegin            (IOT_HTTP_CONTEXT *pSelf);
int             IOT_HTTP_GetPayload          (IOT_HTTP_CONTEXT *pSelf, void *pData, unsigned DataLen);
void            IOT_HTTP_GetEnd              (IOT_HTTP_CONTEXT *pSelf);
void            IOT_HTTP_PutBegin            (IOT_HTTP_CONTEXT *pSelf);
int             IOT_HTTP_PutPayload          (IOT_HTTP_CONTEXT *pSelf, const void *pData, unsigned DataLen);
void            IOT_HTTP_PutEnd              (IOT_HTTP_CONTEXT *pSelf);
int             IOT_HTTP_Send                (IOT_HTTP_CONTEXT *pSelf, const void *pData, unsigned DataLen);
int             IOT_HTTP_SendStr             (IOT_HTTP_CONTEXT *pSelf, const char *sText);
int             IOT_HTTP_SendHex             (IOT_HTTP_CONTEXT *pSelf, const U8 *pData, unsigned DataLen);
int             IOT_HTTP_Recv                (IOT_HTTP_CONTEXT *pSelf, void *pData, unsigned DataLen);
void            IOT_HTTP_EnumerateParas      (IOT_HTTP_CONTEXT *pSelf, IOT_HTTP_PART_TYPE Type, IOT_HTTP_ENUMERATE_CALLBACK_FUNC *pfCallback);
void            IOT_HTTP_ParseURL            (IOT_HTTP_CONTEXT *pContext, char *sText);
const char *    IOT_HTTP_GetErrorText        (int Status);

/*********************************************************************
*
*       Digest authentication.
*/
void            IOT_HTTP_AUTH_DIGEST_InitParas                  (IOT_HTTP_AUTH_DIGEST_PARAS *pParas);
int             IOT_HTTP_AUTH_DIGEST_ParseParas                 (const char *sText, IOT_HTTP_AUTH_DIGEST_PARAS *pParas);
void            IOT_HTTP_AUTH_DIGEST_MD5_WrHeader               (IOT_HTTP_CONTEXT *pHTTP);
void            IOT_HTTP_AUTH_DIGEST_SHA256_WrHeader            (IOT_HTTP_CONTEXT *pHTTP);
void            IOT_HTTP_AUTH_DIGEST_SHA512_256_WrHeader        (IOT_HTTP_CONTEXT *pHTTP);
void            IOT_HTTP_AUTH_DIGEST_RFC7616_SHA512_256_WrHeader(IOT_HTTP_CONTEXT *pHTTP);

/*********************************************************************
*
*       Information functions.
*/
const char *    IOT_HTTP_GetVersionText      (void);
const char *    IOT_HTTP_GetCopyrightText    (void);

/*********************************************************************
*
*       Event-based JSON parser
*/
void            IOT_JSON_Init                (      IOT_JSON_CONTEXT *pSelf, const IOT_JSON_EVENTS *pEvents, char *pBuf, unsigned BufLen);
int             IOT_JSON_Parse               (      IOT_JSON_CONTEXT *pSelf, const char *pBuf, unsigned BufLen);
void            IOT_JSON_SetUserContext      (      IOT_JSON_CONTEXT *pSelf, void *pUserContext);
void *          IOT_JSON_GetUserContext      (const IOT_JSON_CONTEXT *pSelf);
const char *    IOT_JSON_GetVersionText      (void);
const char *    IOT_JSON_GetCopyrightText    (void);
const char *    IOT_JSON_GetErrorText        (int Status);

/*********************************************************************
*
*       Tree-based JSON parser
*/
int             IOT_JSON_ParseTree           (IOT_JSON_NODE **ppTree, const char *pBuf, unsigned BufLen, SEGGER_MEM_CONTEXT *pMem);
void            IOT_JSON_FreeTree            (IOT_JSON_NODE *pTree, SEGGER_MEM_CONTEXT *pMem);
IOT_JSON_TYPE   IOT_JSON_GetType             (IOT_JSON_NODE *pSelf);
int             IOT_JSON_IsNull              (IOT_JSON_NODE *pSelf);
int             IOT_JSON_IsString            (IOT_JSON_NODE *pSelf);
int             IOT_JSON_IsObject            (IOT_JSON_NODE *pSelf);
int             IOT_JSON_IsArray             (IOT_JSON_NODE *pSelf);
int             IOT_JSON_IsTrue              (IOT_JSON_NODE *pSelf);
int             IOT_JSON_IsFalse             (IOT_JSON_NODE *pSelf);
int             IOT_JSON_IsNumber            (IOT_JSON_NODE *pSelf);
unsigned        IOT_JSON_UpperBound          (IOT_JSON_NODE *pSelf);
IOT_JSON_NODE * IOT_JSON_IndexNode           (IOT_JSON_NODE *pSelf, unsigned Index);
IOT_JSON_NODE * IOT_JSON_SelectNode          (IOT_JSON_NODE *pSelf, const char *sKey);
void            IOT_JSON_CopyString          (IOT_JSON_NODE *pSelf, char *sKey, unsigned KeyLen);
int             IOT_JSON_GetInteger          (IOT_JSON_NODE *pSelf);
const char    * IOT_JSON_GetString           (IOT_JSON_NODE *pSelf);

/*********************************************************************
*
*       Support functions.
*/
void            IOT_Panic                    (void);

#ifdef __cplusplus
}
#endif

#endif

/*************************** End of file ****************************/
