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

File        : SSL_Conf.h
Purpose     : Configuration file for configurable defines in SSL module

*/

#ifndef SSL_CONF_H
#define SSL_CONF_H

//
// Define SSL_DEBUG: Debug level for SSL product
//                  0: No checks                      (Smallest and fastest code)
//                  1: Warnings & Panic checks
//                  2: Warnings, logs, & panic checks (Seriously bigger code)
//
#if defined(DEBUG) && (DEBUG > 0)
  #ifndef   SSL_DEBUG
    #define SSL_DEBUG      2      // Default for debug builds
  #endif
#else
  #ifndef   SSL_DEBUG
    #define SSL_DEBUG      0      // Default for release builds
  #endif
#endif

//
// Configure maximum session ticket length.
//
#ifndef   SSL_MAX_SESSION_TICKET_LEN
  #define SSL_MAX_SESSION_TICKET_LEN      256
#endif

//
// Configure profiling support.
//
#if defined(SUPPORT_PROFILE) && SUPPORT_PROFILE
  #ifndef   SSL_SUPPORT_PROFILE
    #define SSL_SUPPORT_PROFILE           1                   // Define as 1 to enable profiling via SystemView.
  #endif
#endif

//
// Inline OS function calls for higher performance in release builds
//
#if !defined(SSL_OS_DO_NOT_INLINE_CALLS) && !defined(WIN32) && !defined(__linux__)
  #if SSL_DEBUG > 0
    #include "RTOS.h"
    extern OS_RSEMA SSL_OS_RSema;
    #define SSL_OS_DISABLE_INTERRUPT()   OS_DI()
    #define SSL_OS_ENABLE_INTERRUPT()    OS_RestoreI()
    #define SSL_OS_GET_TIME()            OS_GetTime32()
    #define SSL_OS_LOCK()                OS_Use(&SSL_OS_RSema)
    #define SSL_OS_UNLOCK()              OS_Unuse(&SSL_OS_RSema)
  #endif
#endif

#endif

/*************************** End of file ****************************/
