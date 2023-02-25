#include "applayer.h"

void UART_PrimaryTask(void) {
  printf("Transmit task\n");
  printf("Successfully initialized main device\n");
  while(1) {
    //BSP_ToggleLED(0);
    uint8_t size = 8;
    uint16_t addr = 0x0000;
    for(int i=0; i < 9; i++) {
      write(size, addr);
      OS_TASK_Delay(250);
      read(size, addr);
      addr += size;
      if(i == 0) {
        size = 4;
      }
      else if(i == 2) {
        size = 2;
      }
    }
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