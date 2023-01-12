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
File        : GUIDRV_Bitplains.h
Purpose     : Interface definition for GUIDRV_Bitplains driver
---------------------------END-OF-HEADER------------------------------
*/

#ifndef GUIDRV_BITPLAINS_H
#define GUIDRV_BITPLAINS_H

#if defined(__cplusplus)
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif

/*********************************************************************
*
*       Configuration structure
*/
typedef struct {
  int Mirror;
} CONFIG_BITPLAINS;

typedef struct {
  U8 * apVRAM[8];
} CONFIG_VRAM_BITPLAINS;

/*********************************************************************
*
*       Display drivers
*/
//
// Addresses
//
extern const GUI_DEVICE_API GUIDRV_Win_API;

extern const GUI_DEVICE_API GUIDRV_BitPlains_API;

//
// Macros to be used in configuration files
//
#if defined(WIN32) && !defined(LCD_SIMCONTROLLER)

  #define GUIDRV_BITPLAINS        &GUIDRV_Win_API

#else

  #define GUIDRV_BITPLAINS        &GUIDRV_BitPlains_API

#endif

/*********************************************************************
*
*       Public routines
*/
void GUIDRV_BitPlains_Config(GUI_DEVICE * pDevice, CONFIG_BITPLAINS * pConfig);

#if defined(__cplusplus)
}
#endif

#endif /* GUIDRV_BITPLAINS_H */

/*************************** End of file ****************************/
