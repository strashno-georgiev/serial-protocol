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
File        : USBH_LAN.h
Purpose     : Header for the USBH LAN module
-------------------------- END-OF-HEADER -----------------------------
*/

#ifndef USBH_LAN_H_
#define USBH_LAN_H_

#include "USBH.h"
#include "IP.h"

#if defined(__cplusplus)
  extern "C" {                 // Make sure we have C-declarations in C++ programs
#endif

#if IP_VERSION < 33000u
  #error "emUSB-Host LAN is only supported with embOS/IP version V3.30 and higher."
#endif

typedef struct _USBH_LAN_INST USBH_LAN_INST;

typedef USBH_STATUS (USBH_LAN_DRV_INIT)       (void);
typedef USBH_STATUS (USBH_LAN_ON_DEVICE_INIT) (USBH_LAN_INST * pInst);
typedef USBH_STATUS (USBH_LAN_ON_DEVICE_EXIT) (USBH_LAN_INST * pInst);
typedef void        (USBH_LAN_START_STOP_READ)(const USBH_LAN_INST * pInst, int OnOff);
typedef USBH_STATUS (USBH_LAN_ONSEND)         (const USBH_LAN_INST * pInst, const U8 * pData, unsigned NumBytes);
typedef int         (USBH_LAN_GET_CAPS)       (U8 * pFlags);
typedef int         (USBH_LAN_SET_FILTER)     (USBH_LAN_INST * pInst, void * pData, unsigned NumBytes);
typedef void        (USBH_LAN_ON_TIMER)       (const USBH_LAN_INST * pInst);
typedef void        (USBH_LAN_DRV_EXIT)       (void);

typedef struct {
  USBH_LAN_DRV_INIT        * pfInit;
  USBH_LAN_ON_DEVICE_INIT  * pfOnDevInit;
  USBH_LAN_ON_DEVICE_EXIT  * pfOnDevExit;
  USBH_LAN_START_STOP_READ * pfStartStopRead;
  USBH_LAN_ONSEND          * pfOnSend;
  USBH_LAN_GET_CAPS        * pfGetCaps;
  USBH_LAN_SET_FILTER      * pfSetFilter;
  USBH_LAN_ON_TIMER        * pfOnTimer;
  USBH_LAN_DRV_EXIT        * pfExit;
} USBH_LAN_DRIVER;

USBH_STATUS      USBH_LAN_Init            (void);
void             USBH_LAN_Exit            (void);
USBH_STATUS      USBH_LAN_RegisterDriver  (const USBH_LAN_DRIVER * pDriver);
USBH_LAN_INST  * USBH_LAN_AllocInst       (U8 DevIndex, const USBH_LAN_DRIVER * pDriverAPI);
void             USBH_LAN_WritePacket     (USBH_LAN_INST * pInst, U8 * pData, unsigned NumBytesInPacket, unsigned NumBytes2Copy, int IsPartial);
USBH_LAN_INST  * USBH_LAN_GetInst         (U8 DevIndex, const USBH_LAN_DRIVER * pDriverAPI);
void             USBH_LAN_RemoveInst      (USBH_LAN_INST * pInst);
void             USBH_LAN_StartInst       (USBH_LAN_INST * pInst);

/*********************************************************************
*
*  Driver for the LAN module
*/
extern const USBH_LAN_DRIVER USBH_LAN_DRIVER_ASIX;
extern const USBH_LAN_DRIVER USBH_LAN_DRIVER_ECM;
extern const USBH_LAN_DRIVER USBH_LAN_DRIVER_RNDIS;
extern const USBH_LAN_DRIVER USBH_LAN_DRIVER_APPLE_IOS;

/*********************************************************************
*
*  ETH driver for embOS/IP
*/
extern const IP_HW_DRIVER IP_Driver_USBH;
#if defined(__cplusplus)
  }
#endif

#endif // USBH_LAN_H_

/*************************** End of file ****************************/
