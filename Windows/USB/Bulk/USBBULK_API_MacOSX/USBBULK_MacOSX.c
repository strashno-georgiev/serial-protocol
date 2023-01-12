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
File    : USBBULK_MacOSX.c
Purpose : USB API functions
---------------------------END-OF-HEADER------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>
#include <poll.h>
#include "USBBULK.h"
#include <sys/ioctl.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <mach/mach.h>
#include <pthread.h>

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#ifndef USBBULK_DEBUG
  #define USBBULK_DEBUG           0
#endif

#define DEFAULT_READ_TIMEOUT   5000
#define DEFAULT_WRITE_TIMEOUT  5000


#define MAX_DEVICE_ITEMS          5
#define MAX_PATH                256

#define READ_BUFF_SIZE       0x8000
#define MAX_WRITE_SIZE       0x4000

#if USBBULK_DEBUG > 0
  #define LOG(Args)  _Log Args
#else
  #define LOG(Args)
#endif

/*********************************************************************
*
*       Defines, non-configurable
*
**********************************************************************
*/

#define USBBULK_VERSION    10000

#define _IOREGISTRY_ATTR_VID         (0)
#define _IOREGISTRY_ATTR_PID         (1)
#define _IOREGISTRY_ATTR_USBSN       (2)
#define _IOREGISTRY_ATTR_LOCATION_ID (3)
#define _IOREGISTRY_ATTR_VENDOR      (4)
#define _IOREGISTRY_ATTR_PRODUCT     (5)

#define _IOREGISTRY_ATTR_TYPE_INT    (0)
#define _IOREGISTRY_ATTR_TYPE_STR    (1)

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

typedef struct _DEV_INST {
  int             Handle;
  U32             LocationId;
  IOUSBInterfaceInterface190 ** ppInterface;
  IOUSBDeviceInterface182    ** ppDevice;
  int             ReadTimeout;
  int             WriteTimeout;
  U8              ShortMode;
  UInt8           InPipe;
  UInt8           OutPipe;
  char            acName[64];
  char            acSN[64];
  char            acVendor[64];
  U16             VendorId;
  U16             ProductId;
  U32             NumBytesInBuffer;
  U8            * pBuffData;
  U8              acBuffer[READ_BUFF_SIZE];
} DEV_INST;

typedef struct _DEVICE_ITEM_LIST {
  U16         VendorId;
  U16         ProductId;
} DEVICE_ITEM_LIST;

typedef struct _USBBULK_GLOBAL {
  int                         NextHandle;
  USBBULK_NOTIFICATION_FUNC * pfOnUserNotification;
  void                      * pfUserNotificationContext;
  pthread_t                   thHandle;
  pthread_mutex_t             Mutex;
  char                        ThreadActive;
  char                        RunLoopActive;
  CFRunLoopRef                RunLoop;
} USBBULK_GLOBAL;

typedef struct {
  USB_BULK_HANDLE hDevice;
  U32             NumBytes;
  const U8        *pData;
} WRITE_JOB;

/*********************************************************************
*
*       Static const
*
**********************************************************************
*/

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static int              _IsInited;
static USBBULK_GLOBAL   _Global;
static DEVICE_ITEM_LIST _DeviceItemList[MAX_DEVICE_ITEMS];
static DEV_INST        *_Devices[USBBULK_MAX_DEVICES];

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _Log
*/
#if USBBULK_DEBUG > 0
static void _Log(const char * pFormat, ...) {
  va_list ArgList;
  static FILE *LogFd = NULL;

  if (LogFd == NULL) {
    LogFd = fopen("/tmp/USBBULK.log", "a");
  }
  if (LogFd != NULL) {
    va_start(ArgList, pFormat);
    vfprintf(LogFd, pFormat, ArgList);
    va_end(ArgList);
    fflush(LogFd);
  }
}
#endif

/*********************************************************************
*
*       _ErrorToString
*/
#if USBBULK_DEBUG > 0
static const char * _ErrorToString(int ErrorCode) {
  static char ac[256];

  switch (ErrorCode) {
    case kIOReturnAborted:
      return "Operation aborted";
    case kIOReturnError:
      return "General error";
    case kIOReturnExclusiveAccess:
      return "Device already opened";
    case kIOReturnNoDevice:
      return "Device not found";
    case kIOReturnNotOpen:
      return "Device not opened";
    case kIOReturnNotResponding:
      return "Device not responding";
    case kIOReturnSuccess:
      return "Success";
    case kIOUSBTransactionTimeout:
      return "Timeout";
    default:
      snprintf(ac, sizeof(ac), "Unknown error code: 0x%08x", ErrorCode);
      return ac;
  }
}
#endif

/*********************************************************************
*
*       _GetPropertyIORegEntry
*
*  Function description
*    Gets a specific property of a IORegistry entry.
*
*  Return value
*    >= 0  O.K.
*     < 0  Error
*/
static int _GetPropertyIORegEntry(io_service_t Service, int Attr, void* pBuffer, U32 BufferSize) {
  U8* p;
  int AttrType;
  U32 ValSize;
  CFTypeRef ObjProperty;
  int r;
  //
  // IORegistryEntryCreateCFProperty returns object of type CFTypeRef.
  // Getting the actual type is a bit tricky as documentation is not really good...
  // We tried for some properties by checking them via:
  //   if (CFGetTypeID(ObjProperty) == CFStringGetTypeID())
  //   if (CFGetTypeID(ObjProperty) == CFNumberGetTypeID())
  //   [...]
  // The know the types for the following attributes:
  //   _IOREGISTRY_ATTR_VID          CFNumber
  //   _IOREGISTRY_ATTR_PID          CFNumber
  //   _IOREGISTRY_ATTR_LOCATION_ID  CFNumber
  //   _IOREGISTRY_ATTR_USBSN        CFString
  //
  memset(pBuffer, 0, BufferSize);
  p = (U8*)pBuffer;
  switch(Attr) {
  case _IOREGISTRY_ATTR_VID:
    ObjProperty = IORegistryEntryCreateCFProperty(Service,CFSTR("idVendor"), kCFAllocatorDefault, 0);
    AttrType = _IOREGISTRY_ATTR_TYPE_INT;
    break;
  case _IOREGISTRY_ATTR_PID:
    ObjProperty = IORegistryEntryCreateCFProperty(Service,CFSTR("idProduct"), kCFAllocatorDefault, 0);
    AttrType = _IOREGISTRY_ATTR_TYPE_INT;
    break;
  case _IOREGISTRY_ATTR_LOCATION_ID:
    ObjProperty = IORegistryEntryCreateCFProperty(Service,CFSTR("locationID"), kCFAllocatorDefault, 0);
    AttrType = _IOREGISTRY_ATTR_TYPE_INT;
    break;
  case _IOREGISTRY_ATTR_USBSN:
    ObjProperty = IORegistryEntryCreateCFProperty(Service,CFSTR("USB Serial Number"), kCFAllocatorDefault, 0);
    AttrType = _IOREGISTRY_ATTR_TYPE_STR;
    break;
  case _IOREGISTRY_ATTR_VENDOR:
    ObjProperty = IORegistryEntryCreateCFProperty(Service,CFSTR("USB Vendor Name"), kCFAllocatorDefault, 0);
    AttrType = _IOREGISTRY_ATTR_TYPE_STR;
    break;
  case _IOREGISTRY_ATTR_PRODUCT:
    ObjProperty = IORegistryEntryCreateCFProperty(Service,CFSTR("USB Product Name"), kCFAllocatorDefault, 0);
    AttrType = _IOREGISTRY_ATTR_TYPE_STR;
    break;
  default:
    LOG(("USBBULK _GetPropertyIORegEntry(): Unknown IO registry attribute (%d) requested.\n", Attr));
    ObjProperty = NULL;
    break;
  }
  if (ObjProperty == NULL) {
    LOG(("USBBULK _GetPropertyIORegEntry(): Property (%d) not found.\n", Attr));
    return -1;
  }
  r = 0;
  switch(AttrType) {
  case _IOREGISTRY_ATTR_TYPE_INT:
    ValSize = CFNumberGetByteSize(ObjProperty);
    if (ValSize <= BufferSize) {
      CFNumberGetValue(ObjProperty, CFNumberGetType(ObjProperty), pBuffer);                  // Make sure that &v is large enough to hold the value. Value size can be checked via CFNumberGetByteSize(ObjProperty)
    } else {
      LOG(("USBBULK _GetPropertyIORegEntry(): Attr value too large: %d bytes.\n", ValSize));
    }
    break;
  case _IOREGISTRY_ATTR_TYPE_STR:
    CFStringGetCString((CFStringRef)ObjProperty, pBuffer, BufferSize, kCFStringEncodingUTF8);
    break;
  default:
    LOG(("USBBULK _GetPropertyIORegEntry(): Unknown IO registry attribute (%d) requested.\n", AttrType));
    r = -1;
    break;
  }
  CFRelease(ObjProperty);
  return r;
}

