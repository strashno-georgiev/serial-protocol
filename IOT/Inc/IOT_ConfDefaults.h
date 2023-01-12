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

File        : IOT_ConfDefaults.h
Purpose     : Defines defaults for most configurable defines used in SEGGER
              IoT library.
              If you want to change a value, please do so in IOT_Conf.h,
              do NOT modify this file.

*/

#ifndef IOT_CONFDEFAULTS_H
#define IOT_CONFDEFAULTS_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "IOT_Conf.h"
#include "SEGGER.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

//
// Define IOT_DEBUG: Debug level for SSL product
//                  0: No checks                      (Smallest and fastest code)
//                  1: Warnings & Panic checks
//                  2: Warnings, logs, & panic checks (Seriously bigger code)
//
#ifndef   IOT_DEBUG
  #define IOT_DEBUG                       0
#endif

#ifndef   IOT_MEMCPY
  #define IOT_MEMCPY                      memcpy
#endif

#ifndef   IOT_MEMSET
  #define IOT_MEMSET                      memset
#endif

#ifndef   IOT_MEMMOVE
  #define IOT_MEMMOVE                     memmove
#endif

#ifndef   IOT_MEMCMP
  #define IOT_MEMCMP                      memcmp
#endif

#ifndef   IOT_STRCPY
  #define IOT_STRCPY                      strcpy
#endif

#ifndef   IOT_STRNCPY
  #define IOT_STRNCPY                     strncpy
#endif

#ifndef   IOT_STRCMP
  #define IOT_STRCMP                      strcmp
#endif

#ifndef   IOT_STRNCMP
  #define IOT_STRNCMP                     strncmp
#endif

#ifndef   IOT_STRLEN
  #define IOT_STRLEN                      strlen
#endif

#ifndef   IOT_STRCAT
  #define IOT_STRCAT                      strcat
#endif

#ifndef   IOT_STRNCAT
  #define IOT_STRNCAT                     strncat
#endif

#ifndef   IOT_STRCHR
  #define IOT_STRCHR                      strchr
#endif

#ifndef   IOT_STRRCHR
  #define IOT_STRRCHR                     strrchr
#endif

#ifndef   IOT_STRTOULL
  #define IOT_STRTOULL                    strtoull
#endif

#ifndef   IOT_STRCASECMP
  #define IOT_STRCASECMP                  SEGGER_strcasecmp
#endif

#ifndef   IOT_SPRINTF
  #define IOT_SPRINTF                     sprintf
#endif

#ifndef   IOT_SNPRINTF
  #define IOT_SNPRINTF                    SEGGER_snprintf
#endif

#ifndef   IOT_STRCASECMP
  #define IOT_STRCASECMP                  SEGGER_strcasecmp
#endif

#ifndef   IOT_STRNCASECMP
  #define IOT_STRNCASECMP                 SEGGER_strncasecmp
#endif

#ifndef   IOT_SSCANF
  #define IOT_SSCANF                      sscanf
#endif


#ifndef   IOT_TOLOWER
  #define IOT_TOLOWER                     tolower
#endif

#ifndef   IOT_USE_PARA                                // Some compiler complain about unused parameters.
  #define IOT_USE_PARA(Para)              (void)Para  // This works for most compilers.
#endif

#if IOT_DEBUG > 0
  #define IOT_ASSERT(X)                   do { if (!(X)) { IOT_PANIC(); } } while (0)
  #define IOT_PANIC()                     IOT_Panic()
#else
  #define IOT_ASSERT(X)
  #define IOT_PANIC()
#endif

#ifdef __cplusplus
}
#endif

#endif

/*************************** End of file ****************************/
