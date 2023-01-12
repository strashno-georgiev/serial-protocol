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
File        : USBH_AUDIO.h
Purpose     : Audio interface class of the USB host stack
-------------------------- END-OF-HEADER -----------------------------
*/

#ifndef USBH_AUDIO_H
#define USBH_AUDIO_H

#include "USBH.h"
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
#ifndef USBH_AUDIO_MAX_NUM_SAM_FREQ
  #define USBH_AUDIO_MAX_NUM_SAM_FREQ                 6u
#endif

#ifndef USBH_AUDIO_MAX_NUM_CTRL_CHANNELS
  #define USBH_AUDIO_MAX_NUM_CTRL_CHANNELS            5u
#endif

#ifndef USBH_AUDIO_MAX_NUM_INPUT_PINS
  #define USBH_AUDIO_MAX_NUM_INPUT_PINS               5u
#endif

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/
// AUDIO subclass definitions
#define USBH_AUDIO_SUBCLASS_CONTROL                   0x01u
#define USBH_AUDIO_SUBCLASS_STREAMING                 0x02u
#define USBH_AUDIO_SUBCLASS_MIDI                      0x03u

// Control unit request types
#define USBH_AUDIO_REQUEST_SET_CUR                    0x01u
#define USBH_AUDIO_REQUEST_GET_CUR                    0x81u
#define USBH_AUDIO_REQUEST_GET_MIN                    0x82u
#define USBH_AUDIO_REQUEST_GET_MAX                    0x83u
#define USBH_AUDIO_REQUEST_GET_RES                    0x84u

// Bitmasks for feature unit controls
#define USBH_AUDIO_10_FU_CONTROL_MASK_MUTE            0x001uL
#define USBH_AUDIO_10_FU_CONTROL_MASK_VOLUME          0x002uL
#define USBH_AUDIO_10_FU_CONTROL_MASK_BASS            0x004uL
#define USBH_AUDIO_10_FU_CONTROL_MASK_MID             0x008uL
#define USBH_AUDIO_10_FU_CONTROL_MASK_TRBLE           0x010uL
#define USBH_AUDIO_10_FU_CONTROL_MASK_EQUALIZER       0x020uL
#define USBH_AUDIO_10_FU_CONTROL_MASK_AUTOGAIN        0x040uL
#define USBH_AUDIO_10_FU_CONTROL_MASK_DELAY           0x080uL
#define USBH_AUDIO_10_FU_CONTROL_MASK_BASSBOOST       0x100uL
#define USBH_AUDIO_10_FU_CONTROL_MASK_LOUDNESS        0x200uL

// Control selector for feature unit controls (bmControls)
#define USBH_AUDIO_SELECTOR_FU_MUTE_CONTROL           0x0100u
#define USBH_AUDIO_SELECTOR_FU_VOLUME_CONTROL         0x0200u
#define USBH_AUDIO_SELECTOR_FU_BASS_CONTROL           0x0300u
#define USBH_AUDIO_SELECTOR_FU_MID_CONTROL            0x0400u
#define USBH_AUDIO_SELECTOR_FU_TREBLE_CONTROL         0x0500u
#define USBH_AUDIO_SELECTOR_FU_EQUALIZER_CONTROL      0x0600u
#define USBH_AUDIO_SELECTOR_FU_AUTOGAIN_CONTROL       0x0700u
#define USBH_AUDIO_SELECTOR_FU_DELAY_CONTROL          0x0800u
#define USBH_AUDIO_SELECTOR_FU_BASSBOOST_CONTROL      0x0900u
#define USBH_AUDIO_SELECTOR_FU_LOUDNESS_CONTROL       0x0A00u

#define USBH_AUDIO_INVALID_HANDLE                      NULL

/*********************************************************************
*
*       USBH_AUDIO_HANDLE
*/

typedef struct _USBH_AUDIO_INST * USBH_AUDIO_HANDLE;
typedef struct _USBH_AUDIO_CHN  * USBH_AUDIO_CHANNEL;

