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
File        : GUIDRV_7528.h
Purpose     : Interface definition for GUIDRV_7528 driver
---------------------------END-OF-HEADER------------------------------
*/

#ifndef GUIDRV_7528_H
#define GUIDRV_7528_H

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

extern const GUI_DEVICE_API GUIDRV_7528_API;

//
// Macros to be used in configuration files
//
#if defined(WIN32) && !defined(LCD_SIMCONTROLLER)

  #define GUIDRV_7528             &GUIDRV_Win_API

#else

  #define GUIDRV_7528             &GUIDRV_7528_API

#endif

#if defined(__cplusplus)
}
#endif

#endif

/*************************** End of file ****************************/
