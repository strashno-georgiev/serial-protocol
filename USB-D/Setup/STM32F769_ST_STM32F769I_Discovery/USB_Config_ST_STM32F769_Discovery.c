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
File    : USB_Config_ST_STM32F769_Discovery.c
Purpose : Config file for ST STM32F769_Discovery
--------  END-OF-HEADER  ---------------------------------------------
*/

#include "USB.h"
#include "BSP_USB.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
#define USB_ISR_ID    (77)
#define USB_ISR_PRIO  254

/*********************************************************************
*
*       Defines, sfrs
*
**********************************************************************
*/

//
// RCC
//
#define RCC_BASE_ADDR             ((unsigned int)(0x40023800))
#define RCC_AHB1RSTR              (*(volatile U32 *)(RCC_BASE_ADDR + 0x10))
#define RCC_AHB2RSTR              (*(volatile U32 *)(RCC_BASE_ADDR + 0x14))
#define RCC_AHB1ENR               (*(volatile U32 *)(RCC_BASE_ADDR + 0x30))
#define RCC_AHB2ENR               (*(volatile U32 *)(RCC_BASE_ADDR + 0x34))

//
// GPIO
//
#define GPIOA_BASE_ADDR           ((unsigned int)0x40020000)
#define GPIOA_MODER               (*(volatile U32 *)(GPIOA_BASE_ADDR + 0x00))
#define GPIOA_OTYPER              (*(volatile U32 *)(GPIOA_BASE_ADDR + 0x04))
#define GPIOA_OSPEEDR             (*(volatile U32 *)(GPIOA_BASE_ADDR + 0x08))
#define GPIOA_PUPDR               (*(volatile U32 *)(GPIOA_BASE_ADDR + 0x0C))
#define GPIOA_IDR                 (*(volatile U32 *)(GPIOA_BASE_ADDR + 0x10))
#define GPIOA_ODR                 (*(volatile U32 *)(GPIOA_BASE_ADDR + 0x14))
#define GPIOA_BSRRL               (*(volatile U16 *)(GPIOA_BASE_ADDR + 0x18))
#define GPIOA_BSRRH               (*(volatile U16 *)(GPIOA_BASE_ADDR + 0x16))
#define GPIOA_LCKR                (*(volatile U32 *)(GPIOA_BASE_ADDR + 0x1C))
#define GPIOA_AFRL                (*(volatile U32 *)(GPIOA_BASE_ADDR + 0x20))
#define GPIOA_AFRH                (*(volatile U32 *)(GPIOA_BASE_ADDR + 0x24))

#define GPIOB_BASE_ADDR           ((unsigned int)0x40020400)
#define GPIOB_MODER               (*(volatile U32 *)(GPIOB_BASE_ADDR + 0x00))
#define GPIOB_OTYPER              (*(volatile U32 *)(GPIOB_BASE_ADDR + 0x04))
#define GPIOB_OSPEEDR             (*(volatile U32 *)(GPIOB_BASE_ADDR + 0x08))
#define GPIOB_PUPDR               (*(volatile U32 *)(GPIOB_BASE_ADDR + 0x0C))
#define GPIOB_IDR                 (*(volatile U32 *)(GPIOB_BASE_ADDR + 0x10))
#define GPIOB_ODR                 (*(volatile U32 *)(GPIOB_BASE_ADDR + 0x14))
#define GPIOB_BSRRL               (*(volatile U16 *)(GPIOB_BASE_ADDR + 0x18))
#define GPIOB_BSRRH               (*(volatile U16 *)(GPIOB_BASE_ADDR + 0x16))
#define GPIOB_LCKR                (*(volatile U32 *)(GPIOB_BASE_ADDR + 0x1C))
#define GPIOB_AFRL                (*(volatile U32 *)(GPIOB_BASE_ADDR + 0x20))
#define GPIOB_AFRH                (*(volatile U32 *)(GPIOB_BASE_ADDR + 0x24))

