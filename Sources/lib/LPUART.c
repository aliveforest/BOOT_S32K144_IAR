/*
 * Copyright (c) 2014 - 2016, Freescale Semiconductor, Inc.
 * Copyright (c) 2016 - 2018, NXP.
 * All rights reserved.
 */

#include "S32K144.h" 	/* include peripheral declarations S32K144 */
#include "LPUART.h"
#include <stdio.h>
#include <string.h>
#include "stdarg.h"
#include <stdint.h>
#include <stdbool.h>

#define YMODEM   /* 使用YMODEM协议 */

/* Init. summary: 115200 baud, 1 stop bit, 8 bit format, no parity */
void LPUART1_init(void)
{
	/* Pins definitions
	 * ===================================================
	 * Pin number        | Function
	 * ----------------- |------------------
	 * PTC6              | UART1 TX
	 * PTC7              | UART1 RX
	 */
	PCC->PCCn[PCC_PORTC_INDEX ]|=PCC_PCCn_CGC_MASK; /* Enable clock for PORTC */
	PORTC->PCR[6]|=PORT_PCR_MUX(2);	/* Port C6: MUX = ALT2, UART1 TX */
	PORTC->PCR[7]|=PORT_PCR_MUX(2);   /* Port C7: MUX = ALT2, UART1 RX */

	PCC->PCCn[PCC_LPUART1_INDEX] &= ~PCC_PCCn_CGC_MASK;    /* Ensure clk disabled for config */
	PCC->PCCn[PCC_LPUART1_INDEX] |= PCC_PCCn_PCS(2u)    /* Clock Src = 2 (SIRCDIV2_CLK) */
                            	 |  PCC_PCCn_CGC_MASK;     /* Enable clock for LPUART1 regs */
	/* 波特率=波特时钟/ ((OSR+1) × SBR) 8M/3/23 = 115942 ~=115200  */
	LPUART1->BAUD = LPUART_BAUD_SBR(3u)  	/* Initialize for 9600 baud, 1 stop: */
                	|LPUART_BAUD_OSR(22u);  /* SBR=23 (0x17): baud divisor = 8M/115200/(2+1) = ~23 */
											/* OSR=22: Over sampling ratio = 22+1=23 */
											/* SBNS=0: One stop bit */
											/* BOTHEDGE=0: receiver samples only on rising edge */
											/* M10=0: Rx and Tx use 7 to 9 bit data characters */
											/* RESYNCDIS=0: Resync during rec'd data word supported */
											/* LBKDIE, RXEDGIE=0: interrupts disable */
											/* TDMAE, RDMAE, TDMAE=0: DMA requests disabled */
											/* MAEN1, MAEN2,  MATCFG=0: Match disabled */

	LPUART1->CTRL =	LPUART_CTRL_RIE(1)      /* RIE=1: 启动接收中断 */
					|LPUART_CTRL_TE_MASK   	/* Enable transmitter & receiver, no parity, 8 bit char: */
					|LPUART_CTRL_RE_MASK;	    /* RE=1: Receiver enabled */
												/* TE=1: Transmitter enabled */
												/* PE,PT=0: No hw parity generation or checking */
												/* M7,M,R8T9,R9T8=0: 8-bit data characters*/
												/* DOZEEN=0: LPUART enabled in Doze mode */
												/* ORIE,NEIE,FEIE,PEIE,TIE,TCIE,RIE,ILIE,MA1IE,MA2IE=0: no IRQ*/
												/* TxDIR=0: TxD pin is input if in single-wire mode */
												/* TXINV=0: TRansmit data not inverted */
												/* RWU,WAKE=0: normal operation; rcvr not in statndby */
												/* IDLCFG=0: one idle character */
												/* ILT=0: Idle char bit count starts after start bit */
												/* SBK=0: Normal transmitter operation - no break char */
												/* LOOPS,RSRC=0: no loop back */
	while((bool)((LPUART1->CTRL & LPUART_CTRL_RE_MASK) != 0U) != 1u) {}
	LPUART1_NVIC_init_IRQs(LPUART1_RxTx_IRQn, 0x00);
}
/* 中断配置 */
void LPUART1_NVIC_init_IRQs (uint32_t vector_number, uint32_t priority) {
	uint8_t shift = (uint8_t) (8U - FEATURE_NVIC_PRIO_BITS);
	/* 清除任何挂起的 IRQ */
	S32_NVIC->ISER[(uint32_t)(vector_number) >> 5U] = (uint32_t)(1U << ((uint32_t)(vector_number) & (uint32_t)0x1FU));
	/* 使能 IRQ */
	S32_NVIC->ICPR[(uint32_t)(vector_number) >> 5U] = (uint32_t)(1U << ((uint32_t)(vector_number) & (uint32_t)0x1FU));
	/* 优先级设置 */
	S32_NVIC->IP[(uint32_t)vector_number] = (uint8_t)(((((uint32_t)priority) << shift)) & 0xFFUL);
}

