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

static OS_STACKPTR int StackHP[256], StackTimer[256];  // Task stacks
static OS_TASK         TCB_HP, TCB_Tim;  
static int LED_no = 0;

static void HPTask(void) {
	while (1) {
		BSP_ToggleLED(LED_no);
		OS_TASK_Delay(250);
	}
}

static void TimerTask(void) {
	uint16_t timer = 0;
	while(1) {
		OS_TASK_Delay(1000);
		timer++;
		safeWrite(&timer, 0, sizeof(timer));

	}
}

void MainTask(void) {
	OS_TASK_EnterRegion();
	OS_TASK_CREATE(&TCB_HP, "led blink", 10, HPTask, StackHP);
	OS_TASK_CREATE(&TCB_Tim, "timer", 10, TimerTask, StackTimer);
	communicationStart(UART5, PRIMARY);
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
	OS_Init();                   /* Initialize OS                 */
	OS_InitHW();                     /* Initialize Hardware for OS    */
	//HAL_Init();
	BSP_Init();                      /* Initialize LED ports          */
	BSP_SetLED(0);                   /* Initially set LED             */
	/* You need to create at least one task before calling OS_Start() */
	OS_TASK_CREATE(&TCB0, "MainTask", 100, MainTask, Stack0);
	OS_Start();                      /* Start multitasking            */
	return 0;
}

/****** End Of File *************************************************/
