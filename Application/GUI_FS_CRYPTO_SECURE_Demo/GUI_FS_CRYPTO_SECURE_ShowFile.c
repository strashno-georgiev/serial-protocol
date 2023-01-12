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
File        : GUI_FS_CRYPTO_SECURE_ShowFile.c
Purpose     : Show BMP and JPEG files from SDCard and verifies with
              the files with the signature placed on the SDCard, too.
              Signature: Match     - The signature matches the file
                                     the public key.
              Signature: Mismatch  - The signature does not fit to the
                                     file and the public key.
              Signature: Not found - No signature found at all.

              To have get this demo application work properly, please
              make sure you have copied image files (BMP, JPEG) on a
              SDCard and plugged it into the board. You might want to
              use the ones which come with this demo application.
              The sample files can be found in the following :
              folder:
              .\Application\GUI\GUI_FS_CRYPTO_SECURE_Demo\SDCard\               

              The images are alphabetically ordered in the sample.
              If you use the supplied images, you should get the 
              following results:

                SEGGER_emPower.jpg     -> Match
                SEGGER_JLink0.jpg      -> Mismatch
                SEGGER_JLink1.jpg      -> Match
                SEGGER_Logo.bmp        -> Match
                SEGGER_Meeting.jpg     -> Match
                SEGGER_Production0.jpg -> Mismatch
                SEGGER_Production1.jpg -> Match
                SEGGER_Products0.bmp   -> Not found 
                SEGGER_Products1.bmp   -> Match
                SEGGER_Wall.bmp        -> Not found
                SEGGER_Warehouse0.bmp  -> Match
                SEGGER_Warehouse1.bmp  -> Match

              If you want to use your own images, you should consider
              the following:
              - The recommended size for the images is 160x113 pixels.
              - Sign them with the emSecure Sign tool and use
                SECURE_RSA_PrivateKey_Expert key.

Requirements: Make sure you have chosen a stack size big enough to run
              this application (5120 Bytes should work). emSecure alone
              requires 2,93KB for verification.

              Use a MMC configuration for emFile
              (FS_ConfigMMC_CardMode_xxx.c).
----------------------------------------------------------------------
*/

#include "RTOS.h"
#include "BSP.h"
#include "DIALOG.h"
#include "FS.h"
#include "SECURE_RSA.h"
#include "CRYPTO.h"
#include "SECURE_RSA_PublicKey_Expert.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/
#define ID_WINDOW_00       (GUI_ID_USER + 0x00)
#define ID_TEXT_00         (GUI_ID_USER + 0x01)
#define ID_BUTTON_00       (GUI_ID_USER + 0x02)
#define ID_BUTTON_01       (GUI_ID_USER + 0x03)
#define ID_BUTTON_02       (GUI_ID_USER + 0x04)

#define MESSAGE_NEXT       (WM_USER + 0x00)
#define MESSAGE_PREV       (WM_USER + 0x01)

#define EXT_BMP      1
#define EXT_JPEG     2
#define EXT_SIG     -1

#define SIG_NO       0
#define SIG_OK       1
#define SIG_INV      2

#define NO_VERIFICATION 3

#define HEADLINE_YSIZE  15

#define TRECT_X0 5
#define TRECT_Y0 0
#define TRECT_X1 240
#define TRECT_Y1 30

#define BUTTON_LR_SIZE       60
#define BUTTON_ONOFF_SIZE    30

#define COLOR_DARKBLUE    GUI_MAKE_COLOR(0x00623700)
#define COLOR_LIGHTGRAY   GUI_MAKE_COLOR(0x00E9E8E7)

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static OS_STACKPTR int _StackGUIVerify[2048];                  /* Task stack */
static OS_TASK         _TCBGUIVerify;                 /* Task-control-block */

