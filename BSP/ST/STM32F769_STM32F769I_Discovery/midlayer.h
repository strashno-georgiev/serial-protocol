#pragma once
#include "protocol_data.h"
#include <stdint.h>
#include <stm32f7xx_hal.h>
typedef uint8_t byte_t;

void isxcpy(int num, char* str, uint8_t numsize);

//uint8_t CRC_f(char* data, int len);

typedef struct {
  uint16_t address;
  uint8_t id;
  uint8_t cmd_type;
  uint8_t size;
  uint8_t crc;
  char data[MAX_PACKET_DATA_HEX_LEN+1];
} packet_t;

enum special_packet {NOT_SPECIAL, INIT, BAD_CRC, END};
enum mode {MULTI_CONTROLLER_MODE, SINGLE_CONTROLLER_MODE, UNDEFINED_MODE};

int Transmit(UART_HandleTypeDef* huart_main, char* str, int len);
int TransmitCommand(UART_HandleTypeDef* huart, uint8_t cmd_type, uint8_t size, uint16_t address, char *str, packet_t* response);
int SecondaryControlled(UART_HandleTypeDef *huart, enum special_packet *spp);
int MainControlled(UART_HandleTypeDef* huart, packet_t * packet, packet_t * incoming);
int CommunicationEndMain(UART_HandleTypeDef* huart, packet_t * res);
int CommunicationInitMain(UART_HandleTypeDef* huart, enum mode com_mode);
int CommunicationInitSecondary(UART_HandleTypeDef* huart);

#define INIT_PACKET_DATA "IN"
#define INIT_PACKET_SIZE 2
#define INIT_PACKET_ADDRESS 0x0000

#define BAD_CRC_PACKET_DATA "BCRC"
#define BAD_CRC_PACKET_SIZE 4
#define BAD_CRC_PACKET_ADDRESS 0x0000

#define END_PACKET_DATA "END"
#define END_PACKET_SIZE 3
#define END_PACKET_ADDRESS 0x0000

