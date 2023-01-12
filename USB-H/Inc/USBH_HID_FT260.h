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
File        : USBH_HID_FT260.h
Purpose     : API of the USB host stack
-------------------------- END-OF-HEADER -----------------------------
*/

#ifndef USBH_HID_FT260_H__
#define USBH_HID_FT260_H__

#include "USBH.h"
#include "USBH_Util.h"
#include "SEGGER.h"

#if defined(__cplusplus)
  extern "C" {                 // Make sure we have C-declarations in C++ programs
#endif

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

/* I2C Master Controller Status
  *   bit 0 = controller busy: all other status bits invalid
  *   bit 1 = error condition
  *   bit 2 = slave address was not acknowledged during last operation
  *   bit 3 = data not acknowledged during last operation
  *   bit 4 = arbitration lost during last operation
  *   bit 5 = controller idle
  *   bit 6 = bus busy
  */
#define USBH_FT260_I2CM_STATUS_CONTROLLER_BUSY   (0x01u)
#define USBH_FT260_I2CM_STATUS_ERROR_CONDITION   (0x02u)
#define USBH_FT260_I2CM_STATUS_SLAVE_NACK        (0x04u)
#define USBH_FT260_I2CM_STATUS_DATA_NACK         (0x08u)
#define USBH_FT260_I2CM_STATUS_ARBITRATION_LOST  (0x10u)
#define USBH_FT260_I2CM_STATUS_CONTROLLER_IDLE   (0x20u)
#define USBH_FT260_I2CM_STATUS_BUS_BUSY          (0x40u)

typedef enum {
  USBH_FT260_GPIO2_GPIO = 0,
  USBH_FT260_GPIO2_SUSPOUT = 1,
  USBH_FT260_GPIO2_PWREN = 2,
  USBH_FT260_GPIO2_TX_LED = 4
} USBH_FT260_GPIO2_PIN;

typedef enum  {
  USBH_FT260_GPIOA_GPIO = 0,
  USBH_FT260_GPIOA_TX_ACTIVE = 3,
  USBH_FT260_GPIOA_TX_LED = 4
} USBH_FT260_GPIOA_PIN;

typedef enum  {
  USBH_FT260_GPIOG_GPIO = 0,
  USBH_FT260_GPIOG_PWREN = 2,
  USBH_FT260_GPIOG_RX_LED = 5,
  USBH_FT260_GPIOG_BCD_DET = 6
} USBH_FT260_GPIOG_PIN;

typedef enum {
  USBH_FT260_SYS_CLK_12M = 0,
  USBH_FT260_SYS_CLK_24M,
  USBH_FT260_SYS_CLK_48M
} USBH_FT260_CLOCK_RATE;

typedef enum {
  USBH_FT260_INTR_RISING_EDGE = 0,
  USBH_FT260_INTR_LEVEL_HIGH,
  USBH_FT260_INTR_FALLING_EDGE,
  USBH_FT260_INTR_LEVEL_LOW
} USBH_FT260_INTERRUPT_TRIGGER_TYPE;

typedef enum {
  USBH_FT260_INTR_DELY_1MS = 1,
  USBH_FT260_INTR_DELY_5MS,
  USBH_FT260_INTR_DELY_30MS
} USBH_FT260_INTERRUPT_LEVEL_TIME_DELAY;

typedef enum {
  USBH_FT260_SUSPEND_OUT_LEVEL_HIGH = 0,
  USBH_FT260_SUSPEND_OUT_LEVEL_LOW
} USBH_FT260_SUSPEND_OUT_POLARITY;

typedef enum {
  USBH_FT260_UART_OFF = 0,
  USBH_FT260_UART_RTS_CTS_MODE,        // hardware flow control RTS, CTS mode
  USBH_FT260_UART_DTR_DSR_MODE,        // hardware flow control DTR, DSR mode
  USBH_FT260_UART_XON_XOFF_MODE,       // software flow control mode
  USBH_FT260_UART_NO_FLOW_CTRL_MODE    // no flow control mode
} USBH_FT260_UART_MODE;

typedef enum {
  USBH_FT260_DATA_BIT_7 = 7,
  USBH_FT260_DATA_BIT_8 = 8
} USBH_FT260_DATA_BIT;

typedef enum {
  USBH_FT260_STOP_BITS_1 = 0,
  USBH_FT260_STOP_BITS_2 = 2
} USBH_FT260_STOP_BIT;

typedef enum  {
  USBH_FT260_PARITY_NONE = 0,
  USBH_FT260_PARITY_ODD,
  USBH_FT260_PARITY_EVEN,
  USBH_FT260_PARITY_MARK,
  USBH_FT260_PARITY_SPACE
} USBH_FT260_PARITY;

typedef enum {
  USBH_FT260_RI_WAKEUP_RISING_EDGE = 0,
  USBH_FT260_RI_WAKEUP_FALLING_EDGE
} USBH_FT260_RI_WAKEUP_TYPE;

typedef enum {
  FT260_GPIO_IN = 0,
  FT260_GPIO_OUT
} USBH_FT260_GPIO_DIR;

typedef enum  {
  USBH_FT260_GPIO_0 = 1uL << 0,
  USBH_FT260_GPIO_1 = 1uL << 1,
  USBH_FT260_GPIO_2 = 1uL << 2,
  USBH_FT260_GPIO_3 = 1uL << 3,
  USBH_FT260_GPIO_4 = 1uL << 4,
  USBH_FT260_GPIO_5 = 1uL << 5,
  USBH_FT260_GPIO_A = 1uL << 6,
  USBH_FT260_GPIO_B = 1uL << 7,
  USBH_FT260_GPIO_C = 1uL << 8,
  USBH_FT260_GPIO_D = 1uL << 9,
  USBH_FT260_GPIO_E = 1uL << 10,
  USBH_FT260_GPIO_F = 1uL << 11,
  USBH_FT260_GPIO_G = 1uL << 12,
  USBH_FT260_GPIO_H = 1uL << 13
} USBH_FT260_GPIO;

typedef enum {
  USBH_FT260_I2C_NONE = 0,
  USBH_FT260_I2C_START = 0x02,
  USBH_FT260_I2C_REPEATED_START = 0x03,
  USBH_FT260_I2C_STOP = 0x04,
  USBH_FT260_I2C_START_AND_STOP = 0x06
} USBH_FT260_I2C_FLAG;

/*********************************************************************
*
*       USBH_FT260_UART_CONFIG
*
*  Description
*    This structure used to retrieve the current UART status of the chip
*/
typedef struct {
  U8  FlowCtrl;           // Specifies the set flow mode control. The following modes are available:
                          //  + 0x00 : OFF, and switch UART pins to GPIO
                          //  + 0x01 : RTS_CTS mode(GPIOB = > RTSN, GPIOE = > CTSN)
                          //  + 0x02 : DTR_DSR mode(GPIOF = > DTRN, GPIOH = > DSRN)
                          //  + 0x03 : XON_XOFF(software flow control)
                          //  + 0x04 : No flow control mode
  U32 BaudRate;           // UART baud rate
                          // The FT260 supports baud rate range from 1200 to 12M.
  U8  DataBit;            // The current data bit mode:
                          //  + 0x07: 7 data bits
                          //  + 0x08: 8 data bits
  U8  Parity;             // The parity that is used by the UART module:
                          //  + 0x00: No parity
                          //  + 0x01: Odd parity. This means that the parity bit is set to either '1' or '0' so that an odd number of 1's are sent.
                          //  + 0x02: Even parity.This means that the parity bit is set to either '1' or '0' so that an even number of 1's are sent.
                          //  + 0x03: Mark parity.This simply means that the parity bit is always High.
                          //  + 0x04: Space parity.This simply means that the parity bit is always Low.
  U8  StopBit;            // The number of stop bits that are used:
                          //  + 0x00: one stop bit
                          //  + 0x02: two stop bits
  U8  Breaking;           // When active the TXD line goes into "spacing" state which causes a  break in the receiving UART.
                          //  + 0x00 : no break
                          //  + 0x02 : break
} USBH_FT260_UART_CONFIG;

/*********************************************************************
*
*       USBH_FT260_GPIO_REPORT
*
*  Description
*    This structure is used to retrieve or to set the status of the
*    various GPIO pins of the FT260 chip.
*    Please refer to the data sheet of the FT260 to check the GPIO capability.
*/
typedef struct {
  U8 Value;       // GPIO[0..5] Pin state
  U8 Dir;         // GPIO[0..5] Pin direction whereas:
                  //  + 1 : Output
                  //  + 0 : Input
  U8 GpioN_Value; // GPIO[A..H] Pin state
  U8 GpioN_Dir;   // GPIO[A..H] Pin direction whereas:
                  //  + 1 : Output
                  //  + 0 : Input
} USBH_FT260_GPIO_REPORT;


/*********************************************************************
*
*       USBH_FT260_VID_PID_PAIR
*
*  Description
*    This structure is used to add a FT260 device that has a
*    different Vendor and Product Id stored in EEPROM.
*    Only the VendorId and ProductId members needs to be filled.
*    Anything else is handled by the FT260 module.
*    Note: Do not modify these values after the
*          USBH_FT260_AddSupportedItem() was called.
*/
typedef struct {
  USBH_DLIST    ListEntry;  // Internal use
  U16           VendorId;   // Identifier for the vendor
  U16           ProductId;  // Identifier for the product
#if USBH_DEBUG > 1
  U32           Magic;      // Internal use
#endif
} USBH_FT260_VID_PID_PAIR;

void              USBH_FT260_Init                       (void);
void              USBH_FT260_AddSupportItem             (USBH_FT260_VID_PID_PAIR * pItem);
void              USBH_FT260_RemoveSupportItem          (USBH_FT260_VID_PID_PAIR * pItem);
USBH_STATUS       USBH_FT260_AddNotification            (USBH_NOTIFICATION_HOOK * pHook, USBH_NOTIFICATION_FUNC * pfNotification, void * pContext);
USBH_STATUS       USBH_FT260_RemoveNotification         (const USBH_NOTIFICATION_HOOK * pHook);
// FT260 General Functions
USBH_HID_HANDLE   USBH_FT260_Open                       (unsigned DevIndex);
USBH_STATUS       USBH_FT260_Close                      (USBH_HID_HANDLE hDevice);

USBH_STATUS       USBH_FT260_SetClock                   (USBH_HID_HANDLE hDevice, USBH_FT260_CLOCK_RATE ClkRate);
USBH_STATUS       USBH_FT260_SetWakeupInterrupt         (USBH_HID_HANDLE hDevice, unsigned int Enable);
USBH_STATUS       USBH_FT260_SetInterruptTriggerType    (USBH_HID_HANDLE hDevice, USBH_FT260_INTERRUPT_TRIGGER_TYPE Type, USBH_FT260_INTERRUPT_LEVEL_TIME_DELAY Delay);
USBH_STATUS       USBH_FT260_SelectGpio2Function        (USBH_HID_HANDLE hDevice, USBH_FT260_GPIO2_PIN Gpio2Function);
USBH_STATUS       USBH_FT260_SelectGpioAFunction        (USBH_HID_HANDLE hDevice, USBH_FT260_GPIOA_PIN GpioAFunction);
USBH_STATUS       USBH_FT260_SelectGpioGFunction        (USBH_HID_HANDLE hDevice, USBH_FT260_GPIOG_PIN GpioGFunction);
USBH_STATUS       USBH_FT260_SetSuspendOutPolarity      (USBH_HID_HANDLE hDevice, USBH_FT260_SUSPEND_OUT_POLARITY Polarity);

USBH_STATUS       USBH_FT260_GetChipVersion             (USBH_HID_HANDLE hDevice, U32 * pChipVersion);

USBH_STATUS       USBH_FT260_EnableI2CPin               (USBH_HID_HANDLE hDevice, unsigned int Enable);
USBH_STATUS       USBH_FT260_SetUartToGPIOPin           (USBH_HID_HANDLE hDevice);
USBH_STATUS       USBH_FT260_EnableDcdRiPin             (USBH_HID_HANDLE hDevice, unsigned int Enable);

// I2C Functions
USBH_STATUS       USBH_FT260_I2CMaster_Init             (USBH_HID_HANDLE hDevice, U16 kbps);
USBH_STATUS       USBH_FT260_I2CMaster_Read             (USBH_HID_HANDLE hDevice, U8 DeviceAddress, USBH_FT260_I2C_FLAG I2cFlag, void * pBuffer, U32 NumBytesToRead,  U32 * pNumBytesReturned);
USBH_STATUS       USBH_FT260_I2CMaster_Write            (USBH_HID_HANDLE hDevice, U8 DeviceAddress, USBH_FT260_I2C_FLAG I2cFlag, void * pBuffer, U32 NumBytesToWrite, U32 * pNumBytesWritten);
USBH_STATUS       USBH_FT260_I2CMaster_ReadReg          (USBH_HID_HANDLE hDevice, U8 DeviceAddress, USBH_FT260_I2C_FLAG I2cFlag, void * pRegAddr, U32 RegAddrSize, void * pBuffer, U32 NumBytesToRead,  U32 * pNumBytesReturned);
USBH_STATUS       USBH_FT260_I2CMaster_WriteReg         (USBH_HID_HANDLE hDevice, U8 DeviceAddress, USBH_FT260_I2C_FLAG I2cFlag, void * pRegAddr, U32 RegAddrSize, void * pBuffer, U32 NumBytesToWrite, U32 * pNumBytesWritten);
USBH_STATUS       USBH_FT260_I2CMaster_GetStatus        (USBH_HID_HANDLE hDevice, U8* pStatus);
USBH_STATUS       USBH_FT260_I2CMaster_Reset            (USBH_HID_HANDLE hDevice);

//UART Functions
USBH_STATUS       USBH_FT260_UART_Init                  (USBH_HID_HANDLE hDevice);
USBH_STATUS       USBH_FT260_UART_SetBaudRate           (USBH_HID_HANDLE hDevice, U32 BaudRate);
USBH_STATUS       USBH_FT260_UART_SetFlowControl        (USBH_HID_HANDLE hDevice, USBH_FT260_UART_MODE FlowControl);
USBH_STATUS       USBH_FT260_UART_SetDataCharacteristics(USBH_HID_HANDLE hDevice, USBH_FT260_DATA_BIT DataBits, USBH_FT260_STOP_BIT StopBits, USBH_FT260_PARITY Parity);
USBH_STATUS       USBH_FT260_UART_SetBreak              (USBH_HID_HANDLE hDevice, unsigned OnOff);
USBH_STATUS       USBH_FT260_UART_SetXonXoffChar        (USBH_HID_HANDLE hDevice, U8 Xon, U8 Xoff);
USBH_STATUS       USBH_FT260_UART_GetConfig             (USBH_HID_HANDLE hDevice, USBH_FT260_UART_CONFIG * pUartConfig);
USBH_STATUS       USBH_FT260_UART_Read                  (USBH_HID_HANDLE hDevice, void * pBuffer, U32 NumBytesToRead,  U32 *  pNumBytesReturned);
USBH_STATUS       USBH_FT260_UART_Write                 (USBH_HID_HANDLE hDevice, void * pBuffer, U32 NumBytesToWrite, U32 *  pNumBytesWritten);
USBH_STATUS       USBH_FT260_UART_Reset                 (USBH_HID_HANDLE hDevice);

USBH_STATUS       USBH_FT260_UART_GetDcdRiStatus        (USBH_HID_HANDLE hDevice, U8* pVal);
USBH_STATUS       USBH_FT260_UART_EnableRiWakeup        (USBH_HID_HANDLE hDevice, unsigned int Enable);
USBH_STATUS       USBH_FT260_UART_SetRiWakeupConfig     (USBH_HID_HANDLE hDevice, USBH_FT260_RI_WAKEUP_TYPE Type);

// Interrupt is transmitted by UART interface
USBH_STATUS       USBH_FT260_GetInterruptFlag           (USBH_HID_HANDLE hDevice, unsigned int* pIntFlag);
USBH_STATUS       USBH_FT260_CleanInterruptFlag         (USBH_HID_HANDLE hDevice, unsigned int* pIntFlag);

// FT260 GPIO Functions
USBH_STATUS       USBH_FT260_GPIO_Set                   (USBH_HID_HANDLE hDevice, const USBH_FT260_GPIO_REPORT * pReport);
USBH_STATUS       USBH_FT260_GPIO_Get                   (USBH_HID_HANDLE hDevice,       USBH_FT260_GPIO_REPORT * pReport);
USBH_STATUS       USBH_FT260_GPIO_SetDir                (USBH_HID_HANDLE hDevice, U16 PinNum, U8 Dir);
USBH_STATUS       USBH_FT260_GPIO_Read                  (USBH_HID_HANDLE hDevice, U16 PinNum, U8 * pVal);
USBH_STATUS       USBH_FT260_GPIO_Write                 (USBH_HID_HANDLE hDevice, U16 PinNum, U8   Val);

// FT260 GPIO open drain
USBH_STATUS       USBH_FT260_GPIO_Set_OD                (USBH_HID_HANDLE hDevice, U8 Pins);
USBH_STATUS       USBH_FT260_GPIO_Reset_OD              (USBH_HID_HANDLE hDevice);


#if defined(__cplusplus)
  }
#endif

#endif // USBH_HID_FT260_H__

/*************************** End of file ****************************/