/*********************************************************************
*
*       _acemSecure_113x113
*/
static GUI_CONST_STORAGE unsigned char _acemSecure_113x113[] = {
  /* RLE: 1183 Pixels @ 000,000 */ 254, 0xFF, 254, 0xFF, 254, 0xFF, 254, 0xFF, 167, 0xFF, 
  /* ABS: 006 Pixels @ 053,010 */ 0, 6, 0xF8, 0xF5, 0xF1, 0xF0, 0xF3, 0xF7, 
  /* RLE: 102 Pixels @ 059,010 */ 102, 0xFF, 
  /* ABS: 016 Pixels @ 048,011 */ 0, 16, 0xF8, 0xE0, 0xCB, 0xBC, 0xAD, 0x9C, 0x9A, 0x93, 0x93, 0x96, 0x9D, 0xA7, 0xB7, 0xC5, 0xD8, 0xEE, 
  /* RLE: 094 Pixels @ 064,011 */ 94, 0xFF, 
  /* ABS: 009 Pixels @ 045,012 */ 0, 9, 0xF3, 0xD2, 0xB6, 0x96, 0x77, 0x5B, 0x45, 0x36, 0x33, 
  /* RLE: 004 Pixels @ 054,012 */ 4, 0x35, 
  /* ABS: 009 Pixels @ 058,012 */ 0, 9, 0x34, 0x33, 0x40, 0x53, 0x6C, 0x8A, 0xAA, 0xC8, 0xE6, 
  /* RLE: 088 Pixels @ 067,012 */ 88, 0xFF, 
  /* ABS: 010 Pixels @ 042,013 */ 0, 10, 0xFD, 0xDF, 0xBD, 0x91, 0x59, 0x35, 0x2F, 0x37, 0x3C, 0x3F, 
  /* RLE: 008 Pixels @ 052,013 */ 8, 0x42, 
  /* ABS: 010 Pixels @ 060,013 */ 0, 10, 0x40, 0x3D, 0x38, 0x33, 0x2C, 0x47, 0x7B, 0xB0, 0xD4, 0xF5, 
  /* RLE: 084 Pixels @ 070,013 */ 84, 0xFF, 
  /* ABS: 008 Pixels @ 041,014 */ 0, 8, 0xE9, 0xBC, 0x71, 0x3D, 0x39, 0x3F, 0x42, 0x43, 
  /* RLE: 015 Pixels @ 049,014 */ 15, 0x42, 
  /* ABS: 008 Pixels @ 064,014 */ 0, 8, 0x43, 0x41, 0x3B, 0x3B, 0x5C, 0xA6, 0xDC, 0xFD, 
  /* RLE: 080 Pixels @ 072,014 */ 80, 0xFF, 
  /* ABS: 005 Pixels @ 039,015 */ 0, 5, 0xF4, 0xD7, 0x7B, 0x3B, 0x38, 
  /* RLE: 024 Pixels @ 044,015 */ 24, 0x42, 
  /* ABS: 005 Pixels @ 068,015 */ 0, 5, 0x3C, 0x37, 0x5D, 0xB8, 0xEB, 
  /* RLE: 077 Pixels @ 073,015 */ 77, 0xFF, 
  /* ABS: 007 Pixels @ 037,016 */ 0, 7, 0xFE, 0xE3, 0x9D, 0x4D, 0x3C, 0x40, 0x41, 
  /* RLE: 025 Pixels @ 044,016 */ 25, 0x42, 
  /* ABS: 006 Pixels @ 069,016 */ 0, 6, 0x41, 0x3C, 0x40, 0x7B, 0xD4, 0xFB, 
  /* RLE: 074 Pixels @ 075,016 */ 74, 0xFF, 
  /* ABS: 006 Pixels @ 036,017 */ 0, 6, 0xFC, 0xD3, 0x65, 0x3B, 0x41, 0x43, 
  /* RLE: 030 Pixels @ 042,017 */ 30, 0x42, 
  /* ABS: 004 Pixels @ 072,017 */ 0, 4, 0x3C, 0x4B, 0xB9, 0xF5, 
  /* RLE: 072 Pixels @ 076,017 */ 72, 0xFF, 
  /* ABS: 005 Pixels @ 035,018 */ 0, 5, 0xFA, 0xC7, 0x52, 0x40, 0x3F, 
  /* RLE: 032 Pixels @ 040,018 */ 32, 0x42, 
  /* ABS: 005 Pixels @ 072,018 */ 0, 5, 0x40, 0x3B, 0x45, 0x9E, 0xEF, 
  /* RLE: 070 Pixels @ 077,018 */ 70, 0xFF, 
  /* ABS: 004 Pixels @ 034,019 */ 0, 4, 0xFC, 0xBB, 0x46, 0x3F, 
  /* RLE: 036 Pixels @ 038,019 */ 36, 0x42, 
  /* ABS: 004 Pixels @ 074,019 */ 0, 4, 0x43, 0x42, 0x8A, 0xEC, 
  /* RLE: 068 Pixels @ 078,019 */ 68, 0xFF, 
  /* ABS: 005 Pixels @ 033,020 */ 0, 5, 0xFE, 0xC2, 0x4A, 0x3F, 0x41, 
  /* RLE: 037 Pixels @ 038,020 */ 37, 0x42, 
  /* ABS: 004 Pixels @ 075,020 */ 0, 4, 0x3D, 0x3F, 0x8C, 0xF1, 
  /* RLE: 066 Pixels @ 079,020 */ 66, 0xFF, 
  /* ABS: 004 Pixels @ 032,021 */ 0, 4, 0xFE, 0xCC, 0x4D, 0x3E, 
  /* RLE: 013 Pixels @ 036,021 */ 13, 0x42, 
  /* ABS: 004 Pixels @ 049,021 */ 0, 4, 0x41, 0x40, 0x3F, 0x3F, 
  /* RLE: 005 Pixels @ 053,021 */ 5, 0x3D, 
  /* ABS: 005 Pixels @ 058,021 */ 0, 5, 0x3E, 0x3E, 0x40, 0x40, 0x41, 
  /* RLE: 013 Pixels @ 063,021 */ 13, 0x42, 
  /* ABS: 004 Pixels @ 076,021 */ 0, 4, 0x40, 0x46, 0x9B, 0xF3, 
  /* RLE: 065 Pixels @ 080,021 */ 65, 0xFF, 
  /* ABS: 004 Pixels @ 032,022 */ 0, 4, 0xDB, 0x5D, 0x3C, 0x41, 
  /* RLE: 011 Pixels @ 036,022 */ 11, 0x42, 
  /* ABS: 018 Pixels @ 047,022 */ 0, 18, 0x43, 0x43, 0x3F, 0x39, 0x3C, 0x52, 0x6F, 0x86, 0x92, 0x95, 0x8B, 0x75, 0x60, 0x40, 0x38, 0x3D, 0x42, 0x43, 
  /* RLE: 012 Pixels @ 065,022 */ 12, 0x42, 
  /* ABS: 004 Pixels @ 077,022 */ 0, 4, 0x3F, 0x3E, 0xB6, 0xF9, 
  /* RLE: 063 Pixels @ 081,022 */ 63, 0xFF, 
  /* ABS: 004 Pixels @ 031,023 */ 0, 4, 0xE8, 0x7A, 0x3D, 0x41, 
  /* RLE: 011 Pixels @ 035,023 */ 11, 0x42, 
  /* ABS: 021 Pixels @ 046,023 */ 0, 21, 0x40, 0x39, 0x40, 0x6C, 0xA8, 0xCD, 0xDC, 0xE4, 0xEB, 0xEF, 0xF0, 0xED, 0xE6, 0xDF, 0xD3, 0xB9, 0x83, 0x4A, 0x39, 0x3F, 0x41, 
  /* RLE: 011 Pixels @ 067,023 */ 11, 0x42, 
  /* ABS: 003 Pixels @ 078,023 */ 0, 3, 0x3A, 0x55, 0xCF, 
  /* RLE: 062 Pixels @ 081,023 */ 62, 0xFF, 
  /* ABS: 004 Pixels @ 030,024 */ 0, 4, 0xF7, 0xA5, 0x3B, 0x41, 
  /* RLE: 010 Pixels @ 034,024 */ 10, 0x42, 
  /* ABS: 007 Pixels @ 044,024 */ 0, 7, 0x43, 0x37, 0x3F, 0x7C, 0xCA, 0xE6, 0xF8, 
  /* RLE: 010 Pixels @ 051,024 */ 10, 0xFF, 
  /* ABS: 006 Pixels @ 061,024 */ 0, 6, 0xFD, 0xED, 0xD8, 0x9C, 0x49, 0x35, 
  /* RLE: 012 Pixels @ 067,024 */ 12, 0x42, 
  /* ABS: 003 Pixels @ 079,024 */ 0, 3, 0x3D, 0x6B, 0xE6, 
  /* RLE: 061 Pixels @ 082,024 */ 61, 0xFF, 
  /* ABS: 003 Pixels @ 030,025 */ 0, 3, 0xC8, 0x4F, 0x39, 
  /* RLE: 010 Pixels @ 033,025 */ 10, 0x42, 
  /* ABS: 005 Pixels @ 043,025 */ 0, 5, 0x41, 0x38, 0x67, 0xB8, 0xE7, 
  /* RLE: 016 Pixels @ 048,025 */ 16, 0xFF, 
  /* ABS: 005 Pixels @ 064,025 */ 0, 5, 0xF6, 0xCA, 0x7F, 0x3F, 0x3E, 
  /* RLE: 010 Pixels @ 069,025 */ 10, 0x42, 
  /* ABS: 004 Pixels @ 079,025 */ 0, 4, 0x41, 0x35, 0xA3, 0xF6, 
  /* RLE: 059 Pixels @ 083,025 */ 59, 0xFF, 
  /* ABS: 003 Pixels @ 029,026 */ 0, 3, 0xED, 0x86, 0x37, 
  /* RLE: 010 Pixels @ 032,026 */ 10, 0x42, 
  /* ABS: 005 Pixels @ 042,026 */ 0, 5, 0x3F, 0x36, 0x81, 0xD6, 0xFD, 
  /* RLE: 019 Pixels @ 047,026 */ 19, 0xFF, 
  /* ABS: 005 Pixels @ 066,026 */ 0, 5, 0xE8, 0x9E, 0x4B, 0x37, 0x43, 
  /* RLE: 009 Pixels @ 071,026 */ 9, 0x42, 
  /* ABS: 003 Pixels @ 080,026 */ 0, 3, 0x3B, 0x56, 0xCF, 
  /* RLE: 059 Pixels @ 083,026 */ 59, 0xFF, 
  /* ABS: 003 Pixels @ 029,027 */ 0, 3, 0xBA, 0x44, 0x40, 
  /* RLE: 009 Pixels @ 032,027 */ 9, 0x42, 
  /* ABS: 004 Pixels @ 041,027 */ 0, 4, 0x41, 0x40, 0x8D, 0xEB, 
  /* RLE: 022 Pixels @ 045,027 */ 22, 0xFF, 
  /* ABS: 005 Pixels @ 067,027 */ 0, 5, 0xFB, 0xAF, 0x55, 0x39, 0x43, 
  /* RLE: 009 Pixels @ 072,027 */ 9, 0x42, 
  /* ABS: 003 Pixels @ 081,027 */ 0, 3, 0x33, 0x8F, 0xF3, 
  /* RLE: 057 Pixels @ 084,027 */ 57, 0xFF, 
  /* ABS: 004 Pixels @ 028,028 */ 0, 4, 0xEE, 0x85, 0x33, 0x43, 
  /* RLE: 008 Pixels @ 032,028 */ 8, 0x42, 
  /* ABS: 004 Pixels @ 040,028 */ 0, 4, 0x41, 0x39, 0x8A, 0xF1, 
  /* RLE: 024 Pixels @ 044,028 */ 24, 0xFF, 
  /* ABS: 005 Pixels @ 068,028 */ 0, 5, 0xF9, 0xB5, 0x4F, 0x3C, 0x43, 
  /* RLE: 008 Pixels @ 073,028 */ 8, 0x42, 
  /* ABS: 003 Pixels @ 081,028 */ 0, 3, 0x3D, 0x52, 0xCC, 
  /* RLE: 057 Pixels @ 084,028 */ 57, 0xFF, 
  /* ABS: 003 Pixels @ 028,029 */ 0, 3, 0xB9, 0x4A, 0x3E, 
  /* RLE: 009 Pixels @ 031,029 */ 9, 0x42, 
  /* ABS: 003 Pixels @ 040,029 */ 0, 3, 0x38, 0x7E, 0xEC, 
  /* RLE: 026 Pixels @ 043,029 */ 26, 0xFF, 
  /* ABS: 004 Pixels @ 069,029 */ 0, 4, 0xFC, 0xA7, 0x45, 0x40, 
  /* RLE: 008 Pixels @ 073,029 */ 8, 0x42, 
  /* ABS: 004 Pixels @ 081,029 */ 0, 4, 0x43, 0x34, 0x98, 0xF8, 
  /* RLE: 055 Pixels @ 085,029 */ 55, 0xFF, 
  /* ABS: 003 Pixels @ 027,030 */ 0, 3, 0xFA, 0x90, 0x30, 
  /* RLE: 009 Pixels @ 030,030 */ 9, 0x42, 
  /* ABS: 003 Pixels @ 039,030 */ 0, 3, 0x3F, 0x59, 0xE1, 
  /* RLE: 028 Pixels @ 042,030 */ 28, 0xFF, 
  /* ABS: 003 Pixels @ 070,030 */ 0, 3, 0xF3, 0x8C, 0x37, 
  /* RLE: 008 Pixels @ 073,030 */ 8, 0x42, 
  /* ABS: 004 Pixels @ 081,030 */ 0, 4, 0x43, 0x38, 0x66, 0xD7, 
  /* RLE: 055 Pixels @ 085,030 */ 55, 0xFF, 
  /* ABS: 003 Pixels @ 027,031 */ 0, 3, 0xD7, 0x6C, 0x38, 
  /* RLE: 008 Pixels @ 030,031 */ 8, 0x42, 
  /* ABS: 004 Pixels @ 038,031 */ 0, 4, 0x41, 0x43, 0xB4, 0xFB, 
  /* RLE: 029 Pixels @ 042,031 */ 29, 0xFF, 
  /* ABS: 003 Pixels @ 071,031 */ 0, 3, 0xE1, 0x55, 0x41, 
  /* RLE: 008 Pixels @ 074,031 */ 8, 0x42, 
  /* ABS: 003 Pixels @ 082,031 */ 0, 3, 0x40, 0x40, 0xAE, 
  /* RLE: 055 Pixels @ 085,031 */ 55, 0xFF, 
  /* ABS: 003 Pixels @ 027,032 */ 0, 3, 0xB0, 0x46, 0x40, 
  /* RLE: 007 Pixels @ 030,032 */ 7, 0x42, 
  /* ABS: 004 Pixels @ 037,032 */ 0, 4, 0x40, 0x3E, 0x76, 0xF1, 
  /* RLE: 030 Pixels @ 041,032 */ 30, 0xFF, 
  /* ABS: 004 Pixels @ 071,032 */ 0, 4, 0xFA, 0xAE, 0x40, 0x41, 
  /* RLE: 007 Pixels @ 075,032 */ 7, 0x42, 
  /* ABS: 004 Pixels @ 082,032 */ 0, 4, 0x43, 0x2F, 0x8A, 0xF8, 
  /* RLE: 054 Pixels @ 086,032 */ 54, 0xFF, 
  /* ABS: 003 Pixels @ 027,033 */ 0, 3, 0x95, 0x35, 0x43, 
  /* RLE: 007 Pixels @ 030,033 */ 7, 0x42, 
  /* ABS: 004 Pixels @ 037,033 */ 0, 4, 0x3F, 0x3E, 0xC2, 0xFA, 
  /* RLE: 031 Pixels @ 041,033 */ 31, 0xFF, 
  /* ABS: 004 Pixels @ 072,033 */ 0, 4, 0xE4, 0x59, 0x40, 0x41, 
  /* RLE: 007 Pixels @ 076,033 */ 7, 0x42, 
  /* ABS: 003 Pixels @ 083,033 */ 0, 3, 0x38, 0x70, 0xDA, 
  /* RLE: 053 Pixels @ 086,033 */ 53, 0xFF, 
  /* ABS: 004 Pixels @ 026,034 */ 0, 4, 0xEE, 0x79, 0x37, 0x43, 
  /* RLE: 007 Pixels @ 030,034 */ 7, 0x42, 
  /* ABS: 003 Pixels @ 037,034 */ 0, 3, 0x3F, 0x5B, 0xE9, 
  /* RLE: 032 Pixels @ 040,034 */ 32, 0xFF, 
  /* ABS: 004 Pixels @ 072,034 */ 0, 4, 0xF4, 0xA1, 0x3E, 0x3F, 
  /* RLE: 007 Pixels @ 076,034 */ 7, 0x42, 
  /* ABS: 003 Pixels @ 083,034 */ 0, 3, 0x3D, 0x53, 0xBF, 
  /* RLE: 053 Pixels @ 086,034 */ 53, 0xFF, 
  /* ABS: 004 Pixels @ 026,035 */ 0, 4, 0xD1, 0x64, 0x3B, 0x43, 
  /* RLE: 007 Pixels @ 030,035 */ 7, 0x42, 
  /* ABS: 003 Pixels @ 037,035 */ 0, 3, 0x45, 0x99, 0xF4, 
  /* RLE: 032 Pixels @ 040,035 */ 32, 0xFF, 
  /* ABS: 004 Pixels @ 072,035 */ 0, 4, 0xFE, 0xD5, 0x3F, 0x3E, 
  /* RLE: 007 Pixels @ 076,035 */ 7, 0x42, 
  /* ABS: 003 Pixels @ 083,035 */ 0, 3, 0x41, 0x40, 0xA9, 
  /* RLE: 053 Pixels @ 086,035 */ 53, 0xFF, 
  /* ABS: 003 Pixels @ 026,036 */ 0, 3, 0xBB, 0x53, 0x3D, 
  /* RLE: 007 Pixels @ 029,036 */ 7, 0x42, 
  /* ABS: 004 Pixels @ 036,036 */ 0, 4, 0x41, 0x49, 0xCD, 0xFE, 
  /* RLE: 033 Pixels @ 040,036 */ 33, 0xFF, 
  /* ABS: 003 Pixels @ 073,036 */ 0, 3, 0xE9, 0x5B, 0x40, 
  /* RLE: 007 Pixels @ 076,036 */ 7, 0x42, 
  /* ABS: 003 Pixels @ 083,036 */ 0, 3, 0x43, 0x34, 0x97, 
  /* RLE: 053 Pixels @ 086,036 */ 53, 0xFF, 
  /* ABS: 003 Pixels @ 026,037 */ 0, 3, 0xA9, 0x45, 0x3F, 
  /* RLE: 007 Pixels @ 029,037 */ 7, 0x42, 
  /* ABS: 003 Pixels @ 036,037 */ 0, 3, 0x41, 0x50, 0xF1, 
  /* RLE: 034 Pixels @ 039,037 */ 34, 0xFF, 
  /* ABS: 003 Pixels @ 073,037 */ 0, 3, 0xF0, 0x83, 0x43, 
  /* RLE: 007 Pixels @ 076,037 */ 7, 0x42, 
  /* ABS: 004 Pixels @ 083,037 */ 0, 4, 0x43, 0x36, 0x87, 0xFC, 
  /* RLE: 052 Pixels @ 087,037 */ 52, 0xFF, 
  /* ABS: 003 Pixels @ 026,038 */ 0, 3, 0x9C, 0x3B, 0x40, 
  /* RLE: 006 Pixels @ 029,038 */ 6, 0x42, 
  /* ABS: 004 Pixels @ 035,038 */ 0, 4, 0x41, 0x42, 0x63, 0xF8, 
  /* RLE: 034 Pixels @ 039,038 */ 34, 0xFF, 
  /* ABS: 003 Pixels @ 073,038 */ 0, 3, 0xF6, 0x9D, 0x45, 
  /* RLE: 007 Pixels @ 076,038 */ 7, 0x42, 
  /* ABS: 004 Pixels @ 083,038 */ 0, 4, 0x43, 0x37, 0x7D, 0xF0, 
  /* RLE: 052 Pixels @ 087,038 */ 52, 0xFF, 
  /* ABS: 003 Pixels @ 026,039 */ 0, 3, 0x95, 0x35, 0x41, 
  /* RLE: 006 Pixels @ 029,039 */ 6, 0x42, 
  /* ABS: 004 Pixels @ 035,039 */ 0, 4, 0x40, 0x43, 0x75, 0xF8, 
  /* RLE: 034 Pixels @ 039,039 */ 34, 0xFF, 
  /* ABS: 003 Pixels @ 073,039 */ 0, 3, 0xF9, 0xB2, 0x48, 
  /* RLE: 007 Pixels @ 076,039 */ 7, 0x42, 
  /* ABS: 004 Pixels @ 083,039 */ 0, 4, 0x43, 0x38, 0x76, 0xE7, 
  /* RLE: 052 Pixels @ 087,039 */ 52, 0xFF, 
  /* ABS: 002 Pixels @ 026,040 */ 0, 2, 0x8F, 0x31, 
  /* RLE: 007 Pixels @ 028,040 */ 7, 0x42, 
  /* ABS: 004 Pixels @ 035,040 */ 0, 4, 0x3F, 0x44, 0x7D, 0xF9, 
  /* RLE: 034 Pixels @ 039,040 */ 34, 0xFF, 
  /* ABS: 004 Pixels @ 073,040 */ 0, 4, 0xFB, 0xBD, 0x48, 0x41, 
  /* RLE: 006 Pixels @ 077,040 */ 6, 0x42, 
  /* ABS: 004 Pixels @ 083,040 */ 0, 4, 0x43, 0x39, 0x72, 0xE2, 
  /* RLE: 052 Pixels @ 087,040 */ 52, 0xFF, 
  /* ABS: 002 Pixels @ 026,041 */ 0, 2, 0x8C, 0x2E, 
  /* RLE: 007 Pixels @ 028,041 */ 7, 0x42, 
  /* ABS: 004 Pixels @ 035,041 */ 0, 4, 0x3F, 0x44, 0x81, 0xF9, 
  /* RLE: 034 Pixels @ 039,041 */ 34, 0xFF, 
  /* ABS: 004 Pixels @ 073,041 */ 0, 4, 0xFC, 0xC2, 0x48, 0x41, 
  /* RLE: 006 Pixels @ 077,041 */ 6, 0x42, 
  /* ABS: 004 Pixels @ 083,041 */ 0, 4, 0x43, 0x39, 0x6F, 0xDE, 
  /* RLE: 052 Pixels @ 087,041 */ 52, 0xFF, 
  /* ABS: 002 Pixels @ 026,042 */ 0, 2, 0x8C, 0x2E, 
  /* RLE: 007 Pixels @ 028,042 */ 7, 0x42, 
  /* ABS: 004 Pixels @ 035,042 */ 0, 4, 0x3F, 0x44, 0x81, 0xF9, 
  /* RLE: 034 Pixels @ 039,042 */ 34, 0xFF, 
  /* ABS: 004 Pixels @ 073,042 */ 0, 4, 0xFC, 0xC2, 0x48, 0x41, 
  /* RLE: 006 Pixels @ 077,042 */ 6, 0x42, 
  /* ABS: 004 Pixels @ 083,042 */ 0, 4, 0x43, 0x39, 0x6E, 0xDE, 
  /* RLE: 052 Pixels @ 087,042 */ 52, 0xFF, 
  /* ABS: 002 Pixels @ 026,043 */ 0, 2, 0x8C, 0x2E, 
  /* RLE: 007 Pixels @ 028,043 */ 7, 0x42, 
  /* ABS: 004 Pixels @ 035,043 */ 0, 4, 0x3F, 0x44, 0x81, 0xF9, 
  /* RLE: 034 Pixels @ 039,043 */ 34, 0xFF, 
  /* ABS: 004 Pixels @ 073,043 */ 0, 4, 0xFC, 0xC2, 0x48, 0x41, 
  /* RLE: 006 Pixels @ 077,043 */ 6, 0x42, 
  /* ABS: 004 Pixels @ 083,043 */ 0, 4, 0x43, 0x39, 0x6E, 0xDE, 
  /* RLE: 052 Pixels @ 087,043 */ 52, 0xFF, 
  /* ABS: 002 Pixels @ 026,044 */ 0, 2, 0x8C, 0x2E, 
  /* RLE: 007 Pixels @ 028,044 */ 7, 0x42, 
  /* ABS: 004 Pixels @ 035,044 */ 0, 4, 0x3F, 0x44, 0x81, 0xF9, 
  /* RLE: 034 Pixels @ 039,044 */ 34, 0xFF, 
  /* ABS: 004 Pixels @ 073,044 */ 0, 4, 0xFC, 0xC2, 0x48, 0x41, 
  /* RLE: 006 Pixels @ 077,044 */ 6, 0x42, 
  /* ABS: 004 Pixels @ 083,044 */ 0, 4, 0x43, 0x39, 0x6E, 0xDE, 
  /* RLE: 052 Pixels @ 087,044 */ 52, 0xFF, 
  /* ABS: 002 Pixels @ 026,045 */ 0, 2, 0x8C, 0x2E, 
  /* RLE: 007 Pixels @ 028,045 */ 7, 0x42, 
  /* ABS: 004 Pixels @ 035,045 */ 0, 4, 0x3F, 0x44, 0x81, 0xF9, 
  /* RLE: 034 Pixels @ 039,045 */ 34, 0xFF, 
  /* ABS: 004 Pixels @ 073,045 */ 0, 4, 0xFC, 0xC2, 0x48, 0x41, 
  /* RLE: 006 Pixels @ 077,045 */ 6, 0x42, 
  /* ABS: 004 Pixels @ 083,045 */ 0, 4, 0x43, 0x39, 0x6E, 0xDE, 
  /* RLE: 052 Pixels @ 087,045 */ 52, 0xFF, 
  /* ABS: 002 Pixels @ 026,046 */ 0, 2, 0x8C, 0x2E, 
  /* RLE: 007 Pixels @ 028,046 */ 7, 0x42, 
  /* ABS: 004 Pixels @ 035,046 */ 0, 4, 0x3F, 0x44, 0x81, 0xF9, 
  /* RLE: 034 Pixels @ 039,046 */ 34, 0xFF, 
  /* ABS: 004 Pixels @ 073,046 */ 0, 4, 0xFC, 0xC2, 0x48, 0x41, 
  /* RLE: 006 Pixels @ 077,046 */ 6, 0x42, 
  /* ABS: 004 Pixels @ 083,046 */ 0, 4, 0x43, 0x39, 0x6E, 0xDE, 
  /* RLE: 052 Pixels @ 087,046 */ 52, 0xFF, 
  /* ABS: 002 Pixels @ 026,047 */ 0, 2, 0x8C, 0x2E, 
  /* RLE: 007 Pixels @ 028,047 */ 7, 0x42, 
  /* ABS: 004 Pixels @ 035,047 */ 0, 4, 0x3F, 0x44, 0x81, 0xF9, 
  /* RLE: 034 Pixels @ 039,047 */ 34, 0xFF, 
  /* ABS: 004 Pixels @ 073,047 */ 0, 4, 0xFC, 0xC2, 0x48, 0x41, 
  /* RLE: 006 Pixels @ 077,047 */ 6, 0x42, 
  /* ABS: 004 Pixels @ 083,047 */ 0, 4, 0x43, 0x39, 0x6E, 0xDE, 
  /* RLE: 052 Pixels @ 087,047 */ 52, 0xFF, 
  /* ABS: 002 Pixels @ 026,048 */ 0, 2, 0x8C, 0x2E, 
  /* RLE: 007 Pixels @ 028,048 */ 7, 0x42, 
  /* ABS: 004 Pixels @ 035,048 */ 0, 4, 0x3F, 0x44, 0x81, 0xF9, 
  /* RLE: 034 Pixels @ 039,048 */ 34, 0xFF, 
  /* ABS: 004 Pixels @ 073,048 */ 0, 4, 0xFC, 0xC2, 0x48, 0x41, 
  /* RLE: 006 Pixels @ 077,048 */ 6, 0x42, 
  /* ABS: 004 Pixels @ 083,048 */ 0, 4, 0x43, 0x39, 0x6E, 0xDE, 
  /* RLE: 052 Pixels @ 087,048 */ 52, 0xFF, 
  /* ABS: 002 Pixels @ 026,049 */ 0, 2, 0x8C, 0x2E, 
  /* RLE: 007 Pixels @ 028,049 */ 7, 0x42, 
  /* ABS: 004 Pixels @ 035,049 */ 0, 4, 0x3F, 0x44, 0x81, 0xF9, 
  /* RLE: 034 Pixels @ 039,049 */ 34, 0xFF, 
  /* ABS: 004 Pixels @ 073,049 */ 0, 4, 0xFC, 0xC2, 0x48, 0x41, 
  /* RLE: 006 Pixels @ 077,049 */ 6, 0x42, 
  /* ABS: 004 Pixels @ 083,049 */ 0, 4, 0x43, 0x39, 0x6E, 0xDE, 
  /* RLE: 052 Pixels @ 087,049 */ 52, 0xFF, 
  /* ABS: 002 Pixels @ 026,050 */ 0, 2, 0x8C, 0x2E, 
  /* RLE: 007 Pixels @ 028,050 */ 7, 0x42, 
  /* ABS: 004 Pixels @ 035,050 */ 0, 4, 0x40, 0x44, 0x81, 0xF9, 
  /* RLE: 034 Pixels @ 039,050 */ 34, 0xFF, 
  /* ABS: 003 Pixels @ 073,050 */ 0, 3, 0xFC, 0xC2, 0x48, 
  /* RLE: 007 Pixels @ 076,050 */ 7, 0x42, 
  /* ABS: 004 Pixels @ 083,050 */ 0, 4, 0x43, 0x3A, 0x6F, 0xDE, 
  /* RLE: 052 Pixels @ 087,050 */ 52, 0xFF, 
  /* ABS: 002 Pixels @ 026,051 */ 0, 2, 0x88, 0x26, 
  /* RLE: 007 Pixels @ 028,051 */ 7, 0x3B, 
  /* ABS: 004 Pixels @ 035,051 */ 0, 4, 0x38, 0x3D, 0x7C, 0xF9, 
  /* RLE: 034 Pixels @ 039,051 */ 34, 0xFF, 
  /* ABS: 003 Pixels @ 073,051 */ 0, 3, 0xFC, 0xBF, 0x41, 
  /* RLE: 007 Pixels @ 076,051 */ 7, 0x3B, 
  /* ABS: 004 Pixels @ 083,051 */ 0, 4, 0x3C, 0x32, 0x69, 0xDC, 
  /* RLE: 052 Pixels @ 087,051 */ 52, 0xFF, 
  /* ABS: 002 Pixels @ 026,052 */ 0, 2, 0x97, 0x42, 
  /* RLE: 007 Pixels @ 028,052 */ 7, 0x55, 
  /* ABS: 004 Pixels @ 035,052 */ 0, 4, 0x52, 0x56, 0x8E, 0xFA, 
  /* RLE: 034 Pixels @ 039,052 */ 34, 0xFF, 
  /* ABS: 004 Pixels @ 073,052 */ 0, 4, 0xFC, 0xC8, 0x5A, 0x54, 
  /* RLE: 007 Pixels @ 077,052 */ 7, 0x55, 
  /* ABS: 003 Pixels @ 084,052 */ 0, 3, 0x4C, 0x7C, 0xE1, 
  /* RLE: 052 Pixels @ 087,052 */ 52, 0xFF, 
  /* ABS: 002 Pixels @ 026,053 */ 0, 2, 0xDE, 0xC4, 
  /* RLE: 007 Pixels @ 028,053 */ 7, 0xC9, 
  /* ABS: 004 Pixels @ 035,053 */ 0, 4, 0xC8, 0xCA, 0xDC, 0xFD, 
  /* RLE: 034 Pixels @ 039,053 */ 34, 0xFF, 
  /* ABS: 003 Pixels @ 073,053 */ 0, 3, 0xFE, 0xEC, 0xCB, 
  /* RLE: 008 Pixels @ 076,053 */ 8, 0xC9, 
  /* ABS: 003 Pixels @ 084,053 */ 0, 3, 0xC6, 0xD5, 0xF6, 
  /* RLE: 052 Pixels @ 087,053 */ 52, 0xFF, 
  /* RLE: 001 Pixels @ 026,054 */ 1, 0xFE, 
  /* RLE: 010 Pixels @ 027,054 */ 10, 0xFD, 
  /* RLE: 001 Pixels @ 037,054 */ 1, 0xFE, 
  /* RLE: 037 Pixels @ 038,054 */ 37, 0xFF, 
  /* RLE: 010 Pixels @ 075,054 */ 10, 0xFD, 
  /* RLE: 001 Pixels @ 085,054 */ 1, 0xFE, 
  /* RLE: 053 Pixels @ 086,054 */ 53, 0xFF, 
  /* RLE: 061 Pixels @ 026,055 */ 61, 0xFE, 
  /* RLE: 047 Pixels @ 087,055 */ 47, 0xFF, 
  /* ABS: 007 Pixels @ 021,056 */ 0, 7, 0xFE, 0xF3, 0xEB, 0xE5, 0xE2, 0xDF, 0xDC, 
  /* RLE: 057 Pixels @ 028,056 */ 57, 0xDD, 
  /* ABS: 006 Pixels @ 085,056 */ 0, 6, 0xDC, 0xE0, 0xE2, 0xE6, 0xED, 0xF6, 
  /* RLE: 043 Pixels @ 091,056 */ 43, 0xFF, 
  /* ABS: 005 Pixels @ 021,057 */ 0, 5, 0xD6, 0x8D, 0x5D, 0x48, 0x43, 
  /* RLE: 061 Pixels @ 026,057 */ 61, 0x45, 
  /* ABS: 005 Pixels @ 087,057 */ 0, 5, 0x42, 0x4A, 0x68, 0xA4, 0xF1, 
  /* RLE: 041 Pixels @ 092,057 */ 41, 0xFF, 
  /* ABS: 006 Pixels @ 020,058 */ 0, 6, 0xF1, 0x63, 0x3E, 0x3E, 0x3C, 0x3B, 
  /* RLE: 061 Pixels @ 026,058 */ 61, 0x3C, 
  /* ABS: 006 Pixels @ 087,058 */ 0, 6, 0x3B, 0x3C, 0x3B, 0x4B, 0x9E, 0xFE, 
  /* RLE: 040 Pixels @ 093,058 */ 40, 0xFF, 
  /* ABS: 004 Pixels @ 020,059 */ 0, 4, 0xEF, 0x58, 0x3B, 0x41, 
  /* RLE: 065 Pixels @ 024,059 */ 65, 0x42, 
  /* ABS: 004 Pixels @ 089,059 */ 0, 4, 0x3D, 0x45, 0x94, 0xFE, 
  /* RLE: 040 Pixels @ 093,059 */ 40, 0xFF, 
  /* ABS: 003 Pixels @ 020,060 */ 0, 3, 0xEF, 0x5A, 0x3D, 
  /* RLE: 066 Pixels @ 023,060 */ 66, 0x42, 
  /* ABS: 004 Pixels @ 089,060 */ 0, 4, 0x3E, 0x47, 0x96, 0xFE, 
  /* RLE: 040 Pixels @ 093,060 */ 40, 0xFF, 
  /* ABS: 003 Pixels @ 020,061 */ 0, 3, 0xEF, 0x5A, 0x3D, 
  /* RLE: 066 Pixels @ 023,061 */ 66, 0x42, 
  /* ABS: 004 Pixels @ 089,061 */ 0, 4, 0x3E, 0x47, 0x96, 0xFE, 
  /* RLE: 040 Pixels @ 093,061 */ 40, 0xFF, 
  /* ABS: 003 Pixels @ 020,062 */ 0, 3, 0xEF, 0x5A, 0x3D, 
  /* RLE: 066 Pixels @ 023,062 */ 66, 0x42, 
  /* ABS: 004 Pixels @ 089,062 */ 0, 4, 0x3E, 0x47, 0x96, 0xFE, 
  /* RLE: 040 Pixels @ 093,062 */ 40, 0xFF, 
  /* ABS: 003 Pixels @ 020,063 */ 0, 3, 0xEF, 0x5A, 0x3D, 
  /* RLE: 066 Pixels @ 023,063 */ 66, 0x42, 
  /* ABS: 004 Pixels @ 089,063 */ 0, 4, 0x3E, 0x47, 0x96, 0xFE, 
  /* RLE: 040 Pixels @ 093,063 */ 40, 0xFF, 
  /* ABS: 003 Pixels @ 020,064 */ 0, 3, 0xEF, 0x5A, 0x3D, 
  /* RLE: 066 Pixels @ 023,064 */ 66, 0x42, 
  /* ABS: 004 Pixels @ 089,064 */ 0, 4, 0x3E, 0x47, 0x96, 0xFE, 
  /* RLE: 040 Pixels @ 093,064 */ 40, 0xFF, 
  /* ABS: 003 Pixels @ 020,065 */ 0, 3, 0xEF, 0x5A, 0x3D, 
  /* RLE: 066 Pixels @ 023,065 */ 66, 0x42, 
  /* ABS: 004 Pixels @ 089,065 */ 0, 4, 0x3E, 0x47, 0x96, 0xFE, 
  /* RLE: 040 Pixels @ 093,065 */ 40, 0xFF, 
  /* ABS: 003 Pixels @ 020,066 */ 0, 3, 0xEF, 0x5A, 0x3D, 
  /* RLE: 066 Pixels @ 023,066 */ 66, 0x42, 
  /* ABS: 004 Pixels @ 089,066 */ 0, 4, 0x3E, 0x47, 0x96, 0xFE, 
  /* RLE: 040 Pixels @ 093,066 */ 40, 0xFF, 
  /* ABS: 003 Pixels @ 020,067 */ 0, 3, 0xEF, 0x5A, 0x3D, 
  /* RLE: 066 Pixels @ 023,067 */ 66, 0x42, 
  /* ABS: 004 Pixels @ 089,067 */ 0, 4, 0x3E, 0x47, 0x96, 0xFE, 
  /* RLE: 040 Pixels @ 093,067 */ 40, 0xFF, 
  /* ABS: 003 Pixels @ 020,068 */ 0, 3, 0xEF, 0x5A, 0x3D, 
  /* RLE: 066 Pixels @ 023,068 */ 66, 0x42, 
  /* ABS: 004 Pixels @ 089,068 */ 0, 4, 0x3E, 0x47, 0x96, 0xFE, 
  /* RLE: 040 Pixels @ 093,068 */ 40, 0xFF, 
  /* ABS: 007 Pixels @ 020,069 */ 0, 7, 0xEF, 0x5A, 0x3D, 0x3F, 0x3E, 0x3D, 0x3D, 
  /* RLE: 005 Pixels @ 027,069 */ 5, 0x3C, 
  /* RLE: 051 Pixels @ 032,069 */ 51, 0x3B, 
  /* RLE: 006 Pixels @ 083,069 */ 6, 0x3C, 
  /* ABS: 004 Pixels @ 089,069 */ 0, 4, 0x38, 0x44, 0x96, 0xFE, 
  /* RLE: 040 Pixels @ 093,069 */ 40, 0xFF, 
  /* ABS: 016 Pixels @ 020,070 */ 0, 16, 0xEF, 0x5A, 0x3B, 0x4F, 0x55, 0x57, 0x58, 0x5B, 0x5E, 0x5D, 0x5D, 0x5F, 0x62, 0x61, 0x61, 0x62, 
  /* RLE: 004 Pixels @ 036,070 */ 4, 0x66, 
  /* RLE: 001 Pixels @ 040,070 */ 1, 0x69, 
  /* RLE: 026 Pixels @ 041,070 */ 26, 0x6A, 
  /* ABS: 002 Pixels @ 067,070 */ 0, 2, 0x69, 0x68, 
  /* RLE: 006 Pixels @ 069,070 */ 6, 0x66, 
  /* RLE: 001 Pixels @ 075,070 */ 1, 0x65, 
  /* RLE: 007 Pixels @ 076,070 */ 7, 0x61, 
  /* RLE: 001 Pixels @ 083,070 */ 1, 0x5E, 
  /* RLE: 005 Pixels @ 084,070 */ 5, 0x5D, 
  /* ABS: 004 Pixels @ 089,070 */ 0, 4, 0x59, 0x58, 0x94, 0xFE, 
  /* RLE: 040 Pixels @ 093,070 */ 40, 0xFF, 
  /* ABS: 022 Pixels @ 020,071 */ 0, 22, 0xEF, 0x59, 0x35, 0x93, 0xB7, 0xBC, 0xC0, 0xC7, 0xCC, 0xCB, 0xCB, 0xCF, 0xD8, 0xD7, 0xD7, 0xD9, 0xE3, 0xE3, 0xE2, 0xE3, 0xEB, 0xED, 
  /* RLE: 026 Pixels @ 042,071 */ 26, 0xEC, 
  /* ABS: 002 Pixels @ 068,071 */ 0, 2, 0xE7, 0xE1, 
  /* RLE: 005 Pixels @ 070,071 */ 5, 0xE2, 
  /* ABS: 003 Pixels @ 075,071 */ 0, 3, 0xE0, 0xD7, 0xD6, 
  /* RLE: 004 Pixels @ 078,071 */ 4, 0xD7, 
  /* ABS: 002 Pixels @ 082,071 */ 0, 2, 0xD6, 0xCF, 
  /* RLE: 004 Pixels @ 084,071 */ 4, 0xCB, 
  /* ABS: 005 Pixels @ 088,071 */ 0, 5, 0xCC, 0xC6, 0x9D, 0x90, 0xFE, 
  /* RLE: 040 Pixels @ 093,071 */ 40, 0xFF, 
  /* ABS: 005 Pixels @ 020,072 */ 0, 5, 0xEF, 0x58, 0x2F, 0xC7, 0xFE, 
  /* RLE: 006 Pixels @ 025,072 */ 6, 0xFD, 
  /* RLE: 009 Pixels @ 031,072 */ 9, 0xFE, 
  /* RLE: 029 Pixels @ 040,072 */ 29, 0xFF, 
  /* RLE: 015 Pixels @ 069,072 */ 15, 0xFE, 
  /* RLE: 005 Pixels @ 084,072 */ 5, 0xFD, 
  /* ABS: 004 Pixels @ 089,072 */ 0, 4, 0xFF, 0xC2, 0x8D, 0xFE, 
  /* RLE: 040 Pixels @ 093,072 */ 40, 0xFF, 
  /* ABS: 016 Pixels @ 020,073 */ 0, 16, 0xEF, 0x58, 0x31, 0xB6, 0xF0, 0xEE, 0xED, 0xEB, 0xEA, 0xEA, 0xEB, 0xE9, 0xE8, 0xE8, 0xE8, 0xE7, 
  /* RLE: 004 Pixels @ 036,073 */ 4, 0xE5, 
  /* RLE: 028 Pixels @ 040,073 */ 28, 0xE3, 
  /* RLE: 001 Pixels @ 068,073 */ 1, 0xE4, 
  /* RLE: 006 Pixels @ 069,073 */ 6, 0xE5, 
  /* RLE: 001 Pixels @ 075,073 */ 1, 0xE6, 
  /* RLE: 007 Pixels @ 076,073 */ 7, 0xE8, 
  /* ABS: 002 Pixels @ 083,073 */ 0, 2, 0xE9, 0xEB, 
  /* RLE: 004 Pixels @ 085,073 */ 4, 0xEA, 
  /* ABS: 004 Pixels @ 089,073 */ 0, 4, 0xE8, 0xB2, 0x8E, 0xFE, 
  /* RLE: 040 Pixels @ 093,073 */ 40, 0xFF, 
  /* ABS: 021 Pixels @ 020,074 */ 0, 21, 0xEF, 0x5A, 0x39, 0x63, 0x77, 0x73, 0x6D, 0x67, 0x60, 0x60, 0x61, 0x5C, 0x54, 0x55, 0x55, 0x52, 0x46, 0x46, 0x47, 0x47, 0x3D, 
  /* RLE: 026 Pixels @ 041,074 */ 26, 0x3A, 
  /* ABS: 003 Pixels @ 067,074 */ 0, 3, 0x3C, 0x41, 0x48, 
  /* RLE: 005 Pixels @ 070,074 */ 5, 0x46, 
  /* ABS: 002 Pixels @ 075,074 */ 0, 2, 0x4B, 0x54, 
  /* RLE: 004 Pixels @ 077,074 */ 4, 0x55, 
  /* ABS: 004 Pixels @ 081,074 */ 0, 4, 0x54, 0x56, 0x5C, 0x61, 
  /* RLE: 004 Pixels @ 085,074 */ 4, 0x60, 
  /* ABS: 004 Pixels @ 089,074 */ 0, 4, 0x5B, 0x59, 0x95, 0xFD, 
  /* RLE: 040 Pixels @ 093,074 */ 40, 0xFF, 
  /* ABS: 012 Pixels @ 020,075 */ 0, 12, 0xEF, 0x5A, 0x3D, 0x41, 0x42, 0x41, 0x41, 0x41, 0x40, 0x40, 0x41, 0x40, 
  /* RLE: 008 Pixels @ 032,075 */ 8, 0x3F, 
  /* RLE: 029 Pixels @ 040,075 */ 29, 0x3E, 
  /* RLE: 014 Pixels @ 069,075 */ 14, 0x3F, 
  /* ABS: 002 Pixels @ 083,075 */ 0, 2, 0x40, 0x41, 
  /* RLE: 004 Pixels @ 085,075 */ 4, 0x40, 
  /* ABS: 004 Pixels @ 089,075 */ 0, 4, 0x3C, 0x46, 0x96, 0xFE, 
  /* RLE: 040 Pixels @ 093,075 */ 40, 0xFF, 
  /* ABS: 003 Pixels @ 020,076 */ 0, 3, 0xEF, 0x5A, 0x3D, 
  /* RLE: 066 Pixels @ 023,076 */ 66, 0x42, 
  /* ABS: 004 Pixels @ 089,076 */ 0, 4, 0x3E, 0x47, 0x96, 0xFE, 
  /* RLE: 040 Pixels @ 093,076 */ 40, 0xFF, 
  /* ABS: 004 Pixels @ 020,077 */ 0, 4, 0xEF, 0x5A, 0x3C, 0x48, 
  /* RLE: 063 Pixels @ 024,077 */ 63, 0x4A, 
  /* ABS: 006 Pixels @ 087,077 */ 0, 6, 0x4B, 0x49, 0x41, 0x47, 0x96, 0xFE, 
  /* RLE: 040 Pixels @ 093,077 */ 40, 0xFF, 
  /* ABS: 005 Pixels @ 020,078 */ 0, 5, 0xEF, 0x58, 0x31, 0xB7, 0xE8, 
  /* RLE: 062 Pixels @ 025,078 */ 62, 0xE7, 
  /* ABS: 006 Pixels @ 087,078 */ 0, 6, 0xE8, 0xD7, 0x7B, 0x41, 0x95, 0xFE, 
  /* RLE: 040 Pixels @ 093,078 */ 40, 0xFF, 
  /* ABS: 004 Pixels @ 020,079 */ 0, 4, 0xEF, 0x58, 0x2F, 0xCE, 
  /* RLE: 064 Pixels @ 024,079 */ 64, 0xFF, 
  /* ABS: 005 Pixels @ 088,079 */ 0, 5, 0xF5, 0x88, 0x40, 0x95, 0xFE, 
  /* RLE: 040 Pixels @ 093,079 */ 40, 0xFF, 
  /* ABS: 005 Pixels @ 020,080 */ 0, 5, 0xEF, 0x58, 0x2E, 0xCF, 0xFF, 
  /* RLE: 063 Pixels @ 025,080 */ 63, 0xFE, 
  /* ABS: 005 Pixels @ 088,080 */ 0, 5, 0xF7, 0x89, 0x40, 0x95, 0xFE, 
  /* RLE: 040 Pixels @ 093,080 */ 40, 0xFF, 
  /* ABS: 005 Pixels @ 020,081 */ 0, 5, 0xEF, 0x59, 0x36, 0x8D, 0xAC, 
  /* RLE: 062 Pixels @ 025,081 */ 62, 0xAA, 
  /* ABS: 006 Pixels @ 087,081 */ 0, 6, 0xAC, 0xA0, 0x65, 0x44, 0x95, 0xFE, 
  /* RLE: 040 Pixels @ 093,081 */ 40, 0xFF, 
  /* ABS: 005 Pixels @ 020,082 */ 0, 5, 0xEF, 0x5A, 0x3C, 0x4B, 0x50, 
  /* RLE: 062 Pixels @ 025,082 */ 62, 0x4E, 
  /* ABS: 006 Pixels @ 087,082 */ 0, 6, 0x50, 0x4D, 0x42, 0x47, 0x96, 0xFE, 
  /* RLE: 040 Pixels @ 093,082 */ 40, 0xFF, 
  /* ABS: 003 Pixels @ 020,083 */ 0, 3, 0xEF, 0x5A, 0x3D, 
  /* RLE: 010 Pixels @ 023,083 */ 10, 0x3F, 
  /* RLE: 050 Pixels @ 033,083 */ 50, 0x3E, 
  /* RLE: 004 Pixels @ 083,083 */ 4, 0x3F, 
  /* ABS: 006 Pixels @ 087,083 */ 0, 6, 0x3E, 0x3F, 0x3D, 0x48, 0x96, 0xFE, 
  /* RLE: 040 Pixels @ 093,083 */ 40, 0xFF, 
  /* ABS: 013 Pixels @ 020,084 */ 0, 13, 0xEF, 0x5A, 0x3D, 0x3C, 0x39, 0x39, 0x39, 0x3A, 0x3A, 0x3A, 0x3B, 0x3B, 0x3B, 
  /* RLE: 004 Pixels @ 033,084 */ 4, 0x3C, 
  /* RLE: 003 Pixels @ 037,084 */ 3, 0x3D, 
  /* RLE: 028 Pixels @ 040,084 */ 28, 0x3E, 
  /* RLE: 008 Pixels @ 068,084 */ 8, 0x3D, 
  /* RLE: 007 Pixels @ 076,084 */ 7, 0x3C, 
  /* RLE: 006 Pixels @ 083,084 */ 6, 0x3B, 
  /* ABS: 004 Pixels @ 089,084 */ 0, 4, 0x37, 0x43, 0x97, 0xFE, 
  /* RLE: 040 Pixels @ 093,084 */ 40, 0xFF, 
  /* ABS: 021 Pixels @ 020,085 */ 0, 21, 0xEF, 0x5A, 0x3A, 0x5E, 0x6E, 0x71, 0x6C, 0x69, 0x68, 0x67, 0x63, 0x61, 0x61, 0x5D, 0x59, 0x59, 0x57, 0x52, 0x52, 0x52, 0x4C, 
  /* RLE: 027 Pixels @ 041,085 */ 27, 0x4B, 
  /* ABS: 002 Pixels @ 068,085 */ 0, 2, 0x4F, 0x53, 
  /* RLE: 005 Pixels @ 070,085 */ 5, 0x52, 
  /* RLE: 001 Pixels @ 075,085 */ 1, 0x54, 
  /* RLE: 006 Pixels @ 076,085 */ 6, 0x59, 
  /* ABS: 011 Pixels @ 082,085 */ 0, 11, 0x5A, 0x5F, 0x62, 0x61, 0x61, 0x61, 0x62, 0x5D, 0x59, 0x95, 0xFE, 
  /* RLE: 040 Pixels @ 093,085 */ 40, 0xFF, 
  /* ABS: 021 Pixels @ 020,086 */ 0, 21, 0xEF, 0x59, 0x33, 0xA7, 0xDE, 0xE0, 0xDC, 0xD7, 0xD6, 0xD4, 0xCF, 0xCD, 0xCC, 0xC7, 0xC2, 0xC3, 0xC0, 0xB9, 0xB9, 0xBA, 0xB2, 
  /* RLE: 026 Pixels @ 041,086 */ 26, 0xB0, 
  /* ABS: 003 Pixels @ 067,086 */ 0, 3, 0xB1, 0xB6, 0xBA, 
  /* RLE: 005 Pixels @ 070,086 */ 5, 0xB9, 
  /* RLE: 001 Pixels @ 075,086 */ 1, 0xBC, 
  /* RLE: 007 Pixels @ 076,086 */ 7, 0xC3, 
  /* ABS: 002 Pixels @ 083,086 */ 0, 2, 0xCA, 0xCE, 
  /* RLE: 004 Pixels @ 085,086 */ 4, 0xCD, 
  /* ABS: 004 Pixels @ 089,086 */ 0, 4, 0xC6, 0x9B, 0x90, 0xFE, 
  /* RLE: 040 Pixels @ 093,086 */ 40, 0xFF, 
  /* ABS: 004 Pixels @ 020,087 */ 0, 4, 0xEF, 0x58, 0x2F, 0xCD, 
  /* RLE: 066 Pixels @ 024,087 */ 66, 0xFF, 
  /* ABS: 002 Pixels @ 090,087 */ 0, 2, 0xC8, 0x8D, 
  /* RLE: 041 Pixels @ 092,087 */ 41, 0xFF, 
  /* ABS: 012 Pixels @ 020,088 */ 0, 12, 0xEF, 0x58, 0x32, 0xA7, 0xD4, 0xD5, 0xD6, 0xD9, 0xDA, 0xDC, 0xDF, 0xE0, 
  /* RLE: 008 Pixels @ 032,088 */ 8, 0xE3, 
  /* RLE: 001 Pixels @ 040,088 */ 1, 0xE7, 
  /* RLE: 007 Pixels @ 041,088 */ 7, 0xE8, 
  /* RLE: 001 Pixels @ 048,088 */ 1, 0xE9, 
  /* RLE: 023 Pixels @ 049,088 */ 23, 0xEB, 
  /* RLE: 003 Pixels @ 072,088 */ 3, 0xE8, 
  /* ABS: 002 Pixels @ 075,088 */ 0, 2, 0xE7, 0xE5, 
  /* RLE: 004 Pixels @ 077,088 */ 4, 0xE3, 
  /* ABS: 012 Pixels @ 081,088 */ 0, 12, 0xE0, 0xDE, 0xDF, 0xDF, 0xDE, 0xDB, 0xDA, 0xDB, 0xD4, 0xA5, 0x90, 0xFE, 
  /* RLE: 040 Pixels @ 093,088 */ 40, 0xFF, 
  /* ABS: 013 Pixels @ 020,089 */ 0, 13, 0xEF, 0x5A, 0x3C, 0x48, 0x49, 0x50, 0x52, 0x59, 0x5D, 0x61, 0x67, 0x6B, 0x72, 
  /* RLE: 006 Pixels @ 033,089 */ 6, 0x71, 
  /* ABS: 003 Pixels @ 039,089 */ 0, 3, 0x73, 0x7B, 0x7D, 
  /* RLE: 005 Pixels @ 042,089 */ 5, 0x7C, 
  /* ABS: 003 Pixels @ 047,089 */ 0, 3, 0x7D, 0x82, 0x87, 
  /* RLE: 020 Pixels @ 050,089 */ 20, 0x86, 
  /* ABS: 023 Pixels @ 070,089 */ 0, 23, 0x87, 0x84, 0x7D, 0x7C, 0x7C, 0x7D, 0x77, 0x72, 0x71, 0x71, 0x72, 0x6A, 0x66, 0x67, 0x68, 0x64, 0x5E, 0x5D, 0x5D, 0x58, 0x59, 0x95, 0xFD, 
  /* RLE: 040 Pixels @ 093,089 */ 40, 0xFF, 
  /* ABS: 012 Pixels @ 020,090 */ 0, 12, 0xEF, 0x5A, 0x3D, 0x41, 0x41, 0x40, 0x3F, 0x3F, 0x3F, 0x3E, 0x3D, 0x3D, 
  /* RLE: 008 Pixels @ 032,090 */ 8, 0x3C, 
  /* RLE: 008 Pixels @ 040,090 */ 8, 0x3B, 
  /* RLE: 024 Pixels @ 048,090 */ 24, 0x3A, 
  /* RLE: 003 Pixels @ 072,090 */ 3, 0x3B, 
  /* RLE: 001 Pixels @ 075,090 */ 1, 0x3A, 
  /* RLE: 005 Pixels @ 076,090 */ 5, 0x3C, 
  /* RLE: 004 Pixels @ 081,090 */ 4, 0x3D, 
  /* ABS: 008 Pixels @ 085,090 */ 0, 8, 0x3E, 0x3F, 0x3F, 0x3F, 0x3A, 0x45, 0x96, 0xFE, 
  /* RLE: 040 Pixels @ 093,090 */ 40, 0xFF, 
  /* ABS: 003 Pixels @ 020,091 */ 0, 3, 0xEF, 0x5A, 0x3D, 
  /* RLE: 005 Pixels @ 023,091 */ 5, 0x42, 
  /* RLE: 060 Pixels @ 028,091 */ 60, 0x41, 
  /* ABS: 005 Pixels @ 088,091 */ 0, 5, 0x42, 0x3D, 0x47, 0x96, 0xFE, 
  /* RLE: 040 Pixels @ 093,091 */ 40, 0xFF, 
  /* ABS: 003 Pixels @ 020,092 */ 0, 3, 0xEF, 0x5A, 0x3D, 
  /* RLE: 066 Pixels @ 023,092 */ 66, 0x42, 
  /* ABS: 004 Pixels @ 089,092 */ 0, 4, 0x3E, 0x47, 0x96, 0xFE, 
  /* RLE: 040 Pixels @ 093,092 */ 40, 0xFF, 
  /* ABS: 003 Pixels @ 020,093 */ 0, 3, 0xEF, 0x5A, 0x3D, 
  /* RLE: 066 Pixels @ 023,093 */ 66, 0x42, 
  /* ABS: 004 Pixels @ 089,093 */ 0, 4, 0x3E, 0x47, 0x96, 0xFE, 
  /* RLE: 040 Pixels @ 093,093 */ 40, 0xFF, 
  /* ABS: 003 Pixels @ 020,094 */ 0, 3, 0xEF, 0x5A, 0x3D, 
  /* RLE: 066 Pixels @ 023,094 */ 66, 0x42, 
  /* ABS: 004 Pixels @ 089,094 */ 0, 4, 0x3E, 0x47, 0x96, 0xFE, 
  /* RLE: 040 Pixels @ 093,094 */ 40, 0xFF, 
  /* ABS: 003 Pixels @ 020,095 */ 0, 3, 0xEF, 0x5A, 0x3D, 
  /* RLE: 066 Pixels @ 023,095 */ 66, 0x42, 
  /* ABS: 004 Pixels @ 089,095 */ 0, 4, 0x3E, 0x47, 0x96, 0xFE, 
  /* RLE: 040 Pixels @ 093,095 */ 40, 0xFF, 
  /* ABS: 003 Pixels @ 020,096 */ 0, 3, 0xEF, 0x5A, 0x3D, 
  /* RLE: 066 Pixels @ 023,096 */ 66, 0x42, 
  /* ABS: 004 Pixels @ 089,096 */ 0, 4, 0x3E, 0x47, 0x96, 0xFE, 
  /* RLE: 040 Pixels @ 093,096 */ 40, 0xFF, 
  /* ABS: 003 Pixels @ 020,097 */ 0, 3, 0xEF, 0x5A, 0x3D, 
  /* RLE: 066 Pixels @ 023,097 */ 66, 0x42, 
  /* ABS: 004 Pixels @ 089,097 */ 0, 4, 0x3E, 0x47, 0x96, 0xFE, 
  /* RLE: 040 Pixels @ 093,097 */ 40, 0xFF, 
  /* ABS: 003 Pixels @ 020,098 */ 0, 3, 0xEE, 0x58, 0x3C, 
  /* RLE: 066 Pixels @ 023,098 */ 66, 0x42, 
  /* ABS: 004 Pixels @ 089,098 */ 0, 4, 0x3E, 0x47, 0x93, 0xFE, 
  /* RLE: 040 Pixels @ 093,098 */ 40, 0xFF, 
  /* ABS: 004 Pixels @ 020,099 */ 0, 4, 0xF6, 0x70, 0x30, 0x2C, 
  /* RLE: 065 Pixels @ 024,099 */ 65, 0x31, 
  /* ABS: 004 Pixels @ 089,099 */ 0, 4, 0x2A, 0x44, 0xAE, 0xFE, 
  /* RLE: 040 Pixels @ 093,099 */ 40, 0xFF, 
  /* ABS: 006 Pixels @ 020,100 */ 0, 6, 0xFE, 0xDE, 0xC3, 0xB5, 0xAA, 0xA5, 
  /* RLE: 062 Pixels @ 026,100 */ 62, 0xA6, 
  /* ABS: 005 Pixels @ 088,100 */ 0, 5, 0xAB, 0xB8, 0xCD, 0xEF, 0xFE, 
  /* RLE: 044 Pixels @ 093,100 */ 44, 0xFF, 
  /* RLE: 001 Pixels @ 024,101 */ 1, 0xFD, 
  /* RLE: 063 Pixels @ 025,101 */ 63, 0xFB, 
  /* RLE: 001 Pixels @ 088,101 */ 1, 0xFD, 
  /* RLE: 1040 Pixels @ 089,101 */ 254, 0xFF, 254, 0xFF, 254, 0xFF, 254, 0xFF, 24, 0xFF, 
  /* ABS: 002 Pixels @ 112,110 */ 0, 2, 0xFC, 0xFE, 
  /* RLE: 111 Pixels @ 001,111 */ 111, 0xFF, 
  /* ABS: 003 Pixels @ 112,111 */ 0, 3, 0xFE, 0xFF, 0xFE, 
  /* RLE: 108 Pixels @ 002,112 */ 108, 0xFF, 
  /* ABS: 003 Pixels @ 110,112 */ 0, 3, 0xFE, 0xFD, 0xFF, 
  0
};  // 2384 bytes for 12769 pixels

