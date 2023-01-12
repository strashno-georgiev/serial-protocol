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
File        : GUIDRV_07X1.h
Purpose     : Interface definition for GUIDRV_07X1 driver
---------------------------END-OF-HEADER------------------------------
*/

#ifndef GUIDRV_07X1_H
#define GUIDRV_07X1_H

#if defined(__cplusplus)
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif

/*********************************************************************
*
*       Display drivers
*/
//
// Addresses
//
extern const GUI_DEVICE_API GUIDRV_Win_API;

extern const GUI_DEVICE_API GUIDRV_07X1_API;

//
// Macros to be used in configuration files
//
#if defined(WIN32) && !defined(LCD_SIMCONTROLLER)

  #define GUIDRV_07X1            &GUIDRV_Win_API

#else

  #define GUIDRV_07X1            &GUIDRV_07X1_API

#endif

/*********************************************************************
*
*       Public routines
*/
int  LCD_L0_07X1_ControlCache(int cmd);
void LCD_L0_07X1_ReInit      (void);

#if defined(__cplusplus)
}
#endif

#endif

/*************************** End of file ****************************/
