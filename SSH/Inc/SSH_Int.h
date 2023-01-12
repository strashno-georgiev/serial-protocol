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

File        : SSH_Int.h
Purpose     : SSH private header.

*/

#ifndef SSH_INT_H
#define SSH_INT_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "SSH.h"
#include "SSH_ConfDefaults.h"
#include "CRYPTO_Int.h"
#include "CRYPTO_ConfDefaults.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
*
*       Lint configuration
*
**********************************************************************
*/

/*lint -esym(534,memcpy)  */
/*lint -esym(534,memset)  */
/*lint -esym(534,memmove) */

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#define SSH_CHECK(X)                 /*lint -e{801}     */ { Status = (X); if (Status < 0) { goto Finally; } }  /*Goto is used for exceptional exits*/
#define SSH_RAISE(X)                 /*lint -e{717,801} */ do { Status = X; goto Finally; } while (0)
#define SSH_ALLOC(LVALUE, SIZE)      /*lint -e{801,717} */ do { if (((LVALUE) = SSH_MEM_Alloc(SIZE)) == 0 && (SIZE) > 0) { Status = SSH_ERROR_OUT_OF_MEMORY; goto Finally; } } while (0)
#define SSH_BUFFER(N)                struct { U8 aData[N]; U32 Len; }

#define SSH_MAC_LENGTH_MAX           64  // Maximum size of a MAC in emSSH (== SHA512).

typedef struct {
  void       * pNext;
  const char * sIANAName;
} SSH_NAMELIST_LINK;

struct SSH_KEY_EXCHANGE_ALGORITHM_tag {
  SSH_NAMELIST_LINK Link;
  unsigned          HashSize;
  //
  void            (* pfCalcHash)     (U8 *pOutput, unsigned OutputLen, const U8 *pInput, unsigned InputLen);
  void            (* pfDeriveKey)    (SSH_SESSION *pSelf, CRYPTO_MPI *pK, U8 Ch, U8 *pDest, unsigned DestLen);
  int             (* pfHandlePacket) (SSH_SESSION *pSelf, unsigned Tag, CRYPTO_TLV *pTLV);
  int             (* pfStartKEX)     (SSH_SESSION *pSelf);
  int             (* pfOAEPDecrypt)  (const CRYPTO_RSA_PRIVATE_KEY *pSelf, U8 *pOutput, unsigned OutputLen, const U8 *pInput, unsigned InputLen, const U8 *pLabel, unsigned LabelLen, CRYPTO_MEM_CONTEXT *pMem);
  //
  const CRYPTO_MPI      *pDHGroupModulus;  // For DH key exchanges.
  const CRYPTO_EC_CURVE *pECDHCurve;       // For ECDH key exchanges.
};

struct SSH_USERAUTH_METHOD_tag {
  SSH_NAMELIST_LINK           Link;
  SSH_USERAUTH_REQUEST_FUNC * pfCallback;
};

struct SSH_CHANNEL_REQUEST_tag {
  SSH_NAMELIST_LINK          Link;
  SSH_CHANNEL_REQUEST_FUNC * pfCallback;
};

struct SSH_SERVICE_tag {
  SSH_NAMELIST_LINK          Link;
  SSH_SERVICE_REQUEST_FUNC * pfCallback;
};

typedef struct {
  const CRYPTO_EC_CURVE * pCurve;
  const char            * sCurveName;      // SSL curve name, e.g. nistp256
  //
  int (*pfSign)(const CRYPTO_EC_CURVE *pParams, const CRYPTO_ECDSA_PRIVATE_KEY *pKey, const U8 *pMessage, unsigned MessageLen, CRYPTO_DSA_SIGNATURE *pSignature, SEGGER_MEM_CONTEXT *pMem);
} SSH_ECDSA_PARAS;

struct SSH_PUBLIC_KEY_ALGORITHM_tag {
  SSH_NAMELIST_LINK Link;
  //
  void (*pfPutPublicKeyBlob) (SSH_PUBLIC_KEY_ALGORITHM *pSelf, CRYPTO_BUFFER *pBuffer);
  int  (*pfPutSignature)     (SSH_PUBLIC_KEY_ALGORITHM *pSelf, CRYPTO_BUFFER *pBuffer, const U8 *pH, unsigned HLen);
  //
  const SSH_ECDSA_PARAS *pECDSAParas;
};