GUI_CONST_STORAGE GUI_BITMAP bmemSecure_113x113 = {
  113, // xSize
  113, // ySize
  113, // BytesPerLine
  GUI_COMPRESS_RLE8, // BitsPerPixel
  (unsigned char *)_acemSecure_113x113,  // Pointer to picture data
  NULL,  // Pointer to palette
  GUI_DRAW_RLEALPHA
};

static GUI_CONST_STORAGE unsigned char _acArrowLeft_30x30[] = {
  /* RLE: 107 Pixels @ 000,000 */ 107, 0xFF, 
  /* ABS: 004 Pixels @ 017,003 */ 0, 4, 0xDA, 0x39, 0x21, 0xB6, 
  /* RLE: 025 Pixels @ 021,003 */ 25, 0xFF, 
  /* ABS: 006 Pixels @ 016,004 */ 0, 6, 0xCE, 0x25, 0x00, 0x00, 0x0A, 0xA7, 
  /* RLE: 023 Pixels @ 022,004 */ 23, 0xFF, 
  /* ABS: 002 Pixels @ 015,005 */ 0, 2, 0xBF, 0x17, 
  /* RLE: 004 Pixels @ 017,005 */ 4, 0x00, 
  /* ABS: 002 Pixels @ 021,005 */ 0, 2, 0x05, 0x93, 
  /* RLE: 021 Pixels @ 023,005 */ 21, 0xFF, 
  /* ABS: 002 Pixels @ 014,006 */ 0, 2, 0xAE, 0x10, 
  /* RLE: 007 Pixels @ 016,006 */ 7, 0x00, 
  /* RLE: 001 Pixels @ 023,006 */ 1, 0x86, 
  /* RLE: 019 Pixels @ 024,006 */ 19, 0xFF, 
  /* ABS: 002 Pixels @ 013,007 */ 0, 2, 0x9C, 0x07, 
  /* RLE: 008 Pixels @ 015,007 */ 8, 0x00, 
  /* RLE: 001 Pixels @ 023,007 */ 1, 0x2A, 
  /* RLE: 018 Pixels @ 024,007 */ 18, 0xFF, 
  /* RLE: 001 Pixels @ 012,008 */ 1, 0x87, 
  /* RLE: 009 Pixels @ 013,008 */ 9, 0x00, 
  /* ABS: 002 Pixels @ 022,008 */ 0, 2, 0x0F, 0xB4, 
  /* RLE: 017 Pixels @ 024,008 */ 17, 0xFF, 
  /* RLE: 001 Pixels @ 011,009 */ 1, 0x7E, 
  /* RLE: 009 Pixels @ 012,009 */ 9, 0x00, 
  /* ABS: 002 Pixels @ 021,009 */ 0, 2, 0x19, 0xBD, 
  /* RLE: 016 Pixels @ 023,009 */ 16, 0xFF, 
  /* ABS: 002 Pixels @ 009,010 */ 0, 2, 0xFA, 0x69, 
  /* RLE: 009 Pixels @ 011,010 */ 9, 0x00, 
  /* ABS: 002 Pixels @ 020,010 */ 0, 2, 0x20, 0xCC, 
  /* RLE: 016 Pixels @ 022,010 */ 16, 0xFF, 
  /* ABS: 002 Pixels @ 008,011 */ 0, 2, 0xF2, 0x54, 
  /* RLE: 009 Pixels @ 010,011 */ 9, 0x00, 
  /* ABS: 002 Pixels @ 019,011 */ 0, 2, 0x30, 0xDA, 
  /* RLE: 016 Pixels @ 021,011 */ 16, 0xFF, 
  /* ABS: 002 Pixels @ 007,012 */ 0, 2, 0xEA, 0x46, 
  /* RLE: 009 Pixels @ 009,012 */ 9, 0x00, 
  /* ABS: 002 Pixels @ 018,012 */ 0, 2, 0x40, 0xE5, 
  /* RLE: 016 Pixels @ 020,012 */ 16, 0xFF, 
  /* ABS: 002 Pixels @ 006,013 */ 0, 2, 0xDF, 0x34, 
  /* RLE: 009 Pixels @ 008,013 */ 9, 0x00, 
  /* ABS: 002 Pixels @ 017,013 */ 0, 2, 0x4F, 0xF2, 
  /* RLE: 017 Pixels @ 019,013 */ 17, 0xFF, 
  /* RLE: 001 Pixels @ 006,014 */ 1, 0x3A, 
  /* RLE: 009 Pixels @ 007,014 */ 9, 0x00, 
  /* ABS: 002 Pixels @ 016,014 */ 0, 2, 0x62, 0xFC, 
  /* RLE: 018 Pixels @ 018,014 */ 18, 0xFF, 
  /* RLE: 001 Pixels @ 006,015 */ 1, 0x40, 
  /* RLE: 009 Pixels @ 007,015 */ 9, 0x00, 
  /* ABS: 002 Pixels @ 016,015 */ 0, 2, 0x5B, 0xF6, 
  /* RLE: 018 Pixels @ 018,015 */ 18, 0xFF, 
  /* ABS: 002 Pixels @ 006,016 */ 0, 2, 0xE6, 0x3D, 
  /* RLE: 009 Pixels @ 008,016 */ 9, 0x00, 
  /* ABS: 002 Pixels @ 017,016 */ 0, 2, 0x45, 0xEC, 
  /* RLE: 018 Pixels @ 019,016 */ 18, 0xFF, 
  /* ABS: 002 Pixels @ 007,017 */ 0, 2, 0xF0, 0x51, 
  /* RLE: 009 Pixels @ 009,017 */ 9, 0x00, 
  /* ABS: 002 Pixels @ 018,017 */ 0, 2, 0x36, 0xDE, 
  /* RLE: 018 Pixels @ 020,017 */ 18, 0xFF, 
  /* ABS: 002 Pixels @ 008,018 */ 0, 2, 0xF7, 0x5F, 
  /* RLE: 009 Pixels @ 010,018 */ 9, 0x00, 
  /* ABS: 002 Pixels @ 019,018 */ 0, 2, 0x27, 0xD1, 
  /* RLE: 018 Pixels @ 021,018 */ 18, 0xFF, 
  /* ABS: 002 Pixels @ 009,019 */ 0, 2, 0xFE, 0x74, 
  /* RLE: 009 Pixels @ 011,019 */ 9, 0x00, 
  /* ABS: 002 Pixels @ 020,019 */ 0, 2, 0x19, 0xC2, 
  /* RLE: 019 Pixels @ 022,019 */ 19, 0xFF, 
  /* ABS: 002 Pixels @ 011,020 */ 0, 2, 0x8A, 0x01, 
  /* RLE: 008 Pixels @ 013,020 */ 8, 0x00, 
  /* ABS: 002 Pixels @ 021,020 */ 0, 2, 0x12, 0xB2, 
  /* RLE: 019 Pixels @ 023,020 */ 19, 0xFF, 
  /* ABS: 002 Pixels @ 012,021 */ 0, 2, 0x93, 0x04, 
  /* RLE: 008 Pixels @ 014,021 */ 8, 0x00, 
  /* ABS: 002 Pixels @ 022,021 */ 0, 2, 0x09, 0xA9, 
  /* RLE: 019 Pixels @ 024,021 */ 19, 0xFF, 
  /* ABS: 002 Pixels @ 013,022 */ 0, 2, 0xA8, 0x0C, 
  /* RLE: 008 Pixels @ 015,022 */ 8, 0x00, 
  /* RLE: 001 Pixels @ 023,022 */ 1, 0x29, 
  /* RLE: 020 Pixels @ 024,022 */ 20, 0xFF, 
  /* ABS: 002 Pixels @ 014,023 */ 0, 2, 0xB9, 0x16, 
  /* RLE: 006 Pixels @ 016,023 */ 6, 0x00, 
  /* ABS: 002 Pixels @ 022,023 */ 0, 2, 0x02, 0x92, 
  /* RLE: 021 Pixels @ 024,023 */ 21, 0xFF, 
  /* ABS: 002 Pixels @ 015,024 */ 0, 2, 0xC9, 0x1E, 
  /* RLE: 004 Pixels @ 017,024 */ 4, 0x00, 
  /* ABS: 002 Pixels @ 021,024 */ 0, 2, 0x09, 0x9F, 
  /* RLE: 023 Pixels @ 023,024 */ 23, 0xFF, 
  /* ABS: 006 Pixels @ 016,025 */ 0, 6, 0xD7, 0x2E, 0x00, 0x00, 0x0F, 0xB2, 
  /* RLE: 025 Pixels @ 022,025 */ 25, 0xFF, 
  /* ABS: 004 Pixels @ 017,026 */ 0, 4, 0xE1, 0x40, 0x24, 0xBF, 
  /* RLE: 099 Pixels @ 021,026 */ 99, 0xFF, 
  0
};  // 264 bytes for 900 pixels

