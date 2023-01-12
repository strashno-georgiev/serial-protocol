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
File    : LCDConf.c
Purpose : Configuration file for emWin.
--------  END-OF-HEADER  ---------------------------------------------
*/
#include "LCDConf.h"
#include "GUI.h"
#include "GUIDRV_Lin.h"
#include "stm32f7xx_hal.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/
/*********************************************************************
*
*       Common
*/
//
// Physical display size
//
#define XSIZE_PHYS      800 
#define YSIZE_PHYS      480 

//
// Buffers and virtual screens
//
#define NUM_BUFFERS     3 // Number of multiple buffers to be used
#define NUM_VSCREENS    1 // Number of virtual screens to be used

//
// Number of a layers
//
#define NUM_LAYERS   1

/*********************************************************************
*
*       Layer 0
*/
//
// Size
//
#define XSIZE_PHYS0 XSIZE_PHYS
#define YSIZE_PHYS0 YSIZE_PHYS

//
// Driver and color conversion
//
#define COLOR_CONVERSION_0  GUICC_M565
#define DISPLAY_DRIVER_0    GUIDRV_LIN_16

#if (NUM_LAYERS > 1)
/*********************************************************************
*
*       Layer 1
*/
//
// Size
//
#define XSIZE_PHYS1  800
#define YSIZE_PHYS1  480

#define XPOS_1       0
#define YPOS_1       0

#define COLOR_CONVERSION_1 GUICC_M1555I
#define DISPLAY_DRIVER_1   GUIDRV_LIN_16

#endif  // NUM_LAYERS > 1

/*********************************************************************
*
*       Configuration checking
*/
#ifndef   XSIZE_PHYS
  #error Physical X size of display is not defined!
#endif
#ifndef   YSIZE_PHYS
  #error Physical Y size of display is not defined!
#endif
#ifndef   NUM_VSCREENS
  #define NUM_VSCREENS 1
#else
  #if (NUM_VSCREENS <= 0)
    #error At least one screeen needs to be defined!
  #endif
#endif
#if (NUM_VSCREENS > 1) && (NUM_BUFFERS > 1)
  #error Virtual screens and multiple buffers are not allowed!
#endif

/*********************************************************************
*
*       Framebuffer base addresses
*/
#if (defined __SES_ARM)   // SES
  #define LCD_LAYER0_FRAME_BUFFER  ((U32)_aVRAM0)  
  #if (NUM_LAYERS > 1)
    #define LCD_LAYER1_FRAME_BUFFER  ((U32)_aVRAM1)  
  #endif
#else
  #define LCD_LAYER0_FRAME_BUFFER  ((U32)0xC040000)  
  #if (NUM_LAYERS > 1)
    #define LCD_LAYER1_FRAME_BUFFER  ((U32)LCD_LAYER0_FRAME_BUFFER + (XSIZE_PHYS * YSIZE_PHYS * NUM_BUFFERS * NUM_VSCREENS * 2))  
  #endif
#endif

//
// OTM8009A Specific defines
//
#define OTM8009A_ORIENTATION_PORTRAIT       0x00                    // Portrait orientation choice of LCD screen
#define OTM8009A_ORIENTATION_LANDSCAPE      0x01                    // Landscape orientation choice of LCD screen
#define OTM8009A_FORMAT_RGB888              0x00                    // Pixel format chosen is RGB888 : 24 bpp
#define OTM8009A_FORMAT_RBG565              0x02                    // Pixel format chosen is RGB565 : 16 bpp
#define OTM8009A_480X800_WIDTH              480                     // LCD PIXEL WIDTH
#define OTM8009A_480X800_HEIGHT             800                     // LCD PIXEL HEIGHT
#define OTM8009A_800X480_WIDTH              800                     // LCD PIXEL WIDTH
#define OTM8009A_800X480_HEIGHT             480                     // LCD PIXEL HEIGHT
#define OTM8009A_480X800_HSYNC              63
#define OTM8009A_480X800_HBP                120                     // Horizontal back porch
#define OTM8009A_480X800_HFP                120                     // Horizontal front porch
#define OTM8009A_480X800_VSYNC              12                      // Vertical synchronization
#define OTM8009A_480X800_VBP                12                      // Vertical back porch
#define OTM8009A_480X800_VFP                12                      // Vertical front porch
#define OTM8009A_800X480_HSYNC              OTM8009A_480X800_HSYNC  // Horizontal synchronization
#define OTM8009A_800X480_HBP                OTM8009A_480X800_HBP    // Horizontal back porch
#define OTM8009A_800X480_HFP                OTM8009A_480X800_HFP    // Horizontal front porch
#define OTM8009A_800X480_VSYNC              OTM8009A_480X800_VSYNC  // Vertical synchronization
#define OTM8009A_800X480_VBP                OTM8009A_480X800_VBP    // Vertical back porch
#define OTM8009A_800X480_VFP                OTM8009A_480X800_VFP    // Vertical front porch
#define OTM8009A_CMD_NOP                    0x00                    // NOP command
#define OTM8009A_CMD_SWRESET                0x01                    // Sw reset command
#define OTM8009A_CMD_RDDMADCTL              0x0B                    // Read Display MADCTR command : read memory display access ctrl
#define OTM8009A_CMD_RDDCOLMOD              0x0C                    // Read Display pixel format
#define OTM8009A_CMD_SLPIN                  0x10                    // Sleep In command
#define OTM8009A_CMD_SLPOUT                 0x11                    // Sleep Out command
#define OTM8009A_CMD_PTLON                  0x12                    // Partial mode On command
#define OTM8009A_CMD_DISPOFF                0x28                    // Display Off command
#define OTM8009A_CMD_DISPON                 0x29                    // Display On command
#define OTM8009A_CMD_CASET                  0x2A                    // Column address set command
#define OTM8009A_CMD_PASET                  0x2B                    // Page address set command
#define OTM8009A_CMD_RAMWR                  0x2C                    // Memory (GRAM) write command
#define OTM8009A_CMD_RAMRD                  0x2E                    // Memory (GRAM) read command
#define OTM8009A_CMD_PLTAR                  0x30                    // Partial area command (4 parameters)
#define OTM8009A_CMD_TEOFF                  0x34                    // Tearing Effect Line Off command : command with no parameter
#define OTM8009A_CMD_TEEON                  0x35                    // Tearing Effect Line On command : command with 1 parameter 'TELOM'
#define OTM8009A_TEEON_TELOM_VBLANKING_INFO_ONLY            0x00
#define OTM8009A_TEEON_TELOM_VBLANKING_AND_HBLANKING_INFO   0x01
#define OTM8009A_CMD_MADCTR                 0x36                    // Memory Access write control command
#define OTM8009A_MADCTR_MODE_PORTRAIT       0x00
#define OTM8009A_MADCTR_MODE_LANDSCAPE      0x60                    // MY = 0, MX = 1, MV = 1, ML = 0, RGB = 0
#define OTM8009A_CMD_IDMOFF                 0x38                    // Idle mode Off command
#define OTM8009A_CMD_IDMON                  0x39                    // Idle mode On command
#define OTM8009A_CMD_COLMOD                 0x3A                    // Interface Pixel format command
#define OTM8009A_COLMOD_RGB565              0x55
#define OTM8009A_COLMOD_RGB888              0x77
#define OTM8009A_CMD_RAMWRC                 0x3C                    // Memory write continue command
#define OTM8009A_CMD_RAMRDC                 0x3E                    // Memory read continue command
#define OTM8009A_CMD_WRTESCN                0x44                    // Write Tearing Effect Scan line command
#define OTM8009A_CMD_RDSCNL                 0x45                    // Read  Tearing Effect Scan line command
#define OTM8009A_CMD_WRDISBV                0x51                    // Write Display Brightness command
#define OTM8009A_CMD_WRCTRLD                0x53                    // Write CTRL Display command
#define OTM8009A_CMD_WRCABC                 0x55                    // Write Content Adaptive Brightness command
#define OTM8009A_CMD_WRCABCMB               0x5E                    // Write CABC Minimum Brightness command
#define OTM8009A_480X800_FREQUENCY_DIVIDER  2

