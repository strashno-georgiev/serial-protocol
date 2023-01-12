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

File    : USB_MIDI_Start.c
Purpose : This sample is designed to demonstrate emUSB-Device's MIDI class.

Additional information:
  Preparations:
    On Linux a third-party tool such as "amidi" can be used to send and receive data from the device.
    On Windows a third-party tool such as "MIDI-OX" can be used.
    On mac OS a third party tool such as "Snoize MIDI Monitor" can be used.

  Expected behavior:
    After the device has been connected to the host the host sees
    MIDI note-on/note-off commands.
    The host is able to send commands to the device which are decoded
    and printed to the debug terminal.

  Sample output:
    Target:
      0:005 MainTask - USBD_Start
      <...>
      36:055 MainTask - Received MIDI packet for jack 0: Note-on, [0x90] [0x4E] [0x30]
      36:055 MainTask - Received MIDI packet for jack 0: Note-off, [0x80] [0x4E] [0x30]
      <...>
      122:395 MainTask - Received start of System Exclusive transfer: [0xF0] [0x43] [0x10]
      122:395 MainTask - System Exclusive continues...: [0x4C] [0x0] [0x0]
      122:395 MainTask - System Exclusive ends: [0x7E] [0x0] [0xF7]

    Host:
      user@linux:~$ amidi -p hw:2,0,0 -d
        90 3A 2D
        80 3A 18
        90 3A 2D
        80 3A 18
*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/
#include <stdio.h>
#include "USB_MIDI.h"
#include "RTOS.h"
#include "BSP.h"

/*********************************************************************
*
*       Information that are used during enumeration
*/
static const USB_DEVICE_INFO _DeviceInfo = {
  0x8765,         // VendorId
  0x1350,         // ProductId
  "Vendor",       // VendorName
  "MIDI device",  // ProductName
  "13245678"      // SerialNumber
};

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static USBD_MIDI_JACK   _Jacks[5];
static USBD_MIDI_PACKET _Packets[64];
static USBD_MIDI_PACKET _p1;
static USBD_MIDI_PACKET _p2;

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _CIN2Str
*
*  Function description
*    Converts the Code Index Number to the command name.
*/
static const char * _CIN2Str(U8 CIN) {
  const char * s;
  switch (CIN) {
    case MIDI_USB_CIN_MISC_EVENT:
      s = "Miscellaneous function codes. Reserved for future extensions.";
      break;
    case MIDI_USB_CIN_CABLE_EVENT:
      s = "Cable events. Reserved for future expansion.";
      break;
    case MIDI_USB_CIN_SYSTEM_COMMON_2BYTE:
      s = "Two-byte System Common messages like MTC, SongSelect, etc.";
      break;
    case MIDI_USB_CIN_SYSTEM_COMMON_3BYTE:
      s = "Three-byte System Common messages like SPP, etc.";
      break;
    case MIDI_USB_CIN_SYSEX_BEGIN_CONT:
      s = "SysEx starts or continues";
      break;
    case MIDI_USB_CIN_SYSEX_END_1BYTE:
      s = "SysEx ends with following single byte.";
      break;
    case MIDI_USB_CIN_SYSEX_END_2BYTE:
      s = "SysEx ends with following two bytes.";
      break;
    case MIDI_USB_CIN_SYSEX_END_3BYTE:
      s = "SysEx ends with following three bytes.";
      break;
    case MIDI_USB_CIN_NOTE_OFF:
      s = "Note-off";
      break;
    case MIDI_USB_CIN_NOTE_ON:
      s = "Note-on";
      break;
    case MIDI_USB_CIN_POLY_AFTERTOUCH:
      s = "Poly-KeyPress";
      break;
    case MIDI_USB_CIN_CONTROL_CHANGE:
      s = "Control Change";
      break;
    case MIDI_USB_CIN_PROGRAM_CHANGE:
      s = "Program Change";
      break;
    case MIDI_USB_CIN_AFTERTOUCH:
      s = "Channel Pressure";
      break;
    case MIDI_USB_CIN_PITCH_BEND:
      s = "PitchBend Change";
      break;
    case MIDI_USB_CIN_SINGLE_BYTE:
      s = "Single Byte";
      break;
    default:
      s = "Unknown event";
  }
  return s;
}