GUI_CONST_STORAGE GUI_BITMAP bmArrowLeft_30x30 = {
  30, // xSize
  30, // ySize
  30, // BytesPerLine
  GUI_COMPRESS_RLE8, // BitsPerPixel
  (unsigned char *)_acArrowLeft_30x30,  // Pointer to picture data
  NULL,  // Pointer to palette
  GUI_DRAW_RLEALPHA
};

static GUI_CONST_STORAGE unsigned char _acArrowRight_30x30[] = {
  /* RLE: 099 Pixels @ 000,000 */ 99, 0xFF, 
  /* ABS: 004 Pixels @ 009,003 */ 0, 4, 0xBF, 0x24, 0x40, 0xE1, 
  /* RLE: 025 Pixels @ 013,003 */ 25, 0xFF, 
  /* ABS: 006 Pixels @ 008,004 */ 0, 6, 0xB2, 0x0F, 0x00, 0x00, 0x2E, 0xD7, 
  /* RLE: 023 Pixels @ 014,004 */ 23, 0xFF, 
  /* ABS: 002 Pixels @ 007,005 */ 0, 2, 0x9F, 0x09, 
  /* RLE: 004 Pixels @ 009,005 */ 4, 0x00, 
  /* ABS: 002 Pixels @ 013,005 */ 0, 2, 0x1E, 0xC9, 
  /* RLE: 021 Pixels @ 015,005 */ 21, 0xFF, 
  /* ABS: 002 Pixels @ 006,006 */ 0, 2, 0x92, 0x02, 
  /* RLE: 006 Pixels @ 008,006 */ 6, 0x00, 
  /* ABS: 002 Pixels @ 014,006 */ 0, 2, 0x16, 0xB9, 
  /* RLE: 020 Pixels @ 016,006 */ 20, 0xFF, 
  /* RLE: 001 Pixels @ 006,007 */ 1, 0x29, 
  /* RLE: 008 Pixels @ 007,007 */ 8, 0x00, 
  /* ABS: 002 Pixels @ 015,007 */ 0, 2, 0x0C, 0xA8, 
  /* RLE: 019 Pixels @ 017,007 */ 19, 0xFF, 
  /* ABS: 002 Pixels @ 006,008 */ 0, 2, 0xA9, 0x09, 
  /* RLE: 008 Pixels @ 008,008 */ 8, 0x00, 
  /* ABS: 002 Pixels @ 016,008 */ 0, 2, 0x04, 0x93, 
  /* RLE: 019 Pixels @ 018,008 */ 19, 0xFF, 
  /* ABS: 002 Pixels @ 007,009 */ 0, 2, 0xB2, 0x12, 
  /* RLE: 008 Pixels @ 009,009 */ 8, 0x00, 
  /* ABS: 002 Pixels @ 017,009 */ 0, 2, 0x01, 0x8A, 
  /* RLE: 019 Pixels @ 019,009 */ 19, 0xFF, 
  /* ABS: 002 Pixels @ 008,010 */ 0, 2, 0xC2, 0x19, 
  /* RLE: 009 Pixels @ 010,010 */ 9, 0x00, 
  /* ABS: 002 Pixels @ 019,010 */ 0, 2, 0x74, 0xFE, 
  /* RLE: 018 Pixels @ 021,010 */ 18, 0xFF, 
  /* ABS: 002 Pixels @ 009,011 */ 0, 2, 0xD1, 0x27, 
  /* RLE: 009 Pixels @ 011,011 */ 9, 0x00, 
  /* ABS: 002 Pixels @ 020,011 */ 0, 2, 0x5F, 0xF7, 
  /* RLE: 018 Pixels @ 022,011 */ 18, 0xFF, 
  /* ABS: 002 Pixels @ 010,012 */ 0, 2, 0xDE, 0x36, 
  /* RLE: 009 Pixels @ 012,012 */ 9, 0x00, 
  /* ABS: 002 Pixels @ 021,012 */ 0, 2, 0x51, 0xF0, 
  /* RLE: 018 Pixels @ 023,012 */ 18, 0xFF, 
  /* ABS: 002 Pixels @ 011,013 */ 0, 2, 0xEC, 0x45, 
  /* RLE: 009 Pixels @ 013,013 */ 9, 0x00, 
  /* ABS: 002 Pixels @ 022,013 */ 0, 2, 0x3D, 0xE6, 
  /* RLE: 018 Pixels @ 024,013 */ 18, 0xFF, 
  /* ABS: 002 Pixels @ 012,014 */ 0, 2, 0xF6, 0x5B, 
  /* RLE: 009 Pixels @ 014,014 */ 9, 0x00, 
  /* RLE: 001 Pixels @ 023,014 */ 1, 0x40, 
  /* RLE: 018 Pixels @ 024,014 */ 18, 0xFF, 
  /* ABS: 002 Pixels @ 012,015 */ 0, 2, 0xFC, 0x62, 
  /* RLE: 009 Pixels @ 014,015 */ 9, 0x00, 
  /* RLE: 001 Pixels @ 023,015 */ 1, 0x3A, 
  /* RLE: 017 Pixels @ 024,015 */ 17, 0xFF, 
  /* ABS: 002 Pixels @ 011,016 */ 0, 2, 0xF2, 0x4F, 
  /* RLE: 009 Pixels @ 013,016 */ 9, 0x00, 
  /* ABS: 002 Pixels @ 022,016 */ 0, 2, 0x34, 0xDF, 
  /* RLE: 016 Pixels @ 024,016 */ 16, 0xFF, 
  /* ABS: 002 Pixels @ 010,017 */ 0, 2, 0xE5, 0x40, 
  /* RLE: 009 Pixels @ 012,017 */ 9, 0x00, 
  /* ABS: 002 Pixels @ 021,017 */ 0, 2, 0x46, 0xEA, 
  /* RLE: 016 Pixels @ 023,017 */ 16, 0xFF, 
  /* ABS: 002 Pixels @ 009,018 */ 0, 2, 0xDA, 0x30, 
  /* RLE: 009 Pixels @ 011,018 */ 9, 0x00, 
  /* ABS: 002 Pixels @ 020,018 */ 0, 2, 0x54, 0xF2, 
  /* RLE: 016 Pixels @ 022,018 */ 16, 0xFF, 
  /* ABS: 002 Pixels @ 008,019 */ 0, 2, 0xCC, 0x20, 
  /* RLE: 009 Pixels @ 010,019 */ 9, 0x00, 
  /* ABS: 002 Pixels @ 019,019 */ 0, 2, 0x69, 0xFA, 
  /* RLE: 016 Pixels @ 021,019 */ 16, 0xFF, 
  /* ABS: 002 Pixels @ 007,020 */ 0, 2, 0xBD, 0x19, 
  /* RLE: 009 Pixels @ 009,020 */ 9, 0x00, 
  /* RLE: 001 Pixels @ 018,020 */ 1, 0x7E, 
  /* RLE: 017 Pixels @ 019,020 */ 17, 0xFF, 
  /* ABS: 002 Pixels @ 006,021 */ 0, 2, 0xB4, 0x0F, 
  /* RLE: 009 Pixels @ 008,021 */ 9, 0x00, 
  /* RLE: 001 Pixels @ 017,021 */ 1, 0x87, 
  /* RLE: 018 Pixels @ 018,021 */ 18, 0xFF, 
  /* RLE: 001 Pixels @ 006,022 */ 1, 0x2A, 
  /* RLE: 008 Pixels @ 007,022 */ 8, 0x00, 
  /* ABS: 002 Pixels @ 015,022 */ 0, 2, 0x07, 0x9C, 
  /* RLE: 019 Pixels @ 017,022 */ 19, 0xFF, 
  /* RLE: 001 Pixels @ 006,023 */ 1, 0x86, 
  /* RLE: 007 Pixels @ 007,023 */ 7, 0x00, 
  /* ABS: 002 Pixels @ 014,023 */ 0, 2, 0x10, 0xAE, 
  /* RLE: 021 Pixels @ 016,023 */ 21, 0xFF, 
  /* ABS: 002 Pixels @ 007,024 */ 0, 2, 0x93, 0x05, 
  /* RLE: 004 Pixels @ 009,024 */ 4, 0x00, 
  /* ABS: 002 Pixels @ 013,024 */ 0, 2, 0x17, 0xBF, 
  /* RLE: 023 Pixels @ 015,024 */ 23, 0xFF, 
  /* ABS: 006 Pixels @ 008,025 */ 0, 6, 0xA7, 0x0A, 0x00, 0x00, 0x25, 0xCE, 
  /* RLE: 025 Pixels @ 014,025 */ 25, 0xFF, 
  /* ABS: 004 Pixels @ 009,026 */ 0, 4, 0xB6, 0x21, 0x39, 0xDA, 
  /* RLE: 107 Pixels @ 013,026 */ 107, 0xFF, 
  0
};  // 264 bytes for 900 pixels

