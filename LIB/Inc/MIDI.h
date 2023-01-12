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

File        : MIDI.h
Purpose     : SEGGER MIDI library API.

*/

#ifndef MIDI_H
#define MIDI_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/

#include "SEGGER.h"
#include "XIFF.h"

/*********************************************************************
*
*       Version number
*
*  Description
*    Symbol expands to a number that identifies the specific emLib-MIDI release.
*/
#define MIDI_VERSION            10000   // Format is "Mmmrr" so, for example, 12304 corresponds to version 1.23d.

/*********************************************************************
*
*       MIDI status bytes
*/
#define MIDI_COMMAND_0x80_NOTE_OFF                  0x80
#define MIDI_COMMAND_0x90_NOTE_ON                   0x90
#define MIDI_COMMAND_0xA0_POLY_AFTERTOUCH           0xA0
#define MIDI_COMMAND_0xB0_CONTROL_CHANGE            0xB0
#define MIDI_COMMAND_0xC0_PROGRAM_CHANGE            0xC0
#define MIDI_COMMAND_0xD0_AFTERTOUCH                0xD0
#define MIDI_COMMAND_0xE0_PITCH_BEND                0xE0
#define MIDI_COMMAND_0xF0_SYSTEM_EXCLUSIVE          0xF0
#define MIDI_COMMAND_0xF1_TIMECODE_QUARTER_FRAME    0xF1
#define MIDI_COMMAND_0xF2_SONG_POSITION             0xF2
#define MIDI_COMMAND_0xF3_SONG_SELECT               0xF3
#define MIDI_COMMAND_0xF4_UNDEFINED                 0xF4
#define MIDI_COMMAND_0xF5_UNDEFINED                 0xF5
#define MIDI_COMMAND_0xF6_TUNE_REQUEST              0xF6
#define MIDI_COMMAND_0xF7_END_OF_EXCLUSIVE          0xF7
#define MIDI_COMMAND_0xF8_TIMING_CLOCK              0xF8
#define MIDI_COMMAND_0xF9_UNDEFINED                 0xF9
#define MIDI_COMMAND_0xFA_START                     0xFA
#define MIDI_COMMAND_0xFB_CONTINUE                  0xFB
#define MIDI_COMMAND_0xFC_STOP                      0xFC
#define MIDI_COMMAND_0xFD_UNDEFINED                 0xFD
#define MIDI_COMMAND_0xFE_ACTIVE_SENSING            0xFE
#define MIDI_COMMAND_0xFF_RESET                     0xFF

