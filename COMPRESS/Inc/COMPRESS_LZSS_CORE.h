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
File        : COMPRESS_LZSS_CORE.h
Purpose     : Core implementation of LZSS codec.
--------  END-OF-HEADER  ---------------------------------------------
*/

#ifndef COMPRESS_LZSS_CORE_H
#define COMPRESS_LZSS_CORE_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "Global.h"
#include "COMPRESS_Int.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  LZSS_NOOP,
  LZSS_LITERAL,
  LZSS_REFERENCE,
  LZSS_DONE,
} COMPRESS_LZSS_CORE_ACTION;

typedef struct {
  COMPRESS_LZSS_CORE_ACTION Action;
  union {
    unsigned Literal;
    struct {
      unsigned Distance;
      unsigned Length;
    } Reference;
  } Data;
} COMPRESS_LZSS_CORE_OUTPUT;

typedef enum {
  COMPRESS_LZSS_CORE_PRIME,
  COMPRESS_LZSS_CORE_COMPRESS,
  COMPRESS_LZSS_CORE_REPLACE,
  COMPRESS_LZSS_CORE_FINALIZE,
  COMPRESS_LZSS_CORE_DONE,
} COMPRESS_LZSS_CORE_STATE;

// LZSS encoder using brute force search.
typedef struct {
  unsigned State;
  unsigned CompressState;
  unsigned CursorPosition;
  unsigned WindowPosition;
  unsigned WindowFill;
  unsigned ReplaceCount;
  unsigned LookaheadCount;
  unsigned Length;
  unsigned Distance;
  unsigned MinLength;
  unsigned MaxLength;
  unsigned MaxDistance;
  unsigned LengthBits;   // Number of bits required to represent a length
  unsigned DistanceBits; // Number of bits needed to represent a distance
  U8     * aWindow;
  SEGGER_MEM_CONTEXT *pMem;
} COMPRESS_LZSS_B_ENCODE_CONTEXT;

// LZSS encoder lists.
typedef struct {
  unsigned State;
  unsigned CompressState;
  unsigned CursorPosition;
  unsigned WindowPosition;
  unsigned WindowFill;
  unsigned ReplaceCount;
  unsigned LookaheadCount;
  unsigned Length;
  unsigned Distance;
  unsigned MinLength;
  unsigned MaxLength;
  unsigned MaxDistance;
  unsigned Optimize;
  unsigned LengthBits;   // Number of bits required to represent a length
  unsigned DistanceBits; // Number of bits needed to represent a distance
  U8     * aWindow;
  U16    * aHead;
  U16    * aNext;
  SEGGER_MEM_CONTEXT *pMem;
} COMPRESS_LZSS_L_ENCODE_CONTEXT;

// LZSS encoder using hash tables.
typedef struct {
  unsigned State;
  unsigned CompressState;
  unsigned CursorPosition;
  unsigned WindowPosition;
  unsigned WindowFill;
  unsigned ReplaceCount;
  unsigned LookaheadCount;
  unsigned Length;
  unsigned Distance;
  unsigned MinLength;
  unsigned MaxLength;
  unsigned MaxDistance;
  unsigned Optimize;
  unsigned LengthBits;   // Number of bits required to represent a length
  unsigned DistanceBits; // Number of bits needed to represent a distance
  unsigned HashTableSize;
  unsigned MaxUsedDistance;  // Maximum distance emitted by the compressor, <= MaxDistance
  U8     * aWindow;
  U16    * aHead;
  U16    * aNext;
  SEGGER_MEM_CONTEXT *pMem;
} COMPRESS_LZSS_H_ENCODE_CONTEXT;

typedef struct {
  unsigned MinLength;
  unsigned MaxLength;
  unsigned MaxDistance;
  unsigned LengthBits;
  unsigned DistanceBits;
  unsigned CursorPosition;
  unsigned MatchPosition;
  unsigned MatchLength;
  unsigned State;
  U8     * aWindow;                // Rolling window we will use for decryption
} COMPRESS_LZSS_CORE_DECODE_CONTEXT;

/*********************************************************************
*
*       Brute-force LZSS encoder.
*/
int  COMPRESS_LZSS_B_Init             (COMPRESS_LZSS_B_ENCODE_CONTEXT *pSelf, SEGGER_MEM_CONTEXT *pMem, COMPRESS_PARA aPara[]);
void COMPRESS_LZSS_B_Exit             (COMPRESS_LZSS_B_ENCODE_CONTEXT *pSelf);
void COMPRESS_LZSS_B_Reset            (COMPRESS_LZSS_B_ENCODE_CONTEXT *pSelf);
int  COMPRESS_LZSS_B_ENCODE_InputByte (COMPRESS_LZSS_B_ENCODE_CONTEXT *pSelf, COMPRESS_LZSS_CORE_OUTPUT *pOutput, unsigned Byte);

/*********************************************************************
*
*       List-based LZSS encoder.
*/
int  COMPRESS_LZSS_L_Init             (COMPRESS_LZSS_L_ENCODE_CONTEXT *pSelf, SEGGER_MEM_CONTEXT *pMem, COMPRESS_PARA aPara[]);
void COMPRESS_LZSS_L_Exit             (COMPRESS_LZSS_L_ENCODE_CONTEXT *pSelf);
void COMPRESS_LZSS_L_Reset            (COMPRESS_LZSS_L_ENCODE_CONTEXT *pSelf);
int  COMPRESS_LZSS_L_ENCODE_InputByte (COMPRESS_LZSS_L_ENCODE_CONTEXT *pSelf, COMPRESS_LZSS_CORE_OUTPUT *pOutput, unsigned Byte);

/*********************************************************************
*
*       Hash-based LZSS encoder.
*/
int  COMPRESS_LZSS_H_Init             (COMPRESS_LZSS_H_ENCODE_CONTEXT *pSelf, SEGGER_MEM_CONTEXT *pMem, COMPRESS_PARA aParas[]);
void COMPRESS_LZSS_H_Exit             (COMPRESS_LZSS_H_ENCODE_CONTEXT *pSelf);
void COMPRESS_LZSS_H_Reset            (COMPRESS_LZSS_H_ENCODE_CONTEXT *pSelf);
int  COMPRESS_LZSS_H_ENCODE_InputByte (COMPRESS_LZSS_H_ENCODE_CONTEXT *pSelf, COMPRESS_LZSS_CORE_OUTPUT *pOutput, unsigned Byte);

#ifdef __cplusplus
}
#endif

#endif