struct SSH_ENCRYPTION_ALGORITHM_tag {
  SSH_NAMELIST_LINK Link;
  U16               ContextSize; // Cipher context size
  U8                KeySize;     // Key size for cipher in octets
  U8                IVSize;      // IV size for cipher in octets
  U8                BlockSize;   // Block size for cipher in octets
  //
  void           (* pfInitEncrypt) (void *pContext, const U8 *pKey);
  void           (* pfInitDecrypt) (void *pContext, const U8 *pKey);
  void           (* pfEncrypt)     (void *pContext, U8 *pOutput, const U8 *pInput, unsigned Len, U8 *pIV);
  void           (* pfDecrypt)     (void *pContext, U8 *pOutput, const U8 *pInput, unsigned Len, U8 *pIV);
  void           (* pfAEADEncrypt) (void *pContext, U8 *pOutput,       U8 *pTag, unsigned TagLen, const U8 *pInput, unsigned InputLen, U8 *pAAD, unsigned AADLen, U8 *aIV, unsigned IVLen, U64 SeqNumber);
  int            (* pfAEADDecrypt) (void *pContext, U8 *pOutput, const U8 *pTag, unsigned TagLen, const U8 *pInput, unsigned InputLen, U8 *pAAD, unsigned AADLen, U8 *aIV, unsigned IVLen, U64 SeqNumber);
  unsigned       (* pfAEADGetLen)  (void *pContext, U8 *pInput, U64 SeqNumber);
};

struct SSH_MAC_ALGORITHM_tag {
  SSH_NAMELIST_LINK Link;
  unsigned          ContextSize;
  unsigned          KeySize;
  unsigned          MACSize;
  unsigned          EncryptThenMAC;
  //
  void           (* pfInit)  (void *pContext, const U8 aKey[],  unsigned KeyByteCnt);
  void           (* pfAdd)   (void *pContext, const U8 aData[], unsigned DataByteCnt);
  void           (* pfFinal) (void *pContext, U8 *pMAC);
};

struct SSH_COMPRESSION_ALGORITHM_tag {
  SSH_NAMELIST_LINK Link;
} ;

typedef enum {
  SSH_SERVER_STATE_FREE,              // Free for reuse
  SSH_SERVER_STATE_IDLE,              // Allocated, waiting to be initialized
  SSH_SERVER_STATE_SEND_BANNER,
  SSH_SERVER_STATE_WAIT_ID,
  SSH_SERVER_STATE_SEND_KEX,
  SSH_SERVER_STATE_MAINLINE,
  SSH_SERVER_STATE_FORCED_DISCONNECT,
  SSH_SERVER_STATE_ERROR
} SSH_SERVER_STATE;

typedef struct {
  U32 SequenceNum;
  U8  aIV[64];  // In this we expect no cipher to require no more than 64 bytes of keying information.
  U8  aCipherKey[64];
  U8  aMACKey[64];
} SSH_CHANNEL_PARA;

struct SSH_SESSION_tag {
  //
  // Pointer to APIs
  //
  const SSH_TRANSPORT_API    * pTransportAPI;
  //
  int                          Socket;
  SSH_SERVER_STATE             State;
  int                          Disconnecting;
  U8                           aServerCookie[16];
  unsigned                     HashSize;
  U8                           aH         [CRYPTO_SHA512_DIGEST_BYTE_COUNT];   // Exchange hash, big enough for all signatures
  U8                           aSessionID [CRYPTO_SHA512_DIGEST_BYTE_COUNT];   // Session ID, big enough for all signatures
  SSH_BUFFER(128)              V_C;                  // Client's identification string
  SSH_BUFFER(128)              V_S;                  // Server's identification string
  SSH_BUFFER(2048)             I_C;
  //
  CRYPTO_MPI                   K;
  struct { 
    U32                        Min;
    U32                        Preferred;
    U32                        Max;
    const CRYPTO_MPI         * pGroupModulus;
    CRYPTO_MPI                 E; 
  } DH;                       // Received from client
  //
  SSH_NAMELIST_LINK          * pKEX;                 // Agreed components.
  SSH_NAMELIST_LINK          * pHostKey;             // ditto
  SSH_NAMELIST_LINK          * pCipher;              // ditto
  SSH_NAMELIST_LINK          * pMAC;                 // ditto
  SSH_NAMELIST_LINK          * pCompress;            // ditto
  int                          KEXIgnoreCount;
  //
  SSH_MAC_ALGORITHM          * pMACAlgorithm;
  SSH_ENCRYPTION_ALGORITHM   * pBulkCipher;
  SSH_PUBLIC_KEY_ALGORITHM   * pPublicKeyAlgorithm;
  SSH_KEY_EXCHANGE_ALGORITHM * pKeyExchangeAlgorithm;
  //
  void                       * pMACContext;
  void                       * pEncryptContext;
  void                       * pDecryptContext;
  //
  SSH_CHANNEL_PARA             ClientToServer;
  SSH_CHANNEL_PARA             ServerToClient;
  //
  U8                         * pRxBuffer;
  unsigned                     RxBufferLen;
  U8                         * pTxBuffer;
  unsigned                     TxBufferLen;
};