/*********************************************************************
*
*       Types
*
**********************************************************************
*/
typedef struct {
  U32                 address;          
  I32                 pending_buffer;   
  I32                 buffer_index;     
  U32                 xSize;            
  U32                 ySize;            
  U32                 BytesPerPixel;
  LCD_API_COLOR_CONV *pColorConvAPI;
} LCD_LayerPropTypedef;

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
//
// Array for speeding up nibble conversion for A4 bitmaps
//
static const U8 _aMirror[] = {
  0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0,
  0x01, 0x11, 0x21, 0x31, 0x41, 0x51, 0x61, 0x71, 0x81, 0x91, 0xA1, 0xB1, 0xC1, 0xD1, 0xE1, 0xF1,
  0x02, 0x12, 0x22, 0x32, 0x42, 0x52, 0x62, 0x72, 0x82, 0x92, 0xA2, 0xB2, 0xC2, 0xD2, 0xE2, 0xF2,
  0x03, 0x13, 0x23, 0x33, 0x43, 0x53, 0x63, 0x73, 0x83, 0x93, 0xA3, 0xB3, 0xC3, 0xD3, 0xE3, 0xF3,
  0x04, 0x14, 0x24, 0x34, 0x44, 0x54, 0x64, 0x74, 0x84, 0x94, 0xA4, 0xB4, 0xC4, 0xD4, 0xE4, 0xF4,
  0x05, 0x15, 0x25, 0x35, 0x45, 0x55, 0x65, 0x75, 0x85, 0x95, 0xA5, 0xB5, 0xC5, 0xD5, 0xE5, 0xF5,
  0x06, 0x16, 0x26, 0x36, 0x46, 0x56, 0x66, 0x76, 0x86, 0x96, 0xA6, 0xB6, 0xC6, 0xD6, 0xE6, 0xF6,
  0x07, 0x17, 0x27, 0x37, 0x47, 0x57, 0x67, 0x77, 0x87, 0x97, 0xA7, 0xB7, 0xC7, 0xD7, 0xE7, 0xF7,
  0x08, 0x18, 0x28, 0x38, 0x48, 0x58, 0x68, 0x78, 0x88, 0x98, 0xA8, 0xB8, 0xC8, 0xD8, 0xE8, 0xF8,
  0x09, 0x19, 0x29, 0x39, 0x49, 0x59, 0x69, 0x79, 0x89, 0x99, 0xA9, 0xB9, 0xC9, 0xD9, 0xE9, 0xF9,
  0x0A, 0x1A, 0x2A, 0x3A, 0x4A, 0x5A, 0x6A, 0x7A, 0x8A, 0x9A, 0xAA, 0xBA, 0xCA, 0xDA, 0xEA, 0xFA,
  0x0B, 0x1B, 0x2B, 0x3B, 0x4B, 0x5B, 0x6B, 0x7B, 0x8B, 0x9B, 0xAB, 0xBB, 0xCB, 0xDB, 0xEB, 0xFB,
  0x0C, 0x1C, 0x2C, 0x3C, 0x4C, 0x5C, 0x6C, 0x7C, 0x8C, 0x9C, 0xAC, 0xBC, 0xCC, 0xDC, 0xEC, 0xFC,
  0x0D, 0x1D, 0x2D, 0x3D, 0x4D, 0x5D, 0x6D, 0x7D, 0x8D, 0x9D, 0xAD, 0xBD, 0xCD, 0xDD, 0xED, 0xFD,
  0x0E, 0x1E, 0x2E, 0x3E, 0x4E, 0x5E, 0x6E, 0x7E, 0x8E, 0x9E, 0xAE, 0xBE, 0xCE, 0xDE, 0xEE, 0xFE,
  0x0F, 0x1F, 0x2F, 0x3F, 0x4F, 0x5F, 0x6F, 0x7F, 0x8F, 0x9F, 0xAF, 0xBF, 0xCF, 0xDF, 0xEF, 0xFF,
};

//
// initialization commands and data for OTM8009A
//
const U8 lcdRegData1[]    = {0x80,0x09,0x01,0xFF};
const U8 lcdRegData2[]    = {0x80,0x09,0xFF};
const U8 lcdRegData3[]    = {0x00,0x09,0x0F,0x0E,0x07,0x10,0x0B,0x0A,0x04,0x07,0x0B,0x08,0x0F,0x10,0x0A,0x01,0xE1};
const U8 lcdRegData4[]    = {0x00,0x09,0x0F,0x0E,0x07,0x10,0x0B,0x0A,0x04,0x07,0x0B,0x08,0x0F,0x10,0x0A,0x01,0xE2};
const U8 lcdRegData5[]    = {0x79,0x79,0xD8};
const U8 lcdRegData6[]    = {0x00,0x01,0xB3};
const U8 lcdRegData7[]    = {0x85,0x01,0x00,0x84,0x01,0x00,0xCE};
const U8 lcdRegData8[]    = {0x18,0x04,0x03,0x39,0x00,0x00,0x00,0x18,0x03,0x03,0x3A,0x00,0x00,0x00,0xCE};
const U8 lcdRegData9[]    = {0x18,0x02,0x03,0x3B,0x00,0x00,0x00,0x18,0x01,0x03,0x3C,0x00,0x00,0x00,0xCE};
const U8 lcdRegData10[]   = {0x01,0x01,0x20,0x20,0x00,0x00,0x01,0x02,0x00,0x00,0xCF};
const U8 lcdRegData11[]   = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xCB};
const U8 lcdRegData12[]   = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xCB};
const U8 lcdRegData13[]   = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xCB};
const U8 lcdRegData14[]   = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xCB};
const U8 lcdRegData15[]   = {0x00,0x04,0x04,0x04,0x04,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xCB};
const U8 lcdRegData16[]   = {0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x04,0x04,0x04,0x04,0x00,0x00,0x00,0x00,0xCB};
const U8 lcdRegData17[]   = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xCB};
const U8 lcdRegData18[]   = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xCB};
const U8 lcdRegData19[]   = {0x00,0x26,0x09,0x0B,0x01,0x25,0x00,0x00,0x00,0x00,0xCC};
const U8 lcdRegData20[]   = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x26,0x0A,0x0C,0x02,0xCC};
const U8 lcdRegData21[]   = {0x25,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xCC};
const U8 lcdRegData22[]   = {0x00,0x25,0x0C,0x0A,0x02,0x26,0x00,0x00,0x00,0x00,0xCC};
const U8 lcdRegData23[]   = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x25,0x0B,0x09,0x01,0xCC};
const U8 lcdRegData24[]   = {0x26,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xCC};
const U8 lcdRegData25[]   = {0xFF,0xFF,0xFF,0xFF};    
const U8 lcdRegData27[]   = {0x00, 0x00, 0x03, 0x1F, OTM8009A_CMD_CASET};
const U8 lcdRegData28[]   = {0x00, 0x00, 0x01, 0xDF, OTM8009A_CMD_PASET};
const U8 ShortRegData1[]  = {OTM8009A_CMD_NOP, 0x00};
const U8 ShortRegData2[]  = {OTM8009A_CMD_NOP, 0x80};
const U8 ShortRegData3[]  = {0xC4, 0x30};
const U8 ShortRegData4[]  = {OTM8009A_CMD_NOP, 0x8A};
const U8 ShortRegData5[]  = {0xC4, 0x40};
const U8 ShortRegData6[]  = {OTM8009A_CMD_NOP, 0xB1};
const U8 ShortRegData7[]  = {0xC5, 0xA9};
const U8 ShortRegData8[]  = {OTM8009A_CMD_NOP, 0x91};
const U8 ShortRegData9[]  = {0xC5, 0x34};
const U8 ShortRegData10[] = {OTM8009A_CMD_NOP, 0xB4};
const U8 ShortRegData11[] = {0xC0, 0x50};
const U8 ShortRegData12[] = {0xD9, 0x4E};
const U8 ShortRegData13[] = {OTM8009A_CMD_NOP, 0x81};
const U8 ShortRegData14[] = {0xC1, 0x66};
const U8 ShortRegData15[] = {OTM8009A_CMD_NOP, 0xA1};
const U8 ShortRegData16[] = {0xC1, 0x08};
const U8 ShortRegData17[] = {OTM8009A_CMD_NOP, 0x92};
const U8 ShortRegData18[] = {0xC5, 0x01};
const U8 ShortRegData19[] = {OTM8009A_CMD_NOP, 0x95};
const U8 ShortRegData20[] = {OTM8009A_CMD_NOP, 0x94};
const U8 ShortRegData21[] = {0xC5, 0x33};
const U8 ShortRegData22[] = {OTM8009A_CMD_NOP, 0xA3};
const U8 ShortRegData23[] = {0xC0, 0x1B};
const U8 ShortRegData24[] = {OTM8009A_CMD_NOP, 0x82};
const U8 ShortRegData25[] = {0xC5, 0x83};
const U8 ShortRegData26[] = {0xC4, 0x83};
const U8 ShortRegData27[] = {0xC1, 0x0E};
const U8 ShortRegData28[] = {OTM8009A_CMD_NOP, 0xA6};
const U8 ShortRegData29[] = {OTM8009A_CMD_NOP, 0xA0};
const U8 ShortRegData30[] = {OTM8009A_CMD_NOP, 0xB0};
const U8 ShortRegData31[] = {OTM8009A_CMD_NOP, 0xC0};
const U8 ShortRegData32[] = {OTM8009A_CMD_NOP, 0xD0};
const U8 ShortRegData33[] = {OTM8009A_CMD_NOP, 0x90};
const U8 ShortRegData34[] = {OTM8009A_CMD_NOP, 0xE0};
const U8 ShortRegData35[] = {OTM8009A_CMD_NOP, 0xF0};
const U8 ShortRegData36[] = {OTM8009A_CMD_SLPOUT, 0x00};
const U8 ShortRegData37[] = {OTM8009A_CMD_COLMOD, OTM8009A_COLMOD_RGB565};
const U8 ShortRegData38[] = {OTM8009A_CMD_COLMOD, OTM8009A_COLMOD_RGB888};
const U8 ShortRegData39[] = {OTM8009A_CMD_MADCTR, OTM8009A_MADCTR_MODE_LANDSCAPE};
const U8 ShortRegData40[] = {OTM8009A_CMD_WRDISBV, 0x7F};
const U8 ShortRegData41[] = {OTM8009A_CMD_WRCTRLD, 0x2C};
const U8 ShortRegData42[] = {OTM8009A_CMD_WRCABC, 0x02};
const U8 ShortRegData43[] = {OTM8009A_CMD_WRCABCMB, 0xFF};
const U8 ShortRegData44[] = {OTM8009A_CMD_DISPON, 0x00};
const U8 ShortRegData45[] = {OTM8009A_CMD_RAMWR, 0x00};
const U8 ShortRegData46[] = {0xCF, 0x00};
const U8 ShortRegData47[] = {0xC5, 0x66};
const U8 ShortRegData48[] = {OTM8009A_CMD_NOP, 0xB6};
const U8 ShortRegData49[] = {0xF5, 0x06};

