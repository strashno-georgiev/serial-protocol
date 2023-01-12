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
File        : GUIDRV_S1D15G00.h
Purpose     : Interface definition for GUIDRV_S1D15G00 driver
---------------------------END-OF-HEADER------------------------------
*/

#ifndef GUIDRV_S1D15G00_H
#define GUIDRV_S1D15G00_H

#if defined(__cplusplus)
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif

/*********************************************************************
*
*       Configuration structure
*/
typedef struct {
  //
  // Driver specific configuration items
  //
  int FirstSEG;
  int FirstCOM;
  int Orientation;
  int UseCache;
} CONFIG_S1D15G00;

/*********************************************************************
*
*       Display drivers
*/
//
// Addresses
//
extern const GUI_DEVICE_API GUIDRV_Win_API;

extern const GUI_DEVICE_API GUIDRV_S1D15G00_API;

//
// Macros to be used in configuration files
//
#if defined(WIN32) && !defined(LCD_SIMCONTROLLER)

  #define GUIDRV_S1D15G00    &GUIDRV_Win_API

#else

  #define GUIDRV_S1D15G00    &GUIDRV_S1D15G00_API

#endif

/*********************************************************************
*
*       Public routines
*/
void GUIDRV_S1D15G00_Config (GUI_DEVICE * pDevice, CONFIG_S1D15G00 * pConfig);
void GUIDRV_S1D15G00_SetBus8(GUI_DEVICE * pDevice, GUI_PORT_API * pHW_API);

#if defined(__cplusplus)
}
#endif

#endif

/*************************** End of file ****************************/
