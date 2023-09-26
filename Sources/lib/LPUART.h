/*
 * Copyright (c) 2014 - 2016, Freescale Semiconductor, Inc.
 * Copyright (c) 2016 - 2018, NXP.
 * All rights reserved.
 */

#ifndef LPUART_H_
#define LPUART_H_
#include "S32K144.h" 
#include "Ymodem.h"
#include "latency.h"

extern uint8_t receivebuff[PACKET_HEAD+PACKET_1024_SIZE+PACKET_TAIL];
extern volatile uint8_t data_c;

void LPUART1_init(void);
uint8_t LPUART1_transmit_char(uint8_t send);
void LPUART1_transmit_string(char *data_string);
void LPUART1_printf(char *fmt, ...);
uint8_t LPUART1_receive_char(uint8_t * rec, uint32_t timeout);
void LPUART1_RxTx_IRQHandler(void);
void LPUART1_NVIC_init_IRQs (uint32_t vector_number, uint32_t priority);
#endif /* LPUART_H_ */
