/* ###################################################################
**     Filename    : main.c
**     Processor   : S32K1xx
**     Abstract    :
**         Main module.
**         This module contains user's application code.
**     Settings    :
**     Contents    :
**         No public methods
**
** ###################################################################*/
/*!
** @file main.c
** @version 01.00
** @brief
**         Main module.
**         This module contains user's application code.
*/         
/*!
**  @addtogroup main_module main module documentation
**  @{
*/         
/* MODULE main */


/* Including necessary module. Cpu.h contains other modules needed for compiling.*/
#include "Cpu.h"
#include "stdlib.h"
#include "LPUART.h"
#include "RGB_LED.h"
#include "latency.h"
#include "FlashWriteRead.h"
#include "Ymodem.h"

volatile int exit_code = 0;

#define APP_START_ADDRESS 0x00009000ul // app 初始化地址

/* 设置栈顶地址  addr:栈顶地址 */
//__asm void MSR_MSP(uint32_t addr) {
//    MSR MSP, r0 ;	//set Main Stack value
//    BX r14;
//}

/* Bootloader to App  */
void Boot_to_App(uint32_t appEntry, uint32_t appstack) {
	static void (*jump_to_application)(void);
	static uint32_t stack_pointer;
	jump_to_application = (void (*)(void))appEntry; /*函数指针指向app的复位向量表的地址。注意将地址强转成函数入口地址 */
	stack_pointer = appstack;
	S32_SCB->VTOR = (uint32_t)APP_START_ADDRESS; // 设置中断向量
	INT_SYS_DisableIRQGlobal();					 // 关闭全局中断
//	MSR_MSP(stack_pointer);  // 设置堆栈指针
//    asm(" LDR R4,  = stack_pointer \n"
//        " LDR r5,  [r4] \n"
//        " msr msp, r5             "); // 设置堆栈指针

	jump_to_application(); // 进行跳转
}


/* 1 字 = 4 字节 (1 word = 4 bytes) 32位
 * 1字节 =8 比特 (1 byte = 8bits)
 * 1 Byte = 8 Bits
 * 1 KB = 1024 Bytes
 * 1 MB = 1024 KB
 * 1 GB = 1024 MB */

/*!  \brief The main function for the project.
  \details The startup initialization sequence is the following:
 * - startup asm routine - main() */
int main(void)
{
  /* Write your local variable definition here */
	uint32_t appEntry, appstack;
//	uint8_t data_buff[Flash_Erase_Size];
//	uint8_t read_buff[2048]={0};
//	uint32_t index;
//	for(index=0;index < Flash_Erase_Size;index++){
//		data_buff[index] = index % 256;
//	}
  /*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/
  #ifdef PEX_RTOS_INIT
    PEX_RTOS_INIT();                   /* Initialization of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of Processor Expert internal initialization.                    ***/
	CLOCK_DRV_Init(&clockMan1_InitConfig0);             /* Initialize clock     */
	systick_later_init();                               /* Initialize later fcn */
	LPUART1_init();                                     /* Initialize LPUART1   */
	RGB_LED_KEY_init();                                 /* Initialize LED KEY   */
	FLASH_DRV_Init(&Flash1_InitConfig0, &flashConfig);  /* Initialize flash     */
  /* Write your code here */
    /* 从给定的内存地址中读取一32位的无符号整数，并将其存储在变量appstack */
	appstack = *(uint32_t *)(APP_START_ADDRESS);
	 /* 偏移4位，取出PC指针(Program Counter)的初始�??--复位向量 */
    appEntry = *(volatile uint32_t *)(APP_START_ADDRESS + 4);

    for(;;) {
		LPUART1_printf("----- BootLoader -----\r\n");
		later_ms(600);
		if(SW2_key()){
			LPUART1_printf("KEY2 press!\r\n");
//			Flash_Read_Data(Flash_Start_Address,2048, read_buff);
			LPUART1_printf("	Enter APP...\r\n");
			later_ms(500);
			Boot_to_App(appEntry, appstack);
		}
		else if(SW3_key()){
			LPUART1_printf("KEY3 press!\r\n");
			LPUART1_printf("Waiting for receive...\r\n");
			if(Ymodem_Receive_File(Flash_Start_Address, NAK_TIMEOUT)==RE_OK){
				LPUART1_printf("	Enter APP...\r\n");
				later_ms(1000);
				Boot_to_App(appEntry, appstack);
			}else
				LPUART1_printf("Failed to Receive!\r\n");
		}
    }


  /*** Don't write any code pass this line, or it will be deleted during code generation. ***/
  /*** RTOS startup code. Macro PEX_RTOS_START is defined by the RTOS component. DON'T MODIFY THIS CODE!!! ***/
  #ifdef PEX_RTOS_START
    PEX_RTOS_START();                  /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of RTOS startup code.  ***/
  /*** Processor Expert end of main routine. DON'T MODIFY THIS CODE!!! ***/
  for(;;) {
    if(exit_code != 0) {
      break;
    }
  }
  return exit_code;
  /*** Processor Expert end of main routine. DON'T WRITE CODE BELOW!!! ***/
} /*** End of main routine. DO NOT MODIFY THIS TEXT!!! ***/

/* END main */
/*!
** @}
*/
/*
** ###################################################################
**
**     This file was created by Processor Expert 10.1 [05.21]
**     for the NXP S32K series of microcontrollers.
**
** ###################################################################
*/
