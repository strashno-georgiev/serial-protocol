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
File        : COMPRESS_HUFF_CANON.h
Purpose     : Huffman core codec API.
--------  END-OF-HEADER  ---------------------------------------------
*/

#ifndef COMPRESS_HUFF_CANON_H
#define COMPRESS_HUFF_CANON_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

// Maximum number of code bits that can be requested for a limited encoding
#define COMPRESS_HUFF_CANON_CODE_BITS      (8 * sizeof(COMPRESS_HUFF_CANON_CODE))

// Maximum number of bits that will be generated unconstrained
#define COMPRESS_HUFF_CANON_MAX_CODE_BITS  32

/*********************************************************************
*
*       Types required for API
*
**********************************************************************
*/

//
// To extend to 32-bit codes, set this to U32.
//
typedef U32 COMPRESS_HUFF_CANON_CODE;

typedef struct {
  U32 Key;        // Used for symbol frequency and the assigned Huffman code
  U16 Index;      // Original symbol
} COMPRESS_HUFF_CANON_ENCODE_FREQ;

typedef struct {
  U16 Symbol;
  U16 Length;
} COMPRESS_HUFF_CANON_DECODE_CODE;

typedef struct {
  U16                               EntryCnt;           // Alphabet size.
  U16                               NonzeroEntryCnt;    // Number of symbols in the alphabet that have nonzero frequencies.
  unsigned                          MaxLengthReqested;  // Maximum length requested by client.
  unsigned                          MaxLengthAssigned;  // Maximum length calculated by emCompress, <= requested length.
  COMPRESS_HUFF_CANON_ENCODE_FREQ * aCounts;            // Counts of symbols occurring in block (probability).
  U8                              * aCodeSizes;         // Generated Huffman code lengths, indexed by alphabet.
  COMPRESS_HUFF_CANON_CODE        * aCodes;             // Generated Huffman codes, indexed by alphabet.
  U16                             * aCodeCount;         // Number of symbols assigned to a given Huffman code length.
  COMPRESS_HUFF_CANON_CODE        * aNextCode;          // Temporary workspace to assign canonical codes for each code length.
  SEGGER_MEM_CONTEXT              * pMem;
} COMPRESS_HUFF_CANON_ENCODE_CONTEXT;

typedef struct {
  COMPRESS_HUFF_CANON_CODE Code;  // Code which is the base of the step.
  U16                      Index; // 
} COMPRESS_HUFF_CANON_DECODE_STEP;

typedef struct {
  unsigned State;
  unsigned EntryCnt;
  unsigned MaxLength;                        // Maximum length of a codeword
  unsigned MaxSteps;
  COMPRESS_HUFF_CANON_DECODE_CODE * aCodes;  // Assigned symbols and code lengths
  COMPRESS_HUFF_CANON_DECODE_STEP * aSteps;  // [0..MaxLength-1]
  SEGGER_MEM_CONTEXT              * pMem;
} COMPRESS_HUFF_CORE_DECODE_CONTEXT;

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

/*********************************************************************
*
*       Canonical Huffman encoder.
*/
int  COMPRESS_HUFF_CANON_ENCODE_Init             (COMPRESS_HUFF_CANON_ENCODE_CONTEXT *pSelf, SEGGER_MEM_CONTEXT *pMem, unsigned EntryCnt, unsigned MaxCodeLen);
void COMPRESS_HUFF_CANON_ENCODE_Exit             (COMPRESS_HUFF_CANON_ENCODE_CONTEXT *pSelf);
void COMPRESS_HUFF_CANON_ENCODE_CountSymbol      (COMPRESS_HUFF_CANON_ENCODE_CONTEXT *pSelf, unsigned Symbol, unsigned N);
void COMPRESS_HUFF_CANON_ENCODE_CreateCoding     (COMPRESS_HUFF_CANON_ENCODE_CONTEXT *pSelf);

/*********************************************************************
*
*       Canonical Huffman decoder.
*/
int  COMPRESS_HUFF_CANON_DECODE_Init             (COMPRESS_HUFF_CORE_DECODE_CONTEXT *pSelf, SEGGER_MEM_CONTEXT *pMem, unsigned MaxSymbol);
void COMPRESS_HUFF_CANON_DECODE_Exit             (COMPRESS_HUFF_CORE_DECODE_CONTEXT *pSelf);
void COMPRESS_HUFF_CANON_DECODE_SetLength        (COMPRESS_HUFF_CORE_DECODE_CONTEXT *pSelf, unsigned Symbol, unsigned Length);
void COMPRESS_HUFF_CANON_DECODE_ReconstructCodes (COMPRESS_HUFF_CORE_DECODE_CONTEXT *pSelf);
U16  COMPRESS_HUFF_CANON_DECODE_RdSymbol         (COMPRESS_HUFF_CORE_DECODE_CONTEXT *pSelf, COMPRESS_BITIO_CONTEXT *pIO);
void COMPRESS_HUFF_CANON_DECODE_Sync             (COMPRESS_HUFF_CORE_DECODE_CONTEXT *pSelf, COMPRESS_BITIO_CONTEXT *pIO);


#ifdef __cplusplus
}
#endif

#endif

/****** End Of File *************************************************/
