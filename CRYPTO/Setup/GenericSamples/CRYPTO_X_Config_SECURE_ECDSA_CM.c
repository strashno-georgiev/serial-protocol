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

File        : CRYPTO_X_Config_SECURE_ECDSA_CM.c
Purpose     : Configure CRYPTO for emSecure-ECDSA.

*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "CRYPTO.h"

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       CRYPTO_X_Panic()
*
*  Function description
*    Hang when something unexpected happens.
*/
void CRYPTO_X_Panic(void) {
  for (;;) {
    /* Hang */
  }
}

/*********************************************************************
*
*       CRYPTO_X_Config()
*
*  Function description
*    Configure hardware assist for CRYPTO component.
*/
void CRYPTO_X_Config(void) {
  //
  // Install pure software implementation of SHA-256.  You can
  // customize this and install a hardware version.  If no
  // implementation is installed, emSecure-ECDSA will automatically
  // install SHA-256 in software.
  //
  CRYPTO_SHA256_Install(&CRYPTO_HASH_SHA256_SW, NULL);
}

/*************************** End of file ****************************/