/*********************************************************************
*
*       USBH_AUDIO_INTERFACE_INFO
*
*  Description
*    Structure containing information about an audio interface.
*/
typedef struct {
  U16         VendorId;             // The Vendor ID of the device.
  U16         ProductId;            // The Product ID of the device.
  USBH_SPEED  Speed;                // The USB speed of the device, see USBH_SPEED.
  U8          InterfaceNo;          // Interface index (from USB descriptor).
  U8          Class;                // The Class value field of the interface.
  U8          SubClass;             // The SubClass value field of the interface.
  U8          Protocol;             // The Protocol value field of the interface.
  USBH_INTERFACE_ID InterfaceID;    // ID of the interface.
  USBH_DEVICE_ID    DeviceId;       // The unique device Id. This Id is assigned if the USB
                                    // device was successfully enumerated. It is valid until the
                                    // device is removed from the host. If the device is reconnected
                                    // a different device Id is assigned.
} USBH_AUDIO_INTERFACE_INFO;

/*********************************************************************
*
*       USBH_AUDIO_EP_INFO
*
*  Description
*    Structure containing information about an audio streaming interface.
*/
typedef struct {
  U8          AlternateSetting;     // Alternate setting number.
  U8          EPAddr;               // Endpoint address.
  U8          TerminalLink;         // ID of the terminal unit, this endpoint is connected to.
  U8          bmAttributes;         // Attributes (SamplingFrequency, Pitch, MaxPacketsOnly).
  U16         MaxPacketSize;        // Maximum packet size of the endpoint.
  U16         FormatTag;            // Audio format tag.
  U8          FormatType;           // Type of format descriptor.
  union {
    //
    // FORMAT_TYPE_I (FormatType == 1)
    //
    struct {
      U8      NrChannels;           // Number of audio channels.
      U8      SubFrameSize;         // Number of bytes in an audio subframe.
      U8      BitResolution;        // Number of bits per audio subframe.
      U8      NumSamFreq;           // Number of discrete sampling frequencies in SamFreq[].
                                    // 0 means sampling frequency range SamFreq[0] to SamFreq[1].
      U32     SamFreq[USBH_AUDIO_MAX_NUM_SAM_FREQ];    // Sampling frequencies.
    } Type1;
    //
    // FORMAT_TYPE_II (FormatType == 2)
    //
    struct {
      U16     MaxBitRate;           // Maximum number of kbits per second, the interface can handle.
      U16     SamplesPerFrame;      // Number of PCM audio samples contained in one encoded audio frame.
      U8      NumSamFreq;           // Number of discrete sampling frequencies in SamFreq[].
                                    // 0 means sampling frequency range SamFreq[0] to SamFreq[1].
      U32     SamFreq[USBH_AUDIO_MAX_NUM_SAM_FREQ];    // Sampling frequencies.
    } Type2;
  } u;
} USBH_AUDIO_EP_INFO;

/*********************************************************************
*
*       USBH_AUDIO_TERMINAL_INFO
*
*  Description
*    Structure containing information about an audio output terminal unit.
*/
typedef struct {
  U8    UnitID;                                        // AUDIO unit ID.
  U8    SourceID;                                      // For input terminals: Value 0.
                                                       // For output terminals: ID of the unit connected to the input.
  U16   TerminalType;                                  // USB Audio Terminal Type.
  U8    NrChannels;                                    // Number of channels (input terminal only).
  U8    StringIndex;                                   // Index of the string descriptor for this terminal.
  U16   wChannelConfig;                                // Describes the spatial location of the logical channels
                                                       // (input terminal only).
} USBH_AUDIO_TERMINAL_INFO;

/*********************************************************************
*
*       USBH_AUDIO_FU_INFO
*
*  Description
*    Structure containing information about an audio feature unit.
*/
typedef struct {
  U8    UnitID;                                        // AUDIO unit ID.
  U8    SourceID;                                      // ID of the unit connected to the input.
  U8    StringIndex;                                   // Index of the string descriptor for this unit.
  U8    NumControlChannels;                            // Number of entries in Controls[].
  U16   bmControls[USBH_AUDIO_MAX_NUM_CTRL_CHANNELS];  // Bit mask containing supported controls.
} USBH_AUDIO_FU_INFO;

/*********************************************************************
*
*       USBH_AUDIO_SU_INFO
*
*  Description
*    Structure containing information about an audio selector unit.
*/
typedef struct {
  U8    UnitID;                                        // AUDIO unit ID.
  U8    StringIndex;                                   // Index of the string descriptor for this unit.
  U8    NumInputs;                                     // Number of inputs of the selector.
  U8    SourceID[USBH_AUDIO_MAX_NUM_INPUT_PINS];       // ID of the unit connected to the input.
} USBH_AUDIO_SU_INFO;

