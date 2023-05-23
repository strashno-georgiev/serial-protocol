#include "fault_injection.h"
#include "stm32f7xx_hal.h"

uint8_t FAULT = 0;

void initFIF(void) {
	GPIO_InitTypeDef btn_pin;
	btn_pin.Pin = GPIO_PIN_0;
    btn_pin.Mode = GPIO_MODE_INPUT;
    btn_pin.Pull = GPIO_PULLDOWN;
    btn_pin.Speed = GPIO_SPEED_FREQ_HIGH;

	__HAL_RCC_GPIOA_CLK_ENABLE();
    
	HAL_GPIO_Init(GPIOA, &btn_pin);
}


uint8_t check_fault(void) {
	return FAULT;
}

