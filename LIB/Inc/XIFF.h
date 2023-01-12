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

File        : XIFF.h
Purpose     : SEGGER xIFF library API.

*/

#ifndef XIFF_H
#define XIFF_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#include "SEGGER.h"

/*********************************************************************
*
*       Version number
*
*  Description
*    Symbol expands to a number that identifies the specific emLib-IFF release.
*/
#define XIFF_VERSION            10000   // Format is "Mmmrr" so, for example, 12304 corresponds to version 1.23d.

/*********************************************************************
*
*       Types required for API
*
**********************************************************************
*/

#define RIFF_BYTE_ORDER_PC    0
#define RIFF_BYTE_ORDER_NET   1

typedef struct {
  U32         Type;
  const U8  * pData;
  unsigned    DataLen;
} XIFF_CHUNK;

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

U8           XIFF_GetU8         (XIFF_CHUNK *pChunk);
I8           XIFF_GetI8         (XIFF_CHUNK *pChunk);
U16          XIFF_GetU16BE      (XIFF_CHUNK *pChunk);
U16          XIFF_GetU16LE      (XIFF_CHUNK *pChunk);
U32          XIFF_GetU24BE      (XIFF_CHUNK *pChunk);
U32          XIFF_GetU24LE      (XIFF_CHUNK *pChunk);
U32          XIFF_GetU32BE      (XIFF_CHUNK *pChunk);
U32          XIFF_GetU32LE      (XIFF_CHUNK *pChunk);
float        XIFF_GetExtendedBE (XIFF_CHUNK *pChunk);
U32          XIFF_GetPrefixU32  (XIFF_CHUNK *pChunk);
void         XIFF_Advance       (XIFF_CHUNK *pChunk, int Len);
void         XIFF_RdChunkHeader (XIFF_CHUNK *pChunk, XIFF_CHUNK *pHeader, unsigned ByteOrder, int Align);
void         XIFF_Rd            (XIFF_CHUNK *pChunk, U8 *pData, unsigned DataLen);

#ifdef __cplusplus
}
#endif

#endif

/*************************** End of file ****************************/
