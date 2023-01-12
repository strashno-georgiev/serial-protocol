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
File    : SYS_Linux.c
Purpose : Operating system specific functions for Linux
---------------------------END-OF-HEADER------------------------------
*/

#include "SYS.h"
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>
#include <libudev.h>

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#define USB_SYS_PATH    "/sys/devices"


/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#define USB_SYS_DEBUG       1

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

typedef struct {
  unsigned BFlag;
  unsigned Baudrate;
} BAUDMAP;

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static unsigned _COMTimeout = 1000;

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/

static const BAUDMAP BaudMap[] = {
  { B50,      50   },
  { B75,      75   },
  { B110,     110   },
  { B134,     134   },
  { B150,     150   },
  { B200,     200   },
  { B300,     300   },
  { B600,     600   },
  { B1200,    1200   },
  { B1800,    1800   },
  { B2400,    2400   },
  { B4800,    4800   },
  { B9600,    9600   },
  { B19200,   19200   },
  { B38400,   38400   },
  { B57600,   57600   },
  { B115200,  115200   },
  { B230400,  230400   },
  { B460800,  460800   },
  { B500000,  500000   },
  { B576000,  576000   },
  { B921600,  921600   },
  { B1000000, 1000000  },
  { B1152000, 1152000  }
};


/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/


/*********************************************************************
*
*       SYS_ListComPorts
*
*  Function description
*    Find all USB CDC devices and list COM Port 
*/
unsigned SYS_ListComPorts(CDC_DEVICE *pDevices, unsigned MaxDevices) {
  unsigned Count = 0;
  struct udev *udev;
  struct udev_enumerate *enumerate;
  struct udev_list_entry *devices, *dev_list_entry;

  /* Create the udev object */
  udev = udev_new();
  if (!udev) {
#if USB_SYS_DEBUG > 0
    fprintf(stderr,"CDC: Can't create udev\n");
#endif
    return -1;
  }
  //
  // Create a list of the devices in the 'tty' subsystem.
  //
  enumerate = udev_enumerate_new(udev);
  udev_enumerate_add_match_subsystem(enumerate, "tty");
  udev_enumerate_scan_devices(enumerate);
  devices = udev_enumerate_get_list_entry(enumerate);
  //
  // For each item, see if it matches the vid/pid
  //
  udev_list_entry_foreach(dev_list_entry, devices) {
    const char *sysfs_path;
    const char *dev_path;
    const char *dev_name;
    struct udev_device *cdc_dev;        // The device's CDC udev node.
    struct udev_device *dev;            // The actual hardware device.

    //
    // Get the filename of the /sys entry for the device
    // and create a udev_device object (dev) representing it
    //
    sysfs_path = udev_list_entry_get_name(dev_list_entry);
    cdc_dev = udev_device_new_from_syspath(udev, sysfs_path);
    dev_path = udev_device_get_devnode(cdc_dev);
    dev = udev_device_get_parent_with_subsystem_devtype(
           cdc_dev,
           "usb",
           "usb_device");
    if (dev) {
      dev_name = udev_device_get_sysattr_value(dev,"product");
      strncpy(pDevices->Device, dev_path, sizeof(pDevices->Device));
      strncpy(pDevices->Desc, dev_name, sizeof(pDevices->Desc));
      Count++;
      pDevices++;
    }
    udev_device_unref(cdc_dev);
    if (Count >= MaxDevices) {
      break;
    }
  }
  return Count;
}

/*********************************************************************
*
*       SYS_COMSetTimeout
*/
void SYS_COMSetTimeout(unsigned Timeout) {
  _COMTimeout = Timeout;
}

/*********************************************************************
*
*       SYS_COMWrite
*
*  Function description
*    Write bytes to COM
*/
int SYS_COMWrite(SYS_COM hCOM, int NumBytes, const U8 *pData) {
  int   r;
  struct pollfd fds;
  int   BytesAtOnces;

  while (NumBytes > 0) {
    fds.fd = hCOM;
    fds.events = POLLOUT | POLLERR;
    if ((r = poll(&fds, 1, _COMTimeout)) <= 0) {
      if (r != 0) {
        r = errno;
      }
      return -1;
    }
    BytesAtOnces = NumBytes;
    if(BytesAtOnces > 512) {
      BytesAtOnces = 512;
    }
    if ((r = write(hCOM, pData, BytesAtOnces)) < 0) {
      return -1;
    }
    NumBytes -= r;
    pData    += r;
  }
  return 0;
}


/*********************************************************************
*
*       SYS_COMRead
*
*  Function description
*    Read bytes from COM
*/
int SYS_COMRead(SYS_COM hCOM, int NumBytes, U8 *pBuff, unsigned Timeout) {
  int   Cnt = 0;
  int   r;
  struct pollfd fds;

  while (NumBytes > 0) {
    fds.fd = hCOM;
    fds.events = POLLIN | POLLERR;
    if ((r = poll(&fds, 1, Timeout)) <= 0) {
      if (r != 0) {
        r = errno;
      }
      return -1;
    }
    if ((r = read(hCOM, pBuff, NumBytes)) < 0) {
      return -1;
    }
    NumBytes -= r;
    pBuff    += r;
    Cnt      += r;
  }
  return Cnt;
}