static   LCD_LayerPropTypedef layer_prop[NUM_LAYERS];
static   LTDC_HandleTypeDef   hltdc_disco;
static   DSI_HandleTypeDef    hdsi_disco;
static   DSI_VidCfgTypeDef    hdsivideo_handle;
volatile I32                  LCD_ActiveRegion    = 1;
volatile I32                  LCD_Refershing      = 0;

#if (defined __SES_ARM)   // SES
  static U8 _aVRAM0[800 * 480 * 2 * NUM_BUFFERS] __attribute__ ((section (".SDRAM1.GUI_RAM")));
  #if (NUM_LAYERS > 1)
    static U8 _aVRAM1[800 * 480 * 2 * NUM_BUFFERS] __attribute__ ((section (".SDRAM1.GUI_RAM")));
  #endif
#endif

static const LCD_API_COLOR_CONV * _apColorConvAPI[] = {
  COLOR_CONVERSION_0,
#if NUM_LAYERS == 2
  COLOR_CONVERSION_1,
#endif
};

static U32 _aBuffer[40 * 40];  // Only required for drawing AA4 characters
//
// DMA2D transfer is ready flag
//
static volatile int _WaitForDMA2D;

/*********************************************************************
*
*       Local code
*
**********************************************************************
*/
/*********************************************************************
*
*       DSI_Write
*/
static void DSI_Write(U32 NbrParams, U8 *pParams) {
  if(NbrParams <= 1) {
    HAL_DSI_ShortWrite(&hdsi_disco, 0, DSI_DCS_SHORT_PKT_WRITE_P1, pParams[0], pParams[1]); 
  } else {
    HAL_DSI_LongWrite(&hdsi_disco,  0, DSI_DCS_LONG_PKT_WRITE, NbrParams, pParams[NbrParams], pParams); 
  } 
}

/*********************************************************************
*
*       _OTM8009A_Init
*
* Copied from STM
*/
static U8 _OTM8009A_Init(U32 ColorCoding, U32 orientation) {
  DSI_Write(0,  (U8 *)ShortRegData1);
  DSI_Write(3,  (U8 *)lcdRegData1);
  DSI_Write(0,  (U8 *)ShortRegData2);
  DSI_Write(2,  (U8 *)lcdRegData2);
  DSI_Write(0,  (U8 *)ShortRegData2);
  DSI_Write(0,  (U8 *)ShortRegData3);
  GUI_X_Delay(10);
  DSI_Write(0,  (U8 *)ShortRegData4);
  DSI_Write(0,  (U8 *)ShortRegData5);
  GUI_X_Delay(10);
  DSI_Write(0,  (U8 *)ShortRegData6);
  DSI_Write(0,  (U8 *)ShortRegData7);
  DSI_Write(0,  (U8 *)ShortRegData8);
  DSI_Write(0,  (U8 *)ShortRegData9);
  DSI_Write(0,  (U8 *)ShortRegData10);
  DSI_Write(0,  (U8 *)ShortRegData11);
  DSI_Write(0,  (U8 *)ShortRegData1);
  DSI_Write(0,  (U8 *)ShortRegData12);
  DSI_Write(0,  (U8 *)ShortRegData13);
  DSI_Write(0,  (U8 *)ShortRegData14);
  DSI_Write(0,  (U8 *)ShortRegData15);
  DSI_Write(0,  (U8 *)ShortRegData16);
  DSI_Write(0,  (U8 *)ShortRegData17);
  DSI_Write(0,  (U8 *)ShortRegData18);
  DSI_Write(0,  (U8 *)ShortRegData19);
  DSI_Write(0,  (U8 *)ShortRegData9); 
  DSI_Write(0,  (U8 *)ShortRegData1);
  DSI_Write(2,  (U8 *)lcdRegData5);  
  DSI_Write(0,  (U8 *)ShortRegData20);
  DSI_Write(0,  (U8 *)ShortRegData21);
  DSI_Write(0,  (U8 *)ShortRegData22);
  DSI_Write(0,  (U8 *)ShortRegData23);
  DSI_Write(0,  (U8 *)ShortRegData24);
  DSI_Write(0,  (U8 *)ShortRegData25);
  DSI_Write(0,  (U8 *)ShortRegData13);
  DSI_Write(0,  (U8 *)ShortRegData26);
  DSI_Write(0,  (U8 *)ShortRegData15);
  DSI_Write(0,  (U8 *)ShortRegData27);
  DSI_Write(0,  (U8 *)ShortRegData28);
  DSI_Write(2,  (U8 *)lcdRegData6);
  DSI_Write(0,  (U8 *)ShortRegData2);
  DSI_Write(6,  (U8 *)lcdRegData7);
  DSI_Write(0,  (U8 *)ShortRegData29);
  DSI_Write(14, (U8 *)lcdRegData8);
  DSI_Write(0,  (U8 *)ShortRegData30);
  DSI_Write(14, (U8 *)lcdRegData9);
  DSI_Write(0,  (U8 *)ShortRegData31);
  DSI_Write(10, (U8 *)lcdRegData10);
  DSI_Write(0,  (U8 *)ShortRegData32);
  DSI_Write(0,  (U8 *)ShortRegData46);
  DSI_Write(0,  (U8 *)ShortRegData2);
  DSI_Write(10, (U8 *)lcdRegData11);
  DSI_Write(0,  (U8 *)ShortRegData33);
  DSI_Write(15, (U8 *)lcdRegData12);
  DSI_Write(0,  (U8 *)ShortRegData29);
  DSI_Write(15, (U8 *)lcdRegData13);
  DSI_Write(0,  (U8 *)ShortRegData30);
  DSI_Write(10, (U8 *)lcdRegData14);
  DSI_Write(0,  (U8 *)ShortRegData31);
  DSI_Write(15, (U8 *)lcdRegData15);
  DSI_Write(0,  (U8 *)ShortRegData32);
  DSI_Write(15, (U8 *)lcdRegData16);
  DSI_Write(0,  (U8 *)ShortRegData34);
  DSI_Write(10, (U8 *)lcdRegData17);
  DSI_Write(0,  (U8 *)ShortRegData35);
  DSI_Write(10, (U8 *)lcdRegData18);
  DSI_Write(0,  (U8 *)ShortRegData2);
  DSI_Write(10, (U8 *)lcdRegData19);
  DSI_Write(0,  (U8 *)ShortRegData33);
  DSI_Write(15, (U8 *)lcdRegData20);
  DSI_Write(0,  (U8 *)ShortRegData29);
  DSI_Write(15, (U8 *)lcdRegData21);
  DSI_Write(0,  (U8 *)ShortRegData30);
  DSI_Write(10, (U8 *)lcdRegData22);
  DSI_Write(0,  (U8 *)ShortRegData31);
  DSI_Write(15, (U8 *)lcdRegData23);
  DSI_Write(0,  (U8 *)ShortRegData32);
  DSI_Write(15, (U8 *)lcdRegData24);
  DSI_Write(0,  (U8 *)ShortRegData13);
  DSI_Write(0,  (U8 *)ShortRegData47);
  DSI_Write(0,  (U8 *)ShortRegData48);
  DSI_Write(0,  (U8 *)ShortRegData49);
  DSI_Write(0,  (U8 *)ShortRegData1);
  DSI_Write(3,  (U8 *)lcdRegData25); 
  DSI_Write(0,  (U8 *)ShortRegData1); 
  DSI_Write(0,  (U8 *)ShortRegData1);
  DSI_Write(16, (U8 *)lcdRegData3); 
  DSI_Write(0,  (U8 *)ShortRegData1);
  DSI_Write(16, (U8 *)lcdRegData4); 
  DSI_Write(0,  (U8 *)ShortRegData36);
  GUI_X_Delay(120);
  switch(ColorCoding) {
  case OTM8009A_FORMAT_RBG565:
    DSI_Write(0, (U8 *)ShortRegData37);
    break;
  case OTM8009A_FORMAT_RGB888:
    DSI_Write(0, (U8 *)ShortRegData38);
    break;
  default :
    break;
  }
  if(orientation == OTM8009A_ORIENTATION_LANDSCAPE) {
    DSI_Write(0, (U8 *)ShortRegData39);
    DSI_Write(4, (U8 *)lcdRegData27);
    DSI_Write(4, (U8 *)lcdRegData28);
  }
  DSI_Write(0, (U8 *)ShortRegData40);
  DSI_Write(0, (U8 *)ShortRegData41);
  DSI_Write(0, (U8 *)ShortRegData42);
  DSI_Write(0, (U8 *)ShortRegData43);
  DSI_Write(0, (U8 *)ShortRegData44);
  DSI_Write(0, (U8 *)ShortRegData1);
  DSI_Write(0, (U8 *)ShortRegData45);
  return 0;
}

