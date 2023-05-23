#ifndef MIDLAYER_HH
#define MIDLAYER_HH

#include "protocol_data.h"
#include "applayer.h"
#include <stdint.h>
#include <stm32f7xx_hal.h>


typedef uint8_t byte_t;

typedef struct {
  uint16_t address;
  uint8_t id;
  uint8_t cmd_type;
  uint8_t size;
  uint8_t crc;
  char data[MAX_PACKET_DATA_SIZE +1];
} packet_t;

enum special_packet {NOT_SPECIAL, INIT, BAD_CRC, END};

enum ReceiveStatus {RECEIVE_SUCCESS, RECEIVE_BAD_CRC, RECEIVE_INVALID, RECEIVE_TIMEOUT};

enum main_state {STATE_TRANSMITTING_COMMAND, STATE_AWAITING_RESPONSE, STATE_MAIN_DONE, STATE_LOST, MAIN_UNDEFINED};
enum secondary_state {STATE_AWAITING_COMMAND, STATE_ACKNOWLEDGING_COMMAND, STATE_SECONDARY_DONE, SEC_UNDEFINED};

int TransmitCommandControlled(uint8_t cmd_type, uint8_t size, uint16_t address, char *str, packet_t* response);

int SecondaryReceive(packet_t *incoming, enum special_packet *spp);
int SecondaryAcknowledge(uint8_t ack_type, uint8_t size, uint16_t address, char *str);


int initMidLayer(UART_HandleTypeDef* huart, USART_TypeDef *, enum deviceRole);

#define INIT_PACKET_DATA {0xBE, 0xEF}
#define INIT_PACKET_SIZE 4 / DATA_WORD_LEN
#define INIT_PACKET_ADDRESS 0x0000

#define BAD_CRC_PACKET_DATA {0xCC, 0xCC}
#define BAD_CRC_PACKET_SIZE 4 / DATA_WORD_LEN
#define BAD_CRC_PACKET_ADDRESS 0x0000

#define END_PACKET_DATA {0xDE, 0xAD}
#define END_PACKET_SIZE 4 / DATA_WORD_LEN
#define END_PACKET_ADDRESS 0x0000

#endif

