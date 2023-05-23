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
#include "midlayer_utils.h"
#include "fault_injection.h"
#define TIMEOUT 250


byte_t ID = 0x00;

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
  if(check_fault()) {
	packet->crc++;
  }
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
    res = ReceivePacketTimed(incoming, TIMEOUT);
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
  enum main_state MAIN_STATE = STATE_TRANSMITTING_COMMAND;
  while(MAIN_STATE != STATE_MAIN_DONE) {
    switch(MAIN_STATE) {
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

// ---------------------------------------------
// Ensures the whole cycle of receiving a command and returns when a response is received
// Gets called directly by the application layer for transmission by the main (primary) device
int TransmitCommandControlled(uint8_t cmd_type, uint8_t size, uint16_t address, char *str, packet_t* response) {
  packet_t p;
  p.address = address;
  p.size = size;
  p.cmd_type = cmd_type;
  memset(p.data, 0, MAX_PACKET_DATA_SIZE+1);
  if(cmd_type == COMMAND_TYPE_WRITE) {
    memcpy(p.data, str, size);
  }
  p.data[size] = 0;
  return MainControlled(&p, response);
}

// ------------------------------------------
// Handles the acknowledgement (transmission) cycle of the secondary device
// Gets called directly by the application layer
int SecondaryAcknowledge(uint8_t ack_type, uint8_t size, uint16_t address, char *str) {
  packet_t p;
  p.address = address;
  p.size = size;
  p.cmd_type = ack_type;
  memset(p.data, 0, MAX_PACKET_DATA_SIZE+1);
  memcpy(p.data, str, size);
  return TransmitPacket(&p);
}

// ------------------------------------------
// Handles the receive cycle of the secondary device
// Gets called directly by the application layer
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


int CommunicationInitMain() {
  //Let a communication init be a write on address 0x00 of 'IN'
  packet_t incoming_packet;
  return MainControlled(&INIT_PACKET, &incoming_packet);
}

int CommunicationInitSecondary() {
  enum special_packet flag = NOT_SPECIAL;
  packet_t inc;

  SecondaryReceive(&inc, &flag);
  SecondaryAcknowledge(COMMAND_TYPE_ACK_WRITE, 0, 0, "");
  
  if(flag != INIT) {
    return -1;
  }
  return 0;
}

int CommunicationEndMain(packet_t * res) {
  return MainControlled(&END_PACKET, res);
}

int initMidLayer(UART_HandleTypeDef *huart, USART_TypeDef *instance, enum deviceRole role) {
  initHardwareLayer(huart, instance);
  if(role == PRIMARY) {
    return CommunicationInitMain();
  }
  else if(role == SECONDARY) {
    return CommunicationInitSecondary();
  }
  return -1;
}