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
#include "protocol_data.h"

#define RECEIVE_TIMEOUT -2
#define TRANSMITTED 1
#define RECEIVED 2
#define BYTE_SIZE 8


uint32_t UART_STATUS = RECEIVED;
enum mode MODE = UNDEFINED_MODE;

enum special_packet {NOT_SPECIAL, INIT};
packet_t INIT_PACKET = {INIT_PACKET_ADDRESS, 0, COMMAND_TYPE_WRITE, INIT_PACKET_SIZE, 0, INIT_PACKET_DATA};


byte_t ID = 0x00;

enum main_state {STATE_TRANSMITTING_COMMAND, STATE_AWAITING_RESPONSE, STATE_MAIN_DONE, STATE_LOST, MAIN_UNDEFINED};
enum secondary_state {STATE_AWAITING_COMMAND, STATE_ACKNOWLEDGING_COMMAND, STATE_SECONDARY_DONE, SEC_UNDEFINED};
enum main_state MAIN_STATE = MAIN_UNDEFINED;
enum secondary_state SECONDARY_STATE = SEC_UNDEFINED;

//CRC-8 DALLAS/MAXIM x^8 + x^5 + x^4 + 1
const uint8_t GENERATOR_POLYNOMIAL = 0x31; 


//Int to string hex copy
void isxcpy(int num, char* str, uint8_t numsize) {
  for(int i=(numsize * 2)-1; i >= 0 ; i--) {
    str[i] = (num % 16 < 10) ? (num % 16 + '0') : (num % 16 - 10 + 'A');
    num /= 16;
  }
}

//String hex to int 
int strnxtoi(char* str, int n) {
  char lstr[9];
  lstr[n] = 0;
  strncpy(lstr, str, n);
  return strtol(lstr, NULL, 16);
}


uint8_t CRC_f(char* data, int len) {
  uint8_t crc8 = 0;
  for(int i=0; i < len-1; i++) {
     crc8 = data[i];
     for(int j=0; j < BYTE_SIZE; j++) {
      if(!!(data[i] & (1 << BYTE_SIZE-1-j))) {
        crc8 = crc8 << 1;
        crc8 |= data[i+1] >> (BYTE_SIZE - j - 1);
        crc8 ^ GENERATOR_POLYNOMIAL; 
      }
     }
  }
  return crc8;
}

int Transmit(UART_HandleTypeDef* huart_main, char* str, int len) {
  printf("Tx: %s\n", str);
  HAL_StatusTypeDef res;
  for(int i=0; i < len; i++) {
    
    if(MODE == SINGLE_CONTROLLER_MODE) while(UART_STATUS == TRANSMITTED) {} //wait for reception

    do {
      res = HAL_UART_Transmit(huart_main, str+i, 1, 100);
    } while(res == HAL_BUSY);
    UART_STATUS = TRANSMITTED;
    if(res != HAL_OK) {
      return -1;
    }
  }
  return 0;
}


//str has to have a size of 270 bytes
int Receive(UART_HandleTypeDef* huart, char* str, int size) {
  int i=0;
  char endflag = 0;
  HAL_StatusTypeDef res;
  if(size == 0) {
    while(1) {
      //if(MODE = SINGLE_CONTROLLER_MODE)while(UART_STATUS == RECEIVED) {} //wait for transmit
      do {
        res = HAL_UART_Receive(huart, str+i, 1, 50);
      } while(res == HAL_BUSY);
      UART_STATUS = RECEIVED;
      if(res != HAL_OK) {
        if(res == HAL_TIMEOUT) {
          return RECEIVE_TIMEOUT;
        }
        return -1;
      }
      //\r has to be changed to \n in production code, \r is here because this is what linux screen sends when you press enter
      if((str[i] == '\r' || str[i] == '\n') && endflag) {
        break;
      }
      else {
        endflag = 0;
      }  
      if(str[i] == ';') {
        endflag=1;
      }
      i++;
     
      printf(str);
      printf("\n");
    }
  }
  else {
    //OS_MUTEX_LockBlocked(&UART_ACCESS);
    res = HAL_UART_Receive(huart, str, size, 50);
    //OS_MUTEX_Unlock(&UART_ACCESS);
    printf(str);
    printf("\n");
  }
  return 0;
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
  
    strncpy(p->data, str + offset, p->size);
    //p->data is not guranteed to be null-terminated
    memset(p->data + p->size, 0, MAX_PACKET_DATA_HEX_LEN - p->size);
    offset += p->size;

    p->crc = strnxtoi(str + offset, PACKET_CRC_HEX_LEN);
}

