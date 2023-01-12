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
File    : GUIDEV_NormalMapping.h
Purpose : HEader for normal mapping of memory devices.
--------  END-OF-HEADER  ---------------------------------------------
*/
#ifndef GUIDEV_NORMALMAPPING_H
#define GUIDEV_NORMALMAPPING_H

#include "GUI.h"

/*********************************************************************
*
*       Typedef
*
**********************************************************************
*/
typedef struct {
  GUI_MEMDEV_Handle   hMemDest;
  const U16         * pNMap;
  const GUI_BITMAP  * pBmImage;
  GUI_MEMDEV_Handle   hMemImage;
  const GUI_BITMAP  * pbmLight;
  int                 xSize;
  int                 ySize;
} GUI_MAPPING_CONTEXT;

/*********************************************************************
*
*       Prototype
*
**********************************************************************
*/
int GUI_MEMDEV_NormalMapping(GUI_MAPPING_CONTEXT * pContext, int xPosLight, int yPosLight);

#endif

/****** End of File *************************************************/
