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

File        : COMPRESS.h
Purpose     : SEGGER Compression Library User-Level API.

*/

#ifndef COMPRESS_H
#define COMPRESS_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "COMPRESS_ConfDefaults.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
*
*       Defines, fixed.
*
**********************************************************************
*/

#define COMPRESS_VERSION        21400

/*********************************************************************
*
*       Types required for API
*
**********************************************************************
*/

// Completely opaque API
typedef struct COMPRESS_DECODE_API_tag COMPRESS_DECODE_API;
  
/*********************************************************************
*
*       COMPRESS_OUTPUT_FUNC
*
*  Description
*    Output compressed data.
*
*  Parameters
*    pSelf   - Pointer to context.
*    pData   - Pointer to decompressed octet string.
*    DataLen - Octet length of the decompressed octet string.
*
*  Return value
*    <  0 - Abort processing
*    >= 0 - Continue decompressing.
*/
typedef int (COMPRESS_OUTPUT_FUNC)(void *pContext, void *pData, unsigned DataLen);

/*********************************************************************
*
*       COMPRESS_CRC_FUNC
*
*  Description
*    Update CRC.
*
*  Parameters
*    pData   - Pointer to octet string to add to CRC.
*    DataLen - Octet length of the octet string.
*    CRC     - Current CRC.
*
*  Return value
*    New CRC.
*/
typedef U32 (COMPRESS_CRC_FUNC)(const U8 *pData, U32 DataLen, U32 CRC);

/*********************************************************************
*
*       COMPRESS_PARA
*
*  Description
*    Compression parameter (internal).
*/
typedef U32 COMPRESS_PARA;

/*********************************************************************
*
*       COMPRESS_ENCODED_DATA
*
*  Description
*    Address-length pair.
*/
typedef struct {
  const U8 * pData;        // Pointer to encoded data
  U32        DataLen;      // Octet length of the encoded data
} COMPRESS_ENCODED_DATA;

/*********************************************************************
*
*       COMPRESS_ENCODED_FILE
*
*  Description
*    Encoded file instance.
*
*  Additional information
*    This type should not be accessed directly; there are query
*    functions that query this data structure.
*/
typedef struct {
  const U8                  * pEncodedData;       // Compressed bitstream.
  U32                         EncodedDataLength;  // Compressed image size.
  U32                         EncodedDataCRC;     // Compressed image CRC.
  const COMPRESS_DECODE_API * pDecoder;           // Pointer to decoder for compressed bitstream.
  U32                         DecodedDataStart;   // Start of slice within original image.
  U32                         DecodedDataLength;  // Size of slice within original image.
  U32                         DecodedDataCRC;     // CRC over original input slice.
  U32                         WorkspaceSize;      // Number of bytes required for workspace on decompression.
  COMPRESS_PARA               aPara[3];
} COMPRESS_ENCODED_FILE;

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

/*********************************************************************
*
*       All-in-one packaged API.
*/
int  COMPRESS_DecompressThruFunc   (const COMPRESS_ENCODED_FILE *pSelf, void *pWorkspace, unsigned WorkspaceLen, COMPRESS_OUTPUT_FUNC pfOutput, void *pContext, U32 Start, U32 Len, COMPRESS_CRC_FUNC pfCalcCRC);
int  COMPRESS_DecompressToMem      (const COMPRESS_ENCODED_FILE *pSelf, void *pWorkspace, unsigned WorkspaceLen, void *pDest,                                   U32 Start, U32 Len, COMPRESS_CRC_FUNC pfCalcCRC);

/*********************************************************************
*
*       Inquiries.
*/
void COMPRESS_QueryEncodedData     (const COMPRESS_ENCODED_FILE *pSelf, COMPRESS_ENCODED_DATA *pData);
U32  COMPRESS_QueryEncodedDataSize (const COMPRESS_ENCODED_FILE *pSelf);
U32  COMPRESS_QueryEncodedDataCRC  (const COMPRESS_ENCODED_FILE *pSelf);
U32  COMPRESS_QueryDecodedDataSize (const COMPRESS_ENCODED_FILE *pSelf);
U32  COMPRESS_QueryDecodedDataCRC  (const COMPRESS_ENCODED_FILE *pSelf);
U32  COMPRESS_QueryWorkspaceSize   (const COMPRESS_ENCODED_FILE *pSelf);

/*********************************************************************
*
*       Failure handling functions.
*/
void COMPRESS_AssertFail           (const char *sFilename, int LineNumber, const char *sAssertion);

/*********************************************************************
*
*       Internal non-public data
*
**********************************************************************
*/

/*********************************************************************
*
*       Internal dispatch APIs for compiling compressed files.
*/
extern const COMPRESS_DECODE_API COMPRESS_RLE_Decode;
extern const COMPRESS_DECODE_API COMPRESS_LZSS_Decode;
extern const COMPRESS_DECODE_API COMPRESS_LZJU90_Decode;
extern const COMPRESS_DECODE_API COMPRESS_LZ4_Decode;
extern const COMPRESS_DECODE_API COMPRESS_LZW_Decode;
extern const COMPRESS_DECODE_API COMPRESS_DEFLATE_Decode;
extern const COMPRESS_DECODE_API COMPRESS_HUFF_Decode;
extern const COMPRESS_DECODE_API COMPRESS_STORE_Decode;

#ifdef __cplusplus
}
#endif

#endif

/*************************** End of file ****************************/
