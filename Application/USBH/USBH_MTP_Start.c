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

File    : USBH_MTP_Start.c
Purpose : This sample is designed to demonstrate the emUSB-Host MTP API.

Additional information:
  Preparations:
    None.

  Expected behavior:
    After an MTP device was connected (usually a phone)
    this sample will, if necessary, wait for the user to unlock
    the connected device before proceeding. After that it will
    print out information about the device and iterate over
    the storages the device has. For every storage the application
    will print out the storage information and information about
    all files and folders in the root directory of the storage.
    If the application finds a file matching "SEGGER_Test.txt" it will
    delete the file. At the very end a file with the name
    from "SEGGER_Test.txt" is re-created with the content
    from _sTestFileContent in the root directory of the storage.

  Sample output:
    <...>
    **** Device added
    Vendor  Id           = 0x1234
    Product Id           = 0x1234
    Serial no.           = 1
    Speed                = FullSpeed
    Manufacturer         : XXXXXX
    Model                : XXXXXXXXXXXXXXXXXXXXXX
    DeviceVersion        : 8.10.12397.0
    MTP SerialNumber     : 844848fb44583cbaa1ecae45545b3

    USBH_MTP_CheckLock returns USBH_STATUS_ERROR
    Please unlock the device to proceed.
    <...>
    _cbOnUserEvent: MTP Event received! EventCode: 0x4004, Para1: 0x00010001, Para2: 0x00000000, Para3: 0x00000000.
    <...>
    USBH_MTP_CheckLock returns USBH_STATUS_SUCCESS
    Found storage with ID: 0
    StorageType          = 0x0003
    FilesystemType       = 0x0002
    AccessCapability     = 0x0000
    MaxCapacity          = 3959422976 bytes
    FreeSpaceInBytes     = 1033814016 bytes
    FreeSpaceInImages    = 0x00000000
    StorageDescription   : Phone
    VolumeLabel          : MTP Volume - 65537

    Found 9 objects in directory 0xFFFFFFFF

    Processing object 0x00000001 in directory 0xFFFFFFFF...
    StorageID            = 0x00010001
    ObjectFormat         = 0x3001
    ParentObject         = 0xFFFFFFFF
    ProtectionStatus     = 0x0001
    Filename             : Documents
    CaptureDate          : 20140522T0
    ModificationDate     : 20160707T1

    Processing object 0x00000002 in directory 0xFFFFFFFF...
    StorageID            = 0x00010001
    ObjectFormat         = 0x3001
    ParentObject         = 0xFFFFFFFF
    ProtectionStatus     = 0x0001
    Filename             : Downloads
    CaptureDate          : 20140522T0
    ModificationDate     : 20160707T1
    <...>
    Creating new object with 135 bytes in folder 0xFFFFFFFF with name SEGGER_Test.txt.
    Created new object in folder 0xFFFFFFFF, ID: 0x000013F9.

    Connection to MTP device closed.
*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/
#include "RTOS.h"
#include "BSP.h"
#include "USBH.h"
#include "USBH_Util.h"
#include "USBH_MTP.h"
#include "SEGGER.h"

/*********************************************************************
*
*       Local defines, configurable
*
**********************************************************************
*/
#define SUPPORTED_NUM_OBJECTS 50
#define EVENT_SUPPORT         0 // Change to 1 to see MTP events.
                                // This will not function with host controllers
                                // which are unable to process multiple read/write transactions
                                // simultaneously (eg. KinetisFS host controller).

/*********************************************************************
*
*       Local data definitions
*
**********************************************************************
*/
enum {
  TASK_PRIO_APP = 150,
  TASK_PRIO_USBH_MAIN,
  TASK_PRIO_USBH_ISR
};

/*********************************************************************
*
*       Const data
*
**********************************************************************
*/
//
// File names have to be in Unicode.
//
const char _sFileName[]        = "S\0E\0G\0G\0E\0R\0_\0T\0e\0s\0t\0.\0t\0x\0t\0\0";
const char _sTestFileContent[] = "This sample is based on the SEGGER emUSB-Host software with the MTP component.\r\nFor further information please visit: www.segger.com\r\n";

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
USBH_MTP_OBJECT            _aObjBuffer[SUPPORTED_NUM_OBJECTS];
USBH_MTP_OBJECT_INFO       _aObjInfoBuffer[SUPPORTED_NUM_OBJECTS];

