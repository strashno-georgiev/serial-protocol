#include "hardware_layer.h"

char RECEIVED_DATA[(MAX_PACKET_HEX_LEN) * 3];
char C;
int index = 0;
UART_HandleTypeDef *huart_used;
OS_MAILBOX receivedMailBox;

void UART_IRQHandler(void) {
  huart_used->Instance->ISR &= (~UART_FLAG_ORE);
  if((huart_used->Instance->ISR & UART_FLAG_RXNE) == UART_FLAG_RXNE) {
    C = huart_used->Instance->RDR & 0xFF;
    OS_MAILBOX_Put1(&receivedMailBox, &C);
  }
  return;
}

void UART5_IRQHandler(void) {
  UART_IRQHandler();
}

void USART6_IRQHandler(void) {
  UART_IRQHandler();
}

void UART_InterruptEnable_RXNE(UART_HandleTypeDef* huart) {
  //HAL_UART_Receive_IT(&huart, RECEIVED, 200);
  if(huart->Instance == UART5) {
    HAL_NVIC_SetPriority(UART5_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(UART5_IRQn);
  }
  else if(huart->Instance == USART6) {
    HAL_NVIC_SetPriority(USART6_IRQn, 0, 1);
    HAL_NVIC_EnableIRQ(USART6_IRQn); 
  }
  HAL_UART_Receive_IT(huart_used, &C, 1);
  __HAL_UART_ENABLE_IT(huart, UART_IT_RXNE);
}



void init_UART(UART_HandleTypeDef *huart, USART_TypeDef* instance) {
  UART_InitTypeDef uart_init;
  
  uart_init.BaudRate = 19200;      
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
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  if(huart->Instance == UART5) {

    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_UART5;
    PeriphClkInitStruct.Uart5ClockSelection = RCC_UART5CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      printf("Error in peripheral clock selection\n");
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
    d2init.Alternate = GPIO_AF8_UART5; //From alternate function mapping in Datasheet

    c12init.Pin = GPIO_PIN_12;
    c12init.Mode = GPIO_MODE_AF_PP;
    c12init.Pull = GPIO_PULLUP;
    c12init.Speed = GPIO_SPEED_FREQ_HIGH;
    c12init.Alternate = GPIO_AF8_UART5;

    HAL_GPIO_Init(GPIOD, &d2init);
    HAL_GPIO_Init(GPIOC, &c12init);
  }
  if(huart->Instance == USART6) {
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART6;
    PeriphClkInitStruct.Uart5ClockSelection = RCC_USART6CLKSOURCE_PCLK2;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      printf("Error in peripheral clock selection\n");
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
  UART_InterruptEnable_RXNE(huart);
}

int initHardwareLayer(UART_HandleTypeDef *huart, USART_TypeDef *instance) {
  HAL_Init();
  OS_MAILBOX_Create(&receivedMailBox, 1, MAX_PACKET_HEX_LEN * 3, RECEIVED_DATA);
  init_UART(huart, instance);
  if(HAL_UART_Init(huart) != HAL_OK) {
    printf("Error in initializing UART handle\n");
    return -1;
  }
  huart_used = huart;
  return 0;
}