/*********************************************************************
*
*       USBH_AUDIO_MU_INFO
*
*  Description
*    Structure containing information about an audio mixer unit.
*/
typedef struct {
  U8    UnitID;                                        // AUDIO unit ID.
  U8    StringIndex;                                   // Index of the string descriptor for this unit.
  U8    NumInputs;                                     // Number of inputs of the selector.
  U8    SourceID[USBH_AUDIO_MAX_NUM_INPUT_PINS];       // ID of the unit connected to the input.
  U8    NumOutChannels;                                // Number of output channels.
  U16   wChannelConfig;                                // Describes the spatial location of the logical channels.
  U8    LenControls;                                   // Size of the bmControls field in bytes.
  U8    bmControls[USBH_AUDIO_MAX_NUM_INPUT_PINS];     // Bit map indicating which mixing Controls are programmable.
                                                       // Contains 'NumInputs * NumOutChannels' bits.
} USBH_AUDIO_MU_INFO;

/*********************************************************************
*
*       USBH_AUDIO_VOLUME_INFO
*
*  Description
*    Structure containing information about the volume setting of a feature unit.
*/
typedef struct {
  I16   Cur;                                           // Current volume value (in decibel * 256).
  I16   Min;                                           // Minimum volume value (in decibel * 256).
  I16   Max;                                           // Maximum volume value (in decibel * 256).
  I16   Res;                                           // Resolution of volume (in decibel * 256).
} USBH_AUDIO_VOLUME_INFO;

/*********************************************************************
*
*       USBH_AUDIO_DESCRIPTOR
*
*  Description
*    Structure containing a USB descriptor
*/
typedef struct {
  U8          AlternateSetting;     // Descriptor belongs to this alternate setting.
  U8          Type;                 // The Type value field of the descriptor.
  U8          SubType;              // The SubType value field of the descriptor.
  const U8  * pDesc;                // Pointer to the descriptor data.
} USBH_AUDIO_DESCRIPTOR;

/*********************************************************************
*
*       Function prototypes
*/
USBH_STATUS       USBH_AUDIO_Init                     (void);
void              USBH_AUDIO_Exit                     (void);
USBH_STATUS       USBH_AUDIO_AddNotification          (USBH_NOTIFICATION_HOOK * pHook, USBH_NOTIFICATION_FUNC * pfNotification, void * pContext);
USBH_STATUS       USBH_AUDIO_RemoveNotification       (const USBH_NOTIFICATION_HOOK * pHook);
USBH_STATUS       USBH_AUDIO_Open                     (unsigned Index, USBH_AUDIO_HANDLE *pHandle);
void              USBH_AUDIO_Close                    (USBH_AUDIO_HANDLE hDevice);
int               USBH_AUDIO_GetIndex                 (USBH_INTERFACE_ID InterfaceID);

USBH_STATUS       USBH_AUDIO_GetInterfaceInfo         (USBH_AUDIO_HANDLE hDevice, USBH_AUDIO_INTERFACE_INFO * pDevInfo);
unsigned          USBH_AUDIO_GetDescriptorList        (USBH_AUDIO_HANDLE hDevice, USBH_AUDIO_DESCRIPTOR ** ppDescList);
void              USBH_AUDIO_FreeDescriptorList       (USBH_AUDIO_HANDLE hDevice, USBH_AUDIO_DESCRIPTOR *pDescList);
USBH_STATUS       USBH_AUDIO_GetEndpointInfo          (USBH_AUDIO_HANDLE hDevice, U8 AlternateSetting, USBH_AUDIO_EP_INFO * pEPInfo);
USBH_STATUS       USBH_AUDIO_GetTerminalInfo          (USBH_AUDIO_HANDLE hDevice, unsigned n, USBH_AUDIO_TERMINAL_INFO * pTermInfo);
USBH_STATUS       USBH_AUDIO_GetFeatureUnitInfo       (USBH_AUDIO_HANDLE hDevice, unsigned n, USBH_AUDIO_FU_INFO * pFUInfo);
USBH_STATUS       USBH_AUDIO_GetSelectorUnitInfo      (USBH_AUDIO_HANDLE hDevice, unsigned n, USBH_AUDIO_SU_INFO * pSUInfo);
USBH_STATUS       USBH_AUDIO_GetMixerUnitInfo         (USBH_AUDIO_HANDLE hDevice, unsigned n, USBH_AUDIO_MU_INFO * pMUInfo);

USBH_INTERFACE_HANDLE USBH_AUDIO_GetInterfaceHandle   (USBH_AUDIO_HANDLE hDevice);