#define GPIOC_BASE_ADDR           ((unsigned int)0x40020800)
#define GPIOC_MODER               (*(volatile U32 *)(GPIOC_BASE_ADDR + 0x00))
#define GPIOC_OTYPER              (*(volatile U32 *)(GPIOC_BASE_ADDR + 0x04))
#define GPIOC_OSPEEDR             (*(volatile U32 *)(GPIOC_BASE_ADDR + 0x08))
#define GPIOC_PUPDR               (*(volatile U32 *)(GPIOC_BASE_ADDR + 0x0C))
#define GPIOC_IDR                 (*(volatile U32 *)(GPIOC_BASE_ADDR + 0x10))
#define GPIOC_ODR                 (*(volatile U32 *)(GPIOC_BASE_ADDR + 0x14))
#define GPIOC_BSRRL               (*(volatile U16 *)(GPIOC_BASE_ADDR + 0x18))
#define GPIOC_BSRRH               (*(volatile U16 *)(GPIOC_BASE_ADDR + 0x16))
#define GPIOC_LCKR                (*(volatile U32 *)(GPIOC_BASE_ADDR + 0x1C))
#define GPIOC_AFRL                (*(volatile U32 *)(GPIOC_BASE_ADDR + 0x20))
#define GPIOC_AFRH                (*(volatile U32 *)(GPIOC_BASE_ADDR + 0x24))

#define GPIOI_BASE_ADDR           ((unsigned int)0x40022000)
#define GPIOI_MODER               (*(volatile U32 *)(GPIOI_BASE_ADDR + 0x00))
#define GPIOI_OTYPER              (*(volatile U32 *)(GPIOI_BASE_ADDR + 0x04))
#define GPIOI_OSPEEDR             (*(volatile U32 *)(GPIOI_BASE_ADDR + 0x08))
#define GPIOI_PUPDR               (*(volatile U32 *)(GPIOI_BASE_ADDR + 0x0C))
#define GPIOI_IDR                 (*(volatile U32 *)(GPIOI_BASE_ADDR + 0x10))
#define GPIOI_ODR                 (*(volatile U32 *)(GPIOI_BASE_ADDR + 0x14))
#define GPIOI_BSRRL               (*(volatile U16 *)(GPIOI_BASE_ADDR + 0x18))
#define GPIOI_BSRRH               (*(volatile U16 *)(GPIOI_BASE_ADDR + 0x16))
#define GPIOI_LCKR                (*(volatile U32 *)(GPIOI_BASE_ADDR + 0x1C))
#define GPIOI_AFRL                (*(volatile U32 *)(GPIOI_BASE_ADDR + 0x20))
#define GPIOI_AFRH                (*(volatile U32 *)(GPIOI_BASE_ADDR + 0x24))

#define GPIOH_BASE_ADDR           ((unsigned int)0x40021C00)
#define GPIOH_MODER               (*(volatile U32 *)(GPIOH_BASE_ADDR + 0x00))
#define GPIOH_OTYPER              (*(volatile U32 *)(GPIOH_BASE_ADDR + 0x04))
#define GPIOH_OSPEEDR             (*(volatile U32 *)(GPIOH_BASE_ADDR + 0x08))
#define GPIOH_PUPDR               (*(volatile U32 *)(GPIOH_BASE_ADDR + 0x0C))
#define GPIOH_IDR                 (*(volatile U32 *)(GPIOH_BASE_ADDR + 0x10))
#define GPIOH_ODR                 (*(volatile U32 *)(GPIOH_BASE_ADDR + 0x14))
#define GPIOH_BSRRL               (*(volatile U16 *)(GPIOH_BASE_ADDR + 0x18))
#define GPIOH_BSRRH               (*(volatile U16 *)(GPIOH_BASE_ADDR + 0x16))
#define GPIOH_LCKR                (*(volatile U32 *)(GPIOH_BASE_ADDR + 0x1C))
#define GPIOH_AFRL                (*(volatile U32 *)(GPIOH_BASE_ADDR + 0x20))
#define GPIOH_AFRH                (*(volatile U32 *)(GPIOH_BASE_ADDR + 0x24))


