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
File        : GUIDEMO_Intro.c
Purpose     : Introduction for emWin generic demo
              (This is also a good file to demo and explain basic
              emWin features by setting breakpoints)
----------------------------------------------------------------------
*/

#include <string.h>

#include "GUIDEMO.h"

/*********************************************************************
*
*       Defines
*/
#define SCREEN_DIV          6  // 2^6 = 64

#define FACTOR_EMWIN        4
#define FACTOR_DESC        11
#define FACTOR_ANY_COMP    22
#define FACTOR_VERSION     31
#define FACTOR_LOGO        38
#define FACTOR_WWW         56

#define DIST_ANY_COMP      18

/*********************************************************************
*
*       GUIDEMO_Intro
*
**********************************************************************
*/
void GUIDEMO_Intro(void) {
  unsigned xCenter;
  unsigned xSize;
  unsigned ySize;
  char     acVersion[30] = "Version of emWin: ";

  xSize   = LCD_GetXSize();
  ySize   = LCD_GetYSize();
  xCenter = xSize / 2;
  GUIDEMO_DrawBk();
  GUI_SetTextMode(GUI_TM_TRANS);
  //
  // emWin
  //
  GUI_SetColor(GUI_WHITE);
  GUI_SetFont(&GUI_FontRounded22);
  GUI_DispStringHCenterAt("emWin",                                                 xCenter, (FACTOR_EMWIN * ySize)     >> SCREEN_DIV);
  //
  // emWin description
  //
  GUI_SetFont(&GUI_FontSouvenir18);
  GUI_DispStringHCenterAt("Universal graphic software\nfor embedded applications", xCenter, (FACTOR_DESC * ySize)      >> SCREEN_DIV);
  //
  // Any text
  //
  GUI_SetColor(GUI_MAKE_COLOR(0x2288ff));
  if (xSize < 320) {
    GUI_DispStringHCenterAt("Any CPU - Any Display\nAny Application",               xCenter, (FACTOR_ANY_COMP * ySize)  >> SCREEN_DIV);
    //
    // Compiled
    //
    GUI_SetFont(&GUI_Font10S_ASCII);
    GUI_DispStringHCenterAt("Compiled " __DATE__ " "__TIME__,                        xCenter, ((FACTOR_ANY_COMP * ySize) >> SCREEN_DIV) + DIST_ANY_COMP + 20);
    //
    // Version
    //
    GUI_SetColor(GUI_WHITE);
    GUI_SetFont(&GUI_FontSouvenir18);
    strcat(acVersion, GUI_GetVersionString());
    GUI_DispStringHCenterAt(acVersion,                                               xCenter,  ((FACTOR_VERSION * ySize)  >> SCREEN_DIV) + 10);
  } else {
    GUI_DispStringHCenterAt("Any CPU - Any Display - Any Application",               xCenter, (FACTOR_ANY_COMP * ySize)  >> SCREEN_DIV);
    //
    // Compiled
    //
    GUI_SetFont(&GUI_Font10S_ASCII);
    GUI_DispStringHCenterAt("Compiled " __DATE__ " "__TIME__,                        xCenter, ((FACTOR_ANY_COMP * ySize) >> SCREEN_DIV) + DIST_ANY_COMP);
    //
    // Version
    //
    GUI_SetColor(GUI_WHITE);
    GUI_SetFont(&GUI_FontSouvenir18);
    strcat(acVersion, GUI_GetVersionString());
    GUI_DispStringHCenterAt(acVersion,                                               xCenter,  (FACTOR_VERSION * ySize)  >> SCREEN_DIV);
  }
  //
  // Logo
  //
  GUI_DrawBitmap(&bmSeggerLogo, (xSize - bmSeggerLogo.XSize) >> 1, (FACTOR_LOGO * ySize) >> SCREEN_DIV);
  //
  // www.segger.com
  //
  GUI_SetColor(GUI_WHITE);
  GUI_SetFont(&GUI_FontRounded22);
  GUI_DispStringHCenterAt("www.segger.com", xCenter, (FACTOR_WWW * ySize) >> SCREEN_DIV);
  GUIDEMO_Delay(5000);
}

/*************************** End of file ****************************/
