#include "RTOS.h"
#include "BSP.h"
#include "../BSP/ST/STM32F769_STM32F769I_Discovery/applayer.h"
#define MAIN 1
#define SEC 2

#ifdef __cplusplus
extern "C" {    /* Make sure we have C-declarations in C++ programs */
#endif
void MainTask(void);
#ifdef __cplusplus
}
#endif

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static OS_STACKPTR int Stack0[2048];                  /* Task stack */
static OS_TASK         TCB0;                  /* Task-control-block */

static OS_STACKPTR int StackUARTTx[8192], StackUARTRx[8192], StackHP[256];  // Task stacks
static OS_TASK         TCB_UARTTx, TCB_UARTRx, TCB_HP;  
static int LED_no = -1;

static void HPTask(void) {
  while (1) {
    BSP_ToggleLED(LED_no);
    //printf("HPTask - %d\n", HAL_GetTick());
    OS_TASK_Delay(250);
  }
}

static void UART_PrimaryTask(void) {
  printf("Transmit task\n");
  if(communicationStart(UART5, PRIMARY, MULTI_CONTROLLER_MODE) != 0) {
    printf("Bad initialization of main device\n");
    OS_TASK_Terminate(NULL);
  }
  printf("Successfully initialized main device\n");
  while(1) {
    BSP_ToggleLED(0);
    uint8_t size = 8;
    uint16_t addr = 0x0000;
    for(int i=0; i < 9; i++) {
      write(size, addr);
      read(size, addr);
      addr += size;
      if(i == 0) {
        size = 4;
      }
      else if(i == 2) {
        size = 2;
      }
    }
    OS_TASK_Terminate(NULL);
  }
}

static void UART_SecondaryTask(void) {
  printf("Receive task\n");
  if(communicationStart(UART5, SECONDARY, MULTI_CONTROLLER_MODE) != 0) {
   printf("Sec bad init\n");
   OS_TASK_Terminate(NULL); 
  }
  printf("Successfully initialized secondary device\n");
  while(1) {
    BSP_ToggleLED(1);
    handleCommand();
  }
}


void MainTask(void) {
  OS_TASK_EnterRegion();
  int m = MAIN;

  if(m == MAIN) {
    OS_TASK_CREATE(&TCB_UARTTx, "UART Tx task", 2, UART_PrimaryTask, StackUARTTx);
    LED_no = 0;
  }
  else if(m == SEC) {
    OS_TASK_CREATE(&TCB_UARTRx, "UART Rx task", 2, UART_SecondaryTask, StackUARTRx);
    LED_no = 1;
  }

  OS_TASK_CREATE(&TCB_HP, "led blink", 10, HPTask, StackHP);

  OS_TASK_Terminate(NULL);
}


/*********************************************************************
*
*       main()
*
* Function description
*   Application entry point
*/
int main(void) {
  OS_InitKern();                   /* Initialize OS                 */
  OS_InitHW();                     /* Initialize Hardware for OS    */
  HAL_Init();
  BSP_Init();                      /* Initialize LED ports          */
  BSP_SetLED(0);                   /* Initially set LED             */
  /* You need to create at least one task before calling OS_Start() */
  OS_CREATETASK(&TCB0, "MainTask", MainTask, 100, Stack0);
  OS_Start();                      /* Start multitasking            */
  return 0;
}

/****** End Of File *************************************************/
