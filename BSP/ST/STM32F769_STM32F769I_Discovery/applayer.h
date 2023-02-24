#pragma once
#include <stdint.h>
#include <string.h>
#include "stm32f7xx_hal.h"
#include "RTOS.h"
#define K 1024
extern char writeBuffer[2 * K], readBuffer[2*K];
extern OS_MUTEX writeMutex, readMutex;
enum deviceRole {PRIMARY, SECONDARY};
enum mode {MULTI_CONTROLLER_MODE, SINGLE_CONTROLLER_MODE, UNDEFINED_MODE};

int write(uint8_t size, uint16_t address);
int read(uint8_t size, uint16_t address);
int communicationStart(USART_TypeDef* , enum deviceRole, enum mode);
int handleCommand(void);
void safeCopy(char *dest, char *src, int n, OS_MUTEX *mutex);