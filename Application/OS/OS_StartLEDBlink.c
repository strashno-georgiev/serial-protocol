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
static char txmsg[120];

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
    OS_TASK_Delay(500);
    //OS_TASK_Terminate(NULL);
  }
}

static void UART_transmit_task(void) {
  printf("Transmit task\n");
  
  while(1) {
    strcpy(txmsg, "Hello, PC, I'm MCU\n");
    printf("Transmit starting\n");
    printf("%d\n", HAL_GetTick());
    HAL_UART_Transmit(&USART6_huart, (uint8_t*)txmsg, strlen(txmsg)+1, 32);  //blocks here
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
    while(HAL_UART_Receive(&USART6_huart, (uint8_t*)message, 16, 32) != HAL_OK); //blocks here
    printf("Message is: %s\n", message);

    OS_TASK_Terminate(NULL);
  }
}

//static void Init_GPIO_J1(GPIO_TypeDef*);

void SystemClock_Config(void);
static void Init_GPIO_J1(void);
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
  //SystemClock_Config();
  OS_TASK_EnterRegion();

  init_UART(&USART6_huart, USART6);      //Defined in stm32f769xx.h
  init_UART(&UART5_huart, UART5);
  Init_GPIO_J1();

  HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_1, GPIO_PIN_SET);

  //HAL_UART_MspInit(&UART5_huart);
  //HAL_UART_MspInit(&USART6_huart);

  if(HAL_UART_Init(&USART6_huart) != HAL_OK) {
    printf("Error in USART6 init\n"); 
  }
  if(HAL_UART_Init(&UART5_huart) != HAL_OK) {
    printf("Error in UART5 init\n");
  }

  //OS_TASK_CREATE(&TCBHP, "HP Task", 100, HPTask, StackHP);
  //OS_TASK_CREATE(&TCBLP, "LP Task",  50, LPTask, StackLP);

  OS_TASK_CREATE(&TCB_UARTRx, "UART Rx task", 90, UART_receive_task, StackUARTRx);
  OS_TASK_CREATE(&TCB_UARTTx, "UART Tx task", 90, UART_transmit_task, StackUARTTx);

  OS_TASK_Terminate(NULL);
}

static void init_UART(UART_HandleTypeDef *huart, USART_TypeDef* instance) {
  UART_InitTypeDef uart_init;
  
  uart_init.BaudRate = 19200;               // 1 250 000
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

static void Init_GPIO_J1() {
    GPIO_InitTypeDef j1init;
    __HAL_RCC_GPIOJ_CLK_ENABLE();

    j1init.Pin = GPIO_PIN_1;
    j1init.Mode = GPIO_MODE_OUTPUT_PP;
    j1init.Pull = GPIO_NOPULL;
    j1init.Speed = GPIO_SPEED_FREQ_HIGH;
  
    HAL_GPIO_Init(GPIOJ, &j1init);
}

void HAL_UART_MspInit(UART_HandleTypeDef* huart) {
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  if(huart->Instance == UART5) {

    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_UART5;
    PeriphClkInitStruct.Uart5ClockSelection = RCC_UART5CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      printf("Error in peripheral clock init\n");
      //Error_Handler();
    }
    //ENABLE PD2-RX and PC12-TX
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_UART5_CLK_ENABLE();
    
    GPIO_InitTypeDef d2init, c12init;

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

    HAL_GPIO_Init(GPIOD, &d2init);
    HAL_GPIO_Init(GPIOC, &c12init);
  }
  if(huart->Instance == USART6) {
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART6;
    PeriphClkInitStruct.Uart5ClockSelection = RCC_USART6CLKSOURCE_PCLK2;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      printf("Error in peripheral clock init\n");
      //Error_Handler();
    }
    //Enable PORT C 6 and 7
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_USART6_CLK_ENABLE();

    GPIO_InitTypeDef c67init;

    c67init.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    c67init.Mode = GPIO_MODE_AF_PP;
    c67init.Pull = GPIO_PULLUP;
    c67init.Speed = GPIO_SPEED_FREQ_HIGH;
    c67init.Alternate = GPIO_AF8_USART6;

    HAL_GPIO_Init(GPIOC, &c67init);
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 216;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    printf("Error in oscillator config\n");
    //Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    printf("Error in overdrive config\n");
    //Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
  {
    printf("Error in clock config\n");
    //Error_Handler();
  }
}

/*************************** End of file ****************************/