#include "applayer.h"

void UART_PrimaryTask(void) {
  printf("Transmit task\n");
  printf("Successfully initialized main device\n");
  while(1) {
	OS_TASK_Delay(250);
	write(sizeof(uint16_t), 0);
  }
  OS_TASK_Terminate(NULL);
}

void UART_SecondaryTask(void) {
  printf("Receive task\n");
  printf("Successfully initialized secondary device\n");
  while(1) {
    //BSP_ToggleLED(1);
    handleCommand();
  }
}