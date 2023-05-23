#ifndef HARDWARE_LAYER_HH
#define HARDWARE_LAYER_HH

#include "stm32f7xx_hal.h"
#include "../LayerMiddle/protocol_data.h"
#include "RTOS.h"
#define DEBUG_PRINT 1

extern char RECEIVED_DATA[(MAX_PACKET_HEX_LEN) * 3];
extern OS_MAILBOX receivedMailBox;

void init_UART(UART_HandleTypeDef *huart, USART_TypeDef* instance);
int initHardwareLayer(UART_HandleTypeDef *huart, USART_TypeDef *instance);

int Transmit(char* str, int len);
int Receive(char* str);
int ReceiveTimed(char* str, OS_TIME timeout);

#endif