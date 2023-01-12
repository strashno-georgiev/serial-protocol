/*********************************************************************
*                     SEGGER Microcontroller GmbH                    *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 2003 - 2022  SEGGER Microcontroller GmbH                 *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------
File    : OS_StartLEDBlink.c
Purpose : embOS sample program running two simple tasks, each toggling
          a LED of the target hardware (as configured in BSP.c).
*/

#include "RTOS.h"
#include "BSP.h"
#include "stm32f7xx_hal.h"
//#include "stm32f7xx_hal_uart.h"


static OS_STACKPTR int StackHP[128], StackLP[128];  // Task stacks
static OS_TASK         TCBHP, TCBLP;                // Task control blocks

static OS_STACKPTR int StackUARTTx[256], StackUARTRx[256];  // Task stacks
static OS_TASK         TCB_UARTTx, TCB_UARTRx;  

static UART_HandleTypeDef USART6_huart, UART5_huart; //2 * 112 bytes
static char message[128];
static char txmsg[16];

static void init_UART(UART_HandleTypeDef *huart, USART_TypeDef* instance);

static void HPTask(void) {
  while (1) {
    BSP_ToggleLED(0);
    printf("HPTask - %d\n", HAL_GetTick());
    OS_TASK_Delay(1000);
  }
}

static void LPTask(void) {
  while (1) {
    BSP_ToggleLED(1);
    printf("LPTask - %d\n", HAL_GetTick());
    OS_TASK_Delay(200);
    //OS_TASK_Terminate(NULL);
  }
}

static void UART_transmit_task(void) {
  printf("Transmit task\n");
  
  while(1) {
    strcpy(txmsg, "Hello");
    printf("Transmit starting\n");
    printf("%d\n", HAL_GetTick());
    HAL_UART_Transmit(&USART6_huart, (uint8_t*)txmsg, 16, 32);  //blocks here
    printf("Transmit done\n");
    printf("%d\n", HAL_GetTick());

    OS_TASK_Terminate(NULL);
  }
}

static void UART_receive_task(void) {
  printf("Receive task\n");
  printf("%d\n", HAL_GetTick());
  printf("%d\n", HAL_RCC_GetPCLK1Freq());
  printf("%d\n", HAL_RCC_GetPCLK2Freq());
  while(1) {
    HAL_UART_Receive(&UART5_huart, (uint8_t*)message, 128, 32); //blocks here
    printf("Message is: %s", message);

    OS_TASK_Terminate(NULL);
  }
}
/*********************************************************************
*
*       MainTask()
*/
#ifdef __cplusplus
extern "C" {     // Make sure we have C-declarations in C++ programs
#endif
void MainTask(void);
#ifdef __cplusplus
}
#endif
void MainTask(void) {
  HAL_Init();
  OS_TASK_EnterRegion();
  init_UART(&USART6_huart, USART6);      //Defined in stm32f769xx.h
  init_UART(&UART5_huart, UART5);

  HAL_UART_MspInit(&UART5_huart);
  HAL_UART_MspInit(&USART6_huart);

  HAL_UART_Init(&USART6_huart);
  HAL_UART_Init(&UART5_huart);

  OS_TASK_CREATE(&TCBHP, "HP Task", 100, HPTask, StackHP);
  OS_TASK_CREATE(&TCBLP, "LP Task",  50, LPTask, StackLP);

  OS_TASK_CREATE(&TCB_UARTRx, "UART Rx task", 90, UART_receive_task, StackUARTRx);
  OS_TASK_CREATE(&TCB_UARTTx, "UART Tx task", 90, UART_transmit_task, StackUARTTx);

  OS_TASK_Terminate(NULL);
}

static void init_UART(UART_HandleTypeDef *huart, USART_TypeDef* instance) {
  UART_InitTypeDef uart_init;
  
  uart_init.BaudRate = 1250000;               // 1 250 000
  uart_init.WordLength = UART_WORDLENGTH_8B;  //Defined in uart_ex.h
  uart_init.StopBits = UART_STOPBITS_1;       //uart.h
  uart_init.Parity = UART_PARITY_NONE;
  uart_init.Mode = UART_MODE_TX_RX;
  uart_init.HwFlowCtl = UART_HWCONTROL_NONE;
  uart_init.OverSampling = UART_OVERSAMPLING_8;
  uart_init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;

  huart->Instance = instance;
  huart->Init = uart_init;
}

void HAL_UART_MspInit(UART_HandleTypeDef* huart) {
  if(huart->Instance == UART5) {
    //ENABLE PD2-RX and PC12-TX
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_UART5_CLK_ENABLE();
    
    GPIO_InitTypeDef d2init, c12init;
    GPIO_TypeDef d2, c12;

    d2init.Pin = GPIO_PIN_2;
    d2init.Mode = GPIO_MODE_AF_PP;
    d2init.Pull = GPIO_PULLUP;
    d2init.Speed = GPIO_SPEED_FREQ_HIGH;
    d2init.Alternate = GPIO_AF1_UART5;

    c12init.Pin = GPIO_PIN_12;
    c12init.Mode = GPIO_MODE_AF_PP;
    c12init.Pull = GPIO_PULLUP;
    c12init.Speed = GPIO_SPEED_FREQ_HIGH;
    c12init.Alternate = GPIO_AF1_UART5;

    HAL_GPIO_Init(&d2, &d2init);
    HAL_GPIO_Init(&c12, &c12init);
  }
  if(huart->Instance == USART6) {
    //Enable PORT C 6 and 7
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_USART6_CLK_ENABLE();

    GPIO_InitTypeDef c6init, c7init;
    GPIO_TypeDef c6, c7;

    c6init.Pin = GPIO_PIN_6;
    c6init.Mode = GPIO_MODE_AF_PP;
    c6init.Pull = GPIO_PULLUP;
    c6init.Speed = GPIO_SPEED_FREQ_HIGH;
    c6init.Alternate = GPIO_AF8_USART6;

    c7init.Pin = GPIO_PIN_7;
    c7init.Mode = GPIO_MODE_AF_PP;
    c7init.Pull = GPIO_PULLUP;
    c7init.Speed = GPIO_SPEED_FREQ_HIGH;
    c7init.Alternate = GPIO_AF8_USART6;

    HAL_GPIO_Init(&c6, &c6init);
    HAL_GPIO_Init(&c7, &c7init);
  }
}

/*************************** End of file ****************************/
