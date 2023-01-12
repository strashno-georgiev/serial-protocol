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

File        : MIDI_LPAD.h
Purpose     : Novation Launchpad and Launchpad mini MIDI control library.

*/

#ifndef MIDI_LPAD_H
#define MIDI_LPAD_H

#include "MIDI.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
*
*       Types required for API
*
**********************************************************************
*/

typedef struct {
  U8 aMatrixPads  [64];
  U8 aTopPads     [8];
  U8 aRightPads   [8];
} MIDI_LPAD_PADS;

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

void MIDI_LPAD_Reset          (MIDI_STREAM *pSelf);
void MIDI_LPAD_SetDutyCycle   (MIDI_STREAM *pSelf, unsigned Numerator, unsigned Denominator);
//
void MIDI_LPAD_InitPads       (MIDI_LPAD_PADS *pSelf);
void MIDI_LPAD_SetTopPad      (MIDI_LPAD_PADS *pSelf, unsigned X, unsigned Color);
void MIDI_LPAD_SetRightPad    (MIDI_LPAD_PADS *pSelf, unsigned Y, unsigned Color);
void MIDI_LPAD_SetPadIndex    (MIDI_LPAD_PADS *pSelf, unsigned Index, unsigned Color);
void MIDI_LPAD_SetPadXY       (MIDI_LPAD_PADS *pSelf, int X, int Y,   unsigned Color);
void MIDI_LPAD_SendPads       (MIDI_LPAD_PADS *pSelf, MIDI_STREAM *pStream);
void MIDI_LPAD_SendPads_Fast  (MIDI_LPAD_PADS *pSelf, MIDI_STREAM *pStream);

#ifdef __cplusplus
}
#endif

#endif

/*************************** End of file ****************************/
