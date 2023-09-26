/*
 * FlashWriteRead.h
 *
 *  Created on: 2023年8月24日
 *      Author: dengtongbei
 */

#ifndef FLASHWRITEREAD_H_
#define FLASHWRITEREAD_H_

#include "Cpu.h"
#include "LPUART.h"
#include "latency.h"

extern flash_ssd_config_t flashConfig;

#define FLASH_PAGE_SIZE         (0x1000)    /* 扇区大小：4 Kbytes */
#define FLASH_SIZE              (0x40000)   /* 给定可读写的flash大小：256 KBytes */

#define Flash_Start_Address 0x00009000ul	/* Flash开始擦除的地址，注意字节对齐，S32K144为16字节对齐 */
#define Flash_Erase_Size 0ul 			/* Flash擦除的大小(单位:字节) 最小4K起(4096) */

uint32_t Flash_Size_Calc(__IO uint32_t Size);
status_t Flash_Erase_Write_Data(const flash_ssd_config_t * pSSDConfig, uint32_t Address, uint32_t size, uint8_t *data);
status_t Flash_Erase_Sector(const flash_ssd_config_t * pSSDConfig, uint32_t Address, uint32_t size);
status_t Flash_Write_Data(const flash_ssd_config_t * pSSDConfig, uint32_t Address, uint32_t size, uint8_t *data); 
void Flash_Read_Data(uint32_t Address, uint32_t size, uint8_t *read_data);

#endif /* FLASHWRITEREAD_H_ */