/*********************************************************************
*
*       MIDI note numbers
*/
#define MIDI_NOTE_B7          107
#define MIDI_NOTE_A7_SHARP    106
#define MIDI_NOTE_A7          105
#define MIDI_NOTE_G7_SHARP    104
#define MIDI_NOTE_G7          103
#define MIDI_NOTE_F7_SHARP    102
#define MIDI_NOTE_F7          101
#define MIDI_NOTE_E7          100
#define MIDI_NOTE_D7_SHARP     99
#define MIDI_NOTE_D7           98
#define MIDI_NOTE_C7_SHARP     97
#define MIDI_NOTE_C7           96
#define MIDI_NOTE_B6           95
#define MIDI_NOTE_A6_SHARP     94
#define MIDI_NOTE_A6           93
#define MIDI_NOTE_G6_SHARP     92
#define MIDI_NOTE_G6           91
#define MIDI_NOTE_F6_SHARP     90
#define MIDI_NOTE_F6           89
#define MIDI_NOTE_E6           88
#define MIDI_NOTE_D6_SHARP     87
#define MIDI_NOTE_D6           86
#define MIDI_NOTE_C6_SHARP     85
#define MIDI_NOTE_C6           84
#define MIDI_NOTE_B5           83
#define MIDI_NOTE_A5_SHARP     82
#define MIDI_NOTE_A5           81
#define MIDI_NOTE_G5_SHARP     80
#define MIDI_NOTE_G5           79
#define MIDI_NOTE_F5_SHARP     78
#define MIDI_NOTE_F5           77
#define MIDI_NOTE_E5           76
#define MIDI_NOTE_D5_SHARP     75
#define MIDI_NOTE_D5           74
#define MIDI_NOTE_C5_SHARP     73
#define MIDI_NOTE_C5           72
#define MIDI_NOTE_B4           71
#define MIDI_NOTE_A4_SHARP     70
#define MIDI_NOTE_A4           69
#define MIDI_NOTE_G4_SHARP     68
#define MIDI_NOTE_G4           67
#define MIDI_NOTE_F4_SHARP     66
#define MIDI_NOTE_F4           65
#define MIDI_NOTE_E4           64
#define MIDI_NOTE_D4_SHARP     63
#define MIDI_NOTE_D4           62
#define MIDI_NOTE_C4_SHARP     61
#define MIDI_NOTE_C4           60
#define MIDI_NOTE_B3           59
#define MIDI_NOTE_A3_SHARP     58
#define MIDI_NOTE_A3           57
#define MIDI_NOTE_G3_SHARP     56
#define MIDI_NOTE_G3           55
#define MIDI_NOTE_F3_SHARP     54
#define MIDI_NOTE_F3           53
#define MIDI_NOTE_E3           52
#define MIDI_NOTE_D3_SHARP     51
#define MIDI_NOTE_D3           50
#define MIDI_NOTE_C3_SHARP     49
#define MIDI_NOTE_C3           48
#define MIDI_NOTE_B2           47
#define MIDI_NOTE_A2_SHARP     46
#define MIDI_NOTE_A2           45
#define MIDI_NOTE_G2_SHARP     44
#define MIDI_NOTE_G2           43
#define MIDI_NOTE_F2_SHARP     42
#define MIDI_NOTE_F2           41
#define MIDI_NOTE_E2           40
#define MIDI_NOTE_D2_SHARP     39
#define MIDI_NOTE_D2           38
#define MIDI_NOTE_C2_SHARP     37
#define MIDI_NOTE_C2           36
#define MIDI_NOTE_B1           35
#define MIDI_NOTE_A1_SHARP     34
#define MIDI_NOTE_A1           33
#define MIDI_NOTE_G1_SHARP     32
#define MIDI_NOTE_G1           31
#define MIDI_NOTE_F1_SHARP     30
#define MIDI_NOTE_F1           29
#define MIDI_NOTE_E1           28
#define MIDI_NOTE_D1_SHARP     27
#define MIDI_NOTE_D1           26
#define MIDI_NOTE_C1_SHARP     25
#define MIDI_NOTE_C1           24
#define MIDI_NOTE_B0           23
#define MIDI_NOTE_A0_SHARP     22
#define MIDI_NOTE_A0           21

/*********************************************************************
*
*       Characteristics.
*/
#define MIDI_SEMITONES_PER_OCTAVE  12

/*********************************************************************
*
*       Reported errors and warnings.
*/
#define MIDI_STATUS_SEQUENCE_NUMBER_BAD_LENGTH   -200
#define MIDI_STATUS_CHANNEL_PREFIX_BAD_LENGTH    -201
#define MIDI_STATUS_PORT_PREFIX_BAD_LENGTH       -202
#define MIDI_STATUS_END_OF_TRACK_BAD_LENGTH      -203
#define MIDI_STATUS_TEMPO_BAD_LENGTH             -204
#define MIDI_STATUS_TIME_SIGNATURE_BAD_LENGTH    -205
#define MIDI_STATUS_KEY_SIGNATURE_BAD_LENGTH     -206
#define MIDI_STATUS_MTHD_CHUNK_BAD_LENGTH        -207
#define MIDI_STATUS_UNKNOWN_META_EVENT           -208
#define MIDI_STATUS_TOO_MANY_MTRK_CHUNKS         -209
#define MIDI_STATUS_MTHD_CHUNK_EXPECTED          -210
#define MIDI_STATUS_MTRK_CHUNK_EXPECTED          -211
#define MIDI_STATUS_MISSING_MTRK_CHUNKS          -212
#define MIDI_STATUS_JUNK_AT_END_OF_FILE          -213
#define MIDI_STATUS_NONEXISTENT_TRACK            -214
#define MIDI_STATUS_NO_NOTES                     -215
#define MIDI_STATUS_UNEXPECTED_STATUS_BYTE       -216
#define MIDI_STATUS_DATA_BYTE_IGNORED            -217
#define MIDI_STATUS_INVALID_MIDI_STREAM          -218

