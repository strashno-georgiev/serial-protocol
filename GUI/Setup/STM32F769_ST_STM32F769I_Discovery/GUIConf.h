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
File        : GUIConf.h
Purpose     : Configuration of available features and default values
----------------------------------------------------------------------
*/

#ifndef GUICONF_H
#define GUICONF_H

/*********************************************************************
*
*       Multi layer/display support
*/
#define GUI_NUM_LAYERS      2   // Maximum number of available layers

/*********************************************************************
*
*       Multi tasking support
*/
#define GUI_OS              (1)  // Compile with multitasking support

/*********************************************************************
*
*       Configuration of available packages
*/
#define GUI_SUPPORT_TOUCH              (1)  // Support a touch screen (req. win-manager)
#define GUI_SUPPORT_MOUSE              (1)  // Support a mouse
#define GUI_SUPPORT_MEMDEV             (1)  // Memory devices available
#define GUI_WINSUPPORT                 (1)  // Window manager package available
#define GUI_USE_ARGB                   (1)  // Use color conversion defined for ST
#define GUI_SUPPORT_SPY                (1)  // Enable support for emWin Spy
#define GUI_MEMDEV_SUPPORT_CUSTOMDRAW  (1)  // Allow to set custom draw functions for memory devices

#endif  /* Avoid multiple inclusion */

/*************************** End of file ****************************/
