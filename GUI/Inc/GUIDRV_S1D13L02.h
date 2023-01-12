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
File        : GUIDRV_S1D13L02.h
Purpose     : Interface definition for GUIDRV_S1D13L02 driver
---------------------------END-OF-HEADER------------------------------
*/

#ifndef GUIDRV_S1D13L02_H
#define GUIDRV_S1D13L02_H

#if defined(__cplusplus)
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif

#define GUIDRV_S1D13L02_USE_MAIN 0
#define GUIDRV_S1D13L02_USE_PIP1 1
#define GUIDRV_S1D13L02_USE_PIP2 2

/*********************************************************************
*
*       Configuration structure
*/
typedef struct {
  //
  // Driver specific configuration items
  //
  U32 BufferOffset;
  int UseLayer;
  int WriteBufferSize;
} CONFIG_S1D13L02;

/*********************************************************************
*
*       Display drivers
*/
//
// Addresses
//
extern const GUI_DEVICE_API GUIDRV_S1D13L02_16_API;
extern const GUI_DEVICE_API GUIDRV_S1D13L02_OY_16_API;
extern const GUI_DEVICE_API GUIDRV_S1D13L02_OX_16_API;
extern const GUI_DEVICE_API GUIDRV_S1D13L02_OXY_16_API;
extern const GUI_DEVICE_API GUIDRV_S1D13L02_OS_16_API;
extern const GUI_DEVICE_API GUIDRV_S1D13L02_OSY_16_API;
extern const GUI_DEVICE_API GUIDRV_S1D13L02_OSX_16_API;
extern const GUI_DEVICE_API GUIDRV_S1D13L02_OSXY_16_API;

//
// Macros to be used in configuration files
//
#if defined(WIN32) && !defined(LCD_SIMCONTROLLER)

  #define GUIDRV_S1D13L02_16       &GUIDRV_Win_API
  #define GUIDRV_S1D13L02_OY_16    &GUIDRV_Win_API
  #define GUIDRV_S1D13L02_OX_16    &GUIDRV_Win_API
  #define GUIDRV_S1D13L02_OXY_16   &GUIDRV_Win_API
  #define GUIDRV_S1D13L02_OS_16    &GUIDRV_Win_API
  #define GUIDRV_S1D13L02_OSY_16   &GUIDRV_Win_API
  #define GUIDRV_S1D13L02_OSX_16   &GUIDRV_Win_API
  #define GUIDRV_S1D13L02_OSXY_16  &GUIDRV_Win_API

#else

  #define GUIDRV_S1D13L02_16       &GUIDRV_S1D13L02_16_API
  #define GUIDRV_S1D13L02_OY_16    &GUIDRV_S1D13L02_OY_16_API
  #define GUIDRV_S1D13L02_OX_16    &GUIDRV_S1D13L02_OX_16_API
  #define GUIDRV_S1D13L02_OXY_16   &GUIDRV_S1D13L02_OXY_16_API
  #define GUIDRV_S1D13L02_OS_16    &GUIDRV_S1D13L02_OS_16_API
  #define GUIDRV_S1D13L02_OSY_16   &GUIDRV_S1D13L02_OSY_16_API
  #define GUIDRV_S1D13L02_OSX_16   &GUIDRV_S1D13L02_OSX_16_API
  #define GUIDRV_S1D13L02_OSXY_16  &GUIDRV_S1D13L02_OSXY_16_API

#endif

/*********************************************************************
*
*       Public routines
*/
void GUIDRV_S1D13L02_Config  (GUI_DEVICE * pDevice, CONFIG_S1D13L02 * pConfig);
void GUIDRV_S1D13L02_SetBus16(GUI_DEVICE * pDevice, GUI_PORT_API * pHW_API);

#if defined(__cplusplus)
}
#endif

#endif

/*************************** End of file ****************************/
