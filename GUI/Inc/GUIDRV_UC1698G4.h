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
File        : GUIDRV_UC1698G4.h
Purpose     : Interface definition for GUIDRV_UC1698G4 driver
---------------------------END-OF-HEADER------------------------------
*/

#ifndef GUIDRV_UC1698G4_H
#define GUIDRV_UC1698G4_H

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

extern const GUI_DEVICE_API GUIDRV_UC1698G4_API;

//
// Macros to be used in configuration files
//
#if defined(WIN32) && !defined(LCD_SIMCONTROLLER)

  #define GUIDRV_UC1698G4 &GUIDRV_Win_API

#else

  #define GUIDRV_UC1698G4 &GUIDRV_UC1698G4_API

#endif

void GUIDRV_UC1698G4_SetBus8(GUI_DEVICE * pDevice, GUI_PORT_API * pPortAPI);

#if defined(__cplusplus)
}
#endif

#endif

/*************************** End of file ****************************/