/*********************************************************************
*
*       _ScanForDevices
*
*  Function description:
*    Scan for attached USB devices and store into _Devices[]
*
*/
static void _ScanForDevices(void) {
  int    i;
  int    DevIdx;
  U8     aUsedFlag[USBBULK_MAX_DEVICES];
  DEV_INST                * pDev;
  CFMutableDictionaryRef    Dict;
  io_iterator_t             Iterator;
  io_service_t              Service;

  if (_Global.ThreadActive) {
    pthread_mutex_lock(&_Global.Mutex);
  }
  LOG(("ScanForDevices\n"));
  memset(aUsedFlag, 0, sizeof(aUsedFlag));
  //
  // Use OS X IORegistry to identify all connected USB devices
  // This does not require to open the device
  //
  // IORegistry requests:
  // We need to create a dictionary which defines the services we search for
  // kIOUSBDeviceClassName defines "search for everything that is of type USB device"
  // As there may be more than only one USB device connected, we iterate through all registered/connected USB devices and filter the ones that match our PID/VID
  //
  // To check against what USB devices are connected to the mac, call the following in a terminal:
  // ioreg -p IOUSBDevice
  //
  // To also show the attributes of all of the connected USB devices:
  // ioreg -p IOUSBDevice -l
  //
  Dict = IOServiceMatching(kIOUSBDeviceClassName);                              // Create dictionary that only contains USB devices
  IOServiceGetMatchingServices(kIOMasterPortDefault, Dict, &Iterator);          // Get list of objects that matches the dictionary
  //
  // Add USB BULK devices found to internal list
  //
  Service = IOIteratorNext(Iterator);                                           // Get first object
  while (Service) {
    U32 VendorId;
    U32 ProductId;
    U32 LocationId;
    //
    // Get properties of the device
    // We mainly need: PID, VID, USB serial number
    //
    LOG(("USBBULK _UpdateDeviceList(): Get properties\n"));
    if (_GetPropertyIORegEntry(Service, _IOREGISTRY_ATTR_PID, &ProductId, sizeof(ProductId))) {
      ProductId = 0xFFFFFFFF;
    }
    if (_GetPropertyIORegEntry(Service, _IOREGISTRY_ATTR_VID, &VendorId,  sizeof(VendorId))) {
      VendorId  = 0xFFFFFFFF;
    }
    if (_GetPropertyIORegEntry(Service, _IOREGISTRY_ATTR_LOCATION_ID, &LocationId, sizeof(LocationId))) {
      LocationId = 0xFFFFFFFF;
    }
    LOG(("  VID=%04x, PID=%04X\n", VendorId, ProductId));
    //
    // Serach in item list
    //
    for (i = 0; i < MAX_DEVICE_ITEMS; i++) {
      if (_DeviceItemList[i].VendorId == VendorId && _DeviceItemList[i].ProductId == ProductId) {
        break;
      }
    }
    if (i == MAX_DEVICE_ITEMS) {
      LOG(("  ignored.\n"));
    } else {
      //
      // Check if device is already known
      //
      pDev = NULL;
      for (DevIdx = 0; DevIdx < USBBULK_MAX_DEVICES; DevIdx++) {
        if (_Devices[DevIdx] != NULL &&
            _Devices[DevIdx]->LocationId == LocationId) {
          pDev = _Devices[DevIdx];
          break;
        }
      }
      if (pDev == NULL) {
        //
        // not found, find free slot
        //
        for (DevIdx = 0; DevIdx < USBBULK_MAX_DEVICES; DevIdx++) {
          if (_Devices[DevIdx] == NULL) {
            pDev = (DEV_INST *)malloc(sizeof(DEV_INST));
            memset(pDev, 0, sizeof(DEV_INST));
            pDev->ReadTimeout  = DEFAULT_READ_TIMEOUT;
            pDev->WriteTimeout = DEFAULT_WRITE_TIMEOUT;
            _Devices[DevIdx] = pDev;
            aUsedFlag[DevIdx] = 2;
            break;
          }
        }
      }
      if (pDev == NULL) {
        LOG(("Could not add device. List is full.\n"));
      } else {
        aUsedFlag[DevIdx]   |= 1;
        pDev->VendorId       = VendorId;
        pDev->ProductId      = ProductId;
        pDev->LocationId     = LocationId;
        LOG(("Add device Index: %d, LocationId: %X.\n", DevIdx, LocationId));
        if (_GetPropertyIORegEntry(Service, _IOREGISTRY_ATTR_USBSN, pDev->acSN, sizeof(pDev->acSN) - 1)) {
          pDev->acSN[0] = 0;
        }
        if (_GetPropertyIORegEntry(Service, _IOREGISTRY_ATTR_VENDOR, pDev->acVendor, sizeof(pDev->acVendor) - 1)) {
          pDev->acVendor[0] = 0;
        }
        if (_GetPropertyIORegEntry(Service, _IOREGISTRY_ATTR_PRODUCT, pDev->acName, sizeof(pDev->acName) - 1)) {
          pDev->acName[0] = 0;
        }
        LOG(("   Product=%s, Vendor=%s, SN=%s\n", pDev->acName, pDev->acVendor, pDev->acSN));
      }
    }
    IOObjectRelease(Service);
    Service = IOIteratorNext(Iterator);                                           // Get next object
  }
  IOObjectRelease(Iterator);                                                      // Free iterator
  for (i = 0; i < USBBULK_MAX_DEVICES; i++) {
    if (aUsedFlag[i] == 0 && _Devices[i] != NULL && _Devices[i]->Handle == 0) {
      free(_Devices[i]);
      _Devices[i] = NULL;
      aUsedFlag[i] = 4;
    }
  }
  if (_Global.ThreadActive) {
    pthread_mutex_unlock(&_Global.Mutex);
  }
  if (_Global.pfOnUserNotification) {
    for (i = 0; i < USBBULK_MAX_DEVICES; i++) {
      if (aUsedFlag[i] & 2) {
        _Global.pfOnUserNotification(_Global.pfUserNotificationContext, i, USBBULK_DEVICE_EVENT_ADD);
      }
      if (aUsedFlag[i] & 4) {
        _Global.pfOnUserNotification(_Global.pfUserNotificationContext, i, USBBULK_DEVICE_EVENT_REMOVE);
      }
    }
  }
}