GUI_CONST_STORAGE GUI_BITMAP bmArrowRight_30x30 = {
  30, // xSize
  30, // ySize
  30, // BytesPerLine
  GUI_COMPRESS_RLE8, // BitsPerPixel
  (unsigned char *)_acArrowRight_30x30,  // Pointer to picture data
  NULL,  // Pointer to palette
  GUI_DRAW_RLEALPHA
};

static GUI_CONST_STORAGE unsigned char _acOnOff_20x20[] = {
  /* RLE: 005 Pixels @ 000,000 */ 5, 0xFF, 
  /* ABS: 010 Pixels @ 005,000 */ 0, 10, 0xFB, 0xEA, 0xD0, 0xA8, 0x82, 0x82, 0xA7, 0xCF, 0xEA, 0xFB, 
  /* RLE: 009 Pixels @ 015,000 */ 9, 0xFF, 
  /* ABS: 003 Pixels @ 004,001 */ 0, 3, 0xE1, 0x85, 0x2B, 
  /* RLE: 006 Pixels @ 007,001 */ 6, 0x00, 
  /* ABS: 003 Pixels @ 013,001 */ 0, 3, 0x2A, 0x86, 0xDC, 
  /* RLE: 007 Pixels @ 016,001 */ 7, 0xFF, 
  /* ABS: 002 Pixels @ 003,002 */ 0, 2, 0xB8, 0x3D, 
  /* RLE: 010 Pixels @ 005,002 */ 10, 0x00, 
  /* ABS: 002 Pixels @ 015,002 */ 0, 2, 0x39, 0xBB, 
  /* RLE: 005 Pixels @ 017,002 */ 5, 0xFF, 
  /* ABS: 002 Pixels @ 002,003 */ 0, 2, 0xB9, 0x25, 
  /* RLE: 012 Pixels @ 004,003 */ 12, 0x00, 
  /* ABS: 007 Pixels @ 016,003 */ 0, 7, 0x23, 0xB1, 0xFF, 0xFF, 0xFF, 0xE1, 0x3D, 
  /* RLE: 014 Pixels @ 003,004 */ 14, 0x00, 
  /* ABS: 005 Pixels @ 017,004 */ 0, 5, 0x3B, 0xDE, 0xFF, 0xFB, 0x86, 
  /* RLE: 016 Pixels @ 002,005 */ 16, 0x00, 
  /* ABS: 004 Pixels @ 018,005 */ 0, 4, 0x95, 0xFC, 0xEA, 0x2C, 
  /* RLE: 016 Pixels @ 002,006 */ 16, 0x00, 
  /* ABS: 003 Pixels @ 018,006 */ 0, 3, 0x29, 0xED, 0xD1, 
  /* RLE: 018 Pixels @ 001,007 */ 18, 0x00, 
  /* ABS: 002 Pixels @ 019,007 */ 0, 2, 0xC5, 0xAA, 
  /* RLE: 018 Pixels @ 001,008 */ 18, 0x00, 
  /* ABS: 002 Pixels @ 019,008 */ 0, 2, 0x86, 0x85, 
  /* RLE: 018 Pixels @ 001,009 */ 18, 0x00, 
  /* ABS: 002 Pixels @ 019,009 */ 0, 2, 0x61, 0x85, 
  /* RLE: 018 Pixels @ 001,010 */ 18, 0x00, 
  /* ABS: 002 Pixels @ 019,010 */ 0, 2, 0x61, 0xAA, 
  /* RLE: 018 Pixels @ 001,011 */ 18, 0x00, 
  /* ABS: 002 Pixels @ 019,011 */ 0, 2, 0x87, 0xD0, 
  /* RLE: 018 Pixels @ 001,012 */ 18, 0x00, 
  /* ABS: 003 Pixels @ 019,012 */ 0, 3, 0xC5, 0xEA, 0x2C, 
  /* RLE: 016 Pixels @ 002,013 */ 16, 0x00, 
  /* ABS: 004 Pixels @ 018,013 */ 0, 4, 0x2B, 0xEE, 0xFB, 0x86, 
  /* RLE: 016 Pixels @ 002,014 */ 16, 0x00, 
  /* ABS: 005 Pixels @ 018,014 */ 0, 5, 0x95, 0xFC, 0xFF, 0xDD, 0x3A, 
  /* RLE: 014 Pixels @ 003,015 */ 14, 0x00, 
  /* ABS: 007 Pixels @ 017,015 */ 0, 7, 0x3D, 0xDF, 0xFF, 0xFF, 0xFF, 0xBC, 0x24, 
  /* RLE: 012 Pixels @ 004,016 */ 12, 0x00, 
  /* ABS: 002 Pixels @ 016,016 */ 0, 2, 0x22, 0xB4, 
  /* RLE: 005 Pixels @ 018,016 */ 5, 0xFF, 
  /* ABS: 002 Pixels @ 003,017 */ 0, 2, 0xB2, 0x3C, 
  /* RLE: 010 Pixels @ 005,017 */ 10, 0x00, 
  /* ABS: 002 Pixels @ 015,017 */ 0, 2, 0x3D, 0xB5, 
  /* RLE: 007 Pixels @ 017,017 */ 7, 0xFF, 
  /* ABS: 003 Pixels @ 004,018 */ 0, 3, 0xDE, 0x96, 0x2A, 
  /* RLE: 006 Pixels @ 007,018 */ 6, 0x00, 
  /* ABS: 003 Pixels @ 013,018 */ 0, 3, 0x2C, 0x95, 0xDF, 
  /* RLE: 009 Pixels @ 016,018 */ 9, 0xFF, 
  /* ABS: 010 Pixels @ 005,019 */ 0, 10, 0xFC, 0xED, 0xC5, 0x85, 0x60, 0x60, 0x85, 0xC5, 0xEE, 0xFC, 
  /* RLE: 005 Pixels @ 015,019 */ 5, 0xFF, 
  0
};  // 194 bytes for 400 pixels