/*********************************************************************
*
*       Static code
*
**********************************************************************
*/
/*********************************************************************
*
*       _EnableISR
*/
static void _EnableISR(USB_ISR_HANDLER * pfISRHandler) {
  BSP_USB_InstallISR_Ex(USB_ISR_ID, pfISRHandler, USB_ISR_PRIO);
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/
/*********************************************************************
*
*       Setup which target USB driver shall be used
*/

/*********************************************************************
*
*       USBD_X_Config
*/
void USBD_X_Config(void) {
  RCC_AHB1ENR |= 0
              | (1 <<  8)  // GPIOIEN: IO port I clock enable
              | (1 <<  7)  // GPIOHEN: IO port H clock enable
              | (1 <<  2)  // GPIOCEN: IO port C clock enable
              | (1 <<  1)  // GPIOBEN: IO port B clock enable
              | (1 <<  0)  // GPIOAEN: IO port A clock enable
              ;
  //
  // UPLI data pins
  // PA3 (OTG_HS_ULPI alternate function, DATA0)
  //
  GPIOA_MODER    =   (GPIOA_MODER  & ~(3UL  <<  6)) | (2UL  <<  6);
  GPIOA_OTYPER  &=  ~(1UL  <<  3);
  GPIOA_OSPEEDR |=   (3UL  <<  6);
  GPIOA_PUPDR   &=  ~(3UL  <<  6);
  GPIOA_AFRL     =   (GPIOA_AFRL  & ~(15UL << 12)) | (10UL << 12);
  //
  //PB0, PB1 (OTG_HS_ULPI alternate function, DATA1, DATA2)
  //
  GPIOB_MODER    =   (GPIOB_MODER  & ~(15UL <<  0)) | (10UL <<  0);
  GPIOB_OTYPER  &=  ~(3UL  <<  0);
  GPIOB_OSPEEDR |=   (15UL <<  0);
  GPIOB_PUPDR   &=  ~(15UL <<  0);
  GPIOB_AFRL     =   (GPIOB_AFRL  & ~(0xFFUL <<  0)) | (0xAA <<  0);
  //
  // PB10..13 (OTG_HS_ULPI alternate function, DATA3 to DATA6)
  //
  GPIOB_MODER    =   (GPIOB_MODER  & ~(0xFFUL << 20)) | (0xAA << 20);
  GPIOB_OTYPER  &=  ~(15UL << 10);
  GPIOB_OSPEEDR |=   (0xFFUL << 20);
  GPIOB_PUPDR   &=  ~(0xFFUL << 20);
  GPIOB_AFRH     =   (GPIOB_AFRH  & ~(0xFFFFUL << 8)) | (0xAAAA << 8);
  //
  // PB5 (OTG_HS_ULPI alternate function, DATA7)
  //
  GPIOB_MODER    =   (GPIOB_MODER  & ~(3UL  <<  10)) | (2UL  <<  10);
  GPIOB_OTYPER  &=  ~(1UL  <<  5);
  GPIOB_OSPEEDR |=   (3UL  << 10);
  GPIOB_PUPDR   &=  ~(3UL  << 10);
  GPIOB_AFRL     =   (GPIOB_AFRL  & ~(15UL <<  20)) | (10UL <<  20);
  //
  // ULPI control pins
  // PC0 (OTG_HS_ULPI alternate function, STP)
  //
  GPIOC_MODER    =   (GPIOC_MODER & ~(3UL  <<   0)) | (2UL  <<  0);
  GPIOC_OSPEEDR |=   (3UL  <<  0);
  GPIOC_AFRL     =   (GPIOC_AFRL  & ~(15UL <<   0)) | (10UL <<  0);
  //
  // PI11 (OTG_HS_ULPI alternate function, DIR)
  //
  GPIOI_MODER    =   (GPIOI_MODER & ~(3UL  <<  22)) | (2UL  << 22);
  GPIOI_OSPEEDR |=   (3UL  << 22);
  GPIOI_AFRH     =   (GPIOI_AFRH  & ~(15UL <<  12)) | (10UL << 12);
  //
  // PH4 (OTG_HS_ULPI alternate function, NXT)
  GPIOH_MODER    =   (GPIOH_MODER & ~(3UL  <<   8)) | (2UL  <<  8);
  GPIOH_OSPEEDR |=   (3UL  <<  8);
  GPIOH_AFRL     =   (GPIOH_AFRL  & ~(15UL <<  16)) | (10UL << 16);
  //
  // PA5 (OTG_HS_ULPI alternate function, CLOCK)
  GPIOA_MODER    =   (GPIOA_MODER & ~(3UL  <<  10)) | (2UL  << 10);
  GPIOA_OSPEEDR |=   (3UL  << 10);
  GPIOA_AFRL     =   (GPIOA_AFRL  & ~(15UL <<  20)) | (10UL << 20);
  //
  //  Enable clock for OTG_HS and OTGHS_ULPI
  //
  RCC_AHB1ENR    |=  (3UL << 29);
  USB_OS_Delay(10);
  //
  // Reset OTGHS clock
  RCC_AHB1RSTR   |=  (1UL << 29);
  USB_OS_Delay(10);
  RCC_AHB1RSTR   &= ~(1UL << 29);
  USB_OS_Delay(10);
  USBD_AddDriver(&USB_Driver_ST_STM32F7xxHS);
  USBD_SetISRMgmFuncs(_EnableISR, USB_OS_IncDI, USB_OS_DecRI);
}

/*************************** End of file ****************************/