/*********************************************************************
*
*       _Handle2Inst
*
*  Function description:
*    Find device instance for a given handle.
*
*  Return value:
*    Pointer on instance on success.
*    NULL, if handle invalid.
*
*/
static DEV_INST *_Handle2Inst(int Handle) {
  int i;

  if (Handle <= 0) {
    return NULL;
  }
  for (i = 0; i < USBBULK_MAX_DEVICES; i++) {
    if (_Devices[i] != NULL && _Devices[i]->Handle == Handle) {
      return _Devices[i];
    }
  }
  return NULL;
}

/*********************************************************************
*
*       _WriteFunc
*/
static void *_WriteFunc(void *pArg) {
  WRITE_JOB *pJob;
  int       Dummy;

  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &Dummy);
  pJob = (WRITE_JOB *)pArg;
  USBBULK_Write(pJob->hDevice, pJob->pData, pJob->NumBytes);
  return NULL;
}

/*********************************************************************
*
*       _Init
*/
static void _Init(void) {
  if (_IsInited == 0) {
    memset(&_Global, 0, sizeof(_Global));
    memset(&_DeviceItemList, 0, sizeof(_DeviceItemList));
    memset(&_Devices, 0, sizeof(_Devices));
    _IsInited = 1;
  }
}

/*********************************************************************
*
*       _GetDevice
*
*  Function description
*    Returns an IOKit interface which can be used to communicate with the USB device.
*/
static int _GetDevice(io_service_t Service, IOUSBDeviceInterface *** pppDevice) {
  IOCFPlugInInterface ** ppPlugIn;
  SInt32                 Score;
  kern_return_t          rKern;
  int                    r;

  //
  // Create a plug-in interface to communicate with the kernel.
  //
  Score    = 0;
  ppPlugIn = NULL;
  rKern = IOCreatePlugInInterfaceForService(Service,
                                            kIOUSBDeviceUserClientTypeID,
                                            kIOCFPlugInInterfaceID,
                                            &ppPlugIn,
                                            &Score);
  if (rKern != kIOReturnSuccess) {
    LOG(("Could not get plug-in interface.\n"));
    return 1;     // Error, could not get plug-in interface.
  }
  if (ppPlugIn == NULL) {
    LOG(("Invalid plug-in interface.\n"));
    return 1;     // Error, the plug in interface is not valid.
  }
  //
  // Now, use the created plug-in interface to get an interface to USB device.
  //
  r = (*ppPlugIn)->QueryInterface(ppPlugIn,
                                  CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID),
                                  (LPVOID *)pppDevice);
  (*ppPlugIn)->Release(ppPlugIn);   // Plug-in object not required anymore.
  if (r) {
    LOG(("Could not get USB device interface.\n"));
    return 1;     // Error, could not get the interface to USB device.
  }
  if (*pppDevice == NULL) {
    LOG(("Invalid USB device interface.\n"));
    return 1;     // Error, the interface is not valid.
  }
  return 0;       // OK, USB device interface created. It must be released by caller.
}

/*********************************************************************
 *
 *       _LocateDevice
 *
 *  Function description
 *    Searches for a device by location id.
 */
static int _LocateDevice(U32 LocationId, IOUSBDeviceInterface *** pppDevice) {
  CFMutableDictionaryRef    Dict;
  CFMutableDictionaryRef    DictProp;
  CFNumberRef               Number;
  io_service_t              Service;
  IOUSBDeviceInterface   ** ppDevice;
  int                       r;

  r           = 1;        // No device found.
  Dict        = NULL;
  DictProp    = NULL;
  Number      = NULL;
  Service     = 0;
  ppDevice    = NULL;
  //
  // Create a matching dictionary to search for a device by its location id.
  //
  Dict = IOServiceMatching(kIOUSBDeviceClassName);
  if (Dict == NULL) {
    LOG(("Could not create matching dictionary.\n"));
    goto Done;     // Error, could not create matching dictionary.
  }
  //
  // Create an other dictionary to match the location id.
  //
  DictProp = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
  if (DictProp == NULL) {
    LOG(("Could not create dictionary property.\n"));
    goto Done;      // Error, could not create dictionary property.
  }
  //
  // Create a number reference to be used by the dictionary property as value for the search key.
  //
  Number = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &LocationId);
  if (Number == NULL) {
    LOG(("Could not create location id number.\n"));
    goto Done;      // Error, could not create location id number.
  }
  CFDictionarySetValue(DictProp, CFSTR(kUSBDevicePropertyLocationID), Number);
  //
  // The key to be used in the search is the dictionary created above.
  //
  CFDictionarySetValue(Dict, CFSTR(kIOPropertyMatchKey), DictProp);
  //
  // Now, search for the device with the given location id.
  //
  Service = IOServiceGetMatchingService(kIOMasterPortDefault, Dict);
  if (Service == 0) {
    LOG(("Could not find device LocationId: 0x%08x.\n", LocationId));
    goto Done;
  }
  //
  // OK, we found the device. Get a IOKit interface to this device.
  //
  r = _GetDevice(Service, &ppDevice);
  if (r == 0) {
    *pppDevice = ppDevice;        // OK, interface to device successfully created.
  }
Done:
  //
  // Release the allocated resources.
  //
  if (DictProp) {
    CFRelease(DictProp);
  }
  if (Number) {
    CFRelease(Number);
  }
  if (Service) {
    IOObjectRelease(Service);
  }
  return r;
}

/*********************************************************************
*
*       _GetInterface
*
*  Function description
*    Returns an IOKit interface to the USB interface.
*/
static int _GetInterface(io_service_t Service, IOUSBInterfaceInterface *** pppInterface) {
  int                    r;
  SInt32                 Score;
  kern_return_t          rKern;
  IOCFPlugInInterface ** ppPlugIn;

  //
  // Create a plug-in interface to communicate with the kernel.
  //
  ppPlugIn = NULL;
  rKern = IOCreatePlugInInterfaceForService(Service,
                                            kIOUSBInterfaceUserClientTypeID,
                                            kIOCFPlugInInterfaceID,
                                            &ppPlugIn,
                                            &Score);
  if (rKern != kIOReturnSuccess) {
    LOG(("Could not get plug-in interface.\n"));
    return 1;       // Error, could not get plug-in interface.
  }
  if (ppPlugIn == NULL) {
    LOG(("Invalid plug-in interface.\n"));
    return 1;       // Error, the plug in interface is not valid.
  }
  //
  // Now, use the created plug-in interface to get an interface to USB interface.
  //
  r = (*ppPlugIn)->QueryInterface(ppPlugIn,
                                  CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID),
                                  (LPVOID *)pppInterface);
  (*ppPlugIn)->Release(ppPlugIn);   // Plug-in object not required anymore.
  if (r) {
    LOG(("Could not get interface to USB interface.\n"));
    return 1;       // Error, could not get the interface to USB device.
  }
  if (*pppInterface == NULL) {
    LOG(("Invalid interface to USB interface.\n"));
    return 1;       // Error, the interface is not valid.
  }
  return 0;
}

