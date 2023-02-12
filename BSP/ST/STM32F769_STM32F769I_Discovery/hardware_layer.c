#include "hardware_layer.h"

void init_UART(UART_HandleTypeDef *huart, USART_TypeDef* instance) {
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
}
