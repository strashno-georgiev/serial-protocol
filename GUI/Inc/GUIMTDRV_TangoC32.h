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
File        : GUIMTDRV_TangoC32.h
Purpose     : Interface definition for GUIMTDRV_TangoC32 driver
---------------------------END-OF-HEADER------------------------------
*/

#ifndef GUIMTDRV_TANGOC32_H
#define GUIMTDRV_TANGOC32_H

#include "GUI_Type.h"

#if defined(__cplusplus)
//extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif

/*********************************************************************
*
*       Types
*
**********************************************************************
*/
typedef struct {
  int LayerIndex;
  //
  // Initialization
  //
  void (* pf_I2C_Init)(unsigned char SlaveAddr);
  //
  // Read- and write access
  //
  int (* pf_I2C_Read  )(unsigned char * pData, int Start, int Stop);
  int (* pf_I2C_ReadM )(unsigned char * pData, int NumItems, int Start, int Stop);
  int (* pf_I2C_Write )(unsigned char Data, int Start, int Stop);
  int (* pf_I2C_WriteM)(unsigned char * pData, int NumItems, int Start, int Stop);
  //
  // 7-bit address to be used to address the I2C slave
  //
  U8 SlaveAddr;
} GUIMTDRV_TANGOC32_CONFIG;

/*********************************************************************
*
*       Interface
*
**********************************************************************
*/
int GUIMTDRV_TangoC32_Init(GUIMTDRV_TANGOC32_CONFIG * pConfig);
int GUIMTDRV_TangoC32_Exec(void);

#endif /* GUIMTDRV_TANGOC32_H */

/*************************** End of file ****************************/