/*********************************************************************
*
*       _FindInterface
*
*  Function description
*    Finds the right USB interface required to communicate with the device.
*/
static int _FindInterface(IOUSBDeviceInterface ** ppDevice, IOUSBInterfaceInterface *** pppInterface) {
  int            r;
  io_iterator_t  Iterator;
  io_service_t   Service;
  UInt8          Class;
  UInt8          SubClass;
  UInt8          NumEndpoints;
  IOUSBFindInterfaceRequest    Request;
  IOUSBInterfaceInterface   ** ppInterface;

  //
  // Configure the search to find all interfaces.
  //
  Request.bInterfaceClass    = kIOUSBFindInterfaceDontCare;
  Request.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
  Request.bAlternateSetting  = kIOUSBFindInterfaceDontCare;
  Request.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
  //
  // Get an iterator for the interfaces on the device first.
  // This is needed to interate through all interfaces on the device to find the one we need to talk to
  //
  r = (*ppDevice)->CreateInterfaceIterator(ppDevice, &Request, &Iterator);
  if (r) {
    LOG(("Could not create interface iterator (%s).\n", _ErrorToString(r)));
    return 1;     // Error, could not create interface iterator.
  }
  //
  // Iterate through all USB interfaces to find to find the first one
  // with the class and subclass set to 0xFF. This is the interface we
  // have to use when communicating with the device.
  //
  r           = 1;      // No interface found so far.
  ppInterface = 0;
  Service     = 0;
  while (1) {
    //
    // Release the reference added by iterator.
    //
    if (Service) {
      IOObjectRelease(Service);
    }
    if (ppInterface) {
      (*ppInterface)->Release(ppInterface);
    }
    //
    // Get next USB device.
    //
    Service = IOIteratorNext(Iterator);
    if (Service == 0) {
      break;            // No more interfaces found.
    }
    r = _GetInterface(Service, &ppInterface);
    if (r) {
      break;            // Error could not get an interface to USB interface.
    }
    //
    // OK, we found an interface. Check if it is the correct one.
    // The interface we need has the class and subclass set to 0xFF (vendor specific)
    // and should also contain 2 endpoints.
    //
    Class        = 0;
    SubClass     = 0;
    NumEndpoints = 0;
    r = (*ppInterface)->GetInterfaceClass(ppInterface, &Class);
    if (r) {
      LOG(("Could not get interface class (%s).\n", _ErrorToString(r)));
    }
    r = (*ppInterface)->GetInterfaceSubClass(ppInterface, &SubClass);
    if (r) {
      LOG(("Could not get interface subclass (%s).\n", _ErrorToString(r)));
    }
    r = (*ppInterface)->GetNumEndpoints(ppInterface, &NumEndpoints);
    if (r) {
      LOG(("Could not get the number of endpoints (%s).\n", _ErrorToString(r)));
    }
    if ((Class == 0xFF) && (SubClass == 0xFF)) {
      *pppInterface = ppInterface;
      r = 0;
      break;            // OK, we have found the interface.
    }
  }
  IOObjectRelease(Iterator);
  return r;
}

/*********************************************************************
*
*       _GetInPipe
*
*  Function description
*    Returns the index of the IN pipe which can be used to receive data from the device.
*/
static UInt8 _GetInPipe(IOUSBInterfaceInterface190 ** ppInterface) {
  int    r;
  UInt8  Pipe;
  UInt8  NumEndpoints;
  UInt8  Direction;
  UInt8  Number;
  UInt8  TransferType;
  UInt16 MaxPacketSize;
  UInt8  Interval;

  r = (*ppInterface)->GetNumEndpoints(ppInterface, &NumEndpoints);
  if (r) {
    LOG(("Could not get the number of endpoints (%s).\n", _ErrorToString(r)));
    return 0;       // Error, failed to read the number of endpoints.
  }
  if (NumEndpoints == 0) {
    LOG(("Invalid number of endpoints.\n"));
    return 0;       // Error, at least 1 endpoint is required.
  }
  Pipe = 1;         // The first pipe has the number 1. Index 0 is reserved for the control pipe.
  do {
    r = (*ppInterface)->GetPipeProperties(ppInterface, Pipe, &Direction, &Number, &TransferType, &MaxPacketSize, &Interval);
    if (r) {
      LOG(("Could not get properties for pipe %d (%s).\n", Pipe, _ErrorToString(r)));
      continue;
    }
    if ((Direction == kUSBIn) && (TransferType == kUSBBulk)) {
      return Pipe;  // OK, found a bulk in pipe.
    }
    ++Pipe;
  } while (--NumEndpoints);
  LOG(("Could not find IN pipe.\n"));
  return 0;         // Error, no in pipe found.
}

/*********************************************************************
*
*       _GetOutPipe
*
*  Function description
*    Returns the index of the OUT pipe which can be used to send data to the device.
*/
static UInt8 _GetOutPipe(IOUSBInterfaceInterface190 ** ppInterface) {
  int    r;
  UInt8  Pipe;
  UInt8  NumEndpoints;
  UInt8  Direction;
  UInt8  Number;
  UInt8  TransferType;
  UInt16 MaxPacketSize;
  UInt8  Interval;

  r = (*ppInterface)->GetNumEndpoints(ppInterface, &NumEndpoints);
  if (r) {
    LOG(("Could not get the number of endpoints (%s).\n", _ErrorToString(r)));
    return 0;       // Error, failed to read the number of endpoints.
  }
  if (NumEndpoints == 0) {
    LOG(("Invalid number of endpoints.\n"));
    return 0;       // Error, at least 1 endpoint is required.
  }
  Pipe = 1;         // The first pipe has the number 1. Index 0 is reserved for the control pipe.
  do {
    r = (*ppInterface)->GetPipeProperties(ppInterface, Pipe, &Direction, &Number, &TransferType, &MaxPacketSize, &Interval);
    if (r) {
      LOG(("Could not get properties for pipe %d (%s).\n", Pipe, _ErrorToString(r)));
      continue;
    }
    if ((Direction == kUSBOut) && (TransferType == kUSBBulk)) {
      return Pipe;  // OK, found a bulk in pipe.
    }
    ++Pipe;
  } while (--NumEndpoints);
  LOG(("Could not find OUT pipe.\n"));
  return 0;         // Error, no in pipe found.
}

/*********************************************************************
*
*       _OnNotifyCb
*/
static void _OnNotifyCb(void *Dummy, io_iterator_t iterator) {
  while (IOIteratorNext(iterator)) {
  }
  LOG(("got USB notify\n"));
  _ScanForDevices();
}

