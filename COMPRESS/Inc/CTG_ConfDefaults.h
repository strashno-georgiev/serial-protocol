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

File        : CTG_ConfDefaults.h
Purpose     : Defines defaults for most configurable defines used in
              the library.  If you want to change a value, please do
              so in CTG_Conf.h, DO NOT modify this file.

*/

#ifndef CTG_CONFDEFAULTS_H
#define CTG_CONFDEFAULTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include "SEGGER.h"
#include "CTG_Conf.h"

// Default to non-debug release build
#ifndef   DEBUG
  #define DEBUG 0
#endif

#if DEBUG > 0
  #ifndef   CTG_DEBUG
    #define CTG_DEBUG      1      // Default for debug builds
  #endif
#else
  #ifndef   CTG_DEBUG
    #define CTG_DEBUG      0      // Default for release builds
  #endif
#endif

#ifndef     CTG_ASSERT
  #if CTG_DEBUG > 0
    #define CTG_ASSERT(X)  /*lint -e{717}*/ do { if (!(X)) { CTG_Panic(__FILE__, __LINE__, #X); } } while (0)
  #else
    #define CTG_ASSERT(X)  /*lint -e{717}*/ do { } while (0)
  #endif
#endif

#ifndef   CTG_USE_PARA                          // Some compilers complain about unused parameters.
  #define CTG_USE_PARA(Para)        (void)Para  // This works for most compilers.
#endif

#ifndef   CTG_MEMSET
  #define CTG_MEMSET     memset
#endif

#ifdef __cplusplus
}
#endif

#endif

/*************************** End of file ****************************/
