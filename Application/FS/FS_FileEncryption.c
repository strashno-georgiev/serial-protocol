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

File    : FS_FileEncryption.c
Purpose : Demonstrates how to use the file encryption.

Additional information:
  Preparations:
    This sample has to be built with FS_SUPPORT_ENCRYPTION set to 1.
    It works without modification with any storage device.

  Expected behavior:
    The application performs the following actions:
    - Formats the storage device if required
    - Opens a test file
    - Configures the encryption for the test file
    - Writes some data to the test file
    - Closes the test file

  Sample output:
    Start
    Write encrypted data to file Cipher.txt
    Finished

  The data written to file is encrypted using the DES encryption algorithm.
  The file can be decrypted on a PC using the FSFileEncrypter.exe command
  line utility using the following parameters:

  FSFileEncrypter.exe -d secret Cipher.txt Plain.txt
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/
#include <stdio.h>
#include "FS.h"
#include "SEGGER.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
#define VOLUME_NAME       ""
#define FILE_NAME         VOLUME_NAME"\\Cipher.txt"

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static char _ac[256];

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       MainTask
*/
#ifdef __cplusplus
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif
void MainTask(void);
#ifdef __cplusplus
}
#endif
void MainTask(void) {
  FS_FILE               * pFile;
  const char            * sPlainText = "This message has been encrypted using emFile";
#if FS_SUPPORT_ENCRYPTION
  const U8                abCryptKey[8] = {(U8)'s', (U8)'e', (U8)'c', (U8)'r', (U8)'e', (U8)'t', 0, 0};
  FS_CRYPT_OBJ            CryptObj;
  static FS_DES_CONTEXT   CryptContext;
#endif // FS_SUPPORT_ENCRYPTION

  FS_X_Log("Start\n");
  //
  // Initialize file system
  //
  FS_Init();
  //
  // Format the storage device if required.
  //
  (void)FS_FormatLLIfRequired(VOLUME_NAME);
  if (FS_IsHLFormatted(VOLUME_NAME) == 0) {
    FS_X_Log("High-level format\n");
    (void)FS_Format(VOLUME_NAME, NULL);
  }
  //
  // Store the sample text encrypted to the test file.
  //
  pFile = FS_FOpen(FILE_NAME, "w");
  if (pFile != NULL) {
#if FS_SUPPORT_ENCRYPTION
    SEGGER_snprintf(_ac, (int)sizeof(_ac), "Write encrypted data to file %s\n", FILE_NAME);
    FS_X_Log(_ac);
    //
    // Configure the encryption operation. This operation has to be performed only once.
    //
    FS_CRYPT_Prepare(&CryptObj, &FS_CRYPT_ALGO_DES, &CryptContext, 512, abCryptKey);
    //
    // Enable the encryption for the test file.
    //
    (void)FS_SetEncryptionObject(pFile, &CryptObj);
#else
    FS_X_Log("ERROR: The support for encryption is not enabled.\n");
#endif // FS_SUPPORT_ENCRYPTION
    //
    // Write the data encrypted to the test file.
    //
    (void)FS_Write(pFile, sPlainText, strlen(sPlainText));
    (void)FS_FClose(pFile);
  } else {
    SEGGER_snprintf(_ac, (int)sizeof(_ac), "ERROR: Could not open file %s for writing.\n", FILE_NAME);
    FS_X_Log(_ac);
  }
  FS_Unmount(VOLUME_NAME);
  FS_X_Log("Finished\n");
  for (;;) {
    ;
  }
}

/*************************** End of file ****************************/