void PacketEncapsulate(packet_t *packet, char *str) {
  int offset = 0;
  isxcpy(ID, str + offset, PACKET_ID_SIZE);
  ID++;
  //-----
  offset += PACKET_ID_HEX_LEN;
  
  isxcpy((byte_t)packet->cmd_type, str + offset, PACKET_CMDTP_SIZE);
  offset += PACKET_CMDTP_HEX_LEN;

  isxcpy(packet->size, str + offset, PACKET_SIZE_SIZE);
  offset += PACKET_SIZE_HEX_LEN;

  isxcpy(packet->address, str + offset, PACKET_ADDRESS_SIZE);
  offset += PACKET_ADDRESS_HEX_LEN;

  if(packet->size != 0) {
    strncpy(str + offset, packet->data, packet->size);
  }
  offset += packet->size;

  isxcpy(CRC_f(packet->data, packet->size), str + offset, PACKET_CRC_SIZE);
  offset += PACKET_CRC_HEX_LEN;

  strncpy(str + offset, ";\n", 2);
}

int ReceivePacket(UART_HandleTypeDef* huart, packet_t* packet) {
  int res;
  char received[MAX_PACKET_HEX_LEN+1];
  memset(received, 0, MAX_PACKET_HEX_LEN+1);

  res = Receive(huart, received, 0);
  if(res < 0) {
    return res;
  }
  PacketDeencapsulate(received, packet);
  return 0;
}

int TransmitPacket(UART_HandleTypeDef* huart, packet_t* packet) {
  char packet_string[MAX_PACKET_HEX_LEN+1];
  memset(packet_string, 0, MAX_PACKET_HEX_LEN+1);
  
  PacketEncapsulate(packet, packet_string);
  printf("%s\n", packet_string);

  return Transmit(huart, packet_string, MIN_PACKET_HEX_LEN + packet->size);
}


int MainControlled(UART_HandleTypeDef* huart, packet_t * packet, packet_t * incoming) {
  //Finite automaton here
  MAIN_STATE = STATE_TRANSMITTING_COMMAND;
  int res;
  while(MAIN_STATE != STATE_MAIN_DONE) {
    switch(MAIN_STATE) {
      //
      case STATE_TRANSMITTING_COMMAND:
        if(TransmitPacket(huart, packet) == 0) {
          MAIN_STATE = STATE_AWAITING_RESPONSE;
        }
        else {
          printf("Error in transmission\n");
          return -1;
        } 
        break;
      //
      case STATE_AWAITING_RESPONSE:

        if(MODE == SINGLE_CONTROLLER_MODE) {
          while(SECONDARY_STATE == STATE_AWAITING_COMMAND) {}
        }

        res = ReceivePacket(huart, incoming); 
        if(res == 0) {
          if((incoming->cmd_type == COMMAND_TYPE_ACK_WRITE || incoming->cmd_type == COMMAND_TYPE_READ) && incoming->address == packet->address) {
            MAIN_STATE = STATE_MAIN_DONE;
          }
        }
        else if(res == RECEIVE_TIMEOUT){
          MAIN_STATE = STATE_LOST;
        }
        break;
      case STATE_LOST:
        MAIN_STATE = STATE_TRANSMITTING_COMMAND;
        break;
    }
  }
  return 0;
}

int packet_compare(packet_t *p1, packet_t *p2) {
  return ((p1->address == p2->address) && (p1->size == p2->size) && !strcmp(p1->data, p2->data));
}

int SecondaryControlled(UART_HandleTypeDef *huart, enum special_packet *spp) {
  packet_t incoming;
  packet_t ack;
  int res;
  SECONDARY_STATE = STATE_AWAITING_COMMAND;
  while(SECONDARY_STATE != STATE_SECONDARY_DONE) {
    if(SECONDARY_STATE == STATE_AWAITING_COMMAND) {
      ReceivePacket(huart, &incoming);
      SECONDARY_STATE = STATE_ACKNOWLEDGING_COMMAND;
    }
    else if(SECONDARY_STATE == STATE_ACKNOWLEDGING_COMMAND) {
      
      if(packet_compare(&incoming, &INIT_PACKET)) {
        *spp = INIT;
      }

      if(incoming.cmd_type == COMMAND_TYPE_WRITE) {
        ack.cmd_type = COMMAND_TYPE_ACK_WRITE;
        ack.size = 0;
      }
      else {
        ack.cmd_type = COMMAND_TYPE_WRITE;
        ack.size = incoming.size;
      }
      ack.address = incoming.address;

      res = TransmitPacket(huart, &ack);
      if(res == 0) {
        SECONDARY_STATE = STATE_SECONDARY_DONE;
        printf("Ack sent successfully\n");
        continue;
      }
      else {
        printf("ERROR IN ACKNOWLEDGMENT TRANSMISSION\n");
      }
    }
  }
  return 0;
}

int CommunicationInitMain(UART_HandleTypeDef* huart, enum mode com_mode) {
  //Let a communication init be a write on address 0x00 of 'IN'
  packet_t incoming_packet;
  MODE = com_mode;
  return MainControlled(huart, &INIT_PACKET, &incoming_packet);
}

int CommunicationInitSecondary(UART_HandleTypeDef* huart) {
  enum special_packet flag = NOT_SPECIAL;

  while(MODE == UNDEFINED_MODE) {}
  SecondaryControlled(huart, &flag);
  if(flag != INIT) {
    return -1;
  }
  return 0;
}