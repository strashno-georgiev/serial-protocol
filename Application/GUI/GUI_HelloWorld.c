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
-------------------------- END-OF-HEADER -----------------------------
File    : GUI_HelloWorld.c
Purpose : A simple 'Hello World'-application.
Literature:
Notes:
Additional information:
  Preparations:
    Works out-of-the-box.
  Expected behavior:
    This sample displays 'Hello World' on the display.
  Sample output:
    Hello World!
*/

#include "GUI.h"

/*********************************************************************
*
*       main()
*/
void MainTask(void);
void MainTask(void) {
  GUI_Init();
  GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
  GUI_SetFont(&GUI_Font24B_1);
  GUI_DispStringAt("Hello World!", LCD_GetXSize() / 2, LCD_GetYSize() / 2);
  while(1) {
    GUI_Delay(500);
  }
}

/****** End of File *************************************************/
