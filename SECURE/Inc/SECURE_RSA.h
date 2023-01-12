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

File        : SECURE_RSA.h
Purpose     : API functions for emSecure-RSA.

*/

#ifndef SECURE_RSA_H
#define SECURE_RSA_H

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/

#include "SECURE_RSA_ConfDefaults.h"
#include "CRYPTO.h"
#include "SEGGER_MEM.h"

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
*    Symbol expands to a number that identifies the specific emSecure-RSA release.
*/
#define SECURE_RSA_VERSION         24600   // Format is "Mmmrr" so, for example, 23600 corresponds to version 2.36.

/*********************************************************************
*
*       Types required for API
*
**********************************************************************
*/

typedef CRYPTO_RSA_PUBLIC_KEY       SECURE_RSA_PUBLIC_KEY;
typedef CRYPTO_RSA_PRIVATE_KEY      SECURE_RSA_PRIVATE_KEY;
typedef CRYPTO_MPI                  SECURE_RSA_KEY_PARAMETER;
#define SECURE_RSA_HASH_CONTEXT     _SECURE_RSA_HASH_CONTEXT

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
int   SECURE_RSA_Sign           (const SECURE_RSA_PRIVATE_KEY * pPrivate, const U8 * pSalt, int SaltLen, const U8 * pMessage, int MessageLen,       U8 * pSignature, int SignatureLen);
int   SECURE_RSA_SignEx         (const SECURE_RSA_PRIVATE_KEY * pPrivate, const U8 * pSalt, int SaltLen, const U8 * pMessage, int MessageLen,       U8 * pSignature, int SignatureLen, SEGGER_MEM_CONTEXT *pMem);
int   SECURE_RSA_Verify         (const SECURE_RSA_PUBLIC_KEY  * pPublic,        U8 * pSalt, int SaltLen, const U8 * pMessage, int MessageLen, const U8 * pSignature, int SignatureLen);
int   SECURE_RSA_VerifyEx       (const SECURE_RSA_PUBLIC_KEY  * pPublic,        U8 * pSalt, int SaltLen, const U8 * pMessage, int MessageLen, const U8 * pSignature, int SignatureLen, SEGGER_MEM_CONTEXT *pMem);

/*********************************************************************
*
*       Chunked message functions
*/
void  SECURE_RSA_HASH_Init      (SECURE_RSA_HASH_CONTEXT *pHash);
void  SECURE_RSA_HASH_Add       (SECURE_RSA_HASH_CONTEXT *pHash, const void *pData, unsigned DataLen);
int   SECURE_RSA_HASH_Sign      (SECURE_RSA_HASH_CONTEXT *pHash, const SECURE_RSA_PRIVATE_KEY * pPrivate, const U8 * pSalt, int SaltLen,       U8 * pSignature, int SignatureLen);
int   SECURE_RSA_HASH_Verify    (SECURE_RSA_HASH_CONTEXT *pHash, const SECURE_RSA_PUBLIC_KEY  * pPublic,        U8 * pSalt, int SaltLen, const U8 * pSignature, int SignatureLen);

/*********************************************************************
*
*       Precomputed digest functions
*/
int   SECURE_RSA_SignDigest     (const SECURE_RSA_PRIVATE_KEY * pPrivate, const U8 * pSalt, int SaltLen, const U8 * pDigest,       U8 * pSignature, int SignatureLen);
int   SECURE_RSA_VerifyDigest   (const SECURE_RSA_PUBLIC_KEY  * pPublic,        U8 * pSalt, int SaltLen, const U8 * pDigest, const U8 * pSignature, int SignatureLen);

/*********************************************************************
*
*       Key setup functions
*/
void  SECURE_RSA_InitPublicKey  (SECURE_RSA_PUBLIC_KEY  * pPublic,  const SECURE_RSA_KEY_PARAMETER * pParamE,  const SECURE_RSA_KEY_PARAMETER * pParamN);
void  SECURE_RSA_InitPrivateKey (SECURE_RSA_PRIVATE_KEY * pPrivate, const SECURE_RSA_KEY_PARAMETER * pParamDP, const SECURE_RSA_KEY_PARAMETER * pParamDQ, const SECURE_RSA_KEY_PARAMETER * pParamP, const SECURE_RSA_KEY_PARAMETER * pParamQ, const SECURE_RSA_KEY_PARAMETER * pParamQInv);

/*********************************************************************
*
*       Initialization.
*/
void  SECURE_RSA_Init           (void);

/*********************************************************************
*
*       Version and copyright information
*/
const char * SECURE_RSA_GetVersionText   (void);
const char * SECURE_RSA_GetCopyrightText (void);

#ifdef __cplusplus
}
#endif

#endif

/*************************** End of file ****************************/
