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
File    : USBHID_macOS.c
Purpose : Implementation of the USBHID functions
---------------------------END-OF-HEADER------------------------------
*/

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDManager.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <pthread.h>
#include <syslog.h>
#include <sys/stat.h>
#include <wchar.h>
#include <locale.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <dlfcn.h>
#include "USBHID.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
#define HID_DEBUG   0

/*********************************************************************
*
*       Defines, non configurable
*
**********************************************************************
*/

//
// HID report descriptor defines for the simple parser.
//
#define USBH_HID_REPORT_COUNT             0x95
#define USBH_HID_REPORT_SIZE              0x75
#define USBH_HID_REPORT_INPUT             0x80
#define USBH_HID_REPORT_OUTPUT            0x90
#define USBH_HID_REPORT_INPUT_OUTPUT_MASK 0xFC
#define USBH_HID_REPORT_LONG_ITEM         0xFE

/*********************************************************************
*
*       Types
*
**********************************************************************
*/
typedef struct {
  IOHIDDeviceRef  hDevice;           // handle to device
  unsigned        InputReportByteLength;
  unsigned        OutputReportByteLength;
  U16             VendorId;
  U16             ProductId;
  CFStringRef     RunLoopMode;
  U8            * pBufRd;
  U8            * pBufWr;
  U8              PacketReceived;
  char            acProductName[64];
  char            acSerial[32];
  char            acPath[512];
} CONN_INFO;

typedef struct {
  uintptr_t a;   // First field of IOHIDDevice struct is <CFRuntimeBase>. This consists of a, b and c: http://opensource.apple.com/source/CF/CF-476.18/CFRuntime.h
  uint8_t b[4];
  #if __LP64__
  uint32_t c;
  #endif
  io_service_t service;  // Service of device
} _IOHIDDDEVICE_INT;

typedef io_service_t (FUNC_IO_HID_DEVICE_GET_SERVICE)(IOHIDDeviceRef hDevice);


/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static CONN_INFO * _apConnInfo[USB_MAX_DEVICES];
static unsigned    _NumDevices;
static U16         _VendorPage;

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/


/*********************************************************************
*
*         _cbOnInReport
*
*  Function description
*    Called whenever a report (packet) is received from the device.
*    As this function can only get called while the run loop of the associated thread is active,
*    in reality we can only get in here when USBHID_Read() calls
*/
static void _cbOnInReport(void* pContext, IOReturn Result, void* pSender, IOHIDReportType ReportType, uint32_t ReportId, uint8_t* pReport, CFIndex ReportLen) {
  CONN_INFO* pConnInfo;
  //
  // When getting here, the packet has already been copied into the buffer we specified when registering the callback
  // We simply set a flag to indicate reception of a packet.
  //
  (void)pContext;
  (void)Result;
  (void)pSender;
  (void)ReportType;
  (void)ReportId;
  (void)pReport;
  (void)ReportLen;
  pConnInfo = (CONN_INFO*)pContext;
  pConnInfo->PacketReceived = 1;  // Indicate packet reception to caller.
}

/*********************************************************************
*
*         _IORegGetPropTypeInt
*
*  Function description
*    Gets an integer property from the IOReg
*/
static int _IORegGetPropTypeInt(IOHIDDeviceRef hDevice, CFStringRef sKey) {
  CFTypeRef hProp;
  int v;

  v = 0;
  hProp = IOHIDDeviceGetProperty(hDevice, sKey);
  if (hProp) {
    if (CFGetTypeID(hProp) == CFNumberGetTypeID()) {                  // Make sure that the actual type of the attribute is an integer
      CFNumberGetValue((CFNumberRef)hProp, kCFNumberSInt32Type, &v);
    }
  }
  return v;
}

/*********************************************************************
*
*         _IORegGetPropTypeStr
*
*  Function description
*    Gets a string property from the IOReg
*
*  Return value
*    >= 0  O.K.
*     < 0  Error
*
*  Notes
*    (1) IO strings consist of 16-bit characters
*/
static int _IORegGetPropTypeStr(IOHIDDeviceRef hDevice, CFStringRef sKey, char* pBuf, U32 BufSize) {
  int Result;
  CFTypeRef ObjProperty;
  //
  // Details / specifics etc: see _GetIORegEntryString()
  //
  memset(pBuf, 0, BufSize);
	ObjProperty = IOHIDDeviceGetProperty(hDevice, sKey);
  if (ObjProperty) {
    if (CFGetTypeID(ObjProperty) == CFStringGetTypeID()) {
      CFStringGetCString((CFStringRef)ObjProperty, pBuf, BufSize, kCFStringEncodingUTF8);
      Result = 0;
    } else {
      Result = -3;
    }
  } else {
    Result = -2;
  }
  return Result;
}

