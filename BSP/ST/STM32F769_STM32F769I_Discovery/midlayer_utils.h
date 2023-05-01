#pragma once

packet_t INIT_PACKET = {INIT_PACKET_ADDRESS, 0, COMMAND_TYPE_WRITE, INIT_PACKET_SIZE, 0, INIT_PACKET_DATA};
packet_t BAD_CRC_PACKET = {BAD_CRC_PACKET_ADDRESS, 0, COMMAND_TYPE_WRITE, BAD_CRC_PACKET_SIZE, 0, BAD_CRC_PACKET_DATA};
packet_t END_PACKET = {END_PACKET_ADDRESS, 0, COMMAND_TYPE_WRITE, END_PACKET_SIZE, 0, END_PACKET_DATA};

void isxcpy(int num, char* str, uint8_t numsize);
int strnxtoi(char* str, int n);
void PacketDeencapsulate(char *str, packet_t * p);
uint8_t CRC_f(char* data, int len);
void PacketEncapsulateCRC(packet_t *packet, char *str);
int comparePackets(packet_t *p1, packet_t *p2);