static char _ac[150]; // For log output.

static OS_STACKPTR int     _StackMain[1536/sizeof(int)];
static OS_TASK             _TCBMain;

static OS_STACKPTR int     _StackIsr[1536/sizeof(int)];
static OS_TASK             _TCBIsr;

static volatile char       _DevIsReady;
static I8                  _DevIndex;

/*********************************************************************
*
*       Static Code
*
**********************************************************************
*/

/*********************************************************************
*
*       _GetPortSpeed
*
*  Function description
*    Returns the given USBH speed as a string.
*/
static const char * _GetPortSpeed(USBH_SPEED Speed) {
  switch (Speed) {
  case USBH_LOW_SPEED:
    return "LowSpeed";
  case USBH_FULL_SPEED:
    return "FullSpeed";
  case USBH_HIGH_SPEED:
    return "HighSpeed";
  default:
    break;
  }
  return "Unknown";
}

/*********************************************************************
*
*       _PrintUnicodeString
*
*  Function description
*    Prints a given Unicode string as ASCII (second byte is discarded).
*/
static void _PrintUnicodeString(char * ac, unsigned BufferSize, const U16 * s){
  while (*s != 0) {
    //
    // Printable ASCII only.
    //
    if ((int)*s < 127) {
      if (BufferSize) {
        *ac++ = ((char)*s++);
        BufferSize--;
      } else {
        *ac = 0; // Current character is overwritten by the terminating zero.
        break;
      }
    } else {
      s++;
    }
  }
  if (BufferSize) {
    *ac = 0; // Next character after the last byte in s is overwritten by the terminating zero.
  }
}

