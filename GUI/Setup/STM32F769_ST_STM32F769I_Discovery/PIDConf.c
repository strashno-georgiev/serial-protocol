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
File        : PIDConf.c
Purpose     : Touch screen controller configuration
---------------------------END-OF-HEADER------------------------------
*/

#include "GUI.h"
#include "PIDConf.h"
#include "RTOS.h"
#include "stm32f769i_discovery_ts.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/
//
// Set this to 1 if you want to use emWin touch calibration.
// emWin uses the sample points measured at the reference points.
//
#define USE_CALIBRATION  0

#if (USE_CALIBRATION == 1)
  #define NUM_CALIB_POINTS  5
#endif

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static U8 _IsInitialized;
static int _LayerIndex;

static OS_STACKPTR int Stack_Touch[128];
static OS_TASK TCB_TOUCH;

static void PID_X_Exec(void);
static void TouchTask(void);

#if (USE_CALIBRATION == 1)
  static int _aRefX[] = {  40, 760, 760,  40, 400 };
  static int _aRefY[] = {  24,  24, 456, 456, 240 };
  static int _aSamX[] = {  19, 790, 782,  10, 425 };
  static int _aSamY[] = {  37,   4, 418, 439, 240 };
#endif

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       TouchTask
*/
static void TouchTask(void) {
  while(1) {
    PID_X_Exec();
    OS_Delay(25);
  }
}

/*********************************************************************
*
*       PID_X_Exec
*/
static void PID_X_Exec(void) {
  TS_StateTypeDef TS_State;
  static GUI_PID_STATE StatePID;
  static int IsTouched;
  static int ySize;

  if (ySize == 0) {
    ySize = LCD_GetYSizeEx(0);
  }
  if (_IsInitialized) {
    BSP_TS_GetState(&TS_State);
    StatePID.Layer = _LayerIndex;
    if (TS_State.touchDetected) {
      IsTouched = 1;
      StatePID.Pressed = 1;
      StatePID.x = (int)(TS_State.touchX[0]);
      StatePID.y = (int)(TS_State.touchY[0]);
      GUI_TOUCH_StoreStateEx(&StatePID);
    } else {
      if (IsTouched == 1) {
        IsTouched = 0;
        StatePID.Pressed = 0;
        GUI_TOUCH_StoreStateEx(&StatePID);
      }
    }
  }
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/
/*********************************************************************
*
*       PID_X_SetLayerIndex
*/
void PID_X_SetLayerIndex(int LayerIndex) {
  _LayerIndex = LayerIndex;
}

/*********************************************************************
*
*       PID_X_Init
*/
void PID_X_Init(void) {
  int xSize, ySize;

  if (_IsInitialized == 0) {
    xSize = LCD_GetXSize();
    ySize = LCD_GetYSize();
    BSP_TS_Init(xSize, ySize);
#if (USE_CALIBRATION == 1)
    //
    // Use calibration with sampled points
    //
    GUI_TOUCH_CalcCoefficients(NUM_CALIB_POINTS, _aRefX, _aRefY, _aSamX, _aSamY, xSize, ySize);
#endif
    OS_CREATETASK(&TCB_TOUCH, "TouchTask", TouchTask, 101, Stack_Touch);
    _IsInitialized = 1;
  }
}

/*************************** End of file ****************************/
