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

File    : FS_Conf.h
Purpose : File system configuration
*/

#ifndef FS_CONF_H           // Avoid multiple inclusion.
#define FS_CONF_H

#ifdef DEBUG
  #if (DEBUG)
    #define FS_DEBUG_LEVEL     5
  #endif
#endif

#define FS_OS_LOCKING          (1)
#define FS_SUPPORT_JOURNAL     (1)
#define FS_SUPPORT_ENCRYPTION  (1)



#endif                      // Avoid multiple inclusion.

/*************************** End of file ****************************/