/*********************************************************************
*
*       _PrintU64
*
*  Function description
*    Stores a given unsigned long long number as ASCII.
*/
static void _PrintU64(char * ac, U64 v) {
  static const char _aV2C[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
  unsigned Div;
  U64 Digit = 1;
  //
  // Count how many digits are required
  //
  while ((v / Digit) >= 10) {
    Digit *= 10;
  }
  //
  // Output digits
  //
  do {
    Div = v / Digit;
    v -= Div * Digit;
    *ac++ = _aV2C[Div];
    Digit /= 10;
  } while (Digit);
  *ac++ = 0;
}

/*********************************************************************
*
*       _LoadU64LE
*
*  Function description
*    Loads an unsigned long long value from a byte array.
*/
static U64 _LoadU64LE(const U8 * pData) {
  U64 r;
  r  = * pData++;
  r |= * pData++ << 8;
  r |= (U64) * pData++ << 16;
  r |= (U64) * pData++ << 24;
  r |= (U64) * pData++ << 32;
  r |= (U64) * pData++ << 40;
  r |= (U64) * pData++ << 48;
  r |= (U64) * pData++ << 56;
  return r;
}

/*********************************************************************
*
*       _PrintDeviceInfo
*
*  Function description
*    Prints information about a connected MTP device.
*/
static void _PrintDeviceInfo(USBH_MTP_DEVICE_INFO * pDeviceInfo){
  USBH_Logf_Application("Vendor  Id           = 0x%0.4X", pDeviceInfo->VendorId);
  USBH_Logf_Application("Product Id           = 0x%0.4X", pDeviceInfo->ProductId);
  USBH_Logf_Application("Serial no.           = %s", pDeviceInfo->acSerialNo);
  USBH_Logf_Application("Speed                = %s", _GetPortSpeed(pDeviceInfo->Speed));
  _PrintUnicodeString(&_ac[0], sizeof(_ac), (const U16 *)pDeviceInfo->sManufacturer);
  USBH_Logf_Application("Manufacturer         : %s", &_ac[0]);
  _PrintUnicodeString(&_ac[0], sizeof(_ac), (const U16 *)pDeviceInfo->sModel);
  USBH_Logf_Application("Model                : %s", &_ac[0]);
  _PrintUnicodeString(&_ac[0], sizeof(_ac), (const U16 *)pDeviceInfo->sDeviceVersion);
  USBH_Logf_Application("DeviceVersion        : %s", &_ac[0]);
  _PrintUnicodeString(&_ac[0], sizeof(_ac), (const U16 *)pDeviceInfo->sSerialNumber);
  USBH_Logf_Application("MTP SerialNumber     : %s\n", &_ac[0]);

}

/*********************************************************************
*
*       _PrintStorageInfo
*
*  Function description
*    Prints information about an MTP device's storage.
*/
static void _PrintStorageInfo(USBH_MTP_STORAGE_INFO * pStorageInfo){
  USBH_Logf_Application ("StorageType          = 0x%0.4X", pStorageInfo->StorageType);
  USBH_Logf_Application ("FilesystemType       = 0x%0.4X", pStorageInfo->FilesystemType);
  USBH_Logf_Application ("AccessCapability     = 0x%0.4X", pStorageInfo->AccessCapability);
  _PrintU64(&_ac[0], _LoadU64LE(pStorageInfo->MaxCapacity));
  USBH_Logf_Application ("MaxCapacity          = %s bytes", &_ac[0]);
  _PrintU64(&_ac[0], _LoadU64LE(pStorageInfo->FreeSpaceInBytes));
  USBH_Logf_Application ("FreeSpaceInBytes     = %s bytes", &_ac[0]);
  USBH_Logf_Application ("FreeSpaceInImages    = 0x%0.8X", pStorageInfo->FreeSpaceInImages);
  _PrintUnicodeString(&_ac[0], sizeof(_ac), (const U16 *)pStorageInfo->sStorageDescription);
  USBH_Logf_Application("StorageDescription   : %s", &_ac[0]);
  _PrintUnicodeString(&_ac[0], sizeof(_ac), (const U16 *)pStorageInfo->sVolumeLabel);
  USBH_Logf_Application("VolumeLabel          : %s\n", &_ac[0]);
}

/*********************************************************************
*
*       _PrintObjInfo
*
*  Function description
*    Prints information about a given MTP object.
*/
static void _PrintObjInfo(USBH_MTP_OBJECT_INFO * pObjInfo){
  USBH_Logf_Application ("StorageID            = 0x%0.8X", pObjInfo->StorageID);
  USBH_Logf_Application ("ObjectFormat         = 0x%0.4X", pObjInfo->ObjectFormat);
  USBH_Logf_Application ("ParentObject         = 0x%0.8X", pObjInfo->ParentObject);
  USBH_Logf_Application ("ProtectionStatus     = 0x%0.4X", pObjInfo->ProtectionStatus);
  _PrintUnicodeString(&_ac[0], sizeof(_ac), (const U16 *)pObjInfo->sFilename);
  USBH_Logf_Application("Filename             : %s", &_ac[0]);
  _PrintUnicodeString(&_ac[0], sizeof(_ac), (const U16 *)pObjInfo->sCaptureDate);
  USBH_Logf_Application("CaptureDate          : %s", &_ac[0]);
  _PrintUnicodeString(&_ac[0], sizeof(_ac), (const U16 *)pObjInfo->sModificationDate);
  USBH_Logf_Application("ModificationDate     : %s\n", &_ac[0]);
}


/*********************************************************************
*
*       _cbOnUserEvent
*
*  Function description
*   Callback which is executed when an MTP event is received.
*   e.g. 0x4004 "StoreAdded" when a device is unlocked.
*/
#if EVENT_SUPPORT == 1
static void _cbOnUserEvent(U16 EventCode, U32 Para1, U32 Para2, U32 Para3) {
  USBH_Logf_Application("_cbOnUserEvent: MTP Event received! EventCode: 0x%0.4X, Para1: 0x%0.8X, Para2: 0x%0.8X, Para3: 0x%0.8X.", EventCode, Para1, Para2, Para3);
}
#endif

/*********************************************************************
*
*       _SendData
*
*  Function description
*   Callback called from USBH task, this callback is responsible
*   for providing data when USBH_MTP_CreateObject is called.
*   In this sample application the file data is extracted from
*   the _sTestFileContent array, in a real application data can
*   for example be read from the file system.
*/
static void _SendData(void * pUserContext, U32 NumBytesSentTotal, U32 * pNumBytesToSend, U8 ** pNextBuffer) {
  U8 * pBuf;

  //
  // Get the pointer to _sTestFileContent
  //
  pBuf = (U8 *)pUserContext;
  //
  // Set the pointer to the first byte which has not been sent yet
  //
  *pNextBuffer = pBuf + NumBytesSentTotal;
  //
  // Set the number of bytes for the next transaction to
  // the amount of bytes remaining in the _sTestFileContent array.
  //
  *pNumBytesToSend = sizeof(_sTestFileContent) - NumBytesSentTotal;
}

/*********************************************************************
*
*       _OnDevReady
*
*  Function description
*    Called by the application task (MainTask) when a device is plugged in.
*    Information about an MTP device is displayed and a new file
*    is created on the MTP storage.
*/
static void _OnDevReady(void) {
  USBH_MTP_DEVICE_HANDLE         hDevice;
  static USBH_MTP_DEVICE_INFO    _DeviceInfo;
  USBH_STATUS                    Status;
  U8                             NumStorages;
  unsigned                       NumObjectsFree;
  U32                            NumObjectsDir;
  U32                            NumObjects;
  U32                            DirObjectID;
  static USBH_MTP_STORAGE_INFO   _aStorageInfo[USBH_MAX_NUM_STORAGES];
  static USBH_MTP_CREATE_INFO    _CreateInfo;
  unsigned                       StorageIndex;
  unsigned                       ObjectIndex;

  memset(&_DeviceInfo, 0, sizeof(_DeviceInfo));
  //
  // Open the device, the device index is retrieved from the notification callback.
  //
  hDevice = USBH_MTP_Open(_DevIndex);
  if (hDevice) {
    //
    // Set the event callback.
    //
#if EVENT_SUPPORT == 1
    Status = USBH_MTP_SetEventCallback(hDevice, &_cbOnUserEvent);
    if (Status != USBH_STATUS_SUCCESS) {
      USBH_Warnf_Application("Could not set event callback, Status: %s\n", USBH_GetStatusStr(Status));
    }
#endif
    //
    // Retrieve the information about the MTP device
    // and print them out.
    //
    Status = USBH_MTP_GetDeviceInfo(hDevice, &_DeviceInfo);
    if (Status == USBH_STATUS_SUCCESS) {
      _PrintDeviceInfo(&_DeviceInfo);
      //
      // Check whether the device is locked, if it is wait for it to be unlocked.
      //
      while (1) {
        Status = USBH_MTP_CheckLock(hDevice);
        USBH_Logf_Application("USBH_MTP_CheckLock returns %s", USBH_GetStatusStr(Status));
        if (Status == USBH_STATUS_SUCCESS) {
          break; //unlocked
        }
        if (Status == USBH_STATUS_ERROR) {
          USBH_Logf_Application("Please unlock the device to proceed.");
          USBH_OS_Delay(1000); // locked
        } else {
          goto End; // An error occurred (e.g. device removed)
        }
      }
      //
      // Retrieve the number of storages the device has
      // (e.g. Internal flash and SD card).
      //
      Status = USBH_MTP_GetNumStorages(hDevice, &NumStorages);
      if (Status == USBH_STATUS_SUCCESS) {
        //
        // Iterate over all storages.
        //
        for (StorageIndex = 0; StorageIndex < NumStorages; StorageIndex++) {
          //
          // Retrieve information about the current storage and print it out.
          //
          Status = USBH_MTP_GetStorageInfo(hDevice, StorageIndex, &_aStorageInfo[StorageIndex]);
          if (Status == USBH_STATUS_SUCCESS) {
            USBH_Logf_Application("Found storage with ID: %u", StorageIndex);
            _PrintStorageInfo(&_aStorageInfo[StorageIndex]);
            //
            // Retrieve the number of objects inside the root directory.
            //
            DirObjectID = USBH_MTP_ROOT_OBJECT_ID;
            NumObjectsFree = SEGGER_COUNTOF(_aObjBuffer);
            Status = USBH_MTP_GetNumObjects(hDevice, StorageIndex, DirObjectID, &NumObjectsDir);
            if (Status == USBH_STATUS_SUCCESS) {
              USBH_Logf_Application("Found %d objects in directory 0x%0.8X \n", NumObjectsDir, DirObjectID);
              NumObjects = USBH_MIN(NumObjectsDir, NumObjectsFree);
              if (NumObjects != NumObjectsDir) {
                USBH_Warnf_Application("Limiting the number of objects which will be processed to %d due to _aObjBuffer being too small. \n", NumObjects);
              }
              //
              // Retrieve a list of object IDs from the root directory.
              //
              Status = USBH_MTP_GetObjectList(hDevice, StorageIndex, DirObjectID, _aObjBuffer, &NumObjects);
              if (Status == USBH_STATUS_SUCCESS) {
                //
                // Iterate over all objects.
                //
                for (ObjectIndex = 0; ObjectIndex < NumObjects; ObjectIndex++) {
                  USBH_Logf_Application("Processing object 0x%0.8X in directory 0x%0.8X...", _aObjBuffer[ObjectIndex].ObjectID, DirObjectID);
                  //
                  // Retrieve information about an object and print it out.
                  //
                  Status = USBH_MTP_GetObjectInfo(hDevice, _aObjBuffer[ObjectIndex].ObjectID, &_aObjInfoBuffer[ObjectIndex]);
                  if (Status == USBH_STATUS_SUCCESS) {
                    _PrintObjInfo(&_aObjInfoBuffer[ObjectIndex]);
                    //
                    // Check if the SEGGER_Test.txt already exists on the device, if yes - delete it first.
                    //
                    if (memcmp(_aObjInfoBuffer[ObjectIndex].sFilename, (const U16 *)&_sFileName[0], sizeof(_sFileName)) == 0 ) {
                      Status = USBH_MTP_DeleteObject(hDevice, _aObjBuffer[ObjectIndex].ObjectID);
                      if (Status == USBH_STATUS_SUCCESS) {
                        _PrintUnicodeString(&_ac[0], sizeof(_ac), (const U16 *)_aObjInfoBuffer[ObjectIndex].sFilename);
                        USBH_Logf_Application("Deleted object 0x%0.8X: \"%s\".", _aObjBuffer[ObjectIndex].ObjectID, &_ac[0]);
                      } else {
                        USBH_Warnf_Application("Could not delete object with ID: 0x%0.8X (Status: %s, ErrorCode: 0x%0.4X).\n", _aObjBuffer[ObjectIndex].ObjectID, USBH_GetStatusStr(Status), USBH_MTP_GetLastErrorCode(hDevice));
                      }
                    }
                  } else {
                    USBH_Warnf_Application("Could not retrieve object info for object with ID: 0x%0.8X (Status: %s, ErrorCode: 0x%0.4X).\n", _aObjBuffer[ObjectIndex].ObjectID, USBH_GetStatusStr(Status), USBH_MTP_GetLastErrorCode(hDevice));
                  }
                }
                //
                // Try to create a new file in the root directory of the storage.
                //
                _CreateInfo.FileNameSize   = sizeof(_sFileName) / 2; // Size is in Unicode characters (Unicode character size is 2 bytes).
                _CreateInfo.sFileName      = (const U16*)&_sFileName[0];
                _CreateInfo.ObjectSize     = sizeof(_sTestFileContent);
                _CreateInfo.ParentObjectID = USBH_MTP_ROOT_OBJECT_ID;
                _CreateInfo.pfGetData      = _SendData; // Callback function
                _CreateInfo.pUserBuf       = (U8 *)_sTestFileContent;
                _CreateInfo.UserBufSize    = sizeof(_sTestFileContent);
                _CreateInfo.isFolder       = 0;
                _CreateInfo.pUserContext   = (void*)_sTestFileContent;
                _PrintUnicodeString(&_ac[0], sizeof(_ac), (const U16 *)&_sFileName[0]);
                USBH_Logf_Application("Creating new object with %d bytes in folder 0x%0.8X with name %s.", _CreateInfo.ObjectSize, _CreateInfo.ParentObjectID, &_ac[0]);
                Status = USBH_MTP_CreateObject(hDevice, StorageIndex, &_CreateInfo);
                if (Status == USBH_STATUS_SUCCESS) {
                  USBH_Logf_Application("Created new object in folder 0x%0.8X, ID: 0x%0.8X.\n", _CreateInfo.ParentObjectID, _CreateInfo.ObjectID);
                } else {
                  USBH_Warnf_Application("Could not create new object (Status: %s, ErrorCode: 0x%0.4X).", USBH_GetStatusStr(Status), USBH_MTP_GetLastErrorCode(hDevice));
                }
              } else {
                USBH_Warnf_Application("Could not retrieve object ID list for folder 0x%0.8X (Status: %s, ErrorCode: 0x%0.4X).", DirObjectID, USBH_GetStatusStr(Status), USBH_MTP_GetLastErrorCode(hDevice));
              }
            } else {
              USBH_Warnf_Application("Could not retrieve the number of objects in folder 0x%0.8X (Status: %s, ErrorCode: 0x%0.4X).", DirObjectID, USBH_GetStatusStr(Status), USBH_MTP_GetLastErrorCode(hDevice));
            }
          }
        }
        //
        // Close the device handle.
        //
End:
        USBH_MTP_Close(hDevice);
        USBH_Logf_Application("Connection to MTP device closed.\n");
      }
    }
  }
  //
  // Wait until MTP is removed.
  //
  while (_DevIsReady) {
    OS_Delay(100);
  }
}

/*********************************************************************
*
*       _OnDevNotify
*
*  Function description
*    Callback, called when a device is added or removed.
*    Called in the context of the USBH_Task.
*    The functionality in this routine should not block!
*/
static void _OnDevNotify(void * pContext, U8 DevIndex, USBH_DEVICE_EVENT Event) {
  (void)pContext;
  switch (Event) {
  case USBH_DEVICE_EVENT_ADD:
    USBH_Logf_Application("**** Device added [%d]\n", DevIndex);
    _DevIndex = DevIndex;
    _DevIsReady = 1;
    break;
  case USBH_DEVICE_EVENT_REMOVE:
    USBH_Logf_Application("**** Device removed [%d]\n", DevIndex);
    _DevIsReady = 0;
    _DevIndex   = -1;
    break;
  default:;   // Should never happen
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
*       MainTask
*/
#ifdef __cplusplus
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif
void MainTask(void);
#ifdef __cplusplus
}
#endif
void MainTask(void) {
  USBH_Init();
  OS_SetPriority(OS_GetTaskID(), TASK_PRIO_APP);                                       // This task has the lowest prio for real-time application.
                                                                                       // Tasks using emUSB-Host API should always have a lower priority than emUSB-Host main and ISR tasks.
  OS_CREATETASK(&_TCBMain, "USBH_Task", USBH_Task, TASK_PRIO_USBH_MAIN, _StackMain);   // Start USBH main task
  OS_CREATETASK(&_TCBIsr, "USBH_isr", USBH_ISRTask, TASK_PRIO_USBH_ISR, _StackIsr);    // Start USBH ISR task
  USBH_MTP_Init();
#if EVENT_SUPPORT == 1
  USBH_MTP_ConfigEventSupport(1);
#endif
  USBH_MTP_RegisterNotification(_OnDevNotify, NULL);

  while (1) {
    BSP_ToggleLED(1);
    OS_Delay(100);
    if (_DevIsReady) {
      _OnDevReady();
    }
  }
}

/*************************** End of file ****************************/