/*********************************************************************
*
*       _GetPixelformat
*/
static U32 _GetPixelformat(int LayerIndex) {
  const LCD_API_COLOR_CONV * pColorConvAPI;

  if ((unsigned)LayerIndex >= GUI_COUNTOF(_apColorConvAPI)) {
    return 0;
  }
  pColorConvAPI = _apColorConvAPI[LayerIndex];
  if        (pColorConvAPI == GUICC_M8888I) {
    return LTDC_PIXEL_FORMAT_ARGB8888;
  } else if (pColorConvAPI == GUICC_M888  ) {
    return LTDC_PIXEL_FORMAT_RGB888;
  } else if (pColorConvAPI == GUICC_M565  ) {
    return LTDC_PIXEL_FORMAT_RGB565;
  } else if (pColorConvAPI == GUICC_M1555I) {
    return LTDC_PIXEL_FORMAT_ARGB1555;
  } else if (pColorConvAPI == GUICC_M4444I) {
    return LTDC_PIXEL_FORMAT_ARGB4444;
  } else if (pColorConvAPI == GUICC_8666  ) {
    return LTDC_PIXEL_FORMAT_L8;
  } else if (pColorConvAPI == GUICC_1616I ) {
    return LTDC_PIXEL_FORMAT_AL44;
  } else if (pColorConvAPI == GUICC_88666I) {
    return LTDC_PIXEL_FORMAT_AL88;
  }
  while (1); // Error
}

/*********************************************************************
*
*       _NVIC_SetPriority
*/
static void _NVIC_SetPriority(int IRQn, U32 priority) {
  if(IRQn < 0) {
    while (1); // Not supported here, stop execution.
  } else {
    NVIC->IP[(U32)(IRQn)] = ((priority << (8 - __NVIC_PRIO_BITS)) & 0xff); /* Set Priority for device specific Interrupts */
  }
}

/*********************************************************************
*
*       _NVIC_EnableIRQ
*/
static void _NVIC_EnableIRQ(int IRQn) {
  NVIC->ISER[(U32)((I32)IRQn) >> 5] = (U32)(1 << ((U32)((I32)IRQn) & (U32)0x1F)); /* Enable interrupt */
}

/*********************************************************************
*
*       _DMA2D_ITConfig
*/
static void _DMA2D_ITConfig(U32 DMA2D_IT, int NewState) {
  if (NewState != DISABLE) {
    DMA2D->CR |= DMA2D_IT;
  } else {
    DMA2D->CR &= (U32)~DMA2D_IT;
  }
}

/*********************************************************************
*
*       _DMA_ExecOperation
*/
static void _DMA_ExecOperation(void) {
  //
  // Invalidate and clean the data cache before executing the DMA2D operation.
  // Otherwise we would have artifacts on the LCD.
  //
  SCB_CleanInvalidateDCache();
  //
  // Set Flag which gets cleared when DMA2D transfer is completed
  //
  _WaitForDMA2D = 1;
  //
  // Execute operation
  //
  DMA2D->CR     |= 1;                               // Control Register (Start operation)
  //
  // Wait until transfer is done
  //
  while (_WaitForDMA2D) {
  }
}

/*********************************************************************
*
*       _DMA_Fill
*/
static void _DMA_Fill(U32 LayerIndex, void * pDst, U32 xSize, U32 ySize, U32 OffLine, U32 ColorIndex) {
  
  U32 PixelFormat;
  
  PixelFormat    = _GetPixelformat(LayerIndex);
  DMA2D->CR      = 0x00030000UL | (1 << 9);        
  DMA2D->OCOLR   = ColorIndex;                     
  DMA2D->OMAR    = (U32)pDst;                      
  DMA2D->OOR     = OffLine;
  DMA2D->OPFCCR  = PixelFormat;                    
  DMA2D->NLR     = (U32)(xSize << 16) | (U16)ySize;  
  _DMA_ExecOperation();
}

/*********************************************************************
*
*       _DMA_DrawAlphaBitmap
*/
/**/
static void _DMA_DrawAlphaBitmap(void * pDst, const void * pSrc, int xSize, int ySize, int OffLineSrc, int OffLineDst, int PixelFormat) {
  DMA2D->CR      = 0x00020000UL | (1 << 9);         // Control Register (Memory to memory with blending of FG and BG and TCIE)
  DMA2D->FGMAR   = (U32)pSrc;                       // Foreground Memory Address Register (Source address)
  DMA2D->BGMAR   = (U32)pDst;                       // Background Memory Address Register (Destination address)
  DMA2D->OMAR    = (U32)pDst;                       // Output Memory Address Register (Destination address)
  DMA2D->FGOR    = OffLineSrc;                      // Foreground Offset Register (Source line offset)
  DMA2D->BGOR    = OffLineDst;                      // Background Offset Register (Destination line offset)
  DMA2D->OOR     = OffLineDst;                      // Output Offset Register (Destination line offset)
  DMA2D->FGPFCCR = LTDC_PIXEL_FORMAT_ARGB8888;      // Foreground PFC Control Register (Defines the input pixel format)
  DMA2D->BGPFCCR = PixelFormat;                     // Background PFC Control Register (Defines the destination pixel format)
  DMA2D->OPFCCR  = PixelFormat;                     // Output     PFC Control Register (Defines the output pixel format)
  DMA2D->NLR     = (U32)(xSize << 16) | (U16)ySize; // Number of Line Register (Size configuration of area to be transfered)
  _DMA_ExecOperation();
}

/*********************************************************************
*
*       _DMA_DrawBitmap
*/
static void _DMA_DrawBitmap(void * pDst, const void * pSrc, int xSize, int ySize, int OffLineSrc, int OffLineDst, int PixelFormatSrc, int PixelFormatDst) {
  DMA2D->CR      = 0x00010000UL | (1 << 9);         // Control Register (Memory-to-memory with PFC and TCIE)
  DMA2D->FGMAR   = (U32)pSrc;                       // Foreground Memory Address Register (Source address)
  DMA2D->BGMAR   = (U32)pDst;                       // Background Memory Address Register (Destination address)
  DMA2D->OMAR    = (U32)pDst;                       // Output Memory Address Register (Destination address)
  DMA2D->FGOR    = OffLineSrc;                      // Foreground Offset Register (Source line offset)
  DMA2D->BGOR    = OffLineDst;                      // Background Offset Register (Destination line offset)
  DMA2D->OOR     = OffLineDst;                      // Output Offset Register (Destination line offset)
  DMA2D->FGPFCCR = PixelFormatSrc;                  // Foreground PFC Control Register (Defines the input pixel format)
  DMA2D->OPFCCR  = PixelFormatDst;                  // Output     PFC Control Register (Defines the output pixel format)
  DMA2D->NLR     = (U32)(xSize << 16) | (U16)ySize; // Number of Line Register (Size configuration of area to be transfered)
  _DMA_ExecOperation();
}

/*********************************************************************
*
*       _DMA_Copy
*/
static void _DMA_Copy(int LayerIndex, const void * pSrc, void * pDst, int xSize, int ySize, int OffLineSrc, int OffLineDst) {
  U32 PixelFormat;

  PixelFormat    = _GetPixelformat(LayerIndex);
  DMA2D->CR      = 0x00000000UL | (1 << 9);         // Control Register (Memory to memory and TCIE)
  DMA2D->FGMAR   = (U32)pSrc;                       // Foreground Memory Address Register (Source address)
  DMA2D->OMAR    = (U32)pDst;                       // Output Memory Address Register (Destination address)
  DMA2D->FGOR    = OffLineSrc;                      // Foreground Offset Register (Source line offset)
  DMA2D->OOR     = OffLineDst;                      // Output Offset Register (Destination line offset)
  DMA2D->FGPFCCR = PixelFormat;                     // Foreground PFC Control Register (Defines the input pixel format)
  DMA2D->NLR     = (U32)(xSize << 16) | (U16)ySize; // Number of Line Register (Size configuration of area to be transfered)
  _DMA_ExecOperation();
}


/*********************************************************************
*
*       _DMA_DrawBitmapL8
*/
static void _DMA_DrawBitmapL8(void * pSrc, void * pDst,  U32 OffSrc, U32 OffDst, U32 PixelFormatDst, U32 xSize, U32 ySize) {
  //
  // Set up mode
  //
  DMA2D->CR      = 0x00010000UL | (1 << 9);         // Control Register (Memory to memory with pixel format conversion and TCIE)
  //
  // Set up pointers
  //
  DMA2D->FGMAR   = (U32)pSrc;                       // Foreground Memory Address Register (Source address)
  DMA2D->OMAR    = (U32)pDst;                       // Output Memory Address Register (Destination address)
  //
  // Set up offsets
  //
  DMA2D->FGOR    = OffSrc;                          // Foreground Offset Register (Source line offset)
  DMA2D->OOR     = OffDst;                          // Output Offset Register (Destination line offset)
  //
  // Set up pixel format
  //
  DMA2D->FGPFCCR = LTDC_PIXEL_FORMAT_L8;             // Foreground PFC Control Register (Defines the input pixel format)
  DMA2D->OPFCCR  = PixelFormatDst;                  // Output PFC Control Register (Defines the output pixel format)
  //
  // Set up size
  //
  DMA2D->NLR     = (U32)(xSize << 16) | ySize;      // Number of Line Register (Size configuration of area to be transfered)
  //
  // Execute operation
  //
  _DMA_ExecOperation();
}

