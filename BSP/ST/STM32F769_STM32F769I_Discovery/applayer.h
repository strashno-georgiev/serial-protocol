#pragma once
#include <stdint.h>
#include <string.h>
#include "stm32f7xx_hal.h"
#define K 1024
extern char writeBuffer[2 * K], readBuffer[2*K];
extern UART_HandleTypeDef *HUART;

enum deviceRole {PRIMARY, SECONDARY};

int write(uint8_t size, uint16_t address);
int read(uint8_t size, uint16_t address);
int communicationStart(UART_HandleTypeDef* huart, enum deviceRole);