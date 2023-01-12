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
File        : GUIMTDRV_FT5336.h
Purpose     : Multitouch driver for FT5336.
---------------------------END-OF-HEADER------------------------------
*/

#ifndef GUIMTDRV_FT5336_H
#define GUIMTDRV_FT5336_H

#include "GUI_Type.h"

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#define GUIMTDRV_FT5336_TOUCH_MAX  10

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

typedef struct {
  unsigned         NumTouch;
  GUI_MTOUCH_INPUT aTouch[GUIMTDRV_FT5336_TOUCH_MAX];
} GUIMTDRV_FT5336_STATE;

typedef struct {
  //
  int LayerIndex;
  //
  // Initialization
  //
  void (*pf_I2C_Init) (unsigned SlaveAddr);
  //
  // Read- and write access
  //
  void (*pf_I2C_Read)  (unsigned SlaveAddr,       U8 *pData, int Addr, int Len);
  void (*pf_I2C_Write) (unsigned SlaveAddr, const U8 *pData, int Addr, int Len);
  //
  // Optionally translate controller frame of reference to emWin frame of reference.
  //
  void (*pf_Translate) (GUI_MTOUCH_INPUT *pCoord);
  //
  // 7-bit address to be used to address the I2C slave
  //
  unsigned SlaveAddr;
} GUIMTDRV_FT5336_CONFIG;

/*********************************************************************
*
*       Interface
*
**********************************************************************
*/

int  GUIMTDRV_FT5336_Exec          (void);
void GUIMTDRV_FT5336_GetTouchState (GUIMTDRV_FT5336_STATE *pState);
void GUIMTDRV_FT5336_Init          (const GUIMTDRV_FT5336_CONFIG *pConfig);
void GUIMTDRV_FT5336_SetLayerIndex (int LayerIndex);

#endif
