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
File        : GUIDRV_SSD1322.h
Purpose     : Interface definition for GUIDRV_SSD1322 driver
---------------------------END-OF-HEADER------------------------------
*/

#ifndef GUIDRV_SSD1322_H
#define GUIDRV_SSD1322_H

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
} CONFIG_SSD1322;

/*********************************************************************
*
*       Interface
*/
void GUIDRV_SSD1322_Config (GUI_DEVICE * pDevice, CONFIG_SSD1322 * pConfig);
void GUIDRV_SSD1322_SetBus8(GUI_DEVICE * pDevice, GUI_PORT_API * pHW_API);

/*********************************************************************
*
*       Display drivers
*/
//
// Addresses
//
extern const GUI_DEVICE_API GUIDRV_Win_API;

extern const GUI_DEVICE_API GUIDRV_SSD1322_API;

//
// Macros to be used in configuration files
//
#if defined(WIN32) && !defined(LCD_SIMCONTROLLER)

  #define GUIDRV_SSD1322 &GUIDRV_Win_API
  #define GUIDRV_SSD1322_Config
  #define GUIDRV_SSD1322_SetBus8

#else

  #define GUIDRV_SSD1322 &GUIDRV_SSD1322_API

#endif

#if defined(__cplusplus)
}
#endif

#endif

/*************************** End of file ****************************/