/*********************************************************************
*
*       USB event decomposition
*/
#define MIDI_USB_GET_CABLE(E)    ((E) >> 28)
#define MIDI_USB_GET_CIN(E)      (((E) >> 24) & 0xF)
#define MIDI_USB_GET_STATUS(E)   (((E) >> 16) & 0xFF)
#define MIDI_USB_GET_FUNCTION(E) (((E) >> 16) & 0xF0)
#define MIDI_USB_GET_CHANNEL(E)  (((E) >> 16) & 0x0F)
#define MIDI_USB_GET_DATA2(E)    (unsigned)(((E) >> 8) & 0x7Fu)
#define MIDI_USB_GET_DATA3(E)    (unsigned)((E) & 0x7Fu)
#define MIDI_USB_GET_DATAX(E)    ((MIDI_USB_GET_DATA3(E) << 7) + MIDI_USB_GET_DATA2(E))

/*********************************************************************
*
*       USB event composition
*/
#define MIDI_USB_PUT_CABLE(E)    (((E) & 0xF) << 28)
#define MIDI_USB_PUT_CIN(E)      (((E) & 0xF) << 24)
#define MIDI_USB_PUT_STATUS(E)   (((E) & 0xFF) << 16)
#define MIDI_USB_PUT_CHANNEL(E)  (((E) & 0xF) << 16)
#define MIDI_USB_PUT_DATA2(E)    (((E) & 0x7Fu) << 8)
#define MIDI_USB_PUT_DATA3(E)    ((E) & 0x7Fu)
#define MIDI_USB_PUT_DATAX(E)    (MIDI_USB_PUT_DATA3((E) >> 7) + MIDI_USB_PUT_DATA2(E))

/*********************************************************************
*
*       USB code index numbers (CINs)
*/
#define MIDI_USB_CIN_MISC_EVENT            0x00    // Miscellaneous function code, for future expansion
#define MIDI_USB_CIN_CABLE_EVENT           0x01    // Cable event, for future expansion
#define MIDI_USB_CIN_SYSTEM_COMMON_2BYTE   0x02    // Two-byte System Common messages (e.g. MIDI Time Code or Song Select).
#define MIDI_USB_CIN_SYSTEM_COMMON_3BYTE   0x03    // Three-byte System Common messages (e.g. Song Position).
#define MIDI_USB_CIN_SYSEX_BEGIN_CONT      0x04    // System Exclusive message starts or continues, three bytes
#define MIDI_USB_CIN_SYSTEM_COMMON_1BYTE   0x05    // One-byte System Common message
#define MIDI_USB_CIN_SYSEX_END_1BYTE       0x05    // System Exclusive message ends with following byte
#define MIDI_USB_CIN_SYSEX_END_2BYTE       0x06    // System Exclusive message ends with following two bytes
#define MIDI_USB_CIN_SYSEX_END_3BYTE       0x07    // System Exclusive message ends with following three bytes
#define MIDI_USB_CIN_NOTE_OFF              0x08    // Note Off event
#define MIDI_USB_CIN_NOTE_ON               0x09    // Note On event
#define MIDI_USB_CIN_POLY_AFTERTOUCH       0x0A    // Polyphonic aftertouch event
#define MIDI_USB_CIN_CONTROL_CHANGE        0x0B    // Control Change event
#define MIDI_USB_CIN_PROGRAM_CHANGE        0x0C    // Program Change event
#define MIDI_USB_CIN_AFTERTOUCH            0x0D    // Channel aftertouch
#define MIDI_USB_CIN_PITCH_BEND            0x0E    // Pitch bend
#define MIDI_USB_CIN_SINGLE_BYTE           0x0F    // Single byte

