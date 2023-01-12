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

File        : CRYPTO_Test_ECDH.c
Purpose     : Run all ECDH self tests.

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
*       Defines, fixed
*
**********************************************************************
*/

#define MAX_CHUNKS  34

/*********************************************************************
*
*       Local types
*
**********************************************************************
*/

typedef CRYPTO_MPI_LIMB MPI_UNIT[CRYPTO_MPI_LIMBS_REQUIRED(2*521)+3];  // +3 as one of the EdDSA divisors is not normalized

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static SEGGER_MEM_CONTEXT       _MemContext;
static SEGGER_MEM_SELFTEST_HEAP _Heap;
static MPI_UNIT                 _aUnits[MAX_CHUNKS];
static int                      _NumDots;
static int                      _NumTests;
static char                     _acTestName[80];
static char                     _acTestSource[80];

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
  if (_Heap.Stats.NumInUse > 0) {
    _SelfTestFail("Heap is not empty at end point -- leak?");
  }
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
  // Run some basic tests that must succeed before progressing to ECDH.
  //
  CRYPTO_EC_SEGGER_SelfTest     (&SelfTestAPI, &_MemContext);
  CRYPTO_EC_NSA_SelfTest        (&SelfTestAPI, &_MemContext);
  CRYPTO_EC_RFC7027_SelfTest    (&SelfTestAPI, &_MemContext);
  //
  CRYPTO_ECDH_KA_SEGGER_SelfTest(&SelfTestAPI, &_MemContext);
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
  //
  CRYPTO_Init();
  SEGGER_MEM_SELFTEST_HEAP_Init(&_MemContext, &_Heap, _aUnits, MAX_CHUNKS, sizeof(MPI_UNIT));
  SEGGER_SYS_Init();
  //
  SEGGER_SYS_IO_Printf("\n");
  SEGGER_SYS_IO_Printf("%s    www.segger.com\n", CRYPTO_GetCopyrightText());
  SEGGER_SYS_IO_Printf("ECDH Self-Test compiled " __DATE__ " " __TIME__ "\n\n");
  //
  _RunTests();
  //
  SEGGER_SYS_IO_Printf("\nAll tests passed.\n");
  SEGGER_SYS_OS_PauseBeforeHalt();
  SEGGER_SYS_OS_Halt(0);
}

/*************************** End of file ****************************/