/*********************************************************************
*
*       _ReadReportDescriptor
*
*  Function description
*    Read and parse report descriptor
*/
static int _ReadReportDescriptor(IOHIDDeviceRef hDevice, CONN_INFO *pInfo, U16 *pVendorPage) {
  char acBuff[512];
  int  Len;
  U8   *p;
  U16  RptSize;
  U16  RptCount;
  U32  ReportLenTemp;
  U32  InRptLen;
  U32  OutRptLen;
  CFDataRef ref;

  pInfo->InputReportByteLength  = 0;
  pInfo->OutputReportByteLength = 0;
  *pVendorPage = 0;
  ref = IOHIDDeviceGetProperty(hDevice, CFSTR(kIOHIDReportDescriptorKey));
  if (!ref) {
    return 1;
  }
  Len = CFDataGetLength(ref);
  memcpy(acBuff, CFDataGetBytePtr(ref), Len);
#if HID_DEBUG
  {
    int i;
    fprintf(stderr,"HID: report descriptor:\n");
    for (i = 0; i < Len; i++) {
      fprintf(stderr," %02X", acBuff[i] & 0xFF);
    }
    fprintf(stderr,"\n");
  }
#endif
  p = (unsigned char *)acBuff;
  RptSize  = 0;
  RptCount = 0;
  ReportLenTemp = 0;
  InRptLen = 0;
  OutRptLen = 0;
  while (Len > 0) {
    unsigned char cc;
    int ItemLen;

    cc = *p;
    if (cc == 0x06 && Len >= 3) {
     *pVendorPage = p[1] | (p[2] << 8);
    }
    if ((cc & USBH_HID_REPORT_INPUT_OUTPUT_MASK) == USBH_HID_REPORT_INPUT) {    // Input tag
      InRptLen += ReportLenTemp;
      ReportLenTemp = 0;
#if HID_DEBUG
      fprintf(stderr, "HID: INPUT report size = %u bits\n", (unsigned)InRptLen);
#endif
    }
    if ((cc & USBH_HID_REPORT_INPUT_OUTPUT_MASK) == USBH_HID_REPORT_OUTPUT) {    // Output tag
      OutRptLen += ReportLenTemp;
      ReportLenTemp = 0;
#if HID_DEBUG
      fprintf(stderr, "HID: OUTPUT report size = %u bits\n", (unsigned)OutRptLen);
#endif
    }
    if (cc == USBH_HID_REPORT_COUNT && Len >= 2) { // Report count
      RptCount = p[1];
#if HID_DEBUG
      fprintf(stderr, "HID: report count = %u\n", RptCount);
#endif
    }
    if (cc == USBH_HID_REPORT_SIZE && Len >= 2) { // Report size
      RptSize = p[1];
#if HID_DEBUG
      fprintf(stderr, "HID: report size (bits) = %u\n", RptSize);
#endif
    }
    //
    // Multiple report size/count pairs can exist
    //
    if (RptCount != 0 && RptSize != 0) {
      ReportLenTemp += RptCount * RptSize;
      RptCount = 0;
      RptSize  = 0;
    }
    //
    // now move to next item
    //
    if ((cc & USBH_HID_REPORT_LONG_ITEM) == USBH_HID_REPORT_LONG_ITEM) {    // Long item
      if (Len < 3) {
        break;
      }
      ItemLen = p[1] + 3;
    } else {
      //
      // short item
      // 0 = 0 bytes
      // 1 = 1 byte
      // 2 = 2 bytes
      // 3 = 4 bytes
      //
      ItemLen = (cc & 3) + 1;
      if (ItemLen == 4) {
        ItemLen = 5;
      }
    }
    p   += ItemLen;
    Len -= ItemLen;
  }
  pInfo->InputReportByteLength = _IORegGetPropTypeInt(hDevice, CFSTR(kIOHIDMaxInputReportSizeKey));
  pInfo->OutputReportByteLength = _IORegGetPropTypeInt(hDevice, CFSTR(kIOHIDMaxOutputReportSizeKey));
#if HID_DEBUG
  fprintf(stderr, "HID: final INPUT report size = %u\n", pInfo->InputReportByteLength);
  fprintf(stderr, "HID: final OUTPUT report size = %u\n", pInfo->OutputReportByteLength);
#endif
  return 0;
}

