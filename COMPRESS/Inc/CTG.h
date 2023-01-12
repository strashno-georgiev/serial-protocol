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

File    : CTG.h
Purpose : emCompress-ToGo public header file.

*/

#ifndef CTG_H            // Avoid multiple inclusion.
#define CTG_H

#if defined(__cplusplus)
  extern "C" {                // Make sure we have C-declarations in C++ programs.
#endif

#include "CTG_ConfDefaults.h"
#include "Global.h"

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

/*********************************************************************
*
*       Version number
*
*  Description
*    Symbol expands to a number that identifies the specific emCompress-ToGo release.
*/
#define CTG_VERSION               21000   // Format is "Mmmrr" so, for example, 21000 corresponds to version 2.10.

/*********************************************************************
*
*       Function status values
*
*  Description
*    Compression and decompression errors.
*
*  Additional information
*    Values returned as errors by the compression and decompression
*    functions.  All errors are negative.
*/
#define CTG_STATUS_OUTPUT_OVERFLOW   (-100)     // The output buffer is not big enough to contain the compressed or decompressed output.
#define CTG_STATUS_INPUT_UNDERFLOW   (-101)     // The input buffer contains a bitstream that is truncated.
#define CTG_STATUS_BUFFER_TOO_SMALL  (-102)     // The work buffer provided to the compressor or decompressor is too smal.
#define CTG_STATUS_BAD_PARAMETER     (-103)     // A single compression parameter, or combination of parameters, is invalid.

/*********************************************************************
*
*       Fixed bits parameter
*
*  Description
*    Number of fixed distance bits.
*
*  Additional information
*    The number of fixed distance bits will affect the encoding
*    of the compressed bitstream and, therefore, its size.
*/
#define CTG_FLAG_FIXED_6BIT     (0 << 0)    // Fixed bits set to 6.
#define CTG_FLAG_FIXED_7BIT     (1 << 0)    // Fixed bits set to 7.
#define CTG_FLAG_FIXED_8BIT     (2 << 0)    // Fixed bits set to 8.
#define CTG_FLAG_FIXED_9BIT     (3 << 0)    // Fixed bits set to 9.

/*********************************************************************
*
*       Maximum distance parameter
*
*  Description
*    Maximum distance encoded in a reference.
*
*  Additional information
*    Smaller distances will make compression run faster, larger
*    distances usually make the compressed output smaller.
*/
#define CTG_FLAG_MAX_DIST_256   (0 << 2)    // Maximum distance set to 256
#define CTG_FLAG_MAX_DIST_512   (1 << 2)    // Maximum distance set to 512
#define CTG_FLAG_MAX_DIST_1K    (2 << 2)    // Maximum distance set to 1K
#define CTG_FLAG_MAX_DIST_2K    (3 << 2)    // Maximum distance set to 2K
#define CTG_FLAG_MAX_DIST_4K    (4 << 2)    // Maximum distance set to 4K
#define CTG_FLAG_MAX_DIST_8K    (5 << 2)    // Maximum distance set to 8K
#define CTG_FLAG_MAX_DIST_16K   (6 << 2)    // Maximum distance set to 16K
#define CTG_FLAG_MAX_DIST_32K   (7 << 2)    // Maximum distance set to 32K
#define CTG_FLAG_MAX_DIST_64K   (8 << 2)    // Maximum distance set to 64K
#define CTG_FLAG_MAX_DIST_128K  (9 << 2)    // Maximum distance set to 128K

/*********************************************************************
*
*       Maximum length parameter
*
*  Description
*    Maximum length encoded in a reference.
*
*  Additional information
*    Smaller lengths will make compression run faster, larger
*    lengths usually make the compressed output smaller.
*/
#define CTG_FLAG_MAX_LEN_16     ( 0 << 28)    // Maximum length set to 16
#define CTG_FLAG_MAX_LEN_32     ( 1 << 28)    // Maximum length set to 32
#define CTG_FLAG_MAX_LEN_64     ( 2 << 28)    // Maximum length set to 64
#define CTG_FLAG_MAX_LEN_128    ( 3 << 28)    // Maximum length set to 128
#define CTG_FLAG_MAX_LEN_256    ( 4 << 28)    // Maximum length set to 256
#define CTG_FLAG_MAX_LEN_512    ( 5 << 28)    // Maximum length set to 512
#define CTG_FLAG_MAX_LEN_1K     ( 6 << 28)    // Maximum length set to 1K
#define CTG_FLAG_MAX_LEN_2K     ( 7 << 28)    // Maximum length set to 2K
#define CTG_FLAG_MAX_LEN_4K     ( 8 << 28)    // Maximum length set to 4K
#define CTG_FLAG_MAX_LEN_8K     ( 9 << 28)    // Maximum length set to 8K
#define CTG_FLAG_MAX_LEN_16K    (10 << 28)    // Maximum length set to 16K
#define CTG_FLAG_MAX_LEN_32K    (11 << 28)    // Maximum length set to 32K
#define CTG_FLAG_MAX_LEN_64K    (12 << 28)    // Maximum length set to 64K

/*********************************************************************
*
*       Types, global
*
**********************************************************************
*/

/*********************************************************************
*
*       CTG_RD_FUNC()
*
*  Description
*    Read data form source.
*
*  Parameters
*    pData   - Pointer to object that receives the data.
*    DataLen - Maximum number of bytes to read into object, will be nonzero.
*    pRdCtx  - Client-supplied read context.
*
*  Return value
*    <  0 - Error reading data.
*    == 0 - Success, end of stream, no further data.
*    >  0 - Success, number of bytes placed into object, more data to read.
*/
typedef int (CTG_RD_FUNC)(U8 *pData, unsigned DataLen, void *pRdCtx);

/*********************************************************************
*
*       CTG_WR_FUNC()
*
*  Description
*    Write data to sink.
*
*  Parameters
*    pData   - Pointer to object to write.
*    DataLen - Number of bytes to write.
*    pWrCtx  - Client-supplied read context.
*
*  Return value
*    <  0 - Error writing data.
*    >= 0 - Success.
*/
typedef int (CTG_WR_FUNC)(const U8 *pData, unsigned DataLen, void *pWrCtx);

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

/*********************************************************************
*
*       One-call compression functions
*/
int  CTG_CompressM2M    (const U8 *pInput, unsigned InputLen, U8 *pOutput, unsigned OutputLen, unsigned Flags);
int  CTG_CompressM2F    (const U8 *pInput, unsigned InputLen, CTG_WR_FUNC *pfWr, void *pWrCtx, unsigned Flags);
int  CTG_CompressF2M    (CTG_RD_FUNC *pfRd, void *pRdCtx, U8 *pOutput, unsigned OutputLen, U8 *pWork, unsigned WorkLen, unsigned Flags);
int  CTG_CompressF2F    (CTG_RD_FUNC *pfRd, void* pRdCtx, CTG_WR_FUNC *pfWr, void *pWrCtx, U8 *pWork, unsigned WorkLen, unsigned Flags);

/*********************************************************************
*
*       One-call decompression functions
*/
int  CTG_DecompressM2M  (const U8 *pInput, unsigned InputLen, U8 *pOutput, unsigned OutputLen);
int  CTG_DecompressM2F  (const U8 *pInput, unsigned InputLen, CTG_WR_FUNC *pfWr, void *pWrCtx, U8 *pWork, unsigned WorkLen);
int  CTG_DecompressF2M  (CTG_RD_FUNC pfRd, void *pRdCtx, U8 *pOutput, unsigned OutputLen);
int  CTG_DecompressF2F  (CTG_RD_FUNC pfRd, void *pRdCtx, CTG_WR_FUNC *pfWr, void *pWrCtx, U8 *pWork, unsigned WorkLen);

/*********************************************************************
*
*       Failure handling functions
*/
void CTG_Panic          (const char *sFilename, int LineNumber, const char *sAssertion);

#if defined(__cplusplus)
}                             // Make sure we have C-declarations in C++ programs.
#endif

#endif

/*************************** End of file ****************************/