/*********************************************************************
*
*       _AddMIDI
*
*  Function description
*    Add generic USB MIDI interface to USB stack
*/
static USBD_MIDI_HANDLE _AddMIDI(void) {
  static U8             _abOutBuffer[USB_HS_BULK_MAX_PACKET_SIZE];
  static U8             _SourcePin;
  USBD_MIDI_INIT_DATA   InitData;
  USB_ADD_EP_INFO       EPBulkIn;
  USB_ADD_EP_INFO       EPBulkOut;
  USBD_MIDI_HANDLE      hInst;

  memset(&InitData, 0, sizeof(InitData));
  EPBulkIn.Flags          = 0;                             // Flags not used.
  EPBulkIn.InDir          = USB_DIR_IN;                    // IN direction (Device to Host)
  EPBulkIn.Interval       = 0;                             // Interval not used for Bulk endpoints.
  EPBulkIn.MaxPacketSize  = USB_HS_BULK_MAX_PACKET_SIZE;   // Maximum packet size (512 for Bulk in high-speed).
  EPBulkIn.TransferType   = USB_TRANSFER_TYPE_BULK;        // Endpoint type - Bulk.
  InitData.EPIn  = USBD_AddEPEx(&EPBulkIn, NULL, 0);

  EPBulkOut.Flags         = 0;                             // Flags not used.
  EPBulkOut.InDir         = USB_DIR_OUT;                   // OUT direction (Host to Device)
  EPBulkOut.Interval      = 0;                             // Interval not used for Bulk endpoints.
  EPBulkOut.MaxPacketSize = USB_HS_BULK_MAX_PACKET_SIZE;   // Maximum packet size (512 for Bulk in high-speed).
  EPBulkOut.TransferType  = USB_TRANSFER_TYPE_BULK;        // Endpoint type - Bulk.
  InitData.EPOut = USBD_AddEPEx(&EPBulkOut, _abOutBuffer, sizeof(_abOutBuffer));
  _Jacks[0].JackType = USB_AUDIO_MIDI_EMBEDDED_JACK;
  _Jacks[0].JackDir  = USB_AUDIO_MIDI_IN_JACK;
  _Jacks[0].JackID   = 64;
  //_Jacks[0].pJackName = "First jack (IN)"; // Increase USB_MAX_STRING_DESC for this to work.
  _Jacks[0].pJackName = NULL;
  _Jacks[0].NrInputPins = 0;

  _Jacks[1].JackType = USB_AUDIO_MIDI_EXTERNAL_JACK;
  _Jacks[1].JackDir  = USB_AUDIO_MIDI_IN_JACK;
  _Jacks[1].JackID   = 128;
  _Jacks[1].pJackName = NULL;
  _Jacks[1].NrInputPins = 0;

  _SourcePin = 1;
  _Jacks[2].JackType = USB_AUDIO_MIDI_EMBEDDED_JACK;
  _Jacks[2].JackDir  = USB_AUDIO_MIDI_OUT_JACK;
  _Jacks[2].JackID   = 32;
  _Jacks[2].pJackName = NULL;
  _Jacks[2].NrInputPins = 1;
  _Jacks[2].paSourceID  = &_Jacks[1].JackID;
  _Jacks[2].paSourcePin = &_SourcePin;

  _Jacks[3].JackType = USB_AUDIO_MIDI_EXTERNAL_JACK;
  _Jacks[3].JackDir  = USB_AUDIO_MIDI_OUT_JACK;
  _Jacks[3].JackID   = 16;
  _Jacks[3].pJackName = NULL;
  _Jacks[3].NrInputPins = 1;
  _Jacks[3].paSourceID  = &_Jacks[0].JackID;
  _Jacks[3].paSourcePin = &_SourcePin;

  _Jacks[4].JackType = USB_AUDIO_MIDI_EMBEDDED_JACK;
  _Jacks[4].JackDir  = USB_AUDIO_MIDI_IN_JACK;
  _Jacks[4].JackID   = 8;
  _Jacks[4].pJackName = NULL;
  _Jacks[4].NrInputPins = 1;
  _Jacks[4].paSourceID  = &_Jacks[0].JackID;
  _Jacks[4].paSourcePin = &_SourcePin;

  InitData.paJackList = _Jacks;
  InitData.NumJacks   = SEGGER_COUNTOF(_Jacks);
  hInst = USBD_MIDI_Add(&InitData);
  return hInst;
}


