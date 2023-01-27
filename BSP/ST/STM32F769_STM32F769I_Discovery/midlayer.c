//IMPLEMENT CRC
//Create state machine for transmit, await ackonowledge and retransmit
//ID -> Command type -> Size -> Address -> Data -> CRC -> End of header

//A single message has a minimum size of 6 * 2 + 2 = 14B and a maximum size of 14 + 255 * 2 = 524B
#include <stdint.h>
#include <string.h>
#include <stm32f7xx_hal.h>

typedef uint8_t byte_t;

byte_t ID = 0x00;

char MESSAGE[255];
char PACKET[269];

int pow(int b, int x) {
  int res = 1;
  if(x >= 0) {
    for(int i=0; i < x; i++) {
      res *= b;
    }
  }
  else {
    for(int i=0; i > x; i--) {
      res /= b;
    }
  }
  return res;
}

//Int to string hex copy
void isxcpy(int num, char* str, uint8_t numsize) {
  for(int i=(numsize * 2)-1; i >= 0 ; i--) {
    str[i] = (num % 16 < 10) ? (num % 16 + '0') : (10 - num % 16 + 'a');
    num /= 16;
  }
}

int8_t CRC_f(char* data) {
  return 0x00;
}

void Transmit(UART_HandleTypeDef* huart_main, char* str, int len) {
  //Here a state machine should be implemented
  int i=0;
  while(i < len) {
    HAL_UART_Transmit(huart_main, str + i, 1, 100);
    i++;
  }
}

void WriteCommand(UART_HandleTypeDef* huart, char* data, byte_t len, uint16_t address) {
  //Should be mutually exclusive with other ID usages - we don't want two packets with the same ID
  isxcpy(ID, PACKET, 1);
  ID++;
  //-----
  isxcpy((byte_t)0x0E, PACKET+2, 1);
  isxcpy(len, PACKET+4, 1);
  isxcpy(address, PACKET+6, 2);
  strncpy(PACKET+10, data, len);
  isxcpy(CRC_f(PACKET), PACKET+10+len, 1);
  strncpy(PACKET, ";\n", 2);
  

  Transmit(huart, PACKET, 6 * 2 + 2 + len);
}

void ReadCommand(UART_HandleTypeDef* huart, int address) {
  //Should be mutually exclusive with other ID usages - we don't want two packets with the same ID
  isxcpy(ID, PACKET, 1);
  ID++;
  //-----
  isxcpy((byte_t)0x0E, PACKET+2, 1);
  isxcpy(0x00, PACKET+4, 1);
  isxcpy(address, PACKET+6, 2);
  isxcpy(CRC_f(PACKET), PACKET+10, 1);
  strncpy(PACKET, ";\n", 2);
  

  Transmit(huart, PACKET, 6 * 2 + 2);
}

void AckWriteCommand(UART_HandleTypeDef* huart, int address) {
  //Should be mutually exclusive with other ID usages - we don't want two packets with the same ID
  isxcpy(ID, PACKET, 1);
  ID++;
  //-----
  isxcpy((byte_t)0x0E, PACKET+2, 1);
  isxcpy(0x00, PACKET+4, 1);
  isxcpy(address, PACKET+6, 2);
  isxcpy(CRC_f(PACKET), PACKET+10, 1);
  strncpy(PACKET, ";\n", 2);
  

  Transmit(huart, PACKET, 6 * 2 + 2);
}

void Communication_Init(UART_HandleTypeDef* huart_main, UART_HandleTypeDef* huart_secondary) {
  //Let a communication init be a write on address 0x00 of 'IN'
}