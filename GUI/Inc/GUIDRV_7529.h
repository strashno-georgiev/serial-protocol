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
File        : GUIDRV_7529.h
Purpose     : Interface definition for GUIDRV_7529 driver
---------------------------END-OF-HEADER------------------------------
*/

#ifndef GUIDRV_7529_H
#define GUIDRV_7529_H

#if defined(__cplusplus)
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif

/*********************************************************************
*
*       Color conversion
*/
unsigned  LCD_Color2Index_User (LCD_COLOR Color);
LCD_COLOR LCD_Index2Color_User (int Index);
unsigned  LCD_GetIndexMask_User(void);

/*********************************************************************
*
*       Display drivers
*/
//
// Addresses
//
extern const GUI_DEVICE_API GUIDRV_Win_API;

extern const GUI_DEVICE_API GUIDRV_7529_API;

//
// Macros to be used in configuration files
//
#if defined(WIN32) && !defined(LCD_SIMCONTROLLER)

  #define GUIDRV_7529             &GUIDRV_Win_API

#else

  #define GUIDRV_7529             &GUIDRV_7529_API

#endif

#if defined(__cplusplus)
}
#endif

#endif

/*************************** End of file ****************************/
