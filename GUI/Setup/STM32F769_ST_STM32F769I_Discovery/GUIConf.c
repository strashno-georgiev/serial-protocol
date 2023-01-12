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
File        : GUIConf.c
Purpose     : Display controller initialization
---------------------------END-OF-HEADER------------------------------
*/
#include "GUI.h"
#include "BSP_GUI.h"
#ifndef _WINDOWS
  #include "PIDConf.h"
#endif

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/
//
// Define the available number of bytes available for the GUI
//
#define GUI_NUMBYTES  (1024 * 1024 * 4)

#if   (defined __ICCARM__)  // IAR
  static __no_init U32 aMemory[GUI_NUMBYTES / 4] @ "GUI_RAM";
#elif (defined __SES_ARM)   // SES
  static U32 aMemory[GUI_NUMBYTES / 4] __attribute__ ((section (".SDRAM1.GUI_RAM")));
#else
  static U32 aMemory[GUI_NUMBYTES / 4];
#endif

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/
/*********************************************************************
*
*       GUI_X_Config
*
* Purpose:
*   Called during the initialization process in order to set up the
*   available memory for the GUI.
*/
void GUI_X_Config(void) {
#ifndef _WINDOWS
  //
  // Call GUI_BSP_SDRAM_Init to initialize SDRAM prior of using it.
  //
  GUI_BSP_SDRAM_Init();
#endif
  //
  // Assign memory to emWin
  //
  GUI_ALLOC_AssignMemory(aMemory, GUI_NUMBYTES);
#ifndef _WINDOWS
  //
  // Init touch
  //
  GUI_PID_SetInitFunc(PID_X_Init);
#endif
}

/*************************** End of file ****************************/