/*********************************************************************
*
*       SYS_COMOpen
*
*  Function description
*    Opens and initializes a COM port
*/
int SYS_COMOpen(SYS_COM *pCOM, const char *pDevice) {
  int  fd;
  struct termios Term;

  if ((fd = open(pDevice, O_RDWR)) < 0) {
    *pCOM = -1;
    return -1;
  }
  *pCOM = fd;
  if (tcgetattr(fd, &Term)) {
    return -1;
  }
  cfmakeraw(&Term);
  if (tcsetattr(fd, TCSANOW, &Term)) {
    return -1;
  }
  return 0;
}

/*********************************************************************
*
*       SYS_COMClose
*
*  Function description
*    Close COM port
*/
void SYS_COMClose(SYS_COM *pCOM) {
  close(*pCOM);
  *pCOM = -1;
}

/*********************************************************************
*
*       SYS_COMSetLine
*
*  Function description
*    Set/Clear modem line
*/
int SYS_COMSetLine(SYS_COM hCOM, int Line, int Value) {
  int ModemCtrl;

  if (Line == 0) {
    //
    // Handle RTS
    //
    ModemCtrl = TIOCM_RTS;
  } else {
    //
    // Handle DTR
    //
    ModemCtrl = TIOCM_DTR;
  }
  if (Value) {
    if (ioctl(hCOM, TIOCMBIS, &ModemCtrl)) {
      return -1;
      }
    } else {
    if (ioctl(hCOM, TIOCMBIC, &ModemCtrl)) {
      return -1;
    }
  }
  return 0;
}

/*********************************************************************
*
*       SYS_COMGetLine
*
*  Function description
*    Get modem line values
*/
int SYS_COMGetLine(SYS_COM hCOM, int *pCTS, int *pDSR) {
  int ModemCtrl;

  if (ioctl(hCOM, TIOCMGET, &ModemCtrl)) {
    return -1;
  }
  if (ModemCtrl & TIOCM_CTS) {
    *pCTS = 1;
  } else {
    *pCTS = 0;
  }
  if (ModemCtrl & TIOCM_DSR) {
    *pDSR = 1;
  } else {
    *pDSR = 0;
  }
  return 0;
}

/*********************************************************************
*
*       SYS_COMBreak
*
*  Function description
*    Get modem line values
*/
int SYS_COMBreak(SYS_COM hCOM, unsigned Duration) {
  if (tcsendbreak(hCOM, Duration)) {
    return -1;
  }
  return 0;
}

/*********************************************************************
*
*       SYS_COMGetParameter
*
*  Function description
*    Get communication parameters
*/
int SYS_COMGetParameter(SYS_COM hCOM, COM_PARAM *p) {
  struct termios Term;
  unsigned BFlag;
  int i;

  if (tcgetattr(hCOM, &Term)) {
    return -1;
  }
  p->Baudrate = 0;
  BFlag = cfgetispeed(&Term);
  for (i = sizeof(BaudMap) / sizeof(BAUDMAP); --i >= 0; ) {
    if (BaudMap[i].BFlag == BFlag) {
      p->Baudrate = BaudMap[i].Baudrate;
      break;
    }
  }
  p->StopBits = ((Term.c_cflag & CSTOPB) != 0) ? 2 : 1;
  switch(Term.c_cflag & CSIZE) {
    case CS5: p->NumBits = 5; break;
    case CS6: p->NumBits = 6; break;
    case CS7: p->NumBits = 7; break;
    case CS8: p->NumBits = 8; break;
  }
  if (Term.c_cflag & PARENB) {
    if (Term.c_cflag & PARODD) {
      p->Parity = 1;
    } else {
      p->Parity = 2;
    }
  } else {
    p->Parity = 0;
  }
  return 0;
}

/*********************************************************************
*
*       SYS_COMSetParameter
*
*  Function description
*    Set communication parameters
*/
int SYS_COMSetParameter(SYS_COM hCOM, COM_PARAM *p) {
  struct termios Term;
  unsigned i;

  if (tcgetattr(hCOM, &Term)) {
    return -1;
  }
  for (i = sizeof(BaudMap) / sizeof(BAUDMAP); --i >= 0; ) {
    if (BaudMap[i].Baudrate == p->Baudrate) {
      cfsetspeed(&Term, BaudMap[i].BFlag);
      break;
    }
  }
  if (i < 0) {
    return -1;
  }
  if (p->StopBits == 2) {
    Term.c_cflag |= CSTOPB;
  } else {
    Term.c_cflag &= ~CSTOPB;
  }
  Term.c_cflag &= ~CSIZE;
  switch (p->NumBits) {
    case 5: Term.c_cflag |= CS5; break;
    case 6: Term.c_cflag |= CS6; break;
    case 7: Term.c_cflag |= CS7; break;
    case 8: Term.c_cflag |= CS8; break;
  }
  Term.c_cflag &= ~(PARENB | PARODD);
  switch (p->Parity) {
    case 1: Term.c_cflag |= PARENB | PARODD; break;
    case 2: Term.c_cflag |= PARENB; break;
  }
  if (tcsetattr(hCOM, TCSANOW, &Term)) {
    return -1;
  }
  return 0;
}

/*************************** end of file ****************************/