/*********************************************************************
*
*       Types required for API
*
**********************************************************************
*/

typedef void MIDI_FILE_HEADER_FUNC         (void *pContext, unsigned Format, unsigned TrackCnt, unsigned Division);
typedef void MIDI_FILE_TRACK_FUNC          (void *pContext, unsigned Id);
typedef void MIDI_FILE_DELTA_FUNC          (void *pContext, U32 Delta);
typedef void MIDI_FILE_KEY_SIGNATURE_FUNC  (void *pContext, int Key, unsigned Quality);
typedef void MIDI_FILE_TIME_SIGNATURE_FUNC (void *pContext, unsigned N, unsigned D, unsigned ClocksPerClick, unsigned TSNPerQN);
typedef void MIDI_FILE_DATA_FUNC           (void *pContext, const U8 *pData, unsigned DataLen);
typedef void MIDI_FILE_SET_TEMPO_FUNC      (void *pContext, U32 Tempo);
typedef void MIDI_FILE_SEQUENCE_NUM_FUNC   (void *pContext, U32 Id);
typedef void MIDI_FILE_PREFIX_FUNC         (void *pContext, unsigned Prefix);
typedef void MIDI_FILE_ERROR_FUNC          (void *pContext, int Status);
typedef void MIDI_FILE_EXCLUSIVE_FUNC      (void *pContext);

typedef struct {
  MIDI_FILE_ERROR_FUNC          * pfError;
  MIDI_FILE_ERROR_FUNC          * pfWarning;
  MIDI_FILE_HEADER_FUNC         * pfHeader;
  MIDI_FILE_TRACK_FUNC          * pfBeginTrack;
  MIDI_FILE_DELTA_FUNC          * pfDelta;
  MIDI_FILE_KEY_SIGNATURE_FUNC  * pfKeySignature;
  MIDI_FILE_TIME_SIGNATURE_FUNC * pfTimeSignature;
  MIDI_FILE_DATA_FUNC           * pfText;
  MIDI_FILE_DATA_FUNC           * pfCopyrightNotice;
  MIDI_FILE_DATA_FUNC           * pfTrackName;
  MIDI_FILE_DATA_FUNC           * pfInstrumentName;
  MIDI_FILE_DATA_FUNC           * pfLyric;
  MIDI_FILE_DATA_FUNC           * pfMarker;
  MIDI_FILE_DATA_FUNC           * pfCuePoint;
  MIDI_FILE_DATA_FUNC           * pfProgramName;
  MIDI_FILE_DATA_FUNC           * pfDeviceName;
  MIDI_FILE_SET_TEMPO_FUNC      * pfSetTempo;
  MIDI_FILE_SEQUENCE_NUM_FUNC   * pfSequenceNum;
  MIDI_FILE_PREFIX_FUNC         * pfChannelPrefix;
  MIDI_FILE_PREFIX_FUNC         * pfPortPrefix;
  MIDI_FILE_DATA_FUNC           * pfMidiMsg;
  MIDI_FILE_EXCLUSIVE_FUNC      * pfBeginSysex;
  MIDI_FILE_DATA_FUNC           * pfSysexData;
  MIDI_FILE_EXCLUSIVE_FUNC      * pfEndSysex;
  MIDI_FILE_TRACK_FUNC          * pfEndTrack;
} MIDI_FILE_API;

typedef struct {
  MIDI_FILE_ERROR_FUNC          * pfError;
  MIDI_FILE_ERROR_FUNC          * pfWarning;
  MIDI_FILE_DATA_FUNC           * pfChannelMsg;
  MIDI_FILE_DATA_FUNC           * pfRealtimeMsg;
  MIDI_FILE_DATA_FUNC           * pfCommonMsg;
  MIDI_FILE_DATA_FUNC           * pfExclusiveMsg;
} MIDI_PARSER_API;

