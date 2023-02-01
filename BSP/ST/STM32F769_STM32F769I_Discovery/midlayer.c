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

OS_MUTEX UART_ACCESS;
byte_t ID = 0x00;
enum states {STATE_TO_TRANSMIT, STATE_AWAITING_RESPONSE, STATE_TRANSMITTED, STATE_LOST};
enum events {EVENT_SENT, EVENT_RECEIVED, EVENT_LOST};

//char PACKET[MAX_PACKET_HEX_LEN+1];

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

uint8_t CRC_f(char* data) {
  return 0x00;
}

int Transmit(UART_HandleTypeDef* huart_main, char* str, int len) {
  //Here a state machine should be implemented
  HAL_StatusTypeDef res;
  OS_MUTEX_LockBlocked(&UART_ACCESS);
  res = HAL_UART_Transmit(huart_main, str, len, 100);
  OS_MUTEX_Unlock(&UART_ACCESS);
  if(res != HAL_OK) {
    return -1;
  }
  //}
  printf("Successfully transmit\n");
  return 0;
}


//str has to have a size of 270 bytes
int ReceivePacket(UART_HandleTypeDef* huart, char* str) {
  int i=0;
  char endflag = 0;
  HAL_StatusTypeDef res;
  while(1) {
    OS_MUTEX_LockBlocked(&UART_ACCESS);
    res = HAL_UART_Receive(huart, str+i, 1, 50);
    OS_MUTEX_Unlock(&UART_ACCESS);
    if(res != HAL_OK) {
      if(res == HAL_TIMEOUT) {
        return RECEIVE_TIMEOUT;
      }
      return -1;
    }
    //\r has to be changed to \n in production code, \r is here because this is what linux screen sends when you press enter
    if(str[i] == '\r' && endflag) {
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
    p->data[p->size] = 0;
    offset += p->size;

    p->crc = strnxtoi(str + offset, PACKET_CRC_HEX_LEN);
}


int PacketEncapsulateAndTransmit(UART_HandleTypeDef* huart, packet_t* packet) {
  //Should be mutually exclusive with other ID usages - we don't want two packets with the same ID
  int offset = 0;
  char packet_string[MAX_PACKET_DATA_HEX_LEN];
  //packet->size = strlen(packet->data);
  isxcpy(ID, packet_string + offset, PACKET_ID_SIZE);
  ID++;
  //-----
  offset += PACKET_ID_HEX_LEN;
  
  isxcpy((byte_t)packet->cmd_type, packet_string + offset, PACKET_CMDTP_SIZE);
  offset += PACKET_CMDTP_HEX_LEN;

  isxcpy(packet->size, packet_string + offset, PACKET_SIZE_SIZE);
  offset += PACKET_SIZE_HEX_LEN;

  isxcpy(packet->address, packet_string+offset, PACKET_ADDRESS_SIZE);
  offset += PACKET_ADDRESS_HEX_LEN;

  if(packet->size != 0) {
    strncpy(packet_string + offset, packet->data, packet->size);
  }
  offset += packet->size;

  isxcpy(CRC_f(packet_string), packet_string + offset, PACKET_CRC_SIZE);
  offset += PACKET_CRC_HEX_LEN;

  strncpy(packet_string + offset, ";\n", 2);
  printf(packet_string);

  return Transmit(huart, packet_string, MIN_PACKET_HEX_LEN + packet->size);
}


int TransmitControlled(UART_HandleTypeDef* huart, packet_t * packet, packet_t * incoming) {
  //Finite automaton here
  enum states state = STATE_TO_TRANSMIT;
  int res;
  char transmitting = 1;
  char received[MAX_PACKET_HEX_LEN];
  while(transmitting) {
    switch(state) {
      //
      case STATE_TO_TRANSMIT:
        if(PacketEncapsulateAndTransmit(huart, packet) == 0) {
          state = STATE_AWAITING_RESPONSE;
        }
        else {
          printf("Error in transmission\n");
          return -1;
        } 
        break;
      //
      case STATE_AWAITING_RESPONSE:
        res = ReceivePacket(huart, received); 
        if(res == 0) {
          PacketDeencapsulate(received, incoming);
          if((incoming->cmd_type == COMMAND_TYPE_ACK_WRITE || incoming->cmd_type == COMMAND_TYPE_READ) && incoming->address == packet->address) {
            state = STATE_TRANSMITTED;
          }
        }
        else if(res == RECEIVE_TIMEOUT){
          state = STATE_LOST;
          memset(received, 0, MAX_PACKET_HEX_LEN);
        }
        break;
      case STATE_LOST:
        state = STATE_TO_TRANSMIT;
        break;
      case STATE_TRANSMITTED:
        transmitting = 0;
        break;
    }
  }
  return 0;
}

int CommunicationInitMain(UART_HandleTypeDef* huart) {
  //Let a communication init be a write on address 0x00 of 'IN'
  packet_t init_packet, incoming_packet;
  OS_MUTEX_Create(&UART_ACCESS);
  init_packet.address = 0x00;
  init_packet.cmd_type = COMMAND_TYPE_WRITE;
  strncpy(init_packet.data, "IN", 2);
  init_packet.size = 2;
  return TransmitControlled(huart, &init_packet, &incoming_packet);
}

int CommunicationInitSecondary(UART_HandleTypeDef* huart) {
  packet_t incoming, out;
  int res;
  char received[MAX_PACKET_HEX_LEN] = {0};
  OS_MUTEX_Create(&UART_ACCESS);
  ReceivePacket(huart, received);
  PacketDeencapsulate(received, &incoming);
  out.cmd_type = COMMAND_TYPE_ACK_WRITE;
  out.address = 0x00;
  out.size = 0;
  
  printf("%02X\n", incoming.cmd_type);
  printf("%02X\n", incoming.size);
  printf("%02X\n", incoming.address);
  printf("%s\n", incoming.data);

  if(strcmp(incoming.data, "IN") == 0 && incoming.address == 0x00) {
    do {
      res = PacketEncapsulateAndTransmit(huart, &out);
    } while(res != 0);
    return 0;
  }
  return -1;
}