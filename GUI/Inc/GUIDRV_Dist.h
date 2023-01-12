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
File        : GUIDRV_Dist.h
Purpose     : Interface definition for GUIDRV_Dist driver
---------------------------END-OF-HEADER------------------------------
*/

#ifndef GUIDRV_DIST_H
#define GUIDRV_DIST_H

#if defined(__cplusplus)
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif

/*********************************************************************
*
*       Display driver
*/
//
// Address
//
extern const GUI_DEVICE_API GUIDRV_Dist_API;

//
// Macros to be used in configuration files
//
#if defined(WIN32) && !defined(LCD_SIMCONTROLLER)

  #define GUIDRV_DIST &GUIDRV_Win_API

#else

  #define GUIDRV_DIST &GUIDRV_Dist_API

#endif

/*********************************************************************
*
*       Public routines
*/
#if defined(WIN32) && !defined(LCD_SIMCONTROLLER)

  #define GUIDRV_Dist_AddDriver(pDevice, pDriver, pRect)

#else

  void GUIDRV_Dist_AddDriver           (GUI_DEVICE * pDevice, GUI_DEVICE * pDriver, GUI_RECT * pRect);
  void GUIDRV_Dist_SetFuncWaitForVSYNC (GUI_DEVICE * pDevice, void(*pFunc)(int));
  U32  GUIDRV_Dist_GetDirtyDisplayIndex(GUI_DEVICE * pDevice);

#endif

#if defined(__cplusplus)
}
#endif

#endif /* GUIDRV_DIST_H */

/*************************** End of file ****************************/