/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       MainTask
*
* Function description
*   USB handling task.
*   Modify to implement the desired protocol
*/
#ifdef __cplusplus
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif
void MainTask(void);
#ifdef __cplusplus
}
#endif
void MainTask(void) {
  USBD_MIDI_HANDLE hInst;
  unsigned i;
  U8 InSysEx;
  U8 CableNumber;
  U8 CIN;
  int r;

  USBD_Init();
  hInst = _AddMIDI();
  USBD_SetDeviceInfo(&_DeviceInfo);
  USBD_Start();

  _p1.CableNumber_and_CIN = 0x09; // CIN "Note on"; Cable Index 0
  _p1.MIDI_0 = 0x90;              // CIN "Note on"; Channel 0
  _p1.MIDI_1 = 0x3A;              // Note A#3
  _p1.MIDI_2 = 0x2D;              // Velocity value
  _p2.CableNumber_and_CIN = 0x08; // CIN "Note off"; Cable Index 0
  _p2.MIDI_0 = 0x80;              // CIN "Note off"; Channel 0
  _p2.MIDI_1 = 0x3A;              // Note A#3
  _p2.MIDI_2 = 0x18;              // Velocity value
  InSysEx = 0;
  while (1) {
    //
    // Wait for configuration
    //
    while ((USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)) != USB_STAT_CONFIGURED) {
      BSP_ToggleLED(0);
      USB_OS_Delay(50);
    }
    //
    // Read MIDI packets from the host.
    // Passing "-1" as the timeout value makes sure that the function does not block.
    //
    r = USBD_MIDI_ReceivePackets(hInst, _Packets, SEGGER_COUNTOF(_Packets), -1);
    if (r > 0) {
      for (i = 0; i < (unsigned)r; i++) {
        if (InSysEx == 1) {
          if (_Packets[i].CableNumber_and_CIN == MIDI_USB_CIN_SYSEX_BEGIN_CONT) {
            USBD_Logf_Application("System Exclusive continues...: [0x%x] [0x%x] [0x%x]", _Packets[i].MIDI_0, _Packets[i].MIDI_1, _Packets[i].MIDI_2);
          } else {
            USBD_Logf_Application("System Exclusive ends: [0x%x] [0x%x] [0x%x]", _Packets[i].MIDI_0, _Packets[i].MIDI_1, _Packets[i].MIDI_2);
            InSysEx = 0;
          }
        } else {
          if (_Packets[i].CableNumber_and_CIN == MIDI_USB_CIN_SYSEX_BEGIN_CONT) {
            USBD_Logf_Application("Received start of System Exclusive transfer: [0x%x] [0x%x] [0x%x]", _Packets[i].MIDI_0, _Packets[i].MIDI_1, _Packets[i].MIDI_2);
            InSysEx = 1;
          } else {
            CableNumber = USBD_MIDI_GET_CABLE(_Packets[i].CableNumber_and_CIN);
            CIN = USBD_MIDI_GET_CID(_Packets[i].CableNumber_and_CIN);
            USBD_Logf_Application("Received MIDI packet for jack %d: %s, [0x%x] [0x%x] [0x%x]", CableNumber, _CIN2Str(CIN), _Packets[i].MIDI_0, _Packets[i].MIDI_1, _Packets[i].MIDI_2);
          }
        }
      }
    } else if (r < 0) {
      USBD_Logf_Application("Warning: USBD_MIDI_Receive returned %d!", r);
      InSysEx = 0;
    }
    //
    // Write "note on" to the host.
    //
    USBD_MIDI_WritePackets(hInst, &_p1, 1, 5);
    USB_OS_Delay(25);
    //
    // Write "note off" to the host.
    //
    USBD_MIDI_WritePackets(hInst, &_p2, 1, 5);
    USB_OS_Delay(25);
  }
}
