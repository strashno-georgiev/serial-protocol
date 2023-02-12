#include "RTOS.h"
#include "BSP.h"
#include "/home/kaloyangeorgiev/Downloads/SeggerEval/SeggerEval_STM32F769_ST_STM32F769I_Discovery_CortexM_SES_220712/BSP/ST/STM32F769_STM32F769I_Discovery/midlayer.h"
#include "/home/kaloyangeorgiev/Downloads/SeggerEval/SeggerEval_STM32F769_ST_STM32F769I_Discovery_CortexM_SES_220712/BSP/ST/STM32F769_STM32F769I_Discovery/hardware_layer.h"


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

static OS_STACKPTR int StackUARTTx[2048], StackUARTRx[2048];  // Task stacks
static OS_TASK         TCB_UARTTx, TCB_UARTRx;  

static UART_HandleTypeDef USART6_huart, UART5_huart; //2 * 112 bytes
static char message[128];
static char txmsg[120];
static OS_MUTEX Mutex;

static void UART_transmit_task(void) {
  printf("Transmit task\n");
  int i=0;
  if(CommunicationInitMain(&USART6_huart, SINGLE_CONTROLLER_MODE) == 0) {
    printf("Successfully initialized main device\n");
  }
  else {
    printf("Bad initialization of main device\n");
  }
  while(1) {
    OS_TASK_Terminate(NULL);
  }
}

static void UART_receive_task(void) {
  printf("Receive task\n");
  printf("%d\n", HAL_GetTick());
  printf("%d\n", HAL_RCC_GetPCLK1Freq());
  printf("%d\n", HAL_RCC_GetPCLK2Freq());
  if(CommunicationInitSecondary(&UART5_huart) == 0) {
    printf("Successfully initialized secondary device\n");
  }
  else {
    printf("Bad initialization message\n");
  }
  while(1) {

    
    printf("Message is: %s\n", message);

    OS_TASK_Terminate(NULL);
  }
}


void MainTask(void) {
  HAL_Init();

  OS_TASK_EnterRegion();

  init_UART(&USART6_huart, USART6);      //Defined in stm32f769xx.h
  init_UART(&UART5_huart, UART5);

  if(HAL_UART_Init(&USART6_huart) != HAL_OK) {
    printf("Error in USART6 init\n"); 
  }
  if(HAL_UART_Init(&UART5_huart) != HAL_OK) {
    printf("Error in UART5 init\n");
  }
  
  OS_TASK_CREATE(&TCB_UARTRx, "UART Rx task", 90, UART_receive_task, StackUARTRx);
  OS_TASK_CREATE(&TCB_UARTTx, "UART Tx task", 90, UART_transmit_task, StackUARTTx);

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
  BSP_Init();                      /* Initialize LED ports          */
  BSP_SetLED(0);                   /* Initially set LED             */
  /* You need to create at least one task before calling OS_Start() */
  OS_CREATETASK(&TCB0, "MainTask", MainTask, 100, Stack0);
  OS_Start();                      /* Start multitasking            */
  return 0;
}

/****** End Of File *************************************************/
