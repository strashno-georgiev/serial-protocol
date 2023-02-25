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

static OS_STACKPTR int StackHP[256];  // Task stacks
static OS_TASK         TCB_HP;  
static int LED_no = 1;

static void HPTask(void) {
	while (1) {
		//printf("hi\n");
		BSP_ToggleLED(LED_no);
		OS_TASK_Delay(250);
	}
}

void MainTask(void) {
	//OS_TASK_EnterRegion();
	printf("Waiting for connection initialization\n");
	fflush(stdout);
	OS_TASK_CREATE(&TCB_HP, "led blink", 10, HPTask, StackHP);
	communicationStart(UART5, SECONDARY, MULTI_CONTROLLER_MODE);

	safeWrite("ab", 0x0000, 2);

	
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
  printf("Waiting for connection initialization\n");
  OS_CREATETASK(&TCB0, "MainTask", MainTask, 100, Stack0);
  OS_Start();                      /* Start multitasking            */
  printf("return from main\n");
  return 0;
}

/****** End Of File *************************************************/
