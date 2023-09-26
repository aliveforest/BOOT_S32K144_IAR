/*

 * RGB_LED.c
 *
 *  Created on: 2023年8月1日
 *      Author: dengtongbei
 */
#include "RGB_LED.h"
#include "S32K144.h"


// RGB_LED初始化
void RGB_LED_KEY_init(void){
	PCC-> PCCn[PCC_PORTD_INDEX] |= PCC_PCCn_CGC_MASK;
	PCC-> PCCn[PCC_PORTC_INDEX] |= PCC_PCCn_CGC_MASK;/* Enable clock for PORTC */

	/* Configure port C12 as GPIO input (BTN 0 [SW2] on EVB) */
	PTC->PDDR &= ~(1<<SW2);   /* Port C12: Data Direction= input (default) */
	PORTC->PCR[SW2] = PORT_PCR_MUX(1) |PORT_PCR_PFE_MASK;// 输入过滤器已启用 /* Port C12: MUX = GPIO, input filter enabled */
	
	/* Configure port C13 as GPIO input (BTN 1 [SW3] on EVB) */
	PTC->PDDR &= ~(1<<SW3);   /* Port C12: Data Direction= input (default) */
	PORTC->PCR[SW3] = PORT_PCR_MUX(1) |PORT_PCR_PFE_MASK;// 输入过滤器已启用 /* Port C12: MUX = GPIO, input filter enabled */

	/* Configure port D0 as GPIO output (LED on EVB) */
	PTD->PDDR |= 1<<BlueLED;       /* Port D0: Data Direction= output */
	PORTD->PCR[BlueLED] = PORT_PCR_MUX(1); /* Port D0: MUX = GPIO */
	PTD->PDDR |= 1<<RedLED;       
	PORTD->PCR[RedLED] = PORT_PCR_MUX(1);
	PTD->PDDR |= 1<<GreenLED;       
	PORTD->PCR[GreenLED] = PORT_PCR_MUX(1);

	// 关闭RGB led
	PTD-> PSOR |= 1<<BlueLED;
	PTD-> PSOR |= 1<<GreenLED;
	PTD-> PSOR |= 1<<RedLED;
}

// RED_LED闪烁
void RED_toggle(void){
	PTD-> PTOR |= 1<<RedLED; 
}

// GREEN_LED闪烁
void GREEN_toggle(void){
	PTD-> PTOR |= 1<<GreenLED; 
}

// BLUE_LED闪烁
void BLUE_toggle(void){
	PTD-> PTOR |= 1<<BlueLED; 
}

// SW2 按键扫描
bool SW2_key(void){
	return (PTC->PDIR & (1<<SW2));
}
// SW3 按键扫描
bool SW3_key(void){
	return (PTC->PDIR & (1<<SW3));
}