/* 发送单个字符函数 */
uint8_t LPUART1_transmit_char(uint8_t send) {   
	while((LPUART1->STAT & LPUART_STAT_TDRE_MASK)>>LPUART_STAT_TDRE_SHIFT==0);
	/* Wait for transmit buffer to be empty */
	LPUART1->DATA=send;              /* Send data */
	return 0;
}
/* 发送字符串函数 */
void LPUART1_transmit_string(char *data_string)  {  
	uint32_t n= strlen(data_string), i=0;
    for(;i<n;++i){
    	LPUART1_transmit_char(data_string[i]);
    }
}

char LPUART1_TX_BUF[200];
/* 串口打印函数 */
void LPUART1_printf(char *fmt, ...) {
//	uint32_t bytesRemaining;
	va_list ap;
	va_start(ap, fmt);
	vsprintf((char *)LPUART1_TX_BUF, fmt, ap);
	va_end(ap);
	LPUART1_transmit_string(LPUART1_TX_BUF);
}
/* 接收单个字符函数 */
uint8_t LPUART1_receive_char(uint8_t * rec, uint32_t timeout) {
	uint32_t i=0;
	for(i=0;i<timeout;++i){
		/* Wait for received buffer to be full */
		if((LPUART1->STAT & LPUART_STAT_RDRF_MASK)>>LPUART_STAT_RDRF_SHIFT==0){
			*rec = (uint8_t)LPUART1->DATA;            /* Read received data*/
			return 0;
		}
	}
//	LPUART1_printf("No receive char\r\n");
	return 2;
}


uint8_t receivebuff[PACKET_HEAD+PACKET_1024_SIZE+PACKET_TAIL];
volatile uint8_t data_c=0;
volatile uint32_t rev_i=0;
void LPUART1_RxTx_IRQHandler(void){
	while((LPUART1->STAT & LPUART_STAT_RDRF_MASK)>>LPUART_STAT_RDRF_SHIFT==0){
		/* Wait for received buffer to be full */
	}
	receivebuff[rev_i++]= LPUART1->DATA;            /* Read received data*/
	if (((receivebuff[0] == YM_SOH) && (rev_i == (PACKET_HEAD+PACKET_128_SIZE+PACKET_TAIL))) ||
		((receivebuff[0] == YM_STX) && (rev_i == (PACKET_HEAD+PACKET_1024_SIZE+PACKET_TAIL))) ){
		rev_i = 0;
		data_c = 0;
		packets_index++;
	}else if(receivebuff[0] < YM_SOH || receivebuff[0] > YM_STX){
		/* 如果接收的不是帧头命令 */
		data_c = receivebuff[0];
		memset(receivebuff,0, sizeof(receivebuff)/sizeof(receivebuff[0])); /* 清空receivebuff数组 */
		rev_i = 0;
	}
	if(rev_i>=PACKET_HEAD+PACKET_1024_SIZE+PACKET_TAIL) rev_i=0;
}



// char receivebuff[200];
// uint16_t rev_i=0;
// void LPUART1_RxTx_IRQHandler(void){
// 	while((LPUART1->STAT & LPUART_STAT_RDRF_MASK)>>LPUART_STAT_RDRF_SHIFT==0){
// 		/* Wait for received buffer to be full */
// 	}
// 	receivebuff[rev_i++]= LPUART1->DATA;            /* Read received data*/
// 	if (receivebuff[rev_i - 1] == '\n' || rev_i >= sizeof(receivebuff)) {
// 		receivebuff[rev_i] = '\0';
// 		rev_i = 0;
// 		LPUART1_printf(receivebuff);
// 	}
// }







