#include "applayer.h"
#include "midlayer.h"
UART_HandleTypeDef *HUART;
char writeBuffer[2 * K], readBuffer[2 * K];

int communicationStart(UART_HandleTypeDef *huart, enum deviceRole role) {
  if(huart == NULL) {
    return -1;
  }
  HUART = huart;
  memset(writeBuffer, 0, 2*K);
  memset(readBuffer, 0, 2*K);
  initMidLayer();
  if(role == PRIMARY) {
    CommunicationInitMain(HUART, MULTI_CONTROLLER_MODE);
  }
  else if(role == SECONDARY) {
    CommunicationInitSecondary(HUART, MULTI_CONTROLLER_MODE);
  }
  return 0;
}

int write(uint8_t size, uint16_t address) {
  //strncpy(writeBuffer+address, str, size);
  //Transmit packet with value at address;
  packet_t res;
  return TransmitCommand(HUART, COMMAND_TYPE_WRITE, size, address, writeBuffer+address, &res);
}

int read(uint8_t size, uint16_t address) {
  //Transmit read packet, write to buffer
  return 0;
}