GUI_CONST_STORAGE GUI_BITMAP bmOnOff_20x20 = {
  20, // xSize
  20, // ySize
  20, // BytesPerLine
  GUI_COMPRESS_RLE8, // BitsPerPixel
  (unsigned char *)_acOnOff_20x20,  // Pointer to picture data
  NULL,  // Pointer to palette
  GUI_DRAW_RLEALPHA
};

/*********************************************************************
*
*       _aDialogCreate
*/
static GUI_WIDGET_CREATE_INFO _aDialogCreate[] = {
  { WINDOW_CreateIndirect, "Window", ID_WINDOW_00,   0,  0, 480, 272, 0, 0x0, 0 },
//  { TEXT_CreateIndirect,   "Text",   ID_TEXT_00,     5,  0, 240,  30, 0, 0x0, 0 },
  { BUTTON_CreateIndirect, "Button", ID_BUTTON_00,  10, 121, 60,   60, 0, 0x0, 0 },
  { BUTTON_CreateIndirect, "Button", ID_BUTTON_01, 410, 121, 60,   60, 0, 0x0, 0 },
  { BUTTON_CreateIndirect, "Button", ID_BUTTON_02, 440,   0, 30,   30, 0, 0x0, 0 },
};

static SECURE_RSA_HASH_CONTEXT _HashContext;
static U8                      _aSignature[SECURE_RSA_MAX_KEY_LENGTH / 8];
static char                    _acFileName[100][50];
static char                  * _pVerifyText[] = {"Signature: Not found", "Signature: Match", "Signature: Mismatch", "Verification disabled"};
static char                    _VerifyOnOff;

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/
/*********************************************************************
*
*       _GetData
*/
static int _GetData(void * p, const U8 ** ppData, unsigned NumBytes, U32 Off) {
  U32          NumBytesRead;
  U32          ReadBytes;
  static U8    aBuffer[4096];
  FS_FILE    * pFile;

  pFile = (FS_FILE *)p;
  //
  // Set file pointer to the offset position
  //
  FS_SetFilePos(pFile, Off, FS_FILE_BEGIN);
  //
  // Check that we don#t read more than the buffer can hold
  //
  if (NumBytes <= sizeof(aBuffer)) {
    ReadBytes = NumBytes;
  } else {
    ReadBytes = sizeof(aBuffer);
  }
  //
  // read from file
  //
  NumBytesRead = FS_Read(pFile, (void *)aBuffer, ReadBytes);
  //
  // Set pointer to the buffer address
  //
  *ppData = aBuffer;
  return NumBytesRead;
}

