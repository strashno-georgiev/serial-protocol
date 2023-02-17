#include "applayer.h"
#include "midlayer.h"
UART_HandleTypeDef HUART;
char writeBuffer[2 * K], readBuffer[2 * K];

int communicationStart(USART_TypeDef *instance, enum deviceRole role, enum mode mode) {
  memset(writeBuffer, 0, 2*K);
  memset(readBuffer, 0, 2*K);
  
  initMidLayer(&HUART, instance);

  if(role == PRIMARY) {
    CommunicationInitMain(&HUART, mode);
  }
  else if(role == SECONDARY) {
    CommunicationInitSecondary(&HUART, mode);
  }
  return 0;
}

int write(uint8_t size, uint16_t address) {
  //strncpy(writeBuffer+address, str, size);
  //Transmit packet with value at address;
  packet_t res;
  strncpy(writeBuffer+address, "neprotivokonstituci", size);
  return TransmitCommand(&HUART, COMMAND_TYPE_WRITE, size, address, writeBuffer+address, &res);
}

int read(uint8_t size, uint16_t address) {
  //Transmit read packet, write to buffer
  packet_t res;
  return TransmitCommand(&HUART, COMMAND_TYPE_READ, size, address, "", &res);
}