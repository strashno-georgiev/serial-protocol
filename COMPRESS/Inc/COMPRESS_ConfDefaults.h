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

File        : COMPRESS_ConfDefaults.h
Purpose     : Defines defaults for most configurable defines used in
              the library.  If you want to change a value, please do
              so in COMPRESS_Conf.h, DO NOT modify this file.

*/

#ifndef COMPRESS_CONFDEFAULTS_H
#define COMPRESS_CONFDEFAULTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include "SEGGER.h"
#include "COMPRESS_Conf.h"

//lint -sem(COMPRESS_AssertFail, r_no)  never returns

// Default to non-debug release build
#ifndef   DEBUG
  #define DEBUG 0
#endif

#if DEBUG > 0
  #ifndef   COMPRESS_DEBUG
    #define COMPRESS_DEBUG      1      // Default for debug builds
  #endif
#else
  #ifndef   COMPRESS_DEBUG
    #define COMPRESS_DEBUG      0      // Default for release builds
  #endif
#endif

#ifndef     COMPRESS_ASSERT
  #if COMPRESS_DEBUG > 0
    #define COMPRESS_ASSERT(X)  /*lint -e{717}*/ do { if (!(X)) { COMPRESS_AssertFail(__FILE__, __LINE__, #X); } } while (0)
  #else
    #define COMPRESS_ASSERT(X)  /*lint -e{717}*/ do { } while (0)
  #endif
#endif

#ifndef   COMPRESS_USE_PARA                          // Some compilers complain about unused parameters.
  #define COMPRESS_USE_PARA(Para)        (void)Para  // This works for most compilers.
#endif

#ifndef   COMPRESS_MEMCPY
  #define COMPRESS_MEMCPY     memcpy
#endif

#ifndef   COMPRESS_MEMSET
  #define COMPRESS_MEMSET     memset
#endif

#ifndef   COMPRESS_MEMMOVE
  #define COMPRESS_MEMMOVE    memmove
#endif

#ifndef   COMPRESS_MEMCMP
  #define COMPRESS_MEMCMP     memcmp
#endif

#ifdef __cplusplus
}
#endif

#endif

/*************************** End of file ****************************/
