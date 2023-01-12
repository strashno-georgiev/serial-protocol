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
File        : CRYPTO_X_Config_SSL_STM32F4F7.c
Purpose     : Configure CRYPTO for full SSL with no hardware accelerators
              but the hardware RNG for STM32F4/F7 devices
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
*    Configure no hardware assist for CRYPTO component.
*/
void CRYPTO_X_Config(void) {
  volatile U32 *pReg;
  //
  // Software ciphers.
  //
  CRYPTO_MD5_Install      (&CRYPTO_HASH_MD5_SW,        NULL);
  CRYPTO_RIPEMD160_Install(&CRYPTO_HASH_RIPEMD160_SW,  NULL);
  CRYPTO_SHA1_Install     (&CRYPTO_HASH_SHA1_SW,       NULL);
  CRYPTO_SHA224_Install   (&CRYPTO_HASH_SHA224_SW,     NULL);
  CRYPTO_SHA256_Install   (&CRYPTO_HASH_SHA256_SW,     NULL);
  CRYPTO_SHA512_Install   (&CRYPTO_HASH_SHA512_SW,     NULL);
  CRYPTO_AES_Install      (&CRYPTO_CIPHER_AES_SW,      NULL);
  CRYPTO_TDES_Install     (&CRYPTO_CIPHER_TDES_SW,     NULL);
  CRYPTO_SEED_Install     (&CRYPTO_CIPHER_SEED_SW,     NULL);
  CRYPTO_ARIA_Install     (&CRYPTO_CIPHER_ARIA_SW,     NULL);
  CRYPTO_CAMELLIA_Install (&CRYPTO_CIPHER_CAMELLIA_SW, NULL);
  //
  // Turn on clocks to the RNG and reset it.
  //
  pReg = (volatile U32 *)0x40023834;  // RCC_AHB2ENR
  *pReg |= 1U << 6;                   // RCC_AHB2ENR.RNGEN=1
  pReg = (volatile U32 *)0x40023814;  // RCC_AHB2RSTR
  *pReg |= 1U << 6;                   // RCC_AHB2RSTR.RNGRST=1
  *pReg &= ~(1U << 6);                // RCC_AHB2RSTR.RNGRST=0
  //
  // Random number generator.
  //
  CRYPTO_RNG_InstallEx    (&CRYPTO_RNG_HW_STM32_RNG, &CRYPTO_RNG_HW_STM32_RNG);
  //
  // Install small modular exponentiation functions.
  //
  CRYPTO_MPI_SetPublicModExp (CRYPTO_MPI_ModExp_Basic_Fast);
  CRYPTO_MPI_SetPrivateModExp(CRYPTO_MPI_ModExp_Basic_Fast);
}

/*************************** End of file ****************************/