/*********************************************************************
*
*       _DMA_DrawBitmapA4
*/
static int _DMA_DrawBitmapA4(void * pSrc, void * pDst,  U32 OffSrc, U32 OffDst, U32 PixelFormatDst, U32 xSize, U32 ySize) {
  U8 * pRD;
  U8 * pWR;
  U32 NumBytes, Color, Index;

  //
  // Check size of conversion buffer
  //
  NumBytes = (((xSize + 1) & ~1) * ySize) >> 1;
  if ((NumBytes > sizeof(_aBuffer)) || (NumBytes == 0)) {
    return 1;
  }
  //
  // Conversion (swapping nibbles)
  //
  pWR = (U8 *)_aBuffer;
  pRD = (U8 *)pSrc;
  do {
    *pWR++ = _aMirror[*pRD++];
  } while (--NumBytes);
  //
  // Get current drawing color (ABGR)
  //
  Index = LCD_GetColorIndex();
  Color = LCD_Index2Color(Index);
  //
  // Set up operation mode
  //
  DMA2D->CR = 0x00020000UL | (1 << 9);
  //
  // Set up source
  //
#if (GUI_USE_ARGB == 0)
  DMA2D->FGCOLR  = ((Color & 0xFF) << 16)  // Red
                 |  (Color & 0xFF00)       // Green
                 | ((Color >> 16) & 0xFF); // Blue
#else
  DMA2D->FGCOLR  = Color;
#endif
  DMA2D->FGMAR   = (U32)_aBuffer;
  DMA2D->FGOR    = 0;
  DMA2D->FGPFCCR = 0xA;                    // A4 bitmap
  DMA2D->NLR     = (U32)((xSize + OffSrc) << 16) | ySize;
  DMA2D->BGMAR   = (U32)pDst;
  DMA2D->BGOR    = OffDst - OffSrc;
  DMA2D->BGPFCCR = PixelFormatDst;
  DMA2D->OMAR    = DMA2D->BGMAR;
  DMA2D->OOR     = DMA2D->BGOR;
  DMA2D->OPFCCR  = DMA2D->BGPFCCR;
  //
  // Execute operation
  //
  _DMA_ExecOperation();
  return 0;
}

/*********************************************************************
*
*       _LCD_Reset
*/
static void _LCD_Reset(void) {
  GPIO_InitTypeDef  gpio_init_structure;
  
  __HAL_RCC_GPIOJ_CLK_ENABLE();
  //
  // Set up GPIO
  //
  gpio_init_structure.Pin   = GPIO_PIN_15;
  gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
  gpio_init_structure.Pull  = GPIO_PULLUP;
  gpio_init_structure.Speed = GPIO_SPEED_HIGH;
  
  HAL_GPIO_Init(GPIOJ, &gpio_init_structure);
  HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_15, GPIO_PIN_RESET);  
  GUI_X_Delay(20);
  HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_15, GPIO_PIN_SET);
  GUI_X_Delay(120);
    
}
     
/*********************************************************************
*
*       _LCD_Init
*/
static void _LCD_Init(void) {   
  DSI_PLLInitTypeDef dsiPllInit;

  static RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct;

  U32 LcdClock        = 27429; // LcdClk = 27429 kHz
  U32 Clockratio      = 0;
  U32 lcd_x_size      = 0;
  U32 lcd_y_size      = 0;
  U32 laneByteClk_kHz = 0;
  U32 VSA; 
  U32 VBP; 
  U32 VFP; 
  U32 VACT;
  U32 HSA; 
  U32 HBP; 
  U32 HFP; 
  U32 HACT;
  
  //
  // Reset display
  //
  _LCD_Reset();  
  //
  // Enable clocks
  //
  __HAL_RCC_LTDC_CLK_ENABLE();
  __HAL_RCC_LTDC_FORCE_RESET();
  __HAL_RCC_LTDC_RELEASE_RESET();
  __HAL_RCC_DMA2D_CLK_ENABLE();
  __HAL_RCC_DMA2D_FORCE_RESET();
  __HAL_RCC_DMA2D_RELEASE_RESET();
  __HAL_RCC_DSI_CLK_ENABLE();
  __HAL_RCC_DSI_FORCE_RESET();
  __HAL_RCC_DSI_RELEASE_RESET();
  //
  // Configure and enable VSYNC interrupt
  //
  HAL_NVIC_SetPriority(LTDC_IRQn, 0xE, 0);
  HAL_NVIC_EnableIRQ(LTDC_IRQn);
  //  
  // Init DSI
  //
  hdsi_disco.Instance = DSI;  
  HAL_DSI_DeInit(&(hdsi_disco));
  //
  // Set up DSI clock
  //
  dsiPllInit.PLLNDIV  = 100;
  dsiPllInit.PLLIDF   = DSI_PLL_IN_DIV5;
  dsiPllInit.PLLODF   = DSI_PLL_OUT_DIV1;
  laneByteClk_kHz     = 62500; // 500 MHz / 8 = 62.5 MHz = 62500 kHz
  //
  // Set two data lanes
  //
  hdsi_disco.Init.NumberOfLanes = DSI_TWO_DATA_LANES;
  hdsi_disco.Init.TXEscapeCkdiv = laneByteClk_kHz / 15620;   
  HAL_DSI_Init(&(hdsi_disco), &(dsiPllInit));
  Clockratio = laneByteClk_kHz/LcdClock;
  //
  // Get timing for the display
  //
  VSA        = OTM8009A_800X480_VSYNC;  // 12
  VBP        = OTM8009A_800X480_VBP;    // 12
  VFP        = OTM8009A_800X480_VFP;    // 12
  HSA        = OTM8009A_800X480_HSYNC;  // 120
  HBP        = OTM8009A_800X480_HBP;    // 120
  HFP        = OTM8009A_800X480_HFP;    // 120
  lcd_x_size = OTM8009A_800X480_WIDTH;  // 800
  lcd_y_size = OTM8009A_800X480_HEIGHT; // 480
  
  HACT = lcd_x_size;
  VACT = lcd_y_size;  
  
  hdsivideo_handle.VirtualChannelID             = 0;
  hdsivideo_handle.ColorCoding                  = 0;  // 565
  hdsivideo_handle.VSPolarity                   = DSI_VSYNC_ACTIVE_HIGH;
  hdsivideo_handle.HSPolarity                   = DSI_HSYNC_ACTIVE_HIGH;
  hdsivideo_handle.DEPolarity                   = DSI_DATA_ENABLE_ACTIVE_HIGH;  
  hdsivideo_handle.Mode                         = DSI_VID_MODE_BURST;
  hdsivideo_handle.NullPacketSize               = 0xFFF;
  hdsivideo_handle.NumberOfChunks               = 0;
  hdsivideo_handle.PacketSize                   = HACT;
  hdsivideo_handle.HorizontalSyncActive         = HSA*Clockratio;
  hdsivideo_handle.HorizontalBackPorch          = HBP*Clockratio;
  hdsivideo_handle.HorizontalLine               = (HACT + HSA + HBP + HFP) * Clockratio;
  hdsivideo_handle.VerticalSyncActive           = VSA;
  hdsivideo_handle.VerticalBackPorch            = VBP;
  hdsivideo_handle.VerticalFrontPorch           = VFP;
  hdsivideo_handle.VerticalActive               = VACT;
  hdsivideo_handle.LPCommandEnable              = DSI_LP_COMMAND_ENABLE; // Enable sending commands in mode LP (Low Power)
  hdsivideo_handle.LPLargestPacketSize          = 64;
  hdsivideo_handle.LPVACTLargestPacketSize      = 64;
  hdsivideo_handle.LPHorizontalFrontPorchEnable = DSI_LP_HFP_ENABLE;   // Allow sending LP commands during HFP period
  hdsivideo_handle.LPHorizontalBackPorchEnable  = DSI_LP_HBP_ENABLE;   // Allow sending LP commands during HBP period
  hdsivideo_handle.LPVerticalActiveEnable       = DSI_LP_VACT_ENABLE;  // Allow sending LP commands during VACT period
  hdsivideo_handle.LPVerticalFrontPorchEnable   = DSI_LP_VFP_ENABLE;   // Allow sending LP commands during VFP period
  hdsivideo_handle.LPVerticalBackPorchEnable    = DSI_LP_VBP_ENABLE;   // Allow sending LP commands during VBP period
  hdsivideo_handle.LPVerticalSyncActiveEnable   = DSI_LP_VSYNC_ENABLE; // Allow sending LP commands during VSync = VSA period
  //
  // Configure DSI video mode with parameter above
  //
  HAL_DSI_ConfigVideoMode(&(hdsi_disco), &(hdsivideo_handle));
  //
  // Start DSI
  //
  HAL_DSI_Start(&(hdsi_disco));
  //
  // Turn of LTDC to configure it
  //
  HAL_LTDC_DeInit(&hltdc_disco);
  //
  // Timing
  //
  hltdc_disco.Init.HorizontalSync     = (HSA - 1);
  hltdc_disco.Init.VerticalSync       =  VSA - 1;
  hltdc_disco.Init.AccumulatedHBP     = (HSA + HBP - 1);
  hltdc_disco.Init.AccumulatedVBP     =  VSA + VBP;
  hltdc_disco.Init.AccumulatedActiveH =  VSA + VBP + VACT;
  hltdc_disco.Init.AccumulatedActiveW = (lcd_x_size + HSA + HBP - 1);
  hltdc_disco.Init.TotalHeigh         =  VSA + VBP + VACT + VFP;
  hltdc_disco.Init.TotalWidth         = (lcd_x_size + HSA + HBP + HFP - 1);
  //
  // Set size
  //
  hltdc_disco.LayerCfg->ImageWidth  = lcd_x_size;
  hltdc_disco.LayerCfg->ImageHeight = lcd_y_size;   
  //
  // Configure clock
  //
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
  PeriphClkInitStruct.PLLSAI.PLLSAIN = 192;
  PeriphClkInitStruct.PLLSAI.PLLSAIR = 3;
  PeriphClkInitStruct.PLLSAIDivR     = RCC_PLLSAIDIVR_2;
  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct); 
  //
  // Set back ground color
  //
  hltdc_disco.Init.Backcolor.Blue  = 0;
  hltdc_disco.Init.Backcolor.Green = 0;
  hltdc_disco.Init.Backcolor.Red   = 0;
  //
  // Set polarity and instance handle
  //
  hltdc_disco.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
  hltdc_disco.Instance        = LTDC;
  //
  // Get values from DSI structure
  //
  HAL_LTDC_StructInitFromVideoConfig(&(hltdc_disco), &(hdsivideo_handle));
  //
  // Initialize and start LTDC
  //
  HAL_LTDC_Init(&hltdc_disco);
  //
  // Initialize OTM8009A controller (on display side)
  //
  _OTM8009A_Init(hdsivideo_handle.ColorCoding, OTM8009A_ORIENTATION_LANDSCAPE);
  //  
  // Set line where VSYNC interrupt should happen
  //
  HAL_LTDC_ProgramLineEvent(&hltdc_disco, 0);
  
  _DMA2D_ITConfig(DMA2D_CR_TCIE, ENABLE);
  _NVIC_SetPriority(DMA2D_IRQn, 0x00);
  _NVIC_EnableIRQ(DMA2D_IRQn);
}

