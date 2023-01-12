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

File        : CRYPTO_Test_SHA1.c
Purpose     : Run all SHA-1 self tests.

*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/

#include "CRYPTO.h"
#include "SEGGER_SYS.h"

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static int  _NumDots;
static int  _NumTests;
static char _acTestName[80];
static char _acTestSource[80];

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _SelfTestBegin()
*
*  Function description
*    Implementation of self-test API: start of test.
*
*  Parameters
*    sText - Test name.
*/
static void _SelfTestBegin(const char *sText) {
  char *sPos;
  //
  strcpy(_acTestName, sText);
  strcpy(_acTestSource, "");
  sPos = strchr(_acTestName, '(');
  if (sPos) {
    *sPos = 0;
    strcpy(_acTestSource, &sPos[1]);
    sPos = strchr(_acTestSource, ')');
    if (sPos) {
      *sPos = 0;
    }
  }
  SEGGER_SYS_IO_Printf("%-19s %-10s  ", _acTestName, _acTestSource);
  _NumDots = 0;
  _NumTests = 0;
}

/*********************************************************************
*
*       _SelfTestProgress()
*
*  Function description
*    Implementation of self-test API: indicate test progressing.
*/
static void _SelfTestProgress(void) {
  if (_NumDots == 40) {
    _NumDots = 0;
    SEGGER_SYS_IO_Printf("\r%78s\r%-19s %-10s  ", "", _acTestName, _acTestSource);
  }
  SEGGER_SYS_IO_Printf(".");
  ++_NumDots;
  ++_NumTests;
}

/*********************************************************************
*
*       _SelfTestFail()
*
*  Function description
*    Implementation of self-test API: indicate test failure with exit.
*
*  Parameters
*    sText - Reason for failure.
*/
static void _SelfTestFail(const char *sText) {
  SEGGER_SYS_IO_Printf("\r%78s", "");
  SEGGER_SYS_IO_Printf("\r%-19s ", _acTestName);
  SEGGER_SYS_IO_Printf("%-10s  %s\n", _acTestSource, sText);
  SEGGER_SYS_OS_Halt(100);
}

/*********************************************************************
*
*       _SelfTestEnd()
*
*  Function description
*    Implementation of self-test API: indicate test finished successfully.
*/
static void _SelfTestEnd(void) {
  SEGGER_SYS_IO_Printf("\r%78s\r%-19s %-10s  PASS %8d\n", "", _acTestName, _acTestSource, _NumTests);
}

/*********************************************************************
*
*       _RunTests()
*
*  Function description
*    Run all tests.
*/
static void _RunTests(void) {
  //
  static const CRYPTO_SELFTEST_API SelfTestAPI = {
    _SelfTestBegin,
    _SelfTestProgress,
    _SelfTestFail,
    _SelfTestEnd
  };
  //
  SEGGER_SYS_IO_Printf("Algorithm           Source      Status  #Test\n");
  SEGGER_SYS_IO_Printf("---------------------------------------------\n");
  //
  CRYPTO_SHA1_FIPS180_SelfTest(&SelfTestAPI);
  CRYPTO_SHA1_CAVS_SelfTest   (&SelfTestAPI);
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       MainTask()
*
*  Function description
*    Main entry point for application to run all the tests.
*/
void MainTask(void);
void MainTask(void) {
  const CRYPTO_HASH_API *pHWAPI;
  const CRYPTO_HASH_API *pSWAPI;
  //
  CRYPTO_Init();
  SEGGER_SYS_Init();
  //
  SEGGER_SYS_IO_Printf("\n");
  SEGGER_SYS_IO_Printf("%s    www.segger.com\n", CRYPTO_GetCopyrightText());
  SEGGER_SYS_IO_Printf("SHA-1 Self-Test compiled " __DATE__ " " __TIME__ "\n\n");
  //
  CRYPTO_SHA1_QueryInstall(&pHWAPI, &pSWAPI);
  if (pHWAPI == NULL && pSWAPI == NULL) {
    SEGGER_SYS_IO_Printf("No SHA-1 hash algorithm installed!\n");
    SEGGER_SYS_OS_Halt(100);
  }
  //
  _RunTests();
  //
  if (pHWAPI && pHWAPI != &CRYPTO_HASH_SHA1_SW) {
    SEGGER_SYS_IO_Printf("\nRerunning with SHA-1 accelerator deactivated\n\n");
    CRYPTO_SHA1_Install(&CRYPTO_HASH_SHA1_SW, 0);
    _RunTests();
  }
  //
  SEGGER_SYS_IO_Printf("\nAll tests passed.\n");
  SEGGER_SYS_OS_PauseBeforeHalt();
  SEGGER_SYS_OS_Halt(0);
}

/*************************** End of file ****************************/
