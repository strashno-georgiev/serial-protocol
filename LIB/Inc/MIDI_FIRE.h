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

File        : MIDI_FIRE.h
Purpose     : Akai Fire MIDI control library.

*/

#ifndef MIDI_FIRE_H
#define MIDI_FIRE_H

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
  U8 aBitmap[1175];
} MIDI_FIRE_OLED;

typedef struct {
  U8 aPads[64*4];
} MIDI_FIRE_PADS;

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

void MIDI_FIRE_SetButtonColor (MIDI_STREAM    *pSelf, unsigned Button, unsigned Color);
void MIDI_FIRE_InitOled       (MIDI_FIRE_OLED *pSelf);
void MIDI_FIRE_Plot           (MIDI_FIRE_OLED *pSelf, unsigned X, unsigned Y, unsigned C);
void MIDI_FIRE_PingOled       (MIDI_STREAM    *pSelf);
void MIDI_FIRE_InitPads       (MIDI_FIRE_PADS *pSelf);
void MIDI_FIRE_SetPadIndex    (MIDI_FIRE_PADS *pSelf, unsigned Index, U32 Color);
void MIDI_FIRE_SetPadXY       (MIDI_FIRE_PADS *pSelf, unsigned X, unsigned Y, U32 Color);
void MIDI_FIRE_SendOled       (MIDI_FIRE_OLED *pSelf, MIDI_STREAM *pStream);
void MIDI_FIRE_SendPads       (MIDI_FIRE_PADS *pSelf, MIDI_STREAM *pStream);

#ifdef __cplusplus
}
#endif

#endif

/*************************** End of file ****************************/
