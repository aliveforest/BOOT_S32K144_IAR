#include "latency.h"
#include "system_S32K144.h"
#include "device_registers.h"

static uint32_t fac_us = 80000000u / 1000000u; // SYS_CLK内核时钟为80MHz


void systick_later_init(void) {
	S32_SysTick->CSR = S32_SysTick_CSR_ENABLE(0);   /**< SysTick 控制和状态寄存器，偏移量：0x0 */
	S32_SysTick->RVR = 0xFFFFFFul; //重装载寄存器 /**< SysTick 重载值寄存器，偏移量：0x4 */
	S32_SysTick->CVR = 0ul; //当前计数  /**< SysTick 当前值寄存器，偏移量：0x8 */
	S32_SysTick->CSR = 0u; //控制寄存器 /**< SysTick 控制和状态寄存器，偏移量：0x0 */
	S32_SysTick->CSR |= S32_SysTick_CSR_ENABLE(1)|S32_SysTick_CSR_CLKSOURCE(1u); // 开启时钟，选择时钟源为 1->内核时钟;(0->内核时钟/16)
}

void later_100_ns(volatile uint32_t n_100ns)
{
	uint32_t told  = S32_SysTick->CVR ;      	//刚进入时的计数器值
	uint32_t ticks;
	uint32_t tcnt=0;
	// uint32_t reload=S32_SysTick->RVR;			//LOAD的值
	ticks = (uint32_t)(n_100ns*8u); 		//需要的节拍数 (四舍五入)

	while(tcnt < ticks) {  /*求减过程中，如果发生第一次32位计数器重新计数，依然可以正确计算*/
		tcnt = told-S32_SysTick->CVR;
	}
	/*两个32位无符号数相减，获取的结果再赋值给32位无符号数依然可以正确的获取差值。假如A,B,C都是32位无符号数。
	如果A>B那么A - B= c，这个很好理解，完全没有问题
	如果A<B那么A -B = C，c的数值就是OxFFFFFFFF -B+A ＋1。这一点要特别注意，正好用于本函数.
	*/
}


void later_1_us(void){
	uint32_t told = S32_SysTick->CVR ;      	//刚进入时的计数器值
	uint32_t tcnt=0;
	while(tcnt < 80u) {  /*求减过程中，如果发生第一次32位计数器重新计数，依然可以正确计算*/
		tcnt = told-S32_SysTick->CVR;
	}
}

void later_us(volatile uint32_t nus)
{
	uint32_t told  = S32_SysTick->CVR ;      	//刚进入时的计数器值
	uint32_t ticks;
	uint32_t tcnt=0;
	// uint32_t reload=S32_SysTick->RVR;			//LOAD的值
	ticks = nus*fac_us; 		//需要的节拍数 (四舍五入)

	while(tcnt < ticks) {  /*求减过程中，如果发生第一次32位计数器重新计数，依然可以正确计算*/
		tcnt = told-S32_SysTick->CVR;
	}
	/*两个32位无符号数相减，获取的结果再赋值给32位无符号数依然可以正确的获取差值。假如A,B,C都是32位无符号数。
	如果A>B那么A - B= c，这个很好理解，完全没有问题
	如果A<B那么A -B = C，c的数值就是OxFFFFFFFF -B+A ＋1。这一点要特别注意，正好用于本函数.
	*/
}

void later_ms(volatile uint32_t nms) {
	uint32_t i;
	for(i=0;i<nms;i++) later_us(1000);
}

// S32_SysTick_CSR_TICKINT此位设为1一定要建立中断服务函数
// 中断服务函数
// void SysTick_Handler(void) {
 
// }
/*******参考*******
 * S32K144（11）systick_s32k144延时函数_Suifqwu的博客-CSDN博客
https://blog.csdn.net/qq_41399894/article/details/113741547
 * STM32延时函数的四种方法：普通延时（2种）、SysTick?定时器延时（2种）_延时函数的几种表达方式
https://blog.csdn.net/weibo1230123/article/details/81136564

*******************/

// void delay_us(uint32_t nus) {
// 	uint32_t ticks=nus*fac_us; 				          // 计算需要的节拍数
// 	S32_SysTick->CSR = S32_SysTick_CSR_ENABLE(0);     // 失能
// 	S32_SysTick->CSR = S32_SysTick_CSR_CLKSOURCE(1u); // 选择时钟源 0：外部时钟源 1：内部时钟
// 	S32_SysTick->RVR = ticks; 						  // 重装载寄存器 当倒数至零时，将被重装载的值
// 	S32_SysTick->CSR = S32_SysTick_CSR_ENABLE(1);     // 使能
// 	do{
// 		ticks = S32_SysTick->CVR ;      		  	  // 读取当前的计数器值
// 		//（计数器是开的）并且（没（到约定的计数个数））＝继续循环
// 	}while((ticks & 0x01)&&!(ticks & (1<<16)));
// 	S32_SysTick->CSR = 0x00;      	 				  //失能 关闭计数器
// 	S32_SysTick->CVR = 0X00;       					  //清空计数器
// }