// One MIDI quarter note is 24 MIDI clocks
typedef struct {
  const MIDI_FILE_API * pAPI;
  void                * pContext;
  XIFF_CHUNK            Master;      // Master chunk covering entire file
  unsigned              Format;
  unsigned              TrackCnt;
  unsigned              CurTrackId;  // 0..TrackCnt-1
  unsigned              ErrorCnt;
  unsigned              WarningCnt;
  int                   FirstError;
  unsigned              N;
  unsigned              D;
  unsigned              ClocksPerClick;
  unsigned              TSNPerQN;    // Number of notated 32nd-notes per MIDI quarter note
  int                   Key;
  unsigned              Quality;
  unsigned              Division;    // Delta-time ticks per quarter note
  U32                   Tempo;       // Microseconds per MIDI quarter note
} MIDI_FILE;

typedef struct {
  const MIDI_PARSER_API * pAPI;           // Pointer to callback API.
  void                  * pContext;       // Pointer to user-provided context.
  unsigned                RunningStatus;  // Current running status byte.
  int                     InSysex;        // Flag indicating if SysEx is active.
  unsigned                Flags;          // User-specified parsing flags
  unsigned                MessageLen;     // Expected number of data bytes in MIDI message.
  unsigned                Cursor;         // Index to next byte in buffer.
  U8                    * pBuffer;        // Pointer to active buffer.
  unsigned                BufferLen;      // Capacity of active buffer.
  U8                      aIntBuffer[32]; // Internal buffer if not set by user.
} MIDI_PARSER;

typedef struct {
  int Low;
  int High;
} MIDI_NOTE_RANGE;

typedef struct {
  int             Channel;              // MIDI channel (0...15), -1 is "not set"
  int             Program;              // Program number (0...127), -1 is "not set"
  int             Volume;               // Volume (0...127), -1 is "not set"
  int             Pan;                  // Pan (0...127), -1 is "not set"
  int             HasLyrics;            // Boolean, 0=no lyrics, 1=lyrics present
  MIDI_NOTE_RANGE NoteRange;            // Range of notes in track
  char            aProgramName   [32];  // Program name taken from Program Name meta-event
  char            aInstrumentName[32];  // Instrument name taken from Instrument Name meta-event
} MIDI_TRACK_INFO;

typedef enum {
  MIDI_ENCODE_FORMAT_MIDI,
  MIDI_ENCODE_FORMAT_USB,
  MIDI_ENCODE_FORMAT_BLUETOOTH
} MIDI_ENCODE_FORMAT;

typedef struct {
  MIDI_FILE_DATA_FUNC * pfWrite;
} MIDI_USB_ENCODER_API;

typedef struct {
  MIDI_FILE_DATA_FUNC * pfWrite;
} MIDI_USB_DECODER_API;

typedef struct {
  MIDI_PARSER                  Parser;    // Incoming MIDI stream parser
  unsigned                     Cable;     // Cable that events are intended for
  int                          Status;    // Encoder status, 0=no error, other is last recorded error
  void                       * pContext;  // Pointer to user-provided context
  const MIDI_USB_ENCODER_API * pAPI;
} MIDI_ENCODER;

typedef struct {
  unsigned                     Cable;     // Cable that events are parsed from
  void                       * pContext;  // Pointer to user-provided context
  const MIDI_USB_DECODER_API * pAPI;
} MIDI_DECODER;

typedef struct {
  unsigned Mode;
  //
  union {
    struct {
      MIDI_ENCODER * pEncoder;
    } Encoder;
    struct {
      void                * pContext;
      MIDI_FILE_DATA_FUNC * pfOutput;
    } Plain;
  } Variant;
} MIDI_STREAM;

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

/*********************************************************************
*
*       Decoding functions
*/
U32          MIDI_ConstructEvent           (const U8 *pData, unsigned DataLen);
int          MIDI_CalcMessageLen           (unsigned StatusByte);
unsigned     MIDI_CalcOctave               (int Note);
unsigned     MIDI_CalcSemitone             (int Note);
const char * MIDI_DecodeNote               (int Note, char *pBuf);
//
const char * MIDI_DecodeStatus             (int Status);
const char * MIDI_DecodeProgram            (int Program);
void         MIDI_DecodeEvent              (char *sText, unsigned TextLen, U32 Event);
void         MIDI_DecodeEventEx            (char *sText, unsigned TextLen, U32 Event);

