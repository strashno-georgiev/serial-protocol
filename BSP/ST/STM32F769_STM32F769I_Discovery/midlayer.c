//IMPLEMENT CRC
//Create state machine for transmit, await ackonowledge and retransmit
//ID -> Command type -> Size -> Address -> Data -> CRC -> End of header

//A single message has a minimum size of 6 * 2 + 2 = 14B and a maximum size of 14 + 255 * 2 = 524B
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stm32f7xx_hal.h>
#include "RTOS.h"
#include "midlayer.h"
#include "applayer.h"
#include "protocol_data.h"
#include "hardware_layer.h"
#define BYTE_SIZE 8
#define TIMEOUT 0
enum mode MODE = UNDEFINED_MODE;

packet_t INIT_PACKET = {INIT_PACKET_ADDRESS, 0, COMMAND_TYPE_WRITE, INIT_PACKET_SIZE, 0, INIT_PACKET_DATA};
packet_t BAD_CRC_PACKET = {BAD_CRC_PACKET_ADDRESS, 0, COMMAND_TYPE_WRITE, BAD_CRC_PACKET_SIZE, 0, BAD_CRC_PACKET_DATA};
packet_t END_PACKET = {END_PACKET_ADDRESS, 0, COMMAND_TYPE_WRITE, END_PACKET_SIZE, 0, END_PACKET_DATA};

byte_t ID = 0x00;

//CRC-8 DALLAS/MAXIM x^8 + x^5 + x^4 + 1 (MSB discarded)
const uint8_t GENERATOR_POLYNOMIAL = 0x31; 


//Int to string hex copy
//numsize - size of number in bytes
void isxcpy(int num, char* str, uint8_t numsize) {
  for(int i=(numsize * 2)-1; i >= 0 ; i--) {
    str[i] = (num % 16 < 10) ? (num % 16 + '0') : (num % 16 - 10 + 'A');
    num /= 16;
  }
}

//String hex to int 
//str - string
//n - length (in characters) of number encoded in string
int strnxtoi(char* str, int n) {
  char lstr[9];
  lstr[n] = 0;
  memcpy(lstr, str, n);
  return strtol(lstr, NULL, 16);
}


void PacketDeencapsulate(char *str, packet_t * p) {
    int offset = 0;
    p->id = strnxtoi(str, PACKET_ID_HEX_LEN);
    offset += PACKET_ID_HEX_LEN;

    p->cmd_type = strnxtoi(str + offset, PACKET_CMDTP_HEX_LEN);
    offset += PACKET_CMDTP_HEX_LEN;
    
    p->size = strnxtoi(str + offset, PACKET_SIZE_HEX_LEN);
    offset += PACKET_SIZE_HEX_LEN;
  
    p->address = strnxtoi(str + offset, PACKET_ADDRESS_HEX_LEN);
    offset += PACKET_ADDRESS_HEX_LEN;
  
    if(p->cmd_type == COMMAND_TYPE_WRITE) {
      if(DATA_WORD_LEN == 1) {
        memcpy(p->data, str + offset, p->size);
        offset += p->size;
      }
      else if(DATA_WORD_LEN == 2) {
        for(int i=0; i < p->size; i++) {
          p->data[i] = strnxtoi(str + i*2, sizeof(uint8_t) * 2);
        }
      }
    }

    p->crc = strnxtoi(str + offset, PACKET_CRC_HEX_LEN);
}

uint8_t CRC_f(char* data, int len) {
  uint8_t crc8 = data[0];
  uint8_t shift_counter = 0;
  char flag = 0;
  for(int i=0; i < len-1;) {
    if(!!(crc8 & (1 << (BYTE_SIZE-1)))) {
      flag = 1;
    }
    crc8 = crc8 << 1;
    crc8 |= data[i+1] >> (BYTE_SIZE - shift_counter - 1);
    shift_counter++;
    if(flag) {
      crc8 = crc8 ^ GENERATOR_POLYNOMIAL;
      flag = 0;
    }
    if(shift_counter == 8) {
      shift_counter = 0;
      i++;
    }
  }
  return crc8;
}

