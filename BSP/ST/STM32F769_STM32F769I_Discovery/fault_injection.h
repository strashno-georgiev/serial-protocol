#define FAULT_HH

#ifndef FAULT_HH
#include <stdint.h>

const uint8_t FAULT_CRC = 1;
const uint8_t NO_FAULT = 0;
extern uint8_t FAULT;


void initFIF(void);
uint8_t check_fault(void);

#endif