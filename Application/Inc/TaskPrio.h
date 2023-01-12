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
--------  END-OF-HEADER  ---------------------------------------------
*/

#ifndef TASKPRIO_H                     /* Avoid multiple inclusion */
#define TASKPRIO_H

/*********************************************************************
*
*       Task priorities
*/
enum {
  TASKPRIO_FTPSCHILD = 100,  // 100
  TASKPRIO_FTPSPARENT,       // 101
  TASKPRIO_FTPS,             // 102
  TASKPRIO_WEBS,             // 103
  TASKPRIO_WEBSCHILD,        // 104
  TASKPRIO_WEBSPARENT,       // 105
  TASKPRIO_USB,              // 106
  TASKPRIO_WINDOW,           // 107
  TASKPRIO_MAINTASK,         // 108
  TASKPRIO_TOUCH,            // 109
  TASKPRIO_VNC,              // 110
  TASKPRIO_USBH_MAIN,        // 111
  TASKPRIO_USBH_ISR,         // 112
  TASKPRIO_IPMAIN,           // 113, should be higher than prio of server applications
  TASKPRIO_IPRX,             // 114, needs to have highest prio except for windows that need to be shown immediately
  TASKPRIO_TRIALWINDOW       // 115, highest prio, needs to be shown immediately
};


#endif                                  /* Avoid multiple inclusion */

/*****  EOF  ********************************************************/