/*********************************************************************
*
*       _MonitorFunc
*/
static void *_MonitorFunc(void *Dummy) {
  io_iterator_t          portIterator;
  mach_port_t            masterPort;
  CFMutableDictionaryRef matchingDict;
  IONotificationPortRef  notificationPort;
  kern_return_t          result;

  result = IOMasterPort(MACH_PORT_NULL, &masterPort);
  if (result  || !masterPort) {
    LOG(("Couldn't create a master I/O Kit port(%08x)\n", result));
    return NULL;
  }
  matchingDict = IOServiceMatching(kIOUSBDeviceClassName);    // Interested in instances of class
  if (!matchingDict) {
    LOG(("Couldn't create a USB matching dictionary\n"));
    goto Error;
  }
  // Set up notification port and add it to the current run loop for addition notifications.
  notificationPort = IONotificationPortCreate(masterPort);
  CFRunLoopAddSource(CFRunLoopGetCurrent(),
                     IONotificationPortGetRunLoopSource(notificationPort),
                     kCFRunLoopDefaultMode);
  // Retain dictionary first because all IOServiceMatching calls consume dictionary.
  CFRetain(matchingDict);
  // Register for notifications when a device is added to the system.
  result = IOServiceAddMatchingNotification(notificationPort,
                                            kIOMatchedNotification,
                                            matchingDict,
                                            _OnNotifyCb,
                                            NULL,
                                            &portIterator);
  if (result) {
    LOG(("Couldn't add notification(%08x)\n", result));
    goto Error;
  }
  // Run out the iterator or notifications won't start.
  while (IOIteratorNext(portIterator)) {
  }
  // Also register for notifications when a device is removed from the system.
  result = IOServiceAddMatchingNotification(notificationPort,
                                            kIOTerminatedNotification,
                                            matchingDict,
                                            _OnNotifyCb,
                                            NULL,
                                            &portIterator);
  if (result) {
    LOG(("Couldn't add notification(%08x)\n", result));
    goto Error;
  }
  // Run out the iterator or notifications won't start.
  while (IOIteratorNext(portIterator)) {
  }
  mach_port_deallocate(mach_task_self(), masterPort);
  //Start the run loop so notifications will be received
  LOG(("Enter monitor thread run loop\n"));
  _Global.RunLoop = CFRunLoopGetCurrent();
  _Global.RunLoopActive = 1;
  CFRunLoopRun();
  return NULL;
Error:
  mach_port_deallocate(mach_task_self(), masterPort);
  return NULL;
}

