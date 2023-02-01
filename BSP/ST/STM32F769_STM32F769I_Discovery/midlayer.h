#pragma once
#include "protocol_data.h"
typedef uint8_t byte_t;
//int pow(int b, int x);

void isxcpy(int num, char* str, uint8_t numsize);

uint8_t CRC_f(char* data);

int Transmit(UART_HandleTypeDef* huart_main, char* str, int len);
void WriteCommand(UART_HandleTypeDef* huart, char* data, byte_t len, uint16_t address);
void ReadCommand(UART_HandleTypeDef* huart, int address);
void AckWriteCommand(UART_HandleTypeDef* huart, int address);

typedef struct {
  uint16_t address;
  uint8_t id;
  uint8_t cmd_type;
  uint8_t size;
  uint8_t crc;
  char data[MAX_PACKET_DATA_HEX_LEN+1];
} packet_t;