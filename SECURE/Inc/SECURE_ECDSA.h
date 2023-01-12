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

File        : SECURE_ECDSA.h
Purpose     : API functions for emSecure-ECDSA.

*/

#ifndef SECURE_ECDSA_H
#define SECURE_ECDSA_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "SECURE_ECDSA_ConfDefaults.h"
#include "CRYPTO.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

/*********************************************************************
*
*       Version number
*
*  Description
*    Symbol expands to a number that identifies the specific emSecure-ECDSA release.
*/
#define SECURE_ECDSA_VERSION             24600   // Format is "Mmmrr" so, for example, 23800 corresponds to version 2.38.

/*********************************************************************
*
*       Types required for API
*
**********************************************************************
*/

typedef CRYPTO_EC_CURVE           SECURE_ECDSA_CURVE;
typedef CRYPTO_MPI                SECURE_ECDSA_KEY_PARAMETER;
typedef CRYPTO_SHA256_CONTEXT     SECURE_ECDSA_HASH_CONTEXT;
typedef CRYPTO_ECDSA_PUBLIC_KEY   SECURE_ECDSA_PUBLIC_KEY;
typedef CRYPTO_ECDSA_PRIVATE_KEY  SECURE_ECDSA_PRIVATE_KEY;

/*********************************************************************
*
*       Public constant data
*
**********************************************************************
*/

#define SECURE_ECDSA_CURVE_P192  CRYPTO_EC_CURVE_P192
#define SECURE_ECDSA_CURVE_P224  CRYPTO_EC_CURVE_P224
#define SECURE_ECDSA_CURVE_P256  CRYPTO_EC_CURVE_P256
#define SECURE_ECDSA_CURVE_P384  CRYPTO_EC_CURVE_P384
#define SECURE_ECDSA_CURVE_P521  CRYPTO_EC_CURVE_P521


/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

/*********************************************************************
*
*       Complete message functions
*/
int   SECURE_ECDSA_Sign           (const SECURE_ECDSA_PRIVATE_KEY * pPrivate, const U8 * pMessage, int MessageLen,       U8 * pSignature, int SignatureLen);
int   SECURE_ECDSA_Verify         (const SECURE_ECDSA_PUBLIC_KEY  * pPublic,  const U8 * pMessage, int MessageLen, const U8 * pSignature, int SignatureLen);

/*********************************************************************
*
*       Chunked message functions
*/
void  SECURE_ECDSA_HASH_Init      (SECURE_ECDSA_HASH_CONTEXT *pHash);
void  SECURE_ECDSA_HASH_Add       (SECURE_ECDSA_HASH_CONTEXT *pHash, const void *pData, unsigned DataLen);
int   SECURE_ECDSA_HASH_Sign      (SECURE_ECDSA_HASH_CONTEXT *pHash, const SECURE_ECDSA_PRIVATE_KEY * pPrivate,       U8 * pSignature, int SignatureLen);
int   SECURE_ECDSA_HASH_Verify    (SECURE_ECDSA_HASH_CONTEXT *pHash, const SECURE_ECDSA_PUBLIC_KEY  * pPublic,  const U8 * pSignature, int SignatureLen);

/*********************************************************************
*
*       Precomputed digest functions
*/
int   SECURE_ECDSA_SignDigest     (const SECURE_ECDSA_PRIVATE_KEY * pPrivate, const U8 * pDigest,       U8 * pSignature, int SignatureLen);
int   SECURE_ECDSA_VerifyDigest   (const SECURE_ECDSA_PUBLIC_KEY  * pPublic,  const U8 * pDigest, const U8 * pSignature, int SignatureLen);

/*********************************************************************
*
*       Key setup functions
*/
void  SECURE_ECDSA_InitPublicKey  (SECURE_ECDSA_PUBLIC_KEY  * pPublic,  const SECURE_ECDSA_KEY_PARAMETER * pParamYX, const SECURE_ECDSA_KEY_PARAMETER * pParamYY, const SECURE_ECDSA_CURVE * pCurve);
void  SECURE_ECDSA_InitPrivateKey (SECURE_ECDSA_PRIVATE_KEY * pPrivate, const SECURE_ECDSA_KEY_PARAMETER * pParamX,  const SECURE_ECDSA_CURVE * pCurve);

/*********************************************************************
*
*       Initialization.
*/
void  SECURE_ECDSA_Init           (void);

/*********************************************************************
*
*       Version and copyright information
*/
const char * SECURE_ECDSA_GetVersionText   (void);
const char * SECURE_ECDSA_GetCopyrightText (void);

#ifdef __cplusplus
}
#endif

#endif

/*************************** End of file ****************************/
