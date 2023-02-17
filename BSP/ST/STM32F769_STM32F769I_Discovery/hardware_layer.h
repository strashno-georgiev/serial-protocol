#pragma once
#include "stm32f7xx_hal.h"
#include "protocol_data.h"
#include "RTOS.h"
extern UART_HandleTypeDef USART6_huart, UART5_huart;
extern char RECEIVED_DATA[(MAX_PACKET_HEX_LEN) * 3];
extern OS_MAILBOX receivedMailBox;

void init_UART(UART_HandleTypeDef *huart, USART_TypeDef* instance);
int initHardwareLayer(UART_HandleTypeDef *huart, USART_TypeDef *instance);