/*********************************************************************
*
*       _GetExtension
*/
static int _GetExtension(char * pStr) {
  char * pFind = NULL;
  int    r;
  
  r = 0;
  pFind = strrchr(pStr, '.');  // Search for the first dot
  if (pFind) {
    //
    // Check if it is a bmp file
    //
    if (strcmp(pFind, ".BMP") == 0 ||
        strcmp(pFind, ".bmp") == 0) {
      r = EXT_BMP;
    //
    // Maybe its a jpg
    //
    } else if (strcmp(pFind, ".JPG")  == 0 ||
               strcmp(pFind, ".JPEG") == 0 ||
               strcmp(pFind, ".jpg")  == 0 ||
               strcmp(pFind, ".jpeg")  == 0) {
      r = EXT_JPEG;
    } else {
      //
      // Maybe there is another dot and its a signature file
      //
      pFind = strrchr(pFind, '.');
      if (strcmp(pFind, ".sig")) {
        r = EXT_SIG;
      } else {
        //
        // If none is true..
        //
        r = 0;
      }
    }
  }
  return r;
}

/*********************************************************************
*
*       _Char2Hex
*/
static char _Char2Hex(char c) {
  if ((c >= '0') && (c <= '9')) {
    return c - '0';
  }
  if ((c >= 'A') && (c <= 'F')) {
    return c- 'A' + 10;
  }
  if ((c >= 'a') && (c <= 'f')) {
    return c- 'a' + 10;
  }
  return -1;
}

