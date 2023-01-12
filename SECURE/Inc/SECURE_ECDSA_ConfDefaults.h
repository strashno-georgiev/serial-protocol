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

File        : SECURE_ECDSA_ConfDefaults.h
Purpose     : Defines defaults defines used in emSecure-ECDSA.
              If you want to change a value, please do so in SECURE_ECDSA_Conf.h,
              do NOT modify this file.

*/

#ifndef SECURE_ECDSA_CONFDEFAULTS_H
#define SECURE_ECDSA_CONFDEFAULTS_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "SECURE_ECDSA_Conf.h"
#include "CRYPTO.h"

/*********************************************************************
*
*       Configuration defaults
*
**********************************************************************
*/

#ifndef   SECURE_ECDSA_MAX_KEY_LENGTH
  #define SECURE_ECDSA_MAX_KEY_LENGTH     256  // Configure for at most P-256
#endif

#ifdef __cplusplus
}
#endif

#endif

/*************************** End of file ****************************/
