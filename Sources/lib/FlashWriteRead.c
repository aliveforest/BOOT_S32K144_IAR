/*
 * FlashWriteRead.c
 *
 *  Created on: 2023年8月24日
 *      Author: dengtongbei
 */
#include "FlashWriteRead.h"

/* 声明1个由 FlashInit 初始化的 FLASH 配置结构，所有闪存操作都将使用该结构 */
flash_ssd_config_t flashConfig;

/* flash 先擦除再写入，size需为 4096 的整数倍  */
status_t Flash_Erase_Write_Data(const flash_ssd_config_t * pSSDConfig, uint32_t Address, uint32_t size, uint8_t *data) {
	status_t ret_status;
	/* Erase */
	INT_SYS_DisableIRQGlobal(); /* 关闭全局中断 */
	ret_status = FLASH_DRV_EraseSector(pSSDConfig, Address, size);
	later_ms(200);
	if(ret_status == STATUS_SUCCESS) {
		LPUART1_printf("Successful Erase!\r\n");
		/* 擦除成功再 Write */
		ret_status = FLASH_DRV_Program(pSSDConfig, Address, size, data);
		later_ms(20);
		if(ret_status == STATUS_SUCCESS) {
				LPUART1_printf("Successful Write!\r\n");
		}else{
			LPUART1_printf("Failed Write,RET:%#X \r\n", ret_status);
			later_ms(500); /* 延时 */
		}
	}else{
		LPUART1_printf("Failed Erase,RET:%#X \r\n", ret_status);
		later_ms(500); /* 延时 */
	}
	INT_SYS_EnableIRQGlobal(); /* 开启全局中断 */
	return ret_status;
}
/* flash擦除扇区，size需为 4096 的整数倍 */
status_t Flash_Erase_Sector(const flash_ssd_config_t * pSSDConfig, uint32_t Address, uint32_t size) {
	status_t ret_status;
	/* Erase */
	INT_SYS_DisableIRQGlobal(); /* 关闭全局中断 */
	ret_status = FLASH_DRV_EraseSector(pSSDConfig, Address, size);
//	later_ms(2);
	if(ret_status == STATUS_SUCCESS) {
//		LPUART1_printf("Successful Erase!\r\n");
	}else{
		LPUART1_printf("Flash Failed Erase! RET:%#X \r\n", ret_status);
		later_ms(500); /* 延时 */
	}
	INT_SYS_EnableIRQGlobal(); /* 开启全局中断 */
	return ret_status;
}
/* flash写入函数，size需为 8 bit 的整数倍*/
status_t Flash_Write_Data(const flash_ssd_config_t * pSSDConfig, uint32_t Address, uint32_t size, uint8_t *data) {
	status_t ret_status;
	INT_SYS_DisableIRQGlobal(); /* 关闭全局中断 */
	/* Write */
	ret_status = FLASH_DRV_Program(pSSDConfig, Address, size, data);
//	later_ms(2);
	if(ret_status == STATUS_SUCCESS) {
//			LPUART1_printf("Successful Write!\r\n");
	}else{
		LPUART1_printf("Failed Write Flash! RET:%#X \r\n", ret_status);
		later_ms(500); /* 延时 */
	}
	INT_SYS_EnableIRQGlobal(); /* 开启全局中断 */
	return ret_status;
}

/* flash读取函数，从内存地址读取 */
void Flash_Read_Data(uint32_t Address, uint32_t size, uint8_t *read_data) {
    uint32_t i;
    for (i = 0; i < size; i++) {
    	read_data[i] = *(uint8_t *)(Address + i * sizeof(uint8_t));
    }
}

/* 计算返回flash扇区的整数倍大小，单位：字节 */
uint32_t Flash_Size_Calc(uint32_t size) {
    uint32_t pagenumber = 0x0;
    if ((size % FLASH_PAGE_SIZE) != 0) {
        pagenumber = (size / FLASH_PAGE_SIZE) + 1;
    }else{
        pagenumber = size / FLASH_PAGE_SIZE;
    }
    return pagenumber*FLASH_PAGE_SIZE;

}