/*********************************************************************
*
*       _GetDevInfo
*
*  Function description
*    Retrieves information about a USBBULK device
*
*  Parameters
*    pDev        - Pointer to device table.
*    pDevInfo    - Pointer to a device info structure.
*
*/
static void _GetDevInfo(const DEV_INST *pDev, USBBULK_DEV_INFO * pDevInfo) {
  memset(pDevInfo, 0, sizeof(USBBULK_DEV_INFO));
  pDevInfo->VendorId  = pDev->VendorId;
  pDevInfo->ProductId = pDev->ProductId;
  strncpy(pDevInfo->acSN, pDev->acSN, sizeof(pDevInfo->acSN) - 1);
  strncpy(pDevInfo->acDevName, pDev->acName, sizeof(pDevInfo->acDevName) - 1);
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       USBBULK_Open
*
* Function description
*/
USB_BULK_HANDLE USBBULK_Open(unsigned Id) {
  DEV_INST   * pDev;
  int          r;
  IOUSBDeviceInterface    ** ppDevice;
  IOUSBInterfaceInterface ** ppInterface;

  LOG(("USBBULK_Open(%d)\n", Id));
  if (Id >= USBBULK_MAX_DEVICES || _Devices[Id] == NULL) {
    LOG(("bad index: %d\n", Id));
    return 0;
  }
  pDev = _Devices[Id];
  if (pDev->Handle > 0) {
    return pDev->Handle;          // already open
  }
  ppDevice    = NULL;
  ppInterface = NULL;
  //
  // Search for the device using the location id we determined earlier.
  //
  r = _LocateDevice(pDev->LocationId, &ppDevice);
  if (r) {
    LOG(("Could not find device from LocationId\n"));
    goto OnError;
  }
  //
  // Search for the bulk interface which should be used to communicate with the device.
  //
  r = _FindInterface(ppDevice, &ppInterface);
  if (r) {
    LOG(("Could not find interface\n"));
    goto OnError;
  }
  //
  // OK, interface found.
  //
  r = (*ppInterface)->USBInterfaceOpen(ppInterface);
  if (r) {
    LOG(("Could not open interface (%s).\n", _ErrorToString(r)));
    goto OnError;
  }
  //
  // Get IN/OUT endpoints / pipes
  //
  pDev->InPipe  = _GetInPipe((IOUSBInterfaceInterface190 **)ppInterface);
  pDev->OutPipe = _GetOutPipe((IOUSBInterfaceInterface190 **)ppInterface);
//  if (pDev->InPipe == 0 || pDev->OutPipe == 0) {
//    LOG(("No EP's found\n"));
//    (*ppInterface)->USBInterfaceClose(ppInterface);
//    goto OnError;
//  }
  pDev->ppDevice    = (IOUSBDeviceInterface182 **)ppDevice;
  pDev->ppInterface = (IOUSBInterfaceInterface190 **)ppInterface;
  pDev->Handle  = ++_Global.NextHandle;
  return pDev->Handle;
OnError:
  //
  // Release the allocated resources.
  //
  if (ppDevice) {
    (*ppDevice)->Release(ppDevice);
  }
  if (ppInterface) {
    (*ppInterface)->Release(ppInterface);
  }
  return 0;
}

/*********************************************************************
*
*       USBBULK_Close
*
* Function description
*/
void USBBULK_Close(USB_BULK_HANDLE hDevice) {
  DEV_INST   * pDev;
  IOUSBDeviceInterface182    ** ppDevice;
  IOUSBInterfaceInterface190 ** ppInterface;

  if ((pDev = _Handle2Inst(hDevice)) == NULL) {
    return;
  }
  if (pDev->Handle > 0) {
    ppInterface = pDev->ppInterface;
    (*ppInterface)->USBInterfaceClose(ppInterface);
    (*ppInterface)->Release(ppInterface);
    ppDevice    = pDev->ppDevice;
    (*ppDevice)->Release(ppDevice);
  }
  pDev->Handle = 0;
}

/*********************************************************************
*
*       USBBULK_Read
*
* Function description
*/
int USBBULK_Read(USB_BULK_HANDLE hDevice, void * pBuffer, int NumBytes) {
  DEV_INST   * pDev;

  if ((pDev = _Handle2Inst(hDevice)) == NULL) {
    return -1;
  }
  return USBBULK_ReadTimed(hDevice, pBuffer, NumBytes, pDev->ReadTimeout);
}

/*********************************************************************
*
*       USBBULK_ReadTimed
*
* Function description
*/
int USBBULK_ReadTimed(USB_BULK_HANDLE hDevice, void * pBuffer, int NumBytesToRead, unsigned ms) {
  U8         * pBuff;
  DEV_INST   * pDev;
  U32          BytesAtOnce;
  int          Result;
  UInt32       NumBytes;
  int          r;
  IOUSBInterfaceInterface190 ** ppInterface;

  if ((pDev = _Handle2Inst(hDevice)) == NULL) {
    return -1;
  }
  Result = 0;
  pBuff = (U8 *)pBuffer;
  while (NumBytesToRead > 0) {
    if (pDev->NumBytesInBuffer == 0) {
      //
      // Buffer empty, issue USB read
      //
      ppInterface  = pDev->ppInterface;
      NumBytes     = READ_BUFF_SIZE;
      r = (*ppInterface)->ReadPipeTO(ppInterface, pDev->InPipe, pDev->acBuffer, &NumBytes, (int)ms, (int)ms);
      if (r) {
        LOG(("Could not read from device (%s).\n", _ErrorToString(r)));
        return 0;           // Error, read operation failed.
      }
      pDev->pBuffData        = pDev->acBuffer;
      pDev->NumBytesInBuffer = NumBytes;
    }
    BytesAtOnce = pDev->NumBytesInBuffer;
    if (BytesAtOnce > (U32)NumBytesToRead) {
      BytesAtOnce = NumBytesToRead;
    }
    memcpy(pBuff, pDev->pBuffData, BytesAtOnce);
    pBuff                  += BytesAtOnce;
    NumBytesToRead         -= BytesAtOnce;
    pDev->pBuffData        += BytesAtOnce;
    pDev->NumBytesInBuffer -= BytesAtOnce;
    Result                 += BytesAtOnce;
    if (pDev->ShortMode & USBBULK_MODE_BIT_ALLOW_SHORT_READ) {
      break;
    }
  }
  return Result;
}

/*********************************************************************
*
*       USBBULK_Write
*/
int USBBULK_Write(USB_BULK_HANDLE hDevice, const void * pBuffer, int NumBytes) {
  DEV_INST   * pDev;

  if ((pDev = _Handle2Inst(hDevice)) == NULL) {
    return -1;
  }
  return USBBULK_WriteTimed(hDevice, pBuffer, NumBytes, pDev->WriteTimeout);
}

/*********************************************************************
*
*       USBBULK_WriteTimed
*/
int USBBULK_WriteTimed(USB_BULK_HANDLE hDevice, const void * pBuffer, int NumBytes, unsigned ms) {
  U8         * pBuff;
  DEV_INST   * pDev;
  U32          BytesAtOnce;
  int          Result;
  int          r;
  IOUSBInterfaceInterface190 ** ppInterface;

  if ((pDev = _Handle2Inst(hDevice)) == NULL) {
    return -1;
  }
  Result = 0;
  pBuff = (U8 *)pBuffer;
  while (NumBytes > 0) {
    BytesAtOnce = NumBytes;
    if (BytesAtOnce > MAX_WRITE_SIZE) {
      BytesAtOnce = MAX_WRITE_SIZE;
    }
    //
    // Issue write.
    //
    ppInterface = pDev->ppInterface;
    r = (*ppInterface)->WritePipeTO(ppInterface, pDev->OutPipe, (void *)pBuffer, NumBytes, (int)ms, (int)ms);
    if (r) {
      LOG(("Could not write data (%s).\n", _ErrorToString(r)));
      return 0;
    }
    Result   += BytesAtOnce;
    pBuff    += BytesAtOnce;
    NumBytes -= BytesAtOnce;
  }
  return Result;
}

/*********************************************************************
*
*       USBBULK_WriteRead
*
* Function description
*   Write to the device & read.
*   Reason to have this function is that it reduces latencies.
* Notes
*   (1)  This function can not be used when short read mode is enabled.
*/
int USBBULK_WriteRead(USB_BULK_HANDLE hDevice, const void * pWrBuffer, int WrNumBytes, void *  pRdBuffer, int RdNumBytes) {
  int r;
  pthread_t ThreadId;
  WRITE_JOB WriteJob;
  void      *Dummy;

  WriteJob.hDevice  = hDevice;
  WriteJob.NumBytes = WrNumBytes;
  WriteJob.pData    = pWrBuffer;
  pthread_create(&ThreadId, NULL, _WriteFunc, &WriteJob);
  r = USBBULK_Read(hDevice, pRdBuffer, RdNumBytes);
  pthread_cancel(ThreadId);
  pthread_join(ThreadId, &Dummy);
  return r;
}

/*********************************************************************
*
*       USBBULK_CancelRead
*
* Function description
*   This cancels an initiated read.
*/
void USBBULK_CancelRead(USB_BULK_HANDLE hDevice) {
}

/*********************************************************************
*
*       USBBULK_GetConfigDescriptor
*/
int USBBULK_GetConfigDescriptor(USB_BULK_HANDLE hDevice, void* pBuffer, int Size) {
  return 0;
}

/*********************************************************************
*
*       USBBULK_ResetINPipe
*/
int USBBULK_ResetINPipe(USB_BULK_HANDLE hDevice) {
  int          r;
  DEV_INST   * pDev;
  IOUSBInterfaceInterface190 ** ppInterface;

  if ((pDev = _Handle2Inst(hDevice)) == NULL) {
    return -1;
  }
  ppInterface = pDev->ppInterface;
  r = (*ppInterface)->ClearPipeStallBothEnds(ppInterface, pDev->InPipe);
  if (r) {
    LOG(("Could not reset IN pipe (%s).\n", _ErrorToString(r)));
    return 0;
  }
  return 1;
}

/*********************************************************************
*
*       USBBULK_ResetOUTPipe
*/
int USBBULK_ResetOUTPipe(USB_BULK_HANDLE hDevice) {
  int          r;
  DEV_INST   * pDev;
  IOUSBInterfaceInterface190 ** ppInterface;

  if ((pDev = _Handle2Inst(hDevice)) == NULL) {
    return -1;
  }
  ppInterface = pDev->ppInterface;
  r = (*ppInterface)->ClearPipeStallBothEnds(ppInterface, pDev->OutPipe);
  if (r) {
    LOG(("Could not reset OUT pipe (%s).\n", _ErrorToString(r)));
    return 0;
  }
  return 1;
}

/*********************************************************************
*
*       USBBULK_ResetDevice
*/
int USBBULK_ResetDevice(USB_BULK_HANDLE hDevice) {
  int          r;
  DEV_INST   * pDev;
  IOUSBDeviceInterface182 ** ppDevice;

  if ((pDev = _Handle2Inst(hDevice)) == NULL) {
    return -1;
  }
  ppDevice = pDev->ppDevice;
  r = (*ppDevice)->ResetDevice(ppDevice);
  if (r) {
    LOG(("Could not reset device. Operation failed.\n"));
    return 0;
  }
  return  1;
}

/*********************************************************************
*
*       USBBULK_GetVersion
*/
unsigned USBBULK_GetVersion(void) {
  return USBBULK_VERSION;
}

/*********************************************************************
*
*       USBBULK_GetDriverVersion
*/
unsigned USBBULK_GetDriverVersion(void) {
  return 0;
}

/*********************************************************************
*
*       USBBULK_GetDriverCompileDate
*/
unsigned USBBULK_GetDriverCompileDate(char * s, unsigned Size) {
  *s = 0;
  return 0;
}

/*********************************************************************
*
*       USBBULK_SetMode
*/
unsigned USBBULK_SetMode(USB_BULK_HANDLE hDevice, unsigned Mode) {
  unsigned Result;
  DEV_INST   * pDev;

  if ((pDev = _Handle2Inst(hDevice)) == NULL) {
    return 0;
  }
  Result = pDev->ShortMode;
  pDev->ShortMode = Mode & (USBBULK_MODE_BIT_ALLOW_SHORT_READ | USBBULK_MODE_BIT_ALLOW_SHORT_WRITE);
  return Result;
}

/*********************************************************************
*
*       USBBULK_GetMode
*/
unsigned USBBULK_GetMode(USB_BULK_HANDLE hDevice) {
  DEV_INST   * pDev;

  if ((pDev = _Handle2Inst(hDevice)) == NULL) {
    return 0;
  }
  return pDev->ShortMode;
}

/*********************************************************************
*
*       USBBULK_GetReadMaxTransferSize
*/
unsigned USBBULK_GetReadMaxTransferSize(USB_BULK_HANDLE hDevice) {
  return READ_BUFF_SIZE;
}

/*********************************************************************
*
*       USBBULK_GetWriteMaxTransferSize
*/
unsigned USBBULK_GetWriteMaxTransferSize(USB_BULK_HANDLE hDevice) {
  return MAX_WRITE_SIZE;
}

/*********************************************************************
*
*       USBBULK_GetSN
*/
int USBBULK_GetSN(USB_BULK_HANDLE hDevice, U8 * pBuffer, unsigned NumBytes) {
  DEV_INST   * pDev;

  if ((pDev = _Handle2Inst(hDevice)) == NULL) {
    return 0;
  }
  strncpy((char *)pBuffer, pDev->acSN, NumBytes);
  pBuffer[NumBytes - 1] = 0;
  return 1;
}

/*********************************************************************
*
*       USBBULK_GetNumAvailableDevices
*/
unsigned USBBULK_GetNumAvailableDevices(U32 * pMask) {
  int i;
  unsigned Cnt;
  U32      Mask;

  _ScanForDevices();
  Cnt  = 0;
  Mask = 0;
  for (i = 0; i < USBBULK_MAX_DEVICES; i++) {
    if (_Devices[i] != NULL) {
      Mask |= (1 << i);
      Cnt++;
    }
  }
  if (pMask) {
    *pMask = Mask;
  }
  return Cnt;
}

/*********************************************************************
*
*       USBBULK_GetEnumTickCount
*
*  Function description
*
*
*  Parameters
*    hDevice     -
*
*/
U32 USBBULK_GetEnumTickCount(USB_BULK_HANDLE hDevice) {
  return 0;
}

/*********************************************************************
*
*       USBBULK_GetReadMaxTransferSizeDown
*/
U32 USBBULK_GetReadMaxTransferSizeDown(USB_BULK_HANDLE hDevice) {
  return 0;
}

/*********************************************************************
*
*       USBBULK_GetWriteMaxTransferSizeDown
*/
U32 USBBULK_GetWriteMaxTransferSizeDown(USB_BULK_HANDLE hDevice) {
  return 0;
}

/*********************************************************************
*
*       USBBULK_SetWriteMaxTransferSizeDown
*/
unsigned USBBULK_SetWriteMaxTransferSizeDown(USB_BULK_HANDLE hDevice, U32 TransferSize) {
  return 0;
}

/*********************************************************************
*
*       USBBULK_SetReadMaxTransferSizeDown
*/
unsigned USBBULK_SetReadMaxTransferSizeDown(USB_BULK_HANDLE hDevice, U32 TransferSize) {
  return 0;
}

/*********************************************************************
*
*       USBBULK_Init
*
*  Function description:
*    This function needs to be called first. This makes sure to have
*    all structures and thread have been initialized.
*    It also sets a callback in order to be notified when a device is added or removed.
*
*  Parameters:
*    pfNotification  - Pointer to the callback
*    pContext        - Context data that shall be called with the callback function
*
*/
void USBBULK_Init(USBBULK_NOTIFICATION_FUNC * pfNotification, void * pContext) {
  _Init();
  _Global.pfOnUserNotification      = pfNotification;
  _Global.pfUserNotificationContext = pContext;
  if (pfNotification) {
    _Global.ThreadActive = 1;
    _Global.RunLoopActive = 0;
    pthread_mutex_init(&_Global.Mutex, NULL);
    pthread_create(&_Global.thHandle, NULL, _MonitorFunc, NULL);
  }
}

/*********************************************************************
*
*       USBBULK_Exit
*
*  Function description:
*    Shall be called at the end of the function in order to clean up
*
*/
void USBBULK_Exit(void) {
  int i;
  DEV_INST * pDev;

  _Global.pfOnUserNotification = NULL;
  if (_Global.ThreadActive) {
    if (_Global.RunLoopActive) {
      CFRunLoopStop(_Global.RunLoop);
    }
    pthread_mutex_lock(&_Global.Mutex);
  }
  for (i = 0; i < USBBULK_MAX_DEVICES; i++) {
    if (_Devices[i] != NULL) {
      pDev = _Devices[i];
      if (pDev->Handle > 0) {
        IOUSBDeviceInterface182    ** ppDevice;
        IOUSBInterfaceInterface190 ** ppInterface;

        ppInterface = pDev->ppInterface;
        (*ppInterface)->USBInterfaceClose(ppInterface);
        (*ppInterface)->Release(ppInterface);
        ppDevice    = pDev->ppDevice;
        (*ppDevice)->Release(ppDevice);
      }
      free(pDev);
    }
  }
  if (_Global.ThreadActive) {
    pthread_mutex_unlock(&_Global.Mutex);
  }
  _IsInited = 0;
}

/*********************************************************************
*
*       USBBULK_SetReadTimeout
*
*  Function description:
*    Setups the read timeout for an opened device.
*
*  Parameters:
*    hDevice     - Handle to the opened device
*    Timeout     - Timeout to set.
*
*/
void USBBULK_SetReadTimeout(USB_BULK_HANDLE hDevice, int Timeout) {
  DEV_INST   * pDev;

  if ((pDev = _Handle2Inst(hDevice)) == NULL) {
    return;
  }
  pDev->ReadTimeout = Timeout;
}

/*********************************************************************
*
*       USBBULK_SetWriteTimeout
*
*  Function description:
*    Setups the write timeout for an opened device.
*
*  Parameters:
*    hDevice     - Handle to the opened device
*    Timeout     - Timeout to set.
*
*/
void USBBULK_SetWriteTimeout(USB_BULK_HANDLE hDevice, int Timeout) {
  DEV_INST   * pDev;

  if ((pDev = _Handle2Inst(hDevice)) == NULL) {
    return;
  }
  pDev->WriteTimeout = Timeout;
}

/*********************************************************************
*
*       USBBULK_GetDevInfo
*
*  Function description
*    Retrieves information about an opened USBBULK device.
*
*  Parameters
*    hDevice:      Handle to the opened device.
*    pDevInfo:     Pointer to a device info structure.
*/
void USBBULK_GetDevInfo(USB_BULK_HANDLE hDevice, USBBULK_DEV_INFO * pDevInfo) {
  DEV_INST   * pDev;

  if ((pDev = _Handle2Inst(hDevice)) == NULL) {
    return;
  }
  _GetDevInfo(pDev, pDevInfo);
}

/*********************************************************************
*
*       USBBULK_GetDevInfoByIdx
*
*  Function description
*    Retrieves information about a USB device.
*
*  Parameters
*    Idx:          Index of the device.
*    pDevInfo:     Pointer to a device info structure.
*
*  Return value
*    == 0;      Error, bad device index.
*    != 0:      Success
*
*/
int USBBULK_GetDevInfoByIdx(unsigned Idx, USBBULK_DEV_INFO * pDevInfo) {
  DEV_INST   * pDev;

  if (Idx >= USBBULK_MAX_DEVICES || _Devices[Idx] == NULL) {
    LOG(("bad index: %d\n", Idx));
    return 0;
  }
  pDev = _Devices[Idx];
  _GetDevInfo(pDev, pDevInfo);
  return 1;
}

/*********************************************************************
*
*       USBBULK_GetVendorName
*
*  Function description:
*    Retrieves the vendor name if available.
*
*  Parameters:
*    hDevice     - Handle to the opened device
*    sVendorName - Pointer to a buffer that should contain the string.
*    BufferSize  - Size of the buffer, given in bytes.
*
*  Return value:
*    0     - Error, vendor name not available or buffer to small.
*    1     - Success, vendor name stored in buffer pointed by pVendorName.
*
*/
int USBBULK_GetVendorName(USB_BULK_HANDLE hDevice, char * sVendorName, unsigned BufferSize) {
  DEV_INST   * pDev;

  if ((pDev = _Handle2Inst(hDevice)) == NULL) {
    return 0;
  }
  strncpy(sVendorName, pDev->acVendor, BufferSize);
  sVendorName[BufferSize - 1] = 0;
  return 1;
}

/*********************************************************************
*
*       USBBULK_GetProductName
*
*  Function description:
*    Retrieves the device/product name if available.
*
*  Parameters:
*    hDevice      - Handle to the opened device
*    sProductName - Pointer to a buffer that should contain the string.
*    BufferSize   - Size of the buffer, given in bytes.
*
*  Return value:
*    0     - Error, vendor name not available or buffer to small.
*    1     - Success, vendor name stored in buffer pointed by sProductName.
*
*/
int USBBULK_GetProductName(USB_BULK_HANDLE hDevice, char * sProductName, unsigned BufferSize) {
  DEV_INST   * pDev;

  if ((pDev = _Handle2Inst(hDevice)) == NULL) {
    return 0;
  }
  strncpy(sProductName, pDev->acName, BufferSize);
  sProductName[BufferSize - 1] = 0;
  return 1;
}

/*********************************************************************
*
*       USBBULK_FlushRx
*
*  Function description
*    Flush any received data.
*
*  Parameters
*    hDevice:     Handle to the opened device.
*
*  Return value
*    == 0:    Error, bad handle.
*    != 0:    Success, flushing the RX buffer was successful.
*
*/
int USBBULK_FlushRx(USB_BULK_HANDLE hDevice) {
  DEV_INST   * pDev;

  if ((pDev = _Handle2Inst(hDevice)) == NULL) {
    return 0;
  }
  pDev->NumBytesInBuffer = 0;
  return 1;
}

/*********************************************************************
*
*       USBBULK_AddAllowedDeviceItem
*
*  Function description:
*
*
*  Parameters:
*    VendorId     -
*    ProductId     -
*
*/
void USBBULK_AddAllowedDeviceItem(U16 VendorId, U16 ProductId) {
  int i;

  for (i = 0; i < MAX_DEVICE_ITEMS; i++) {
    if (_DeviceItemList[i].VendorId == VendorId && _DeviceItemList[i].ProductId == ProductId) {
      return;
    }
  }
  for (i = 0; i < MAX_DEVICE_ITEMS; i++) {
    if (_DeviceItemList[i].VendorId == 0 && _DeviceItemList[i].ProductId == 0) {
      _DeviceItemList[i].VendorId = VendorId;
      _DeviceItemList[i].ProductId = ProductId;
      break;
    }
  }
}

/*********************************************************************
*
*       USBBULK_RemoveAllowedDeviceItem
*
*  Function description:
*
*
*  Parameters:
*    VendorId     -
*    ProductId     -
*
*  Return value:
*    USBBULK_API void WINAPI
*
*/
void USBBULK_RemoveAllowedDeviceItem(U16 VendorId, U16 ProductId) {
  int i;

  for (i = 0; i < MAX_DEVICE_ITEMS; i++) {
    if (_DeviceItemList[i].VendorId == VendorId && _DeviceItemList[i].ProductId == ProductId) {
      _DeviceItemList[i].VendorId  = 0;
      _DeviceItemList[i].ProductId = 0;
    }
  }
}

/*********************************************************************
*
*       USBBULK_SetupRequest
*
*  Function description:
*    Performs a setup request on the device within a given timeout.
*
*  Parameters:
*    hDevice:         Handle to the opened device.
*    pSetupRequest:   Pointer to the setup request.
*    pBuffer:         Pointer to a buffer to store the string
*    pNumBytesBuffer: [in]Size of the buffer in bytes.
*                     [out] Number of bytes that have been transferred.
*    Timeout:         Timeout given in ms.
*
*  Return value:
*    == 0   - Success
*    != 0   - Error, setup request failed.
*
*/
USBBULK_API int WINAPI USBBULK_SetupRequest(USB_BULK_HANDLE hDevice, USBBULK_SETUP_REQUEST * pSetupRequest, void * pBuffer, unsigned * pBufferSize, unsigned Timeout) {
  DEV_INST   * pDev;
  IOUSBDevRequestTO DevRequest;
  int                r = 0;
  IOUSBDeviceInterface182 ** ppDevice;
  IOReturn                      rFunc;

  if ((pDev = _Handle2Inst(hDevice)) == NULL) {
    return -1;
  }
  r = -1;
  ppDevice = pDev->ppDevice;
  DevRequest.bmRequestType = pSetupRequest->bRequestType;
  DevRequest.bRequest      = pSetupRequest->bRequest;
  DevRequest.wValue        = pSetupRequest->wValue;
  DevRequest.wIndex        = pSetupRequest->wIndex;
  DevRequest.wLength       = pSetupRequest->wLength;
  DevRequest.pData         = pBuffer;
  DevRequest.wLenDone      = 0;
  DevRequest.noDataTimeout = 0;
  DevRequest.completionTimeout = Timeout;
  rFunc = (*ppDevice)->DeviceRequestTO(ppDevice, &DevRequest);
  if (rFunc == kIOReturnSuccess) {
    if (pBufferSize) {
      *pBufferSize = DevRequest.wLenDone;
    }
    r = 0;
  }
  return r;
}

/*********************************************************************
*
*       USBBULK_SetAlternateSetting
*
*  Function description:
*     Sets an alternate setting no. for the current interface.
*
*  Parameters:
*    hDevice:         Handle to the opened device.
*    AlternateSetting: Alternate number to be set.
*
*  Return value:
*    == 0   - Success
*    != 0   - Error, failed to set alternate setting.
*/
USBBULK_API int WINAPI USBBULK_SetAlternateSetting(USB_BULK_HANDLE hDevice, U8 AlternateSetting) {
  DEV_INST   * pDev;
  int                r = 0;
  IOUSBInterfaceInterface190 ** ppInterface;
  IOReturn                      rFunc;

  if ((pDev = _Handle2Inst(hDevice)) == NULL) {
    return -1;
  }
  r = -1;
  ppInterface = pDev->ppInterface;
  rFunc = (*ppInterface)->SetAlternateInterface(ppInterface, AlternateSetting);
  if (rFunc == kIOReturnSuccess) {
    //
    // Get IN/OUT endpoints / pipes
    //
    pDev->InPipe  = _GetInPipe((IOUSBInterfaceInterface190 **)ppInterface);
    pDev->OutPipe = _GetOutPipe((IOUSBInterfaceInterface190 **)ppInterface);
    r = 0;
  }
  return r;
}

/*************************** End of file ****************************/
