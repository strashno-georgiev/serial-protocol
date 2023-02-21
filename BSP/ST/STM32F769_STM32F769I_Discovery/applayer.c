#include "applayer.h"
#include "midlayer.h"
#include "RTOS.h"
UART_HandleTypeDef HUART;
char writeBuffer[2 * K], readBuffer[2 * K];
OS_MUTEX writeMutex, readMutex;

void safeCopy(char *dest, char *src, int n, OS_MUTEX *mutex) {
  OS_MUTEX_LockBlocked(mutex);
  memcpy(dest, src, n);
  OS_MUTEX_Unlock(mutex);
}

int communicationStart(USART_TypeDef *instance, enum deviceRole role, enum mode mode) {
  memset(writeBuffer, 0, 2*K);
  memset(readBuffer, 0, 2*K);
  OS_MUTEX_Create(&writeMutex);
  OS_MUTEX_Create(&readMutex);

  initMidLayer(&HUART, instance, role, mode);
  return 0;
}

int write(uint8_t size, uint16_t address) {
  //Transmit packet with value at address;
  packet_t res;
  //safeCopy(writeBuffer+address, "neprotivokonstituci", size, &writeMutex);
  //Uses layer 2 verification of acknowledgement
  return TransmitCommandControlled(&HUART, COMMAND_TYPE_WRITE, size, address, writeBuffer+address, &res);
}

int read(uint8_t size, uint16_t address) {
  //Transmit read packet, write to buffer
  int res;
  packet_t inc;
  res = TransmitCommandControlled(&HUART, COMMAND_TYPE_READ, size, address, "", &inc);
  if(res < 0) {
    return res;
  }
  safeCopy(readBuffer, inc.data, inc.size, &readMutex);
  return 0;
}

int handleCommand() {
  packet_t incoming;
  int res;
  
  do {
  res = SecondaryControlled(&HUART, &incoming, NULL);
  } while(res == STATE_AWAITING_COMMAND);

  if(incoming.cmd_type == COMMAND_TYPE_WRITE) {
    safeCopy(readBuffer, incoming.data, incoming.size, &readMutex);
    return TransmitAck(&HUART, COMMAND_TYPE_ACK_WRITE, 0, incoming.address, "");
  }
  else if(incoming.cmd_type == COMMAND_TYPE_READ) {
    //Lock readBuffer
    OS_MUTEX_Lock(&writeMutex);
    res = TransmitAck(&HUART, COMMAND_TYPE_WRITE, incoming.size, incoming.address, writeBuffer+incoming.address);
    OS_MUTEX_Unlock(&writeMutex);
    return res;
  }
  return -1;
}