void PacketEncapsulateCRC(packet_t *packet, char *str) {
  int offset = 0;
  isxcpy(packet->id, str + offset, PACKET_ID_SIZE);
  //-----
  offset += PACKET_ID_HEX_LEN;
  
  isxcpy((byte_t)packet->cmd_type, str + offset, PACKET_CMDTP_SIZE);
  offset += PACKET_CMDTP_HEX_LEN;

  isxcpy(packet->size, str + offset, PACKET_SIZE_SIZE);
  offset += PACKET_SIZE_HEX_LEN;

  isxcpy(packet->address, str + offset, PACKET_ADDRESS_SIZE);
  offset += PACKET_ADDRESS_HEX_LEN;

  if(packet->cmd_type == COMMAND_TYPE_WRITE) {
    if(DATA_WORD_LEN == 1) {
      memcpy(str + offset, packet->data, packet->size);
      offset += packet->size;
    }
    else if(DATA_WORD_LEN == 2) {
      for(int i=0; i < packet->size; i++) {
        isxcpy(packet->data[i], str[i*2], sizeof(uint8_t));
      }
      offset += packet->size * 2;
    }
  }

  isxcpy(CRC_f(str, offset), str + offset, PACKET_CRC_SIZE);
  offset += PACKET_CRC_HEX_LEN;

  memcpy(str + offset, ";\n", 2);
}

enum ReceiveStatus ReceivePacket(packet_t* packet) {
  int res;
  uint8_t crc;
  char received[MAX_PACKET_HEX_LEN+1];
  memset(received, 0, MAX_PACKET_HEX_LEN+1);
  
  res = Receive(received);
  if(res < MIN_PACKET_HEX_LEN) {
    return RECEIVE_INVALID;
  }
  
  crc = CRC_f(received, res - PACKET_CRC_HEX_LEN - PACKET_ENDING_HEX_LEN);
  
  PacketDeencapsulate(received, packet);
  if(crc != packet->crc) {
    return RECEIVE_BAD_CRC;
  }
  return RECEIVE_SUCCESS;
}

enum ReceiveStatus ReceivePacketTimed(packet_t *packet, OS_TIME timeout) {
  int res;
  uint8_t crc;
  char received[MAX_PACKET_HEX_LEN+1];
  memset(received, 0, MAX_PACKET_HEX_LEN+1);

  res = ReceiveTimed(received, timeout);

  if(res < 0) {
    return RECEIVE_TIMEOUT;
  }
  if(res < MIN_PACKET_HEX_LEN) {
    return RECEIVE_INVALID;
  }

  crc = CRC_f(received, res - PACKET_CRC_HEX_LEN - PACKET_ENDING_HEX_LEN);
  
  PacketDeencapsulate(received, packet);
  if(crc != packet->crc) {
    return RECEIVE_BAD_CRC;
  }
  
  return RECEIVE_SUCCESS; 
}

int TransmitPacket(packet_t* packet) {
  char packet_string[MAX_PACKET_HEX_LEN+1];
  memset(packet_string, 0, MAX_PACKET_HEX_LEN+1);
  packet->id = ID;
  PacketEncapsulateCRC(packet, packet_string);
  return Transmit(packet_string, MIN_PACKET_HEX_LEN + (packet->cmd_type == COMMAND_TYPE_READ ? 0 : (packet->size * DATA_WORD_LEN)));
}

int comparePackets(packet_t *p1, packet_t *p2) {
  return ((p1->address == p2->address) && (p1->size == p2->size) && !strcmp(p1->data, p2->data));
}

enum main_state MainControlled_TransmittingCommand(packet_t * packet) {
  if(TransmitPacket(packet) == 0) {
    return STATE_AWAITING_RESPONSE;
  }
  else {
    return -1;
  } 
}

enum main_state MainControlled_AwaitingResponse(packet_t * packet, packet_t * incoming) {
  int res;
  if(TIMEOUT) {
    res = ReceivePacketTimed(incoming, 250);
  } 
  else {
    res = ReceivePacket(incoming);
  }
  
  if(res == RECEIVE_SUCCESS) {
    if(comparePackets(incoming, &BAD_CRC_PACKET)) {
      printf("Bad CRC\n");
      return STATE_TRANSMITTING_COMMAND;
    } 
    //Valid acknowledgement
    else if(incoming->address == packet->address) {
      ID = incoming->id + 1;
      return STATE_MAIN_DONE;
    }
  }
  else if(res == RECEIVE_BAD_CRC) {
    return STATE_TRANSMITTING_COMMAND;
  }
  else if(res == RECEIVE_TIMEOUT){
    printf("Command lost, retransmitting\n");
     return STATE_LOST;
  }
  return -1;
}

