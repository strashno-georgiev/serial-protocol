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
File        : MB.h
Purpose     : API of the Modbus stack.
---------------------------END-OF-HEADER------------------------------
*/

#ifndef _MB_H_                // Avoid multiple/recursive inclusion.
#define _MB_H_  1

#include "SEGGER.h"           // Some SEGGER-specific, global defines.
#include "MB_ConfDefaults.h"

#if defined(__cplusplus)
extern "C" {  /* Make sure we have C-declarations in C++ programs */
#endif

#define MB_VERSION  10208  // Format: Mmmrr. Example: 10208 is 1.02h

/*********************************************************************
*
*       MB_ERR_
*
* Error codes.
*
**********************************************************************
*/

//
// Errors reported by slave. Minus sign is removed when sent in message as these are standardized codes used by Modbus.
//
#define MB_ERR_ILLEGAL_FUNC       -1
#define MB_ERR_ILLEGAL_DATA_ADDR  -2
#define MB_ERR_ILLEGAL_DATA_VAL   -3
#define MB_ERR_SLAVE_FAIL         -4
#define MB_ERR_ACK                -5
#define MB_ERR_SLAVE_BUSY         -6
#define MB_ERR_NACK               -7
#define MB_ERR_MEM_PARITY_ERR     -8

//
// Errors, stack internal. Error codes are not allowed to collide with Modbus error codes.
//
#define MB_ERR_MISC               -20  // Any error, unspecified.
#define MB_ERR_CONNECT            -21  // Error while connecting.
#define MB_ERR_CONNECT_TIMEOUT    -22  // Timeout while connecting.
#define MB_ERR_DISCONNECT         -23  // Interface signalled disconnect.
#define MB_ERR_TIMEOUT            -24  // No answer to request received.
#define MB_ERR_CHECKSUM           -25  // Received message did not pass the LRC/CRC check.
#define MB_ERR_PARAM              -26  // Parameter error in API call.
#define MB_ERR_SLAVE_ADDR         -27  // Received a valid response with wrong slave addr.
#define MB_ERR_FUNC_CODE          -28  // Received a valid response with wrong function code.
#define MB_ERR_REF_NO             -29  // Received a valid response with wrong reference number.
#define MB_ERR_NUM_ITEMS          -30  // Received a valid response with more or less items than in requested.
#define MB_ERR_DATA               -31  // Received a valid response for a single write with different data than written.
#define MB_ERR_TRIAL_LIMIT        -32  // Trial limit exceeded.
#define MB_ERR_WOULD_BLOCK        -33  // TCP non-blocking recv() would block situation.

/*********************************************************************
*
*       Slave function table (returns Modbus errorcode)
*
**********************************************************************
*/

typedef struct {
  int (*pfWriteCoil)(U16 Addr, char OnOff);  // Write coil.
  int (*pfReadCoil) (U16 Addr);              // Read coil status.
  int (*pfReadDI)   (U16 Addr);              // Read Discrete Input status.
  int (*pfWriteReg) (U16 Addr, U16   Val);   // Write register.
  int (*pfReadHR)   (U16 Addr, U16 *pVal);   // Read Holding Register.
  int (*pfReadIR)   (U16 Addr, U16 *pVal);   // Read Input Register.
} MB_SLAVE_API;

/*********************************************************************
*
*       Interface configuration structures
*
**********************************************************************
*/

typedef void * MB_SOCKET;

typedef struct {
  U32 Cnt;               // RTU timeout countdown.
  U32 CntReload;         // RTU countdown reload value.
  U32 Baudrate;          // Baudrate to use.
  U8  DataBits;          // Number of data bits.
  U8  Parity;            // Parity as interpreted by application.
  U8  StopBits;          // Number of stop bits.
  U8  Port;              // Interface index.
} MB_IFACE_CONFIG_UART;

typedef struct {
  MB_SOCKET Sock;        // Socket used for send and receive.
  MB_SOCKET ListenSock;  // Socket used by TCP for listen() and accept(). Not needed for UDP.
  U32       IPAddr;      // Master: Addr. to connect to; Slave: Filter addr. If set only connections on this addr. should be accepted.
  U16       Port;        // Master: Port to connect to; Slave: Port that accepts connections for this channel.
  U16       xID;         // Master: Transaction ID that is incremented for each send. Slave: Ignored.
} MB_IFACE_CONFIG_IP;

typedef MB_IFACE_CONFIG_UART MB_IFACE_CONFIG;

/*********************************************************************
*
*       Interface function tables
*
**********************************************************************
*/

//
// Used for all interfaces.
// Interface function table is implemented two times with the same functionality
// to avoid casting of interface specific pointers such as pConfig that might be
// a pointer to a MB_IFACE_CONFIG_UART or MB_IFACE_CONFIG_IP in the application.
//
typedef struct {
  void (*pfSendByte)   (MB_IFACE_CONFIG_UART *pConfig, U8 Data);                                     // Send first byte. Every next byte will be sent via MB_OnTx() from interrupt. NULL if stream oriented interface.
  int  (*pfInit)       (MB_IFACE_CONFIG_UART *pConfig);                                              // Init hardware. NULL if not needed.
  void (*pfDeInit)     (MB_IFACE_CONFIG_UART *pConfig);                                              // De-Init hardware. NULL if not needed.
  int  (*pfSend)       (MB_IFACE_CONFIG_UART *pConfig, const U8 *pData, U32 NumBytes);               // Send data for stream oriented interface. NULL if byte oriented interface is used. In this case pfSendByte will be used.
  int  (*pfRecv)       (MB_IFACE_CONFIG_UART *pConfig,       U8 *pData, U32 NumBytes, U32 Timeout);  // Typically data is received via MB_OnRx() from interrupt. NULL if not using polling mode.
  int  (*pfConnect)    (MB_IFACE_CONFIG_UART *pConfig, U32 Timeout);                                 // NULL.
  void (*pfDisconnect) (MB_IFACE_CONFIG_UART *pConfig);                                              // NULL.
  void (*pfInitTimer)  (U32 MaxFreq);                                                                // Typically only needed for RTU interfaces. Initializes a timer needed for RTU timeout.
  void (*pfDeInitTimer)(void);                                                                       // De-initialize RTU timer.
} MB_IFACE_UART_API;

typedef struct {
  void (*pfSendByte)   (MB_IFACE_CONFIG_IP *pConfig, U8 Data);                                       // Send first byte. Every next byte will be sent via MB_OnTx() from interrupt. NULL if stream oriented interface.
  int  (*pfInit)       (MB_IFACE_CONFIG_IP *pConfig);                                                // Init IP and get listen socket and bring it in listen state if needed. NULL if not needed.
  void (*pfDeInit)     (MB_IFACE_CONFIG_IP *pConfig);                                                // Close listen socket and de-init IP. NULL if not needed.
  int  (*pfSend)       (MB_IFACE_CONFIG_IP *pConfig, const U8 *pData, U32 NumBytes);                 // Send data for stream oriented interface. NULL if byte oriented interface is used. In this case pfSendByte will be used.
  int  (*pfRecv)       (MB_IFACE_CONFIG_IP *pConfig,       U8 *pData, U32 NumBytes, U32 Timeout);    // Request more data.
  int  (*pfConnect)    (MB_IFACE_CONFIG_IP *pConfig, U32 Timeout);                                   // Master: Connect to slave; Slave: Accept connection if needed. NULL if not needed.
  void (*pfDisconnect) (MB_IFACE_CONFIG_IP *pConfig);                                                // Master: Disconnect from slave; Slave: Close connection if needed. NULL if not needed.
  void (*pfInitTimer)  (U32 MaxFreq);                                                                // NULL.
  void (*pfDeInitTimer)(void);                                                                       // NULL.
} MB_IFACE_IP_API;

typedef MB_IFACE_UART_API MB_IFACE_API;

/*********************************************************************
*
*       Types/structures
*
**********************************************************************
*/

typedef struct {
  U8  *pData;       // Beginning of input/output buffer.
  U32  DataLen;     // Data length received.
  U32  BufferSize;  // Max. buffer size that can be used for an answer.
  U8   SlaveAddr;   // Slave addr. for which the message has been received.
  U8   Function;    // Function code received.
} MB_CUSTOM_FUNC_CODE_PARA;

struct MB_CHANNEL;
typedef int (*MB_pfCustomFunctionCodeHandler)(struct MB_CHANNEL *pChannel, MB_CUSTOM_FUNC_CODE_PARA *pPara);

typedef struct MB_CHANNEL {
  struct MB_CHANNEL                     *pNext;                        // Next pointer to chain channels for internal list.
         U8                             *pData;                        // Pointer to pure RTU message in buffer.
         U8                              aBuffer[MB_BUFFER_SIZE];      // Buffer that holds received message and message to send. We accept 256 bytes RTU message + overhead (ASCII: 5 bytes; Modbus/TCP: 6 bytes).
         U32                             NumBytesInBuffer;             // Number of bytes currently in buffer.
  const  MB_SLAVE_API                   *pSlaveAPI;                    // Pointer to slave hardware API. NULL for master channels.
  const  MB_IFACE_API                   *pIFaceAPI;                    // Pointer to interface API.
  const  void                           *pIFaceProtAPI;                // Pointer to internal API handling wrapper protocol.
  const  void                           *pSignalAPI;                   // Pointer to internal API for signalling a channel.
         MB_pfCustomFunctionCodeHandler  pfCustomFunctionCodeHandler;  // Function pointer for handling custom function codes received as slave.
         MB_IFACE_CONFIG                *pConfig;                      // Pointer to interface configuration storage.
         U32                             Timeout;                      // Master receive timeout [ms].
         U8                              SlaveAddr;                    // Slave addr. itself or slave addr. to connect to.
         U8                              SlaveAddrIgnored;             // Ignore the slave addr. ? Can be used with Modbus/TCP channels as they are unique by their port.
         U8                              DisableWrite;                 // Is write access to this channel disabled ?
         U8                              IsConnected;                  // Is channel currently connected ?
         U8                              IsSignalled;                  // Channel is signalled to have Modbus data ready to be processed by the stack.
         U8                              Endpoint;                     // Is this a master or slave channel ?
         U8                              HasError;                     // Is an error pending on that channel ?
} MB_CHANNEL;

/*********************************************************************
*
*       Config I/O
*
**********************************************************************
*/

void MB_Log  (const char *s);
void MB_Panic(const char *s);
void MB_Warn (const char *s);

/*********************************************************************
*
*       Core API
*
**********************************************************************
*/

void MB_ConfigTimerFreq(U32 Freq);
void MB_OnRx           (MB_CHANNEL *pChannel, U8 Data);
int  MB_OnTx           (MB_CHANNEL *pChannel);
void MB_TimerTick      (void);

/*********************************************************************
*
*       MB_CHANNEL_
*
**********************************************************************
*/

void MB_CHANNEL_Disconnect(MB_CHANNEL *pChannel);

/*********************************************************************
*
*       MB_MASTER_
*
**********************************************************************
*/

void MB_MASTER_AddASCIIChannel      (MB_CHANNEL *pChannel, MB_IFACE_CONFIG_UART *pConfig, const MB_IFACE_UART_API *pIFaceAPI, U32 Timeout, U8 SlaveAddr, U32 Baudrate, U8 DataBits, U8 Parity, U8 StopBits, U8 Port);
void MB_MASTER_AddRTUChannel        (MB_CHANNEL *pChannel, MB_IFACE_CONFIG_UART *pConfig, const MB_IFACE_UART_API *pIFaceAPI, U32 Timeout, U8 SlaveAddr, U32 Baudrate, U8 DataBits, U8 Parity, U8 StopBits, U8 Port);
void MB_MASTER_AddIPChannel         (MB_CHANNEL *pChannel, MB_IFACE_CONFIG_IP   *pConfig, const MB_IFACE_IP_API   *pIFaceAPI, U32 Timeout, U8 SlaveAddr, U32 IPAddr, U16 Port);

void MB_MASTER_DeInit               (void);
void MB_MASTER_Init                 (void);

int  MB_MASTER_ReadCoils            (MB_CHANNEL *pChannel, U8 *pData, U16 Addr, U16 NumItems);
int  MB_MASTER_ReadDI               (MB_CHANNEL *pChannel, U8 *pData, U16 Addr, U16 NumItems);
int  MB_MASTER_ReadHR               (MB_CHANNEL *pChannel, U8 *pData, U16 Addr, U16 NumItems);
int  MB_MASTER_ReadIR               (MB_CHANNEL *pChannel, U8 *pData, U16 Addr, U16 NumItems);
int  MB_MASTER_WriteCoil            (MB_CHANNEL *pChannel, U16 Addr, U8 OnOff);
int  MB_MASTER_WriteReg             (MB_CHANNEL *pChannel, U16 Data, U16 Addr);
int  MB_MASTER_WriteCoils           (MB_CHANNEL *pChannel, U8  *pData, U16 Addr, U16 NumItems);
int  MB_MASTER_WriteRegs            (MB_CHANNEL *pChannel, U16 *pData, U16 Addr, U16 NumItems);

/*********************************************************************
*
*       MB_SLAVE_
*
**********************************************************************
*/

void MB_SLAVE_AddASCIIChannel             (MB_CHANNEL *pChannel, MB_IFACE_CONFIG_UART *pConfig, const MB_SLAVE_API *pSlaveAPI, const MB_IFACE_UART_API *pIFaceAPI, U8 SlaveAddr, U8 DisableWrite, U32 Baudrate, U8 DataBits, U8 Parity, U8 StopBits, U8 Port);
void MB_SLAVE_AddRTUChannel               (MB_CHANNEL *pChannel, MB_IFACE_CONFIG_UART *pConfig, const MB_SLAVE_API *pSlaveAPI, const MB_IFACE_UART_API *pIFaceAPI, U8 SlaveAddr, U8 DisableWrite, U32 Baudrate, U8 DataBits, U8 Parity, U8 StopBits, U8 Port);
void MB_SLAVE_AddIPChannel                (MB_CHANNEL *pChannel, MB_IFACE_CONFIG_IP   *pConfig, const MB_SLAVE_API *pSlaveAPI, const MB_IFACE_IP_API   *pIFaceAPI, U8 SlaveAddr, U8 DisableWrite, U32 IPAddr, U16 Port);

void MB_SLAVE_ConfigIgnoreSlaveAddr       (MB_CHANNEL *pChannel, U8 OnOff);
void MB_SLAVE_DeInit                      (void);
void MB_SLAVE_Exec                        (void);
void MB_SLAVE_Init                        (void);
int  MB_SLAVE_PollChannel                 (MB_CHANNEL *pChannel);
void MB_SLAVE_SetCustomFunctionCodeHandler(MB_CHANNEL *pChannel, MB_pfCustomFunctionCodeHandler pf);
void MB_SLAVE_Task                        (void);

/*********************************************************************
*
*       MB_OS_
*
**********************************************************************
*/

      void  MB_OS_DeInitMaster    (void);
      void  MB_OS_InitMaster      (void);
      void  MB_OS_DeInitSlave     (void);
      void  MB_OS_InitSlave       (void);
      void  MB_OS_DisableInterrupt(void);
      void  MB_OS_EnableInterrupt (void);
      U32   MB_OS_GetTime         (void);
const char* MB_OS_GetTaskName     (void *pTask);
      void  MB_OS_SignalNetEvent  (void);
      void  MB_OS_WaitNetEvent    (unsigned ms);
      void  MB_OS_SignalItem      (void *pWaitItem);
      void  MB_OS_WaitItemTimed   (void *pWaitItem, unsigned Timeout);

/*********************************************************************
*
*       Utility
*
**********************************************************************
*/

U32  MB_LoadU16BE (const U8 *pData);
void MB_StoreU16BE(      U8 *pData, U16 v);


#if defined(__cplusplus)
  }     // Make sure we have C-declarations in C++ programs
#endif

#endif  // Avoid multiple inclusion

/****** End Of File *************************************************/
