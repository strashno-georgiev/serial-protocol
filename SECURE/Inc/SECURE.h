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

File        : SECURE.h
Purpose     : API functions for emSecure-RSA.

*/

#ifndef SECURE_H
#define SECURE_H

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/

#include "SECURE_RSA.h"

/*********************************************************************
*
*       Migrate v2.10 API to v2.20 and later API
*
**********************************************************************
*/

#define SECURE_PUBLIC_KEY       SECURE_RSA_PUBLIC_KEY
#define SECURE_PRIVATE_KEY      SECURE_RSA_PRIVATE_KEY
#define SECURE_KEY_PARAMETER    SECURE_RSA_KEY_PARAMETER
#define SECURE_Sign             SECURE_RSA_Sign
#define SECURE_Verify           SECURE_RSA_Verify
#define SECURE_InitPublicKey    SECURE_RSA_InitPublicKey
#define SECURE_InitPrivateKey   SECURE_RSA_InitPrivateKey 

#endif

/*************************** End of file ****************************/
