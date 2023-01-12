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
File    : MB_ConfDefaults.h
Purpose : Defines defaults for most configurable defines used in the stack.
          If you want to change a value, please do so in MB_Conf.h,
          do NOT modify this file.
--------  END-OF-HEADER  ---------------------------------------------
*/

#ifndef MB_CONFDEFAULTS_H     // Avoid multiple/recursive inclusion.
#define MB_CONFDEFAULTS_H  1

#include <string.h>           // Required for memset.
#include "MB_Conf.h"

/*********************************************************************
*
*       Defaults
*
**********************************************************************
*/

//
// Operating system interface. For embOS, the functions can be inlined.
//
#ifndef   MB_OS_DISABLE_INTERRUPT
  #define MB_OS_DISABLE_INTERRUPT          MB_OS_DisableInterrupt
#endif

#ifndef   MB_OS_ENABLE_INTERRUPT
  #define MB_OS_ENABLE_INTERRUPT           MB_OS_EnableInterrupt
#endif

#ifndef   MB_OS_GET_TIME
  #define MB_OS_GET_TIME                   MB_OS_GetTime
#endif

//
// Configuration defaults.
//
#ifndef   MB_DEBUG
  #define MB_DEBUG                         0            // Debug level: 0: Release, 1: Support "Panic" checks, 2: Support warn & log.
#endif

#ifndef   MB_IS_BIG_ENDIAN
  #define MB_IS_BIG_ENDIAN                 0            // Little endian is default
#endif

#ifndef   MB_USE_PARA                                   // Some compiler complain about unused parameters.
  #define MB_USE_PARA(Para)                (void)Para   // This works for most compilers.
#endif

#ifndef MB_SUPPORT_LOG
  #if   (MB_DEBUG > 1)
    #define MB_SUPPORT_LOG                 1
  #else
    #define MB_SUPPORT_LOG                 0
  #endif
#endif

#ifndef MB_SUPPORT_WARN
  #if   (MB_DEBUG > 1)
    #define MB_SUPPORT_WARN                1
  #else
    #define MB_SUPPORT_WARN                0
  #endif
#endif

#ifndef   MB_ALLOW_DEINIT
  #define MB_ALLOW_DEINIT                  1            // MB_DeInit() can be used to de-initialize the stack.
#endif

#ifndef   MB_TIMER_FREQ
  #define MB_TIMER_FREQ                    1000         // 1kHz timer frequency for RTU timeout timer.
#endif

#ifndef   MB_DISCONNECT_ON_MSG_TOO_BIG
  #define MB_DISCONNECT_ON_MSG_TOO_BIG     1            // Disconnect if a message bigger than MB_MAX_MESSAGE_SIZE is received (determined by header length field).
#endif

#ifndef   MB_ALLOW_STREAM_HDR_UNDERFLOW
  #define MB_ALLOW_STREAM_HDR_UNDERFLOW    0            // Allow to receive the header on a streaming interface (Modbus/TCP or Modbus/UDP) in multiple recv() calls.
#endif                                                  // Typically this should never happen and if it happens this typically means that communication got out of sync.

//
// Porting macros.
//
#ifndef   MB_MEMCPY
  #define MB_MEMCPY                        memcpy
#endif

#ifndef   MB_MEMSET
  #define MB_MEMSET                        memset
#endif

#ifndef   MB_MEMMOVE
  #define MB_MEMMOVE                       memmove
#endif

#ifndef   MB_MEMCMP
  #define MB_MEMCMP                        memcmp
#endif

#ifndef MB_PANIC
  #if   MB_DEBUG
    #define MB_PANIC(s)                    MB_Panic(s)
  #else
    #define MB_PANIC(s)
  #endif
#endif

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#define MB_MAX_MESSAGE_SIZE   256  // 256 bytes is the maximum size of a Modbus RTU/ASCII message.
#define MB_MAX_PROT_OVERHEAD  6    // Protocol overhead for Modbus/TCP or Modbus/UDP.
#define MB_BUFFER_SIZE        (MB_MAX_MESSAGE_SIZE + MB_MAX_PROT_OVERHEAD)


#endif // Avoid multiple inclusion

/****** End Of File *************************************************/