/*********************************************************************
*
*       _ParseSignature
*/
static int _ParseSignature(void * pData) {
  U8 * pNew;
  int  Value;
  U32  i;
  int  j;

  pNew  = (U8 *)strchr(pData, '=');  // Get a pointer to the '=' sign every signature has "=0x"
  pNew += 3;                         // so we increment the pointer about 3 and it points to the first digit of the signature
  i     = 0;
  memset(_aSignature, 0, sizeof(_aSignature));
  while (i < GUI_COUNTOF(_aSignature)) {
    //
    // Fill the signature array
    //
    for (j = 0; j < 2; j++) {
        Value = _Char2Hex(*pNew) << (4 - (4 * j));  // Check if value is in range of hex digits and convert ASCII to hex
      if (Value >= 0) {
        _aSignature[i] += (U8)Value;                // Add it to the signature
      } else {
        return 0;                                   // Error
      }
      pNew++;                                       // Increment pointer
    }
    i++;
  }
  return 1;                                         // Success
}

/*********************************************************************
*
*       _Verify
*/
static int _Verify(FS_FILE * pFile, char * pFileName, char * pSigText) {
  FS_FILE  * pFileSig;
  GUI_HMEM   hMemSignature;
  void     * pSignature;
  int        SizeOfSignature;
  U32        NumBytesRead;
  U8         aBuffer[1024];
  int        r;

  //
  // Build file name of the signature belonging to this image and open the file
  //
  sprintf(pSigText, "%s.sig", pFileName);
  pFileSig = FS_FOpen(pSigText, "r");
  if (pFileSig) {
    //
    // On success allocate memory and load the signature into RAM
    //
    SizeOfSignature = FS_GetFileSize(pFileSig);
    hMemSignature   = GUI_ALLOC_AllocNoInit(SizeOfSignature);
    pSignature      = GUI_ALLOC_h2p(hMemSignature);
    if (pSignature) {
      //
      // Read from file and parse the signature into an array
      //
      NumBytesRead = FS_Read(pFileSig, pSignature, SizeOfSignature);
      _ParseSignature(pSignature);
      //
      // Close file and free memory for the signature
      //
      FS_FClose(pFileSig);
      GUI_ALLOC_Free(hMemSignature);
    }
    //
    // Initialize hash context
    //
    SECURE_RSA_HASH_Init(&_HashContext);
    FS_FSeek(pFile, 0, FS_FILE_BEGIN);
    //
    // Read chunks from the file and add to the hash
    //
    do {
      NumBytesRead = FS_Read(pFile, (void *)aBuffer, sizeof(aBuffer));
      SECURE_RSA_HASH_Add(&_HashContext, (void *)aBuffer, NumBytesRead);
    } while (NumBytesRead == sizeof(aBuffer));
    r = SECURE_RSA_HASH_Verify(&_HashContext, &_SECURE_RSA_PublicKey_Expert, NULL, 0, &_aSignature[0], sizeof(_aSignature));
    if (r != SIG_OK) {
      r = SIG_INV;
    }
  } else {
    r = SIG_NO;
  }
  return r;
}

/*********************************************************************
*
*       _cbButton
*/
static void _cbButton(WM_MESSAGE * pMsg) {
  const GUI_BITMAP * pBm;
  int                xPos;
  int                yPos;
  int                Id;
  GUI_COLOR          Color;

  switch (pMsg->MsgId) {
  case WM_PAINT:
    Id = WM_GetId(pMsg->hWin);
    if (Id == ID_BUTTON_00) {
      pBm = &bmArrowLeft_30x30;
      Color = COLOR_LIGHTGRAY;
    } else if (Id == ID_BUTTON_01) {
      pBm = &bmArrowRight_30x30;
      Color = COLOR_LIGHTGRAY;
    } else {
      pBm = &bmOnOff_20x20;
      if (_VerifyOnOff) {
        Color = GUI_GREEN;
      } else {
        Color = GUI_RED;
      }
    }
    xPos = (WM_GetWindowSizeX(pMsg->hWin) - pBm->XSize) / 2;
    yPos = (WM_GetWindowSizeY(pMsg->hWin) - pBm->YSize) / 2;
    GUI_SetColor(Color);
    GUI_DrawBitmap(pBm, xPos, yPos);
    break;
  default:
    BUTTON_Callback(pMsg);
    break;
  }
}

/*********************************************************************
*
*       _cbDialog
*/
static void _cbDialog(WM_MESSAGE * pMsg) {
  WM_HWIN          hItem;
  static char      acVolumeName[10];
  char             acFileName[64];
  FS_FIND_DATA     FindData;
  static FS_FILE * pFile;
  static int       NumFiles;
  static int       Index;
  static int       Verify;
  int              xPos;
  int              yPos;
  int              xSize;
  int              ySize;
  int              Id;
  int              NCode;
  GUI_JPEG_INFO    Info;
  GUI_RECT         Rect;

  switch (pMsg->MsgId) {
  case WM_INIT_DIALOG:
    //
    // Initialization of 'Window'
    //
    hItem = pMsg->hWin;
    //
    // Initialization of left, right and OnOff button
    //
    hItem = WM_GetDialogItem(pMsg->hWin, ID_BUTTON_00);
    WM_SetCallback(hItem, _cbButton);
    hItem = WM_GetDialogItem(pMsg->hWin, ID_BUTTON_01);
    WM_SetCallback(hItem, _cbButton);
    hItem = WM_GetDialogItem(pMsg->hWin, ID_BUTTON_02);
    WM_SetCallback(hItem, _cbButton);

    NumFiles = 0;
    FS_GetVolumeName(0, acVolumeName, sizeof(acVolumeName));
    if (FS_Mount(acVolumeName) > 0) {
      //
      // Get the first file and add it to the LISTBOX
      //
      if (FS_FindFirstFile(&FindData, "", acFileName, 50) == 0) {
        if (_GetExtension(acFileName) > 0) {
          sprintf(&_acFileName[NumFiles][0], acFileName);
          NumFiles++;
        }
      }
      //
      // Search for more image files (bmp and jpeg) and if there is one add it to the LISTBOX
      //
      while (FS_FindNextFile(&FindData)) {
        if (_GetExtension(acFileName) > 0) {
          sprintf(&_acFileName[NumFiles][0], acFileName);
          NumFiles++;
        }
      }
    }
    Index = 0;
    break;
  case MESSAGE_NEXT:
    Index++;
    if (Index == NumFiles) {
      Index = 0;
    }
    WM_Invalidate(pMsg->hWin);
    break;
  case MESSAGE_PREV:
    Index--;
    if (Index < 0) {
      Index = NumFiles - 1;
    }
    WM_Invalidate(pMsg->hWin);
    break;
  case WM_PRE_PAINT:
    //
    // Verify next file and remember result
    //
    if (_VerifyOnOff) {
      pFile = FS_FOpen(_acFileName[Index], "r");
      Verify = _Verify(pFile, _acFileName[Index], acFileName);
      FS_FClose(pFile);
    } else {
      Verify = NO_VERIFICATION;
    }
    break;
  case WM_PAINT:
    xSize = WM_GetWindowSizeX(pMsg->hWin);
    ySize = WM_GetWindowSizeY(pMsg->hWin);
    //
    // Clear background
    //
    GUI_SetBkColor(COLOR_DARKBLUE);
    GUI_Clear();
    //
    // Draw a line between text and images
    //
    hItem = WM_GetDialogItem(pMsg->hWin, ID_TEXT_00);
    GUI_SetColor(COLOR_LIGHTGRAY);
    GUI_DrawHLine(TRECT_Y1, 0, xSize);
    
    Rect.x0 = TRECT_X0;
    Rect.y0 = TRECT_Y0;
    Rect.x1 = TRECT_X1;
    Rect.y1 = TRECT_Y1;
    GUI_SetFont(&GUI_Font20_1);
    GUI_DispStringInRect(_pVerifyText[Verify], &Rect, GUI_TA_LEFT | GUI_TA_VCENTER);

    //
    // Check for extension of the next file. Depending on extension choose the proper drawing routine.
    //
    pFile = FS_FOpen(_acFileName[Index], "r");
    switch (_GetExtension(_acFileName[Index])) {
    case EXT_BMP:
      xPos =   (xSize                   - GUI_BMP_GetXSizeEx(_GetData, (void *)pFile)) >> 1;
      yPos = (((ySize - HEADLINE_YSIZE) - GUI_BMP_GetYSizeEx(_GetData, (void *)pFile)) >> 1) + HEADLINE_YSIZE;
      GUI_BMP_DrawEx(_GetData, (void *)pFile, xPos, yPos);
      break;
    case EXT_JPEG:
      GUI_JPEG_GetInfoEx(_GetData, (void *)pFile, &Info);
      xPos =   (xSize                   - Info.XSize) >> 1;
      yPos = (((ySize - HEADLINE_YSIZE) - Info.YSize) >> 1) + HEADLINE_YSIZE;
      GUI_JPEG_DrawEx(_GetData, (void *)pFile, xPos, yPos);
      break;
    }
    FS_FClose(pFile);
    //
    // If verification was not successful, draw a lock on top of the image
    //
    if (_VerifyOnOff) {
      if (Verify != SIG_OK) {
        GUI_SetColor(GUI_RED);
        xPos =   (xSize                   - bmemSecure_113x113.XSize) >> 1;
        yPos = (((ySize - HEADLINE_YSIZE) - bmemSecure_113x113.YSize) >> 1) + HEADLINE_YSIZE;
        GUI_DrawBitmap(&bmemSecure_113x113, xPos, yPos);
      }
    }
    break;
  case WM_NOTIFY_PARENT:
    Id    = WM_GetId(pMsg->hWinSrc);
    NCode = pMsg->Data.v;
    switch (Id) {
    case ID_BUTTON_00:
      switch (NCode) {
      case WM_NOTIFICATION_RELEASED:
        WM_SendMessageNoPara(pMsg->hWin, MESSAGE_PREV);
        break;
      }
      break;
    case ID_BUTTON_01:
      switch (NCode) {
      case WM_NOTIFICATION_RELEASED:
        WM_SendMessageNoPara(pMsg->hWin, MESSAGE_NEXT);
        break;
      }
      break;
    case ID_BUTTON_02:
      switch (NCode) {
      case WM_NOTIFICATION_RELEASED:
        _VerifyOnOff = !_VerifyOnOff;
        WM_Invalidate(pMsg->hWin);
        break;
      }
      break;
    }
    break;
  default:
    WM_DefaultProc(pMsg);
    break;
  }
}

/*********************************************************************
*
*       _GUIVerifyTask
*/
static void _GUIVerifyTask(void) {
  //
  // Initialize emWin
  //
  GUI_Init();
  //
  // Enable multi buffering
  //
  WM_MULTIBUF_Enable(1);
  //
  // Initialize emFile and enable long file name support
  //
  FS_Init();
  FS_FAT_SupportLFN();
  //
  // Adjust widget position according to display size
  //
  _aDialogCreate[0].xSize =  LCD_GetXSize();
  _aDialogCreate[0].ySize =  LCD_GetYSize();
  _aDialogCreate[1].y0    = (LCD_GetYSize() - BUTTON_LR_SIZE)   / 2;
  _aDialogCreate[2].x0    =  LCD_GetXSize() - BUTTON_LR_SIZE    - 10;
  _aDialogCreate[2].y0    = (LCD_GetYSize() - BUTTON_LR_SIZE)   / 2;
  _aDialogCreate[3].x0    =  LCD_GetXSize() - BUTTON_ONOFF_SIZE - 10;
  //
  // Create a dialog to show images from SDCard
  //
  GUI_CreateDialogBox(_aDialogCreate, GUI_COUNTOF(_aDialogCreate), _cbDialog, WM_HBKWIN, 0, 0);
  _VerifyOnOff = 1;
  while(1) {
    GUI_Delay(100);
  }
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
  // Create benchmark task
  //
  OS_CREATETASK(&_TCBGUIVerify, "VerifyImages", _GUIVerifyTask, 150, _StackGUIVerify);
  //
  // Do something in the idle time...
  //
  while (1) {
    BSP_ToggleLED(1);
    OS_Delay (200);
  }
}

/****** End of File *************************************************/
