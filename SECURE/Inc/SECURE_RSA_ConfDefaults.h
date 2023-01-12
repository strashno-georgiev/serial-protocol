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

File        : SECURE_RSA_ConfDefaults.h
Purpose     : Defines defaults defines used in emSecure-RSA.
              If you want to change a value, please do so in
              SECURE_RSA_Conf.h, do NOT modify this file.

              */

#ifndef SECURE_RSA_CONFDEFAULTS_H
#define SECURE_RSA_CONFDEFAULTS_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "CRYPTO.h"
#include "SECURE_RSA_Conf.h"

/*********************************************************************
*
*       Configuration defaults
*
**********************************************************************
*/

#define _SECURE_PASTE3(X,Y,Z)       X##Y##Z
#define _SECURE_SELECT3(X,Y,Z)      _SECURE_PASTE3(X,Y,Z)
#define _SECURE_PASTE5(X,Y,Z,U,V)   X##Y##Z##U##V
#define _SECURE_SELECT5(X,Y,Z,U,V)  _SECURE_PASTE5(X,Y,Z,U,V)
#define _SECURE_RSA_HASH_CONTEXT    _SECURE_SELECT3(CRYPTO_,SECURE_RSA_HASH_FUNCTION,_CONTEXT)


/*********************************************************************
*
*       Configuration defaults
*
**********************************************************************
*/

#ifndef   SECURE_RSA_MAX_KEY_LENGTH
  #define SECURE_RSA_MAX_KEY_LENGTH                   (2048)
#endif

//
// By default select SHA-1; other selections are SHA-256 (SHA256) and SHA-512 (SHA512)
//
#ifndef   SECURE_RSA_HASH_FUNCTION
  #define SECURE_RSA_HASH_FUNCTION                    SHA1
#endif

//
// By default select RSASSA-PSS (PSS); the other selection is RSASSA-PKCS1-v1_5 (PKCS1)
//
#ifndef   SECURE_RSA_SIGNATURE_SCHEME
  #define SECURE_RSA_SIGNATURE_SCHEME                 PSS
#endif

#ifdef __cplusplus
}
#endif

#endif

/*************************** End of file ****************************/