/*********************************************************************
*
*       Standard MIDI File functions
*/
unsigned     MIDI_FILE_QueryTrackCnt       (MIDI_FILE *pFile);
int          MIDI_FILE_QueryNoteRange      (MIDI_FILE *pFile, unsigned TrackId, MIDI_NOTE_RANGE *pRange);
int          MIDI_FILE_QueryProgram        (MIDI_FILE *pFile, unsigned TrackId);
int          MIDI_FILE_QueryPan            (MIDI_FILE *pFile, unsigned TrackId);
int          MIDI_FILE_QueryVolume         (MIDI_FILE *pFile, unsigned TrackId);
int          MIDI_FILE_QueryLyrics         (MIDI_FILE *pFile, unsigned TrackId);
int          MIDI_FILE_QueryTrackInfo      (MIDI_FILE *pFile, unsigned TrackId, MIDI_TRACK_INFO *pInfo);
int          MIDI_FILE_QueryLyricTracks    (MIDI_FILE *pFile);
//
int          MIDI_FILE_Prepare             (MIDI_FILE *pFile, const U8 *pData, unsigned DataLen);
unsigned     MIDI_FILE_ProcessTrack        (MIDI_FILE *pFile, unsigned TrackId, const MIDI_FILE_API *pAPI, void *pContext);
int          MIDI_FILE_ProcessAllTracks    (MIDI_FILE *pFile, const MIDI_FILE_API *pAPI, void *pContext);

/*********************************************************************
*
*       MIDI stream parser
*/
void         MIDI_PARSER_Init              (MIDI_PARSER *pStream, void *pContext, const MIDI_PARSER_API *pAPI);
void         MIDI_PARSER_AssignBuffer      (MIDI_PARSER *pStream, U8 *pBuffer, unsigned BufferLen);
void         MIDI_PARSER_AddByte           (MIDI_PARSER *pStream, U8 Byte);
void         MIDI_PARSER_AddData           (MIDI_PARSER *pStream, const U8 *pData, unsigned DataLen);

/*********************************************************************
*
*       MIDI stream encoder
*/
void         MIDI_ENCODER_InitUSB          (MIDI_ENCODER *pEncoder, unsigned Cable, void *pContext, const MIDI_USB_ENCODER_API *pAPI);
void         MIDI_ENCODER_AddByte          (MIDI_ENCODER *pEncoder, U8 Byte);
void         MIDI_ENCODER_AddData          (MIDI_ENCODER *pEncoder, const U8 *pData, unsigned DataLen);

/*********************************************************************
*
*       MIDI stream decoder
*/
void         MIDI_DECODER_InitUSB          (MIDI_DECODER *pDecoder, unsigned Cable, void *pContext, const MIDI_USB_DECODER_API *pAPI);
void         MIDI_DECODER_AddEvent         (MIDI_DECODER *pDecoder, U32 Event);

/*********************************************************************
*
*       Abstract MIDI stream interface
*/
void         MIDI_STREAM_InitEncoder       (MIDI_STREAM *pSelf, MIDI_ENCODER *pEncoder);
void         MIDI_STREAM_InitPlain         (MIDI_STREAM *pSelf, MIDI_FILE_DATA_FUNC *pfOutput, void *pContext);
void         MIDI_STREAM_AddByte           (MIDI_STREAM *pSelf, U8 Byte);
void         MIDI_STREAM_AddData           (MIDI_STREAM *pSelf, const U8 *pData, unsigned DataLen);

/*********************************************************************
*
*       Wrap standard MIDI messages
*/
void         MIDI_SendDeviceInquiry       (MIDI_STREAM *pSelf, U8 DeviceId);
void         MIDI_SendBulkTuningDumpReq   (MIDI_STREAM *pSelf, U8 DeviceId, U8 ProgramNum);

#ifdef __cplusplus
}
#endif

#endif

/*************************** End of file ****************************/
