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

File    : USBH_Printer_Start.c
Purpose : This sample is designed to present emUSBH's capability to
          enumerate Printers and to communicate with them.

Additional information:
  Preparations:
    None.

  Expected behavior:
    After a printer has been connected the sample will
    show information about the connected printer and will
    print one (paper) page.

  Sample output:
    <...>

    0:010 USBH_isr - INIT:    INIT: USBH_ISRTask started
    8:360 USBH_Task - **** Device added [0]

    8:410 MainTask - Device Id  = MFG:Hewlett-Packard;CMD:PJL,PML,POSTSCRIPT,
    PCLXL,PCL;MDL:HP LaserJet P2015 Series;CLS:PRINTER;DES:Hewlett-Packard
    LaserJet P2015 Series;MEM:MEM=23MB;OPTRAY:250Sheets;COMMENT:RES=1200x1;
    8:410 MainTask - PortStatus = 0x18 ->NoError=1, Select/OnLine=1, PaperEmpty=0
    8:410 MainTask - Printing "It simply works!" to printer
    8:410 MainTask - Printing completed

    <...>
*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/
#include "RTOS.h"
#include "BSP.h"
#include "USBH.h"
#include "USBH_PrinterClass.h"

/*********************************************************************
*
*       Local data definitions
*
**********************************************************************
*/
enum {
  TASK_PRIO_APP = 150,
  TASK_PRIO_USBH_MAIN,
  TASK_PRIO_USBH_ISR
};

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static OS_STACKPTR int     _StackMain[1536/sizeof(int)];
static OS_TASK             _TCBMain;

static OS_STACKPTR int     _StackIsr[1024/sizeof(int)];
static OS_TASK             _TCBIsr;
static volatile char       _PrinterReady;
static U8                  _DevIndex;
static unsigned char       _acPrinterJob[] = {"It simply works!\f"};

/*********************************************************************
*
*       Static Code
*
**********************************************************************
*/
/*********************************************************************
*
*       _OnPrinterReady
*
*  Function description
*    Called by the application task (MainTask) when an printer device is
*    plugged in.
*    It reads and displays information about the printer and
*    prints out text (on paper).
*/
static void _OnPrinterReady(void) {
  char         ac[20];
  USBH_PRINTER_HANDLE hPrinter;
  U8                  PortStatus;
  U8                  acDeviceId[255] = {0};

  SEGGER_snprintf(ac, sizeof(ac), "prt%.3d", _DevIndex);
  hPrinter = USBH_PRINTER_Open(ac);
  if (hPrinter) {
    USBH_PRINTER_GetDeviceId(hPrinter, acDeviceId, sizeof(acDeviceId));
    USBH_PRINTER_GetPortStatus(hPrinter, &PortStatus);
    USBH_Logf_Application("Device Id  = %s", &acDeviceId[2]);
    USBH_Logf_Application("PortStatus = 0x%x ->NoError=%d, Select/OnLine=%d, PaperEmpty=%d", PortStatus, (PortStatus & (1 << 3)) >> 3, (PortStatus & (1 << 4)) >> 4, (PortStatus & (1 << 5)) >> 5);
    USBH_Logf_Application("Printing \"%s\" to printer", _acPrinterJob);
    USBH_PRINTER_Write(hPrinter, _acPrinterJob, sizeof(_acPrinterJob));
    USBH_Logf_Application("Printing completed");
    USBH_PRINTER_Close(hPrinter);
  }
  //
  // Wait until Printer is removed.
  //
  while (_PrinterReady) {
    OS_Delay(100);
  }
}

/*********************************************************************
*
*       _cbOnAddRemoveDevice
*
*  Function description
*    Callback, called when a device is added or removed.
*    Called in the context of the USBH_Task.
*    The functionality in this routine should not block
*/
static void _cbOnAddRemoveDevice(void * pContext, U8 DevIndex, USBH_DEVICE_EVENT Event) {
  (void)pContext;
  switch (Event) {
  case USBH_DEVICE_EVENT_ADD:
    USBH_Logf_Application("**** Device added [%d]\n", DevIndex);
    _PrinterReady = 1;
    break;
  case USBH_DEVICE_EVENT_REMOVE:
    USBH_Logf_Application("**** Device removed [%d]\n", DevIndex);
    _PrinterReady = 0;
    break;
  default:;   // Should never happen
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
*       MainTask
*/
#ifdef __cplusplus
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif
void MainTask(void);
#ifdef __cplusplus
}
#endif
void MainTask(void) {
  USBH_Init();
  OS_SetPriority(OS_GetTaskID(), TASK_PRIO_APP);                                       // This task has the lowest prio for real-time application.
                                                                                       // Tasks using emUSB-Host API should always have a lower priority than emUSB-Host main and ISR tasks.
  OS_CREATETASK(&_TCBMain, "USBH_Task", USBH_Task, TASK_PRIO_USBH_MAIN, _StackMain);   // Start USBH main task
  OS_CREATETASK(&_TCBIsr, "USBH_isr", USBH_ISRTask, TASK_PRIO_USBH_ISR, _StackIsr);    // Start USBH ISR task
  USBH_PRINTER_Init();
  USBH_PRINTER_RegisterNotification(_cbOnAddRemoveDevice, NULL);
  while (1) {
    BSP_ToggleLED(1);
    OS_Delay(100);
    if (_PrinterReady) {
      _OnPrinterReady();
    }
  }
}

/*************************** End of file ****************************/
