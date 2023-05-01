#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "midlayer.h"
#define BYTE_SIZE 8

//CRC-8 DALLAS/MAXIM x^8 + x^5 + x^4 + 1 (NOT USED)
//CRC-8-CCIT ITU-T x^8 + x^2 + x + 1 (MSB Dicarded) = 0x07
const uint8_t GENERATOR_POLYNOMIAL = 0x07; 
const uint8_t XOR_OUT = 0x55;

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
          p->data[i] = strnxtoi(str + offset + i*2, sizeof(uint8_t) * 2);
        }
        offset += p->size * 2;
      }
    }

    p->crc = strnxtoi(str + offset, PACKET_CRC_HEX_LEN);
}

uint8_t CRC_f(char* data, int len) { 
  uint8_t crc8 = data[0];
  uint8_t shift_counter = 0;
  char flag = 0; 
  for(int i=0; i < len;) {
    if(!!(crc8 & (1 << (BYTE_SIZE-1)))) {
      flag = 1;
    }
	//Discarding the first bit
    crc8 = crc8 << 1;
	//Adding the (BYTE_SIZE-shift_counter)th bit from the next octet
    crc8 |= (i+1 != len ? ((data[i+1] >> (BYTE_SIZE - shift_counter - 1)) & (uint8_t)1) : 0);
    shift_counter++;
    //If the discarded bit was set, XOR the polynomial
	if(flag) {
      crc8 = crc8 ^ GENERATOR_POLYNOMIAL;
      flag = 0;
    }
    if(shift_counter == 8) {
      shift_counter = 0;
      i++;
    }
  }
  return crc8 ^ XOR_OUT;
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
        isxcpy(packet->data[i], str + offset + i*2, sizeof(uint8_t));
      }
      offset += packet->size * 2;
    }
  }

  isxcpy(CRC_f(str, offset), str + offset, PACKET_CRC_SIZE);
  offset += PACKET_CRC_HEX_LEN;

  memcpy(str + offset, ";\n", 2);
}

int comparePackets(packet_t *p1, packet_t *p2) {
  return ((p1->address == p2->address) && (p1->size == p2->size) && !strcmp(p1->data, p2->data));
}