USBH_STATUS       USBH_AUDIO_SetAlternateInterface    (USBH_AUDIO_HANDLE hDevice, U8 AltInterfaceSetting);
USBH_STATUS       USBH_AUDIO_GetSampleFrequency       (USBH_AUDIO_HANDLE hDevice, U8 EPAddr, U32 * pSampleFrequency);
USBH_STATUS       USBH_AUDIO_SetSampleFrequency       (USBH_AUDIO_HANDLE hDevice, U8 EPAddr, U32 * pSampleFrequency);
USBH_STATUS       USBH_AUDIO_GetVolume                (USBH_AUDIO_HANDLE hDevice, U8 Unit, U8 Channel, USBH_AUDIO_VOLUME_INFO * pVolumeInfo);
USBH_STATUS       USBH_AUDIO_SetVolume                (USBH_AUDIO_HANDLE hDevice, U8 Unit, U8 Channel, I16 Volume);
USBH_STATUS       USBH_AUDIO_GetMute                  (USBH_AUDIO_HANDLE hDevice, U8 Unit, U8 Channel, U8 * pMute);
USBH_STATUS       USBH_AUDIO_SetMute                  (USBH_AUDIO_HANDLE hDevice, U8 Unit, U8 Channel, U8 Mute);
USBH_STATUS       USBH_AUDIO_GetFeatureUnitControl    (USBH_AUDIO_HANDLE hDevice, U8 RequestType, U8 Unit, U16 Selector, U16 Channel, U32 *pDataLen, U8 *pBuff);
USBH_STATUS       USBH_AUDIO_SetFeatureUnitControl    (USBH_AUDIO_HANDLE hDevice, U8 Unit, U16 Selector, U16 Channel, U32 DataLen, const U8 *pData);
USBH_STATUS       USBH_AUDIO_GetMixerUnitControl      (USBH_AUDIO_HANDLE hDevice, U8 RequestType, U8 Unit, U8 InChannel, U8 OutChannel, I16 *pValue);
USBH_STATUS       USBH_AUDIO_SetMixerUnitControl      (USBH_AUDIO_HANDLE hDevice, U8 Unit, U8 InChannel, U8 OutChannel, I16 Value);
USBH_STATUS       USBH_AUDIO_GetSelectorUnitControl   (USBH_AUDIO_HANDLE hDevice, U8 RequestType, U8 Unit, U8 *pValue);
USBH_STATUS       USBH_AUDIO_SetSelectorUnitControl   (USBH_AUDIO_HANDLE hDevice, U8 Unit, U8 Value);

USBH_STATUS       USBH_AUDIO_OpenOutChannel           (USBH_AUDIO_HANDLE hDevice, U8 EPAddr, U16 SampleSize, U32 SampleFrequency, USBH_AUDIO_CHANNEL *pHandle);
USBH_STATUS       USBH_AUDIO_OpenInChannel            (USBH_AUDIO_HANDLE hDevice, U8 EPAddr, USBH_AUDIO_CHANNEL *pHandle);
void              USBH_AUDIO_CloseOutChannel          (USBH_AUDIO_CHANNEL Handle);
void              USBH_AUDIO_CloseInChannel           (USBH_AUDIO_CHANNEL Handle);
USBH_STATUS       USBH_AUDIO_Write                    (USBH_AUDIO_CHANNEL Handle, U32 Len, const U8 *pData);
USBH_STATUS       USBH_AUDIO_CheckBufferIdle          (USBH_AUDIO_CHANNEL Handle, const U8 *pBuff, int bWait);
USBH_STATUS       USBH_AUDIO_Receive                  (USBH_AUDIO_CHANNEL Handle, U32 *pLen, const U8 ** ppData, int Timeout);
USBH_STATUS       USBH_AUDIO_Ack                      (USBH_AUDIO_CHANNEL Handle);
USBH_STATUS       USBH_AUDIO_Read                     (USBH_AUDIO_CHANNEL Handle, U32 * pLen, U8 * pBuff, int Timeout);
unsigned          USBH_AUDIO_GetNumQueueEntries       (USBH_AUDIO_CHANNEL Handle);
unsigned          USBH_AUDIO_GetFreeQueueEntries      (USBH_AUDIO_CHANNEL Handle);
unsigned          USBH_AUDIO_GetOutPacketSize         (USBH_AUDIO_CHANNEL Handle, int Max);

#if defined(__cplusplus)
  }
#endif

#endif // USBH_AUDIO_H

/*************************** End of file ****************************/