typedef struct {
  const SSH_CHANNEL_API * pAPI;
  SSH_SESSION           * pSession;                   // Session this channel is associated with.
  unsigned                RemoteChannel;
  U32                     IncomingWindowSize;
  U32                     IncomingMaxPacketSize;
  U32                     OutgoingWindowSize;
  U32                     OutgoingMaxPacketSize;
  void                  * pUserContext;
  int                     Closing;
} SSH_CHANNEL;

typedef struct {
  const SSH_HOSTKEY_API * pIdentityAPI;
  SSH_NAMELIST_LINK     * pKeyExchangeAlgorithms;
  SSH_NAMELIST_LINK     * pPublicKeyAlgorithms;
  SSH_NAMELIST_LINK     * pEncryptionAlgorithms;
  SSH_NAMELIST_LINK     * pMacAlgorithms;
  SSH_NAMELIST_LINK     * pCompressionAlgorithms;
  SSH_NAMELIST_LINK     * pUserauthMethods;
  SSH_NAMELIST_LINK     * pChannelRequests;
  SSH_NAMELIST_LINK     * pServices;
  SSH_CHANNEL             aChannel[SSH_CONFIG_MAX_CHANNELS];
  SSH_SESSION             aSession[SSH_CONFIG_MAX_SESSIONS];
  const char            * sInitFlag;
} SSH_GLOBALS;

/*********************************************************************
*
*       Public types
*
**********************************************************************
*/

typedef enum {
  SSH_SCP_SINK_STATE_FREE,
  SSH_SCP_SINK_STATE_INIT,
  SSH_SCP_SINK_STATE_HEADER,
  SSH_SCP_SINK_STATE_FILE_DATA,
  SSH_SCP_SINK_STATE_CONFIRM,
} SSH_SCP_SINK_STATE;

struct SSH_SCP_SINK_CONTEXT_tag {
  SSH_SCP_SINK_STATE State;              // Internal sink state machine state
  unsigned           Channel;            // Associated SSH channel
  int                Index;              // SCP session index
  U32                DataPhaseLen;       // Number of bytes required in the data phase
  U32                ModTime;            // Modification time, Unix format, seconds since 1 Jan 1970.
  U32                AccTime;            // Last access time, Unix format, seconds since 1 Jan 1970.
  unsigned           HeaderLen;          // Length of header
  char               aHeader[SSH_SCP_CONFIG_PATH_MAX];
};

typedef enum {
  SSH_SCP_SOURCE_STATE_FREE,
  SSH_SCP_SOURCE_STATE_INIT,
  SSH_SCP_SOURCE_STATE_NEXT,
  SSH_SCP_SOURCE_STATE_SETUP,
  SSH_SCP_SOURCE_STATE_SEND,
  SSH_SCP_SOURCE_STATE_ERROR,
} SSH_SCP_SOURCE_STATE;

struct SSH_SCP_SOURCE_CONTEXT_tag {
  SSH_SCP_SOURCE_STATE     State;        // Internal source state machine state
  unsigned                 Channel;      // Associated SSH channel
  int                      Index;        // SCP session index
  int                      AwaitAck;     // Awaiting Ack or Nack?
  char                     aPath[SSH_SCP_CONFIG_PATH_MAX];
  SSH_SCP_SOURCE_FILE_INFO FileInfo;
};

/*********************************************************************
*
*       Public data
*
**********************************************************************
*/

/*********************************************************************
*
*       Management.
*/
extern SSH_GLOBALS          SSH_Globals;
extern SEGGER_MEM_CONTEXT * SSH_MEM__pDefaultContext;

/*********************************************************************
*
*       SSH memory wrappers.
*/
void *  SSH_MEM_Alloc             (unsigned ByteCnt);
void *  SSH_MEM_ZeroAlloc         (unsigned ByteCnt);
void    SSH_MEM_Free              (void *pMemory);

/*********************************************************************
*
*       SSH memory architecture.
*/
void    SSH_MEM_ConfigBestFit     (void);
void    SSH_MEM_ConfigUser        (SEGGER_MEM_CONTEXT *pMem);

#ifdef __cplusplus
}
#endif

#endif

/*************************** End of file ****************************/
