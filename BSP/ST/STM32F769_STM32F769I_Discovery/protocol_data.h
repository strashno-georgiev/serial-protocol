#pragma once


//Size of packet headers in bytes
#define PACKET_ID_SIZE 1
#define PACKET_CMDTP_SIZE 1
#define PACKET_SIZE_SIZE 1
#define PACKET_ADDRESS_SIZE 2
#define MAX_PACKET_DATA_SIZE 255
#define PACKET_CRC_SIZE 1

#define MIN_PACKET_SIZE (PACKET_ID_SIZE + PACKET_CMDTP_SIZE + PACKET_SIZE_SIZE + PACKET_ADDRESS_SIZE + PACKET_CRC_SIZE + 1)

#define MAX_PACKET_SIZE (PACKET_ID_SIZE + PACKET_CMDTP_SIZE + PACKET_SIZE_SIZE + PACKET_ADDRESS_SIZE + MAX_PACKET_DATA_SIZE + PACKET_CRC_SIZE)

//Length of packet headers as a hex number in a string
#define PACKET_ID_HEX_LEN (PACKET_ID_SIZE*2)
#define PACKET_CMDTP_HEX_LEN (PACKET_CMDTP_SIZE*2)
#define PACKET_SIZE_HEX_LEN (PACKET_SIZE_SIZE*2)
#define PACKET_ADDRESS_HEX_LEN (PACKET_ADDRESS_SIZE*2)
#define MAX_PACKET_DATA_HEX_LEN (MAX_PACKET_DATA_SIZE)
#define PACKET_CRC_HEX_LEN (PACKET_CRC_SIZE*2)

#define MIN_PACKET_HEX_LEN ((MIN_PACKET_SIZE) * 2)
#define MAX_PACKET_HEX_LEN (MIN_PACKET_HEX_LEN + MAX_PACKET_DATA_SIZE)

//Constant values
#define COMMAND_TYPE_WRITE 0x0E
#define COMMAND_TYPE_ACK_WRITE 0x0A
#define COMMAND_TYPE_READ 0x0D