/*********************************************************************
*
*       _LCD_LayerInit
*/
static void _LCD_LayerInit(U32 LayerIndex, U32 address) {   
  LTDC_LayerCfgTypeDef  Layercfg;
  //
  // Configure layer
  //
  Layercfg.PixelFormat     = _GetPixelformat(LayerIndex);
  Layercfg.FBStartAdress   = address;
  Layercfg.Alpha           = 255;
  Layercfg.Alpha0          = 0;
  Layercfg.Backcolor.Blue  = 0;
  Layercfg.Backcolor.Green = 0;
  Layercfg.Backcolor.Red   = 0;
  Layercfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  Layercfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
  if (LayerIndex == 0) {
    Layercfg.WindowX0        = 0;         
    Layercfg.WindowY0        = 0; 
    Layercfg.WindowX1        = XSIZE_PHYS0;
    Layercfg.WindowY1        = YSIZE_PHYS0;
    Layercfg.ImageWidth      = XSIZE_PHYS0;
    Layercfg.ImageHeight     = YSIZE_PHYS0;
  } else {
#if (NUM_LAYERS > 1)
    Layercfg.WindowX0        = XPOS_1;
    Layercfg.WindowY0        = YPOS_1;
    Layercfg.WindowX1        = XPOS_1 + XSIZE_PHYS1;
    Layercfg.WindowY1        = YPOS_1 + YSIZE_PHYS1;
    Layercfg.ImageWidth      = XSIZE_PHYS1;
    Layercfg.ImageHeight     = YSIZE_PHYS1;
#endif
  }
  
  HAL_LTDC_ConfigLayer(&hltdc_disco, &Layercfg, LayerIndex);
}

/*********************************************************************
*
*       _GetBufferSize
*/
static U32 _GetBufferSize(U32 LayerIndex) {
  return (layer_prop[LayerIndex].xSize * layer_prop[LayerIndex].ySize * layer_prop[LayerIndex].BytesPerPixel);
}

/*********************************************************************
*
*       _LCD_CopyBuffer
*/
static void _LCD_CopyBuffer(int LayerIndex, int IndexSrc, int IndexDst) {
  U32 BufferSize, AddrSrc, AddrDst;
  
  BufferSize = _GetBufferSize(LayerIndex);
  AddrSrc    = layer_prop[LayerIndex].address + BufferSize * IndexSrc;
  AddrDst    = layer_prop[LayerIndex].address + BufferSize * IndexDst;
  _DMA_Copy(LayerIndex, (void *)AddrSrc, (void *)AddrDst, layer_prop[LayerIndex].xSize, layer_prop[LayerIndex].ySize, 0, 0);
  layer_prop[LayerIndex].buffer_index = IndexDst;
}

/*********************************************************************
*
*       _LCD_CopyRect
*/
static void _LCD_CopyRect(int LayerIndex, int x0, int y0, int x1, int y1, int xSize, int ySize) {
  U32 BufferSize, AddrSrc, AddrDst;
  
  BufferSize = _GetBufferSize(LayerIndex);
  AddrSrc = layer_prop[LayerIndex].address + BufferSize * layer_prop[LayerIndex].pending_buffer + (y0 * layer_prop[LayerIndex].xSize + x0) * layer_prop[LayerIndex].BytesPerPixel;
  AddrDst = layer_prop[LayerIndex].address + BufferSize * layer_prop[LayerIndex].pending_buffer + (y1 * layer_prop[LayerIndex].xSize + x1) * layer_prop[LayerIndex].BytesPerPixel;
  _DMA_Copy(LayerIndex, (void *)AddrSrc, (void *)AddrDst, xSize, ySize, layer_prop[LayerIndex].xSize - xSize, 0);
}

/*********************************************************************
*
*       _LCD_FillRect
*/
static void _LCD_FillRect(int LayerIndex, int x0, int y0, int x1, int y1, U32 PixelIndex) {
  U32 BufferSize, AddrDst;
  int xSize, ySize;
  
  if (GUI_GetDrawMode() == GUI_DM_XOR) {		
    LCD_SetDevFunc(LayerIndex, LCD_DEVFUNC_FILLRECT, NULL);
    LCD_FillRect(x0, y0, x1, y1);
    LCD_SetDevFunc(LayerIndex, LCD_DEVFUNC_FILLRECT, (void(*)(void))_LCD_FillRect);
  } else {
    xSize = x1 - x0 + 1;
    ySize = y1 - y0 + 1;
    BufferSize = _GetBufferSize(LayerIndex);
    AddrDst = layer_prop[LayerIndex].address + BufferSize * layer_prop[LayerIndex].buffer_index + (y0 * layer_prop[LayerIndex].xSize + x0) * layer_prop[LayerIndex].BytesPerPixel;
    _DMA_Fill(LayerIndex, (void *)AddrDst, xSize, ySize, layer_prop[LayerIndex].xSize - xSize, PixelIndex);
  }	
}

/*********************************************************************
*
*       _LCD_DrawBitmapM565
*/
static void _LCD_DrawBitmapM565(int LayerIndex, int x, int y, const void * p, int xSize, int ySize, int BytesPerLine) {
  U32 BufferSize, AddrDst;
  int OffLineSrc, OffLineDst;
  U32 PixelFormatDst;

  PixelFormatDst = _GetPixelformat(LayerIndex);
  BufferSize = _GetBufferSize(LayerIndex);
  AddrDst    = layer_prop[LayerIndex].address + BufferSize * layer_prop[LayerIndex].buffer_index + (y * layer_prop[LayerIndex].xSize + x) * layer_prop[LayerIndex].BytesPerPixel;
  OffLineSrc = (BytesPerLine / 2) - xSize;
  OffLineDst = layer_prop[LayerIndex].xSize - xSize;
  _DMA_DrawBitmap((void *)AddrDst, p, xSize, ySize, OffLineSrc, OffLineDst, LTDC_PIXEL_FORMAT_RGB565, PixelFormatDst);
}

/*********************************************************************
*
*       _LCD_DrawBitmapAlpha
*/
static void _LCD_DrawBitmapAlpha(int LayerIndex, int x, int y, void const * p, int xSize, int ySize, int BytesPerLine) {
  U32 BufferSize, AddrDst;
  int OffLineSrc, OffLineDst;
  U32 PixelFormat;

  PixelFormat = _GetPixelformat(LayerIndex);
  BufferSize = _GetBufferSize(LayerIndex);
  AddrDst = layer_prop[LayerIndex].address + BufferSize * layer_prop[LayerIndex].buffer_index + (y * layer_prop[LayerIndex].xSize + x) * layer_prop[LayerIndex].BytesPerPixel;
  OffLineSrc = (BytesPerLine / 4) - xSize;
  OffLineDst = layer_prop[LayerIndex].xSize - xSize;
  _DMA_DrawAlphaBitmap((void *)AddrDst, p, xSize, ySize, OffLineSrc, OffLineDst, PixelFormat);
}

/*********************************************************************
*
*       _LCD_DrawBitmap32bpp
*/
static void _LCD_DrawBitmap32bpp(int LayerIndex, int x, int y, U16 const * p, int xSize, int ySize, int BytesPerLine) {
  U32 BufferSize, AddrDst;
  int OffLineSrc, OffLineDst;

  BufferSize = _GetBufferSize(LayerIndex);
  AddrDst = layer_prop[LayerIndex].address + BufferSize * layer_prop[LayerIndex].buffer_index + (y * layer_prop[LayerIndex].xSize + x) * layer_prop[LayerIndex].BytesPerPixel;
  OffLineSrc = (BytesPerLine / 4) - xSize;
  OffLineDst = layer_prop[LayerIndex].xSize - xSize;
  _DMA_Copy(LayerIndex, (void *)p, (void *)AddrDst, xSize, ySize, OffLineSrc, OffLineDst);
}

/*********************************************************************
*
*       _LCD_DrawBitmap16bpp
*/
static void _LCD_DrawBitmap16bpp(int LayerIndex, int x, int y, U16 const * p, int xSize, int ySize, int BytesPerLine) {
  U32 BufferSize, AddrDst;
  int OffLineSrc, OffLineDst;

  BufferSize = _GetBufferSize(LayerIndex);
  AddrDst = layer_prop[LayerIndex].address + BufferSize * layer_prop[LayerIndex].buffer_index + (y * layer_prop[LayerIndex].xSize + x) * layer_prop[LayerIndex].BytesPerPixel;
  OffLineSrc = (BytesPerLine / 2) - xSize;
  OffLineDst = layer_prop[LayerIndex].xSize - xSize;
  _DMA_Copy(LayerIndex, (void *)p, (void *)AddrDst, xSize, ySize, OffLineSrc, OffLineDst);
}

/*********************************************************************
*
*       _LCD_DrawBitmap8bpp
*/
static void _LCD_DrawBitmap8bpp(int LayerIndex, int x, int y, U8 const * p, int xSize, int ySize, int BytesPerLine) {
  U32 BufferSize, AddrDst;
  int OffLineSrc, OffLineDst;
  U32 PixelFormat;

  PixelFormat = _GetPixelformat(LayerIndex);
  BufferSize = _GetBufferSize(LayerIndex);
  AddrDst = layer_prop[LayerIndex].address + BufferSize * layer_prop[LayerIndex].buffer_index + (y * layer_prop[LayerIndex].xSize + x) * layer_prop[LayerIndex].BytesPerPixel;
  OffLineSrc = BytesPerLine - xSize;
  OffLineDst = layer_prop[LayerIndex].xSize - xSize;
  _DMA_DrawBitmapL8((void *)p, (void *)AddrDst, OffLineSrc, OffLineDst, PixelFormat, xSize, ySize);
}

/*********************************************************************
*
*       _LCD_DrawBitmap4bpp
*/
static int _LCD_DrawBitmap4bpp(int LayerIndex, int x, int y, U8 const * p, int xSize, int ySize, int BytesPerLine) {
  U32 BufferSize, AddrDst;
  int OffLineSrc, OffLineDst;
  U32 PixelFormat;
  const GUI_RECT * pRect;

  pRect = GUI_GetClipRect();
  if (x < pRect->x0) {
    return 1;
  }
  if ((x + xSize) >= pRect->x1) {
    return 1;
  }
  if (y < pRect->y0) {
    return 1;
  }
  if ((y + ySize) >= pRect->y1) {
    return 1;
  }
  PixelFormat = _GetPixelformat(LayerIndex);
  //
  // Check if destination has direct color mode
  //
  if (PixelFormat > LTDC_PIXEL_FORMAT_ARGB4444) {
    return 1;
  }
  BufferSize = _GetBufferSize(LayerIndex);
  AddrDst = layer_prop[LayerIndex].address + BufferSize * layer_prop[LayerIndex].buffer_index + (y * layer_prop[LayerIndex].xSize + x) * layer_prop[LayerIndex].BytesPerPixel;
  OffLineSrc = (BytesPerLine * 2) - xSize;
  OffLineDst = layer_prop[LayerIndex].xSize - xSize;
  return _DMA_DrawBitmapA4((void *)p, (void *)AddrDst, OffLineSrc, OffLineDst, PixelFormat, xSize, ySize);;
}

/*********************************************************************
*
*       _LCD_DrawMemdevAlpha
*/
static void _LCD_DrawMemdevAlpha(void * pDst, const void * pSrc, int xSize, int ySize, int BytesPerLineDst, int BytesPerLineSrc) {
  int OffLineSrc, OffLineDst;

  OffLineSrc = (BytesPerLineSrc / 4) - xSize;
  OffLineDst = (BytesPerLineDst / 4) - xSize;
  _DMA_DrawAlphaBitmap(pDst, pSrc, xSize, ySize, OffLineSrc, OffLineDst, LTDC_PIXEL_FORMAT_ARGB8888);
}

/*********************************************************************
*
*       _LCD_DrawMemdevM565
*
* Purpose:
*   Copy data with conversion
*/
static void _LCD_DrawMemdevM565(void * pDst, const void * pSrc, int xSize, int ySize, int BytesPerLineDst, int BytesPerLineSrc) {
  int OffLineSrc, OffLineDst;

  OffLineSrc = (BytesPerLineSrc / 2) - xSize;
  OffLineDst = (BytesPerLineDst / 4) - xSize;
  _DMA_DrawBitmap(pDst, pSrc, xSize, ySize, OffLineSrc, OffLineDst, LTDC_PIXEL_FORMAT_RGB565, LTDC_PIXEL_FORMAT_ARGB8888);
}

#if GUI_MEMDEV_SUPPORT_CUSTOMDRAW

/*********************************************************************
*
*       _DMA_CopyRGB565
*/
static void _DMA_CopyRGB565(const void * pSrc, void * pDst, int xSize, int ySize, int OffLineSrc, int OffLineDst) {
  DMA2D->CR      = 0x00000000UL | (1 << 9);         // Control Register (Memory to memory and TCIE)
  DMA2D->FGMAR   = (U32)pSrc;                       // Foreground Memory Address Register (Source address)
  DMA2D->OMAR    = (U32)pDst;                       // Output Memory Address Register (Destination address)
  DMA2D->FGOR    = OffLineSrc;                      // Foreground Offset Register (Source line offset)
  DMA2D->OOR     = OffLineDst;                      // Output Offset Register (Destination line offset)
  DMA2D->FGPFCCR = LTDC_PIXEL_FORMAT_RGB565;        // Foreground PFC Control Register (Defines the input pixel format)
  DMA2D->NLR     = (U32)(xSize << 16) | (U16)ySize; // Number of Line Register (Size configuration of area to be transfered)
  _DMA_ExecOperation();
}

/*********************************************************************
*
*       _LCD_DrawMemdev16bpp
*/
static void _LCD_DrawMemdev16bpp(void * pDst, const void * pSrc, int xSize, int ySize, int BytesPerLineDst, int BytesPerLineSrc) {
  int OffLineSrc, OffLineDst;

  OffLineSrc = (BytesPerLineSrc / 2) - xSize;
  OffLineDst = (BytesPerLineDst / 2) - xSize;
  _DMA_CopyRGB565(pSrc, pDst, xSize, ySize, OffLineSrc, OffLineDst);
}
#endif

/*********************************************************************
*
*       _DMA_MixColors
*
* Purpose:
*   Function for mixing up 2 colors with the given intensity.
*   If the background color is completely transparent the
*   foreground color should be used unchanged.
*/
static LCD_COLOR _DMA_MixColors(LCD_COLOR Color, LCD_COLOR BkColor, U8 Intens) {
  volatile U32 ColorDst;

#if (GUI_USE_ARGB == 0)
  Color   ^= 0xFF000000;
  BkColor ^= 0xFF000000;
#endif
  //
  // Set up mode
  //
  DMA2D->CR      = 0x00020000UL | (1 << 9);         // Control Register (Memory to memory with blending of FG and BG and TCIE)
  //
  // Set up pointers
  //
  DMA2D->FGMAR   = (U32)&Color;                   // Foreground Memory Address Register
  DMA2D->BGMAR   = (U32)&BkColor;                   // Background Memory Address Register
  DMA2D->OMAR    = (U32)&ColorDst;                  // Output Memory Address Register (Destination address)
  //
  // Set up pixel format
  //
  DMA2D->FGPFCCR = LTDC_PIXEL_FORMAT_ARGB8888
                 | (1UL << 16)
                 | ((U32)Intens << 24);
  DMA2D->BGPFCCR = LTDC_PIXEL_FORMAT_ARGB8888
                 | (0UL << 16)
                 | ((U32)(255 - Intens) << 24);
  DMA2D->OPFCCR  = LTDC_PIXEL_FORMAT_ARGB8888;
  //
  // Set up size
  //
  DMA2D->NLR     = (U32)(1 << 16) | 1;              // Number of Line Register (Size configuration of area to be transfered)
  //
  // Execute operation
  //
  _DMA_ExecOperation();
#if (GUI_USE_ARGB == 0)
  ColorDst ^= 0xFF000000;
#endif
  return ColorDst;
}

/*********************************************************************
*
*       HAL_LTDC_LineEvenCallback
*/
void HAL_LTDC_LineEvenCallback(LTDC_HandleTypeDef *hltdc) {
  
  U32 Addr;
  U32 layer;

  for (layer = 0; layer < NUM_LAYERS; layer++) {
    if (layer_prop[layer].pending_buffer >= 0) {
      //
      // Depending on the buffer to be shown, calculate a new buffer address
      //
      Addr = layer_prop[layer].address + layer_prop[layer].xSize * layer_prop[layer].ySize * layer_prop[layer].pending_buffer * layer_prop[layer].BytesPerPixel;      
      //
      // Set address
      //
      __HAL_LTDC_LAYER(hltdc, layer)->CFBAR = Addr;     
      //
      // Reload config
      //
      __HAL_LTDC_RELOAD_CONFIG(hltdc);
      //
      // Confirm buffer change
      //
      GUI_MULTIBUF_ConfirmEx(layer, layer_prop[layer].pending_buffer);
      //
      // Reset bending buffer flag
      //
      layer_prop[layer].pending_buffer = -1;
    }
  }
  HAL_LTDC_ProgramLineEvent(hltdc, 0);
}

/*********************************************************************
*
*       LCD_TFT_IRQHandler
*/
void LCD_TFT_IRQHandler(void);
void LCD_TFT_IRQHandler(void) {
  //
  // Call HAL IRW handler
  //
  HAL_LTDC_IRQHandler(&hltdc_disco);
}

/*********************************************************************
*
*       DMA2D_IRQHandler
*
* Purpose:
*   Transfer-complete-interrupt of DMA2D
*/
void DMA2D_IRQHandler(void);
void DMA2D_IRQHandler(void) {
  if (DMA2D->ISR & DMA2D_ISR_TCIF) {
    _WaitForDMA2D = 0;
    DMA2D->IFCR |= (U32)DMA2D_IFSR_CTCIF;
  } else {
    _WaitForDMA2D = 0;
  }
}

/*********************************************************************
*
*       LCD_X_Config
*/
void LCD_X_Config(void) {
  U32 i;
  //
  // Configure multibuffering
  //
#if (NUM_BUFFERS > 1)
  for (i = 0; i < NUM_LAYERS; i++) {
    GUI_MULTIBUF_ConfigEx(i, NUM_BUFFERS);
  }
#endif  
  //
  // Create display driver for layer 0 and link it into the device chain
  //
  GUI_DEVICE_CreateAndLink(DISPLAY_DRIVER_0, COLOR_CONVERSION_0, 0, 0);
  //
  // Set size of layer 0
  //
  if (LCD_GetSwapXYEx(0)) {
    LCD_SetSizeEx (0, YSIZE_PHYS0, XSIZE_PHYS0);
    LCD_SetVSizeEx(0, YSIZE_PHYS0 * NUM_VSCREENS, XSIZE_PHYS0);
  } else {
    LCD_SetSizeEx (0, XSIZE_PHYS0, YSIZE_PHYS0);
    LCD_SetVSizeEx(0, XSIZE_PHYS0, YSIZE_PHYS0 * NUM_VSCREENS);
  }
#if (NUM_LAYERS > 1)
  //
  // Create display driver for layer 1 and link it into the device chain
  //
  GUI_DEVICE_CreateAndLink(DISPLAY_DRIVER_1, COLOR_CONVERSION_1, 0, 1);
  //
  // Set size of layer 1
  //
  if (LCD_GetSwapXYEx(1)) {
    LCD_SetSizeEx (1, YSIZE_PHYS1, XSIZE_PHYS1);
    LCD_SetVSizeEx(1, YSIZE_PHYS1 * NUM_VSCREENS, XSIZE_PHYS1);
    LCD_SetPosEx(1, YPOS_1, XPOS_1);
  } else {
    LCD_SetSizeEx (1, XSIZE_PHYS1, YSIZE_PHYS1);
    LCD_SetVSizeEx(1, XSIZE_PHYS1, YSIZE_PHYS1 * NUM_VSCREENS);
    LCD_SetPosEx(1, XPOS_1, YPOS_1);
  }
#endif
  //
  // Init layer struct for layer 0
  //
  layer_prop[0].address = LCD_LAYER0_FRAME_BUFFER;
#if (NUM_LAYERS > 1)    
  //
  // Init layer struct for layer 1
  //
  layer_prop[1].address = LCD_LAYER1_FRAME_BUFFER; 
#endif
  //
  // Set up custom functions
  //
  for (i = 0; i < NUM_LAYERS; i++) {    
    layer_prop[i].pColorConvAPI  = (LCD_API_COLOR_CONV *)_apColorConvAPI[i];    
    layer_prop[i].pending_buffer = -1;    
    layer_prop[i].BytesPerPixel  = LCD_GetBitsPerPixelEx(i) >> 3;
    LCD_SetDevFunc(i, LCD_DEVFUNC_COPYBUFFER,    (void(*)(void))_LCD_CopyBuffer);
    LCD_SetDevFunc(i, LCD_DEVFUNC_COPYRECT,      (void(*)(void))_LCD_CopyRect);
    LCD_SetDevFunc(i, LCD_DEVFUNC_FILLRECT,      (void(*)(void))_LCD_FillRect);
    LCD_SetDevFunc(i, LCD_DEVFUNC_DRAWBMP_8BPP,  (void(*)(void))_LCD_DrawBitmap8bpp);
    LCD_SetDevFunc(i, LCD_DEVFUNC_DRAWBMP_16BPP, (void(*)(void))_LCD_DrawBitmap16bpp);  
    LCD_SetDevFunc(i, LCD_DEVFUNC_DRAWBMP_32BPP, (void(*)(void))_LCD_DrawBitmap32bpp); 
    //
    // Set VRAM address
    //
    LCD_SetVRAMAddrEx(i, (void *)(layer_prop[i].address));
  } 
#if GUI_MEMDEV_SUPPORT_CUSTOMDRAW
  GUI_MEMDEV_SetDrawMemdev16bppFunc(_LCD_DrawMemdev16bpp);
#endif
  GUI_SetFuncMixColors(_DMA_MixColors);
  GUI_AA_SetpfDrawCharAA4(_LCD_DrawBitmap4bpp);
  GUI_SetFuncDrawAlpha(_LCD_DrawMemdevAlpha, _LCD_DrawBitmapAlpha);
  GUI_SetFuncDrawM565(_LCD_DrawMemdevM565, _LCD_DrawBitmapM565);
}

/*********************************************************************
*
*       LCD_X_DisplayDriver
*/
int LCD_X_DisplayDriver(unsigned LayerIndex, unsigned Cmd, void * pData) {
  int r = 0;
  U32 addr;
  U32 Color;
  int xPos;
  int yPos;
  
  LCD_X_SHOWBUFFER_INFO * p;  
  switch (Cmd) 
  {   
  case LCD_X_INITCONTROLLER:
    if (LayerIndex == 0) {
      _LCD_Init ();
      _LCD_LayerInit(0, LCD_LAYER0_FRAME_BUFFER);
#if NUM_LAYERS == 2
      _LCD_LayerInit(1, LCD_LAYER1_FRAME_BUFFER);
#endif
    }
    break;
  case LCD_X_SETORG: 
    addr = layer_prop[LayerIndex].address + ((LCD_X_SETORG_INFO *)pData)->yPos * layer_prop[LayerIndex].xSize * layer_prop[LayerIndex].BytesPerPixel;
    HAL_LTDC_SetAddress(&hltdc_disco, addr, LayerIndex);
    break;    
  case LCD_X_SHOWBUFFER:
    p = (LCD_X_SHOWBUFFER_INFO *)pData;
    layer_prop[LayerIndex].pending_buffer = p->Index;
    break;    
  case LCD_X_ON: 
    __HAL_LTDC_ENABLE(&hltdc_disco);
    break;    
  case LCD_X_OFF: 
    __HAL_LTDC_DISABLE(&hltdc_disco);
    break;    
  case LCD_X_SETVIS:
    if(((LCD_X_SETVIS_INFO *)pData)->OnOff  == ENABLE) {
      __HAL_LTDC_LAYER_ENABLE(&hltdc_disco, LayerIndex); 
    } else {
      __HAL_LTDC_LAYER_DISABLE(&hltdc_disco, LayerIndex); 
    }
    __HAL_LTDC_RELOAD_CONFIG(&hltdc_disco);
    break;    
  case LCD_X_SETPOS:
    #if 0
    {
      //
      // Required for setting the layer position which is passed in the 'xPos' and 'yPos' element of pData
      //
      LCD_X_SETPOS_INFO * p;

      p = (LCD_X_SETPOS_INFO *)pData;
      _LTDC_SetLayerPosEx(LayerIndex, p);
      break;
    }
   #else
    HAL_LTDC_SetWindowPosition(&hltdc_disco, ((LCD_X_SETPOS_INFO *)pData)->xPos, ((LCD_X_SETPOS_INFO *)pData)->yPos, LayerIndex);
    #endif
    break;    
  case LCD_X_SETSIZE:
    layer_prop[LayerIndex].xSize = ((LCD_X_SETSIZE_INFO *)pData)->xSize;
    layer_prop[LayerIndex].ySize = ((LCD_X_SETSIZE_INFO *)pData)->ySize;
    HAL_LTDC_SetWindowSize(&hltdc_disco, layer_prop[LayerIndex].xSize, layer_prop[LayerIndex].ySize, LayerIndex);
    GUI_GetLayerPosEx(LayerIndex, &xPos, &yPos);
    HAL_LTDC_SetWindowPosition(&hltdc_disco, xPos, yPos, LayerIndex);
    break;
  case LCD_X_SETALPHA:
    HAL_LTDC_SetAlpha(&hltdc_disco, ((LCD_X_SETALPHA_INFO *)pData)->Alpha, LayerIndex);
    break;    
  case LCD_X_SETCHROMAMODE:
    if(((LCD_X_SETCHROMAMODE_INFO *)pData)->ChromaMode != 0) {
      HAL_LTDC_EnableColorKeying(&hltdc_disco, LayerIndex);
    } else {
      HAL_LTDC_DisableColorKeying(&hltdc_disco, LayerIndex);      
    }
    break;    
  case LCD_X_SETCHROMA:    
    Color = ((((LCD_X_SETCHROMA_INFO *)pData)->ChromaMin & 0xFF0000) >> 16) |\
            ((( LCD_X_SETCHROMA_INFO *)pData)->ChromaMin & 0x00FF00)        |\
            ((((LCD_X_SETCHROMA_INFO *)pData)->ChromaMin & 0x0000FF) << 16);    
    HAL_LTDC_ConfigColorKeying(&hltdc_disco, Color, LayerIndex);
    break;    
  default:
    r = -1;
  }
  return r;
}

/*************************** End of file ****************************/