/*********************************************************************
*
*       _Enumerate
*
*  Function description
*    Finds HID devices that comply with the Vendor page set.
*/
static void _Enumerate(void) {
  IOHIDManagerRef hHIDManager;
  void* pIOKitFramework;
  FUNC_IO_HID_DEVICE_GET_SERVICE* pf;
  IOHIDDeviceRef* pahDevice;
  CFIndex NumDevices;
  CFSetRef DeviceSet;
  int i;
  int r;
  int NumDevicesFound;
  U32 AllocSize;
  io_object_t ObjIOKit;
  char ac[512];
  U32 VendorId;
  U32 ProductId;
  IOHIDDeviceRef  hDevice;
  CONN_INFO       Info;
  U16             VendorPage;
  //
  // Init HID manager, if necessary
  //
  pahDevice       = NULL;
  NumDevicesFound = 0;
  hHIDManager     = 0;
  //
  // Determine IOHIDDeviceGetService()
  // However, under OS X <= 10.5 this API did not exist yet, so we extract the info from device handle itself
  // OS X 10.5: The device handle is really a pointer to a struct that contains the service
  //
  pIOKitFramework = dlopen("/System/Library/IOKit.framework/IOKit", RTLD_LAZY);
  if (pIOKitFramework != NULL) {
    pf = (FUNC_IO_HID_DEVICE_GET_SERVICE*)dlsym(pIOKitFramework, "IOHIDDeviceGetService");
  } else {
    pf = NULL;
  }
  //
  // Open HID manager
  //
  hHIDManager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
  IOHIDManagerSetDeviceMatching(hHIDManager, NULL);
  IOHIDManagerScheduleWithRunLoop(hHIDManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);  // Assign event handling of HID manager instance to run loop of this thread (needed for asynchronous HID manager calls)
  //
  // Wait until HID manager has performed an update
  //
  do {
    r = CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.001, FALSE);   // Give HID manager some time to finalize any outstanding operations
    if (r == kCFRunLoopRunFinished) {                              // Run loop no longer exists? => Done
      break;
    }
    if (r == kCFRunLoopRunTimedOut) {                              // Run loop timed out? => No events were pending. HID manager finished...
      break;
    }
  } while (1);
  //
  // Get a list of connected devices
  // and iterate through that list to find matching ones
  //
  IOHIDManagerSetDeviceMatching(hHIDManager, NULL);            // Get a list of the Devices
  DeviceSet   = IOHIDManagerCopyDevices(hHIDManager);          // Must be freed if no longer used!
  NumDevices  = CFSetGetCount(DeviceSet);
  AllocSize   = NumDevices * sizeof(IOHIDDeviceRef);
  pahDevice   = (IOHIDDeviceRef*)calloc(1, AllocSize);
  CFSetGetValues(DeviceSet, (const void**)pahDevice);          // Get list into array, so we can easily iterate through it
  for (i = 0; i < NumDevices; i++) {
    hDevice = pahDevice[i];
    if (!hDevice) {
      continue;
    }
    //
    // Get VID, PID, S/N of device
    //
    VendorId = _IORegGetPropTypeInt(hDevice, CFSTR(kIOHIDVendorIDKey));
    ProductId = _IORegGetPropTypeInt(hDevice, CFSTR(kIOHIDProductIDKey));
    if (pf) {                  // OS X >= 10.6
      ObjIOKit = pf(hDevice);
    } else {
      ObjIOKit = ((_IOHIDDDEVICE_INT*)hDevice)->service;
    }
    r = IORegistryEntryGetPath(ObjIOKit, kIOServicePlane, ac);            // Get path of device from IOReg
    if (r != KERN_SUCCESS) {
#if HID_DEBUG
      fprintf(stderr, "_GetDeviceList(): Error while retrieving device path from IO registry\n");
#endif
      continue;
    }
    //
    // Remember enumeration information
    //
    Info.VendorId = VendorId;
    Info.ProductId = ProductId;
    strncpy(Info.acPath, ac, sizeof(Info.acPath) - 1);
    _IORegGetPropTypeStr(hDevice, CFSTR(kIOHIDProductKey), ac, sizeof(ac));
    strncpy(Info.acProductName, ac, sizeof(Info.acProductName) - 1);
    _IORegGetPropTypeStr(hDevice, CFSTR(kIOHIDSerialNumberKey), ac, sizeof(ac));
    strncpy(Info.acSerial, ac, sizeof(Info.acSerial) - 1);
#if HID_DEBUG
    fprintf(stderr,"HID: Found device '%s' VID=%04X PID=%04X -> %s\n", Info.acProductName, Info.VendorId, Info.ProductId, Info.acPath);
#endif
    if (_ReadReportDescriptor(hDevice, &Info, &VendorPage) == 0 &&
        VendorPage == _VendorPage &&
        _NumDevices < USB_MAX_DEVICES) {
      //
      // Allocate memory for connection info if necessary.
      //
      if (_apConnInfo[_NumDevices] == NULL) {
        _apConnInfo[_NumDevices] = (CONN_INFO *)calloc(1, sizeof(CONN_INFO));
      }
      Info.hDevice   = NULL;
      *_apConnInfo[_NumDevices] = Info;
      _NumDevices++;
    }
    NumDevicesFound++;
  }
  //
  // Free temporary resources
  //
  free(pahDevice);
  CFRelease(DeviceSet);
  if (pIOKitFramework) {
    dlclose(pIOKitFramework);
  }
  if (hHIDManager) {
    IOHIDManagerClose(hHIDManager, kIOHIDOptionsTypeNone);
    CFRelease(hHIDManager);
  }
}

