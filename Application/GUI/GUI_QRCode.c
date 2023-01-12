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
File    : GUI_QRCode.c
Purpose : A simple QRCode-application.
Literature:
Notes:
Additional information:
  Preperations:
    Works out-of-the-box.
  Expected behavior:
    This sample dispalys a QRCode on the display.
  Sample output:
    A QRCode
*/

#include "GUI.h"

/*********************************************************************
*
*       Defines
*/
#define QRCODE_STRING     "https://www.segger.com"
#define QRCODE_PIXELSIZE  2

/*********************************************************************
*
*       MainTask()
*/
void MainTask(void);
void MainTask(void) {
  GUI_HMEM    hQr;
  GUI_QR_INFO Info;
  int         xSize;
  int         ySize;
  GUI_RECT    Rect;

  GUI_Init();
  //
  // Get display dimension
  //
  xSize = LCD_GetXSize();
  ySize = LCD_GetYSize();
  //
  // Create QR code
  //
  hQr = GUI_QR_Create(QRCODE_STRING, QRCODE_PIXELSIZE, GUI_QR_ECLEVEL_M, 0);
  //
  // Get information about the QR code
  //
  GUI_QR_GetInfo(hQr, &Info);
  //
  // Set up rectangle to draw a white background
  //
  Rect.x0 = (xSize - Info.Size) / 2 - QRCODE_PIXELSIZE;
  Rect.y0 = (ySize - Info.Size) / 2 - QRCODE_PIXELSIZE;
  Rect.x1 = Rect.x0 + Info.Size + QRCODE_PIXELSIZE * 2;
  Rect.y1 = Rect.y0 + Info.Size + QRCODE_PIXELSIZE * 2;
  //
  // Fill background
  //
  GUI_FillRectEx(&Rect);
  //
  // Draw QR code
  //
  GUI_QR_Draw(hQr, Rect.x0 + QRCODE_PIXELSIZE, Rect.y0 + QRCODE_PIXELSIZE);
  //
  // Display URL slightly below QR code
  //
  GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
  GUI_DispStringAt(QRCODE_STRING, LCD_GetXSize() / 2, Rect.y1 + 10);
  while(1) {
    GUI_Delay(500);
  }
}

/****** End of File *************************************************/
