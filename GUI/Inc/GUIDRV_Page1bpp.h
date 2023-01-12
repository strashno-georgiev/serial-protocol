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
File        : GUIDRV_Page1bpp.h
Purpose     : Interface definition for GUIDRV_Page1bpp driver
---------------------------END-OF-HEADER------------------------------
*/

#ifndef GUIDRV_Page1bpp_H
#define GUIDRV_Page1bpp_H

/*********************************************************************
*
*       Display drivers
*/
//
// Addresses
//
extern const GUI_DEVICE_API GUIDRV_Win_API;

extern const GUI_DEVICE_API GUIDRV_Page1bpp_API;

//
// Macros to be used in configuration files
//
#if defined(WIN32) && !defined(LCD_SIMCONTROLLER)

  #define GUIDRV_PAGE1BPP         &GUIDRV_Win_API

#else

  #define GUIDRV_PAGE1BPP         &GUIDRV_Page1bpp_API

#endif

#endif

/*************************** End of file ****************************/
