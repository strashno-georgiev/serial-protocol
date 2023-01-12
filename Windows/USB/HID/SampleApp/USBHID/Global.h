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
----------------------------------------------------------------------
File    : Global.h
Purpose : Global types
          In case your application already has a Global.h, you should
          merge the files. In order to use Segger code, the types
          U8, U16, U32, I8, I16, I32 need to be defined in Global.h;
          additional defintions do not hurt.
---------------------------END-OF-HEADER------------------------------
*/

#ifndef GLOBAL_H            // Guard against multiple inclusion
#define GLOBAL_H

#define U8    unsigned char
#define U16   unsigned short
#define U32   unsigned long
#define U64   unsigned __int64
#define I8    signed char
#define I16   signed short
#define I32   signed long
#define I64   signed __int64

#ifndef   COUNTOF
  #define COUNTOF(a)    (sizeof(a)/sizeof(a[0]))
#endif

#ifndef   ZEROFILL
  #define ZEROFILL(Obj) memset(&Obj, 0, sizeof(Obj))
#endif

#ifndef   LIMIT
  #define LIMIT(a,b)    if (a > b) a = b;
#endif

#ifndef   MIN
  #define MIN(a, b)     (((a) < (b)) ? (a) : (b))
#endif

#ifndef   MAX
  #define MAX(a, b)     (((a) > (b)) ? (a) : (b))
#endif


typedef enum {IS_NOINIT, IS_RUNNING, IS_EXIT} INIT_STATE;


#endif                      // Avoid multiple inclusion

/*************************** End of file ****************************/