int MainControlled(packet_t * packet, packet_t * incoming) {
  //Finite automaton here
  enum main_state MAIN_STATE = STATE_TRANSMITTING_COMMAND;
  while(MAIN_STATE != STATE_MAIN_DONE) {
    switch(MAIN_STATE) {
      //
      case STATE_TRANSMITTING_COMMAND:
        MAIN_STATE = MainControlled_TransmittingCommand(packet);
        break;
      case STATE_AWAITING_RESPONSE:
        MAIN_STATE = MainControlled_AwaitingResponse(packet, incoming);
        break;
      case STATE_LOST:
        MAIN_STATE = STATE_TRANSMITTING_COMMAND;
        break;
      case -1:
        printf("Error in main controlled transmit sequence\n");
        break;
    }
  }
  return 0;
}


int TransmitCommandControlled(uint8_t cmd_type, uint8_t size, uint16_t address, char *str, packet_t* response) {
  packet_t p;
  p.address = address;
  p.size = size;
  p.cmd_type = cmd_type;
  memset(p.data, 0, MAX_PACKET_DATA_HEX_LEN);
  if(cmd_type == COMMAND_TYPE_WRITE) {
    memcpy(p.data, str, size);
  }
  p.data[size] = 0;
  return MainControlled(&p, response);
}

int SecondaryAcknowledge(uint8_t ack_type, uint8_t size, uint16_t address, char *str) {
  packet_t p;
  p.address = address;
  p.size = size;
  p.cmd_type = ack_type;
  memset(p.data, 0, MAX_PACKET_DATA_HEX_LEN);
  memcpy(p.data, str, size);
  return TransmitPacket(&p);
}


int SecondaryReceive(packet_t *incoming, enum special_packet *spp) {
  int res;
  enum secondary_state state = STATE_AWAITING_COMMAND;
  while(state == STATE_AWAITING_COMMAND) {
    res = ReceivePacket(incoming);
    ID = incoming->id + 1;
    if(res == RECEIVE_INVALID) {
      return STATE_AWAITING_COMMAND;
      continue;
    }
    if(res == RECEIVE_BAD_CRC) {
      res = TransmitPacket(&BAD_CRC_PACKET);
      state = STATE_AWAITING_COMMAND;
      continue;
    }
    if(spp != NULL) {
      if(comparePackets(incoming, &INIT_PACKET)) {
        *spp = INIT;
      }
      else if(comparePackets(incoming, &END_PACKET)) {
        *spp = END;
      }
    }
    state = STATE_ACKNOWLEDGING_COMMAND;
  }
  return 0;
}


int CommunicationInitMain(enum mode com_mode) {
  //Let a communication init be a write on address 0x00 of 'IN'
  packet_t incoming_packet;
  MODE = com_mode;
  return MainControlled(&INIT_PACKET, &incoming_packet);
}

int CommunicationInitSecondary(enum mode com_mode) {
  enum special_packet flag = NOT_SPECIAL;
  packet_t inc;
  int res;
  while(MODE == UNDEFINED_MODE && com_mode == UNDEFINED_MODE) {}
  MODE = com_mode;
  if(MODE == MULTI_CONTROLLER_MODE) {
    ID = 1;
  }
  do {
    res = SecondaryReceive(&inc, &flag);
  }
  while(res == STATE_AWAITING_COMMAND);
  SecondaryAcknowledge(COMMAND_TYPE_ACK_WRITE, 0, 0, "");
  if(flag != INIT) {
    return -1;
  }
  return 0;
}

int CommunicationEndMain(packet_t * res) {
  return MainControlled(&END_PACKET, res);
}

int initMidLayer(UART_HandleTypeDef *huart, USART_TypeDef *instance, enum deviceRole role, enum mode mode) {
  initHardwareLayer(huart, instance);
  if(role == PRIMARY) {
    return CommunicationInitMain(mode);
  }
  else if(role == SECONDARY) {
    return CommunicationInitSecondary(mode);
  }
  return -1;
}