/*********************************************************************
*
*       _Id2Info
*
*  Function description
*    returns CONN_INFO pointer for a device with given id.
*/
static CONN_INFO *_Id2Info(unsigned Id) {
  if (Id < USB_MAX_DEVICES) {
    return(_apConnInfo[Id]);
  }
  return NULL;
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       USBHID_Open
*
*  Function description
*    Opens a handle to the device that shall be opened.
*
*  Return value:
*    0       -  O.K. Opening was successful or already opened.
*    1       -  Error. Handle to the device could not opened.
*/
int USBHID_Open(unsigned Id) {
  CONN_INFO * pConnInfo;
  int r;
  int Result;
  io_registry_entry_t hIORegEntry;

  Result = 0;   // Indicate success
  pConnInfo = _Id2Info(Id);
  if (pConnInfo == NULL)  {
    return 1;   // Error device Id does not exist.
  }
  if (pConnInfo->hDevice == NULL) {
    hIORegEntry = IORegistryEntryFromPath(kIOMasterPortDefault, pConnInfo->acPath);
    if (hIORegEntry == MACH_PORT_NULL) {                                              // No valid path? (E.g. device disconnected) => Done
#if HID_DEBUG
      fprintf(stderr,"_OpenConnection(): Error while opening IO registry entry for '%s'\n", pConnInfo->acPath);
#endif
      Result = 1;
      goto Done;
    }
    pConnInfo->hDevice = IOHIDDeviceCreate(kCFAllocatorDefault, hIORegEntry);         // Create device instance we can work with (allocates resources) => Must be explicitly freed after close
    if (pConnInfo->hDevice == NULL) {
#if HID_DEBUG
      fprintf(stderr,"_OpenConnection(): Error while creating HID device object\n");
#endif
      Result = 1;
      goto Done;
    }
    r = IOHIDDeviceOpen(pConnInfo->hDevice, kIOHIDOptionsTypeSeizeDevice);            // Open the device instance for communication
    if (r != kIOReturnSuccess) {
#if HID_DEBUG
      fprintf(stderr,"_OpenConnection(): Error while opening HID device\n");
#endif
      Result = 1;
      goto Done;
    }
    pConnInfo->InputReportByteLength = _IORegGetPropTypeInt(pConnInfo->hDevice, CFSTR(kIOHIDMaxInputReportSizeKey));
    if (pConnInfo->InputReportByteLength <= 0) {
      Result = 1;
      goto Done;
    }
    pConnInfo->OutputReportByteLength = _IORegGetPropTypeInt(pConnInfo->hDevice, CFSTR(kIOHIDMaxOutputReportSizeKey));
    if (pConnInfo->OutputReportByteLength <= 0) {
      Result = 1;
      goto Done;
    }
    pConnInfo->pBufRd = malloc(pConnInfo->InputReportByteLength);
    if (pConnInfo->pBufRd == NULL) {
      Result = 1;
      goto Done;
    }
    pConnInfo->pBufWr = malloc(pConnInfo->OutputReportByteLength);
    if (pConnInfo->pBufWr == NULL) {
      free(pConnInfo->pBufRd);
      Result = 1;
      goto Done;
    }
    //
    // Create a custom run mode for the run loop (allows us to fire only for incoming packet events when running the run loop and no other events)
    // Register callback for incoming packets
    // Associate the device events with the run loop of this thread
    //
    pConnInfo->RunLoopMode = CFStringCreateWithCString(NULL, "RunMode12345678", kCFStringEncodingASCII);      // Custom run mode for the run loop that handles incoming packets
    IOHIDDeviceRegisterInputReportCallback(pConnInfo->hDevice, pConnInfo->pBufRd, pConnInfo->InputReportByteLength, _cbOnInReport, pConnInfo);  // Register callback for incoming USB packets
    IOHIDDeviceScheduleWithRunLoop(pConnInfo->hDevice, CFRunLoopGetCurrent(), pConnInfo->RunLoopMode);        // Associate device events with run loop of this thread
    //
    // In case of error, free all persistent resources
    // Always free temporary ones
    //
Done:
    if (Result != 0) {
      if (pConnInfo->hDevice != NULL) {       // Close device, if necessary
        CFRelease(pConnInfo->hDevice);
        pConnInfo->hDevice = NULL;
      }
    } else {

    }
    if (hIORegEntry != MACH_PORT_NULL) {      // Free IOReg entry, if necessary
      IOObjectRelease(hIORegEntry);
    }
  }
  return 0;
}

/*********************************************************************
*
*       USBHID_Close
*
*  Function description
*    Close the connection to an open device.
*/
void USBHID_Close(unsigned Id) {
  CONN_INFO * pConnInfo;

  pConnInfo = _Id2Info(Id);
  if (pConnInfo) {
    if (pConnInfo->hDevice != NULL) {
      //
      // Unregister callbacks and run loop mode
      //
      IOHIDDeviceRegisterInputReportCallback(pConnInfo->hDevice, pConnInfo->pBufRd, pConnInfo->InputReportByteLength, NULL, NULL);  // Unregister callback for incoming packets
      IOHIDDeviceUnscheduleFromRunLoop(pConnInfo->hDevice, CFRunLoopGetCurrent(), pConnInfo->RunLoopMode);
      IOHIDDeviceScheduleWithRunLoop(pConnInfo->hDevice, CFRunLoopGetMain(), kCFRunLoopDefaultMode);                         // Assign run loop of main thread to events of device instance
      //
      // Close device and HID manager
      //
      IOHIDDeviceClose(pConnInfo->hDevice, kIOHIDOptionsTypeSeizeDevice);
      CFRelease(pConnInfo->hDevice);
      pConnInfo->hDevice = NULL;
      if (pConnInfo->RunLoopMode) {
        CFRelease(pConnInfo->RunLoopMode);           // Run loop mode is no longer needed
        pConnInfo->RunLoopMode = 0;
      }
      if (pConnInfo->pBufRd != NULL) {
        free(pConnInfo->pBufRd);
      }
      if (pConnInfo->pBufWr != NULL) {
        free(pConnInfo->pBufWr);
      }
    }
  }
}

/*********************************************************************
*
*       USBHID_Init
*
*  Function description
*   Initializes the USB HID User API.
*/
void USBHID_Init(U8 VendorPage) {
  USBHID_SetVendorPage(VendorPage);
  _Enumerate();
}

/*********************************************************************
*
*       USBHID_Exit
*
*  Function description
*    Close the connection an open device.
*/
void USBHID_Exit(void) {
  unsigned    i;

  for (i = 0; i < _NumDevices; i++) {
    USBHID_Close(i);
  }
  _NumDevices = 0;
}

/*********************************************************************
*
*       USBHID_Read
*
*  Function description
*    Reads an input report from device via the interrupt endpoint.
*
*  Return value:
*    On Error:   -1, No valid device Id used or the report size does not match with device.
*    On success: Number of bytes that have be written.
*/
int USBHID_Read(unsigned Id, void * pBuffer, unsigned NumBytes) {
  CONN_INFO * pConnInfo;
  int         r;

  pConnInfo = _Id2Info(Id);
  if (pConnInfo == NULL || pConnInfo->hDevice == NULL)  {
    return -1;   // Error device Id does not exist or not open.
  }
  if (NumBytes != (pConnInfo->InputReportByteLength)) {
    return -1;   // Error report size does not match
  }
#if HID_DEBUG
  fprintf(stderr,"HID: wait for %d bytes to read\n", NumBytes);
#endif
  do {
    r = CFRunLoopRunInMode(pConnInfo->RunLoopMode, 5, TRUE);
    if (r != kCFRunLoopRunHandledSource) {
      break;
    }
    if (pConnInfo->PacketReceived) {   // Packet received? => Done
      pConnInfo->PacketReceived = 0;
      //
      // macOS API does not provide a way to check how many bytes were really read.
      // So we just return NumBytes if the read was successful.
      //
      r = NumBytes;
      memcpy(pBuffer, pConnInfo->pBufRd, NumBytes);
      break;
    }
  } while (1);
#if HID_DEBUG
  fprintf(stderr,"HID: USBHID_Read returns %d\n", r);
#endif
  return r;
}

/*********************************************************************
*
*       USBHID_GetReport
*
*  Function description
*    Reads an input report the from device via a setup request.
*
*  Return value:
*    On Error:   -1, No valid device Id used or the report size does not match with device.
*    On success: Number of bytes that have be read.
*/
int USBHID_GetReport(unsigned Id, void * pBuffer, unsigned NumBytes, unsigned ReportId) {
  CONN_INFO * pConnInfo;

  pConnInfo = _Id2Info(Id);
  if (pConnInfo == NULL || pConnInfo->hDevice == NULL)  {
    return -1;   // Error device Id does not exist or not open.
  }
  if (NumBytes != (pConnInfo->InputReportByteLength)) {
    return -1;   // Error report size does not match
  }
  return -1;     // not implemented yet
}

/*********************************************************************
*
*       USBHID_Write
*
*  Function description
*    Writes an output report to device.
*
*  Return value:
*    On Error:   -1, No valid device Id used or the report size does not match with device.
*    On success: Number of bytes that have be written.
*/
int USBHID_Write(unsigned Id, const void * pBuffer, unsigned NumBytes) {
  CONN_INFO * pConnInfo;
  int res;
  U8 *pTmpBuff;

  pConnInfo = _Id2Info(Id);
  if (pConnInfo == NULL || pConnInfo->hDevice == NULL)  {
    return -1;   // Error device Id does not exist or not open.
  }
  if (NumBytes != (pConnInfo->OutputReportByteLength)) {
    return -1;   // Error report size does not match
  }
  pTmpBuff = alloca(pConnInfo->OutputReportByteLength + 1);
  memcpy(pTmpBuff, pBuffer, NumBytes);
#if HID_DEBUG
  fprintf(stderr,"HID: IOHIDDeviceSetReport %d bytes\n", NumBytes);
#endif
  res = IOHIDDeviceSetReport(pConnInfo->hDevice, kIOHIDReportTypeOutput, 0x00, pTmpBuff, NumBytes);
  if (res != 0) {
    fprintf(stderr, "errno=%d\n",errno);
    perror("HID: write");
  } else {
    //
    // macOS API does not provide a way to check how many bytes were really written.
    // So we just return NumBytes if the write was successful.
    //
    res = NumBytes;
  }
  return res;
}

/*********************************************************************
*
*       USBHID_GetNumAvailableDevices
*
*  Function description
*    Returns the number of available devices.
*    pMask will be filled by this routine.
*    pMask shall be interpreted as bit mask
*    where a bit set means this device is available.
*    eg. *pMask = 0x00000005 -> Device 0 and device 2 are available.
*
*  Return value:
*    Number of available device(s).
*/
unsigned USBHID_GetNumAvailableDevices(U32 * pMask) {
  unsigned      i;
  U32           Mask;
  CONN_INFO   * pConnInfo;

  if (_NumDevices == 0) {
    _Enumerate();
  }
  Mask  = 0;
  for (i = 0; i < _NumDevices; i++) {
    pConnInfo = _apConnInfo[i];
    if (pConnInfo != NULL) {
      Mask |= (1 << i);
    }
  }
  if (pMask) {
    *pMask = Mask;
  }
  return _NumDevices;
}

/*********************************************************************
*
*       USBHID_GetProductName
*
*  Function description
*    Stores the name of the device into pBuffer.
*
*  Return value:
*    0    - Error
*    1    - O.K.
*/
int USBHID_GetProductName(unsigned Id, char * pBuffer, unsigned NumBytes) {
  CONN_INFO    * pConnInfo;

  pConnInfo = _Id2Info(Id);
  if (pConnInfo) {
    strncpy(pBuffer, pConnInfo->acProductName, NumBytes);
    return 1;
  }
  return 0;
}

/*********************************************************************
*
*       USBHID_GetSerialNumber
*
*  Function description
*    Stores the serial number string of the device into pBuffer.
*
*  Return value:
*    0    - Error
*    1    - O.K.
*/
int USBHID_GetSerialNumber(unsigned Id, char * pBuffer, unsigned NumBytes) {
  CONN_INFO    * pConnInfo;

  pConnInfo = _Id2Info(Id);
  if (pConnInfo) {
    strncpy(pBuffer, pConnInfo->acSerial, NumBytes);
    return 1;
  }
  return 0;
}

/*********************************************************************
*
*       USBHID_GetInputReportSize
*
*  Function description
*    Returns the input report size of the device.
*
*  Return value:
*    == 0    - Error
*    <> 0    - report size in bytes.
*/
int USBHID_GetInputReportSize(unsigned Id) {
  CONN_INFO    * pConnInfo;

  pConnInfo = _Id2Info(Id);
  if (pConnInfo) {
    return pConnInfo->InputReportByteLength;
  }
  return 0;
}

/*********************************************************************
*
*       USBHID_GetOutputReportSize
*
*  Function description
*    Returns the outpur report size of the device.
*
*  Return value:
*    == 0    - Error
*    <> 0    - report size in bytes.
*/
int USBHID_GetOutputReportSize(unsigned Id) {
  CONN_INFO    * pConnInfo;

  pConnInfo = _Id2Info(Id);
  if (pConnInfo) {
    return pConnInfo->OutputReportByteLength;
  }
  return 0;
}

/*********************************************************************
*
*       USBHID_GetProductId
*
*  Function description
*    Returns the USB product id of the device.
*
*  Return value:
*    == 0    - Error
*    <> 0    - Product Id
*/
U16 USBHID_GetProductId(unsigned Id) {
  CONN_INFO    * pConnInfo;

  pConnInfo = _Id2Info(Id);
  if (pConnInfo) {
    return pConnInfo->ProductId;
  }
  return 0;
}

/*********************************************************************
*
*       USBHID_GetVendorId
*
*  Function description
*    Returns the USB vendor id of the device.
*
*  Return value:
*    == 0    - Error
*    <> 0    - Vendor Id
*/
U16 USBHID_GetVendorId(unsigned Id) {
  CONN_INFO    * pConnInfo;

  pConnInfo = _Id2Info(Id);
  if (pConnInfo) {
    return pConnInfo->VendorId;
  }
  return 0;
}

/*********************************************************************
*
*       USBHID_SetVendorPage
*
*  Function description
*    Sets the vendor page so that all HID devices with the specified
*    page will be found.
*/
void USBHID_SetVendorPage(U8 Page) {
  _VendorPage = (0xff << 8) | Page;
}

/*********************************************************************
*
*       USBHID_RefreshList
*
*  Function description
*    Refreshes the connection info list.
*
*    Notes:
*      (1)   - Be aware. Any open handles to the device will be closed.
*/
void USBHID_RefreshList(void) {
  USBHID_Exit();
  _Enumerate();
}

/*************************** End of file ****************************/
