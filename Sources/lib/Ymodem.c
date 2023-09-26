/*
 * Ymodem.c
 *
 *  Created on: 2023年8月25日
 *      Author: dengtongbei
 *		Modify: 2023-9-26: 去除malloc（易报错），改为先申请结构体实例，再赋值其地址给类型指针
 */
#include "Ymodem.h"
#include <stdlib.h>
#include <string.h>
#include "LPUART.h"
#include "latency.h"

/* 变量声明 */
static uint32_t Erase_Pages = 0;
static volatile uint32_t FlashDestination = Flash_Start_Address; /* 存储Flash偏移 */
static uint32_t PageSize = Flash_Erase_Size;
/* 函数声明 */
uint16_t crc16(uint8_t *data, uint32_t length);
ret_t Receive_Packet(YmFrame_t *Ym_F);

/* 发送单个字符 */
static ret_t Send_Byte(uint8_t c){
    LPUART1_transmit_char(c);
    return RE_OK;
}
/* Ymodem 命令处理 */
static ret_t Ym_Cmd_Process(uint8_t cmd_c){
    switch(cmd_c){
        case YM_EOT: Send_Byte(YM_NAK); Send_Byte(YM_C);
            LPUART1_printf("End of transmit\r\n"); later_ms(500); return RE_OK; /* 结束传输 */
        case YM_CAN: LPUART1_printf("CANSEL\r\n");later_ms(500);
            return RE_CANSEL; /* 取消传输 */
        case ABORT1: LPUART1_printf("CANSEL1\r\n"); later_ms(200);
            return RE_CANSEL; /* 用户终止 */
        case ABORT2: LPUART1_printf("CANSEL2\r\n");later_ms(200);
            return RE_CANSEL; /* 用户终止 */
        default : LPUART1_printf("Unsupport!: %d\r\n",cmd_c);later_ms(200);
            return RE_UNSUPT; /* 不支持的命令 */
    }
}

/* 清空 Ym_F 结构体 */
static void Clear_YM(YmFrame_t *Ym_x){
    Ym_x->HEAD=0u;
    Ym_x->Index=0u;
    Ym_x->Index_Inversion=0u;
    Ym_x->file_size=0u;
    Ym_x->data_length=0u;
    Ym_x->CRC_H=0u;
    Ym_x->CRC_L=0u;
    Ym_x->CRC16=0u;
    memset(Ym_x->data,0u, sizeof(Ym_x->data)/sizeof(Ym_x->data[0])); /* 清空data数组 */
}

static uint8_t file_name[FILE_NAME_LENGTH]={0}, file_size[FILE_SIZE_LENGTH]={0};
/* 文件信息存储 ASCII形式*/
static ret_t File_Information_Storage(YmFrame_t *Ym_F){
    uint16_t i,j;
    for (i = 0; (Ym_F->data[i] != 0) && (i < FILE_NAME_LENGTH); i++) {
        file_name[i] = Ym_F->data[i];
    }
    file_name[i++] = '\0';
    Ym_F->file_size = 0;
    for (j=0; (Ym_F->data[i] != ' ') && (j < FILE_SIZE_LENGTH); j++,i++) {
        file_size[j] = Ym_F->data[i];
        Ym_F->file_size *=10u;
        Ym_F->file_size += (Ym_F->data[i]-'0');
    }
    file_size[j] = ' ';
//    Ym_F->file_size = strtol(file_size, NULL, 10); /* 16进制字符串转数字 */
    if (Ym_F->file_size > (FLASH_SIZE - 1)){ /* 判断数据包是否过大 */
        Send_Byte(YM_CAN); Send_Byte(YM_CAN);  /* 取消传输，大于给定的flash大小，则结束 */
        Clear_YM(Ym_F);/* 清空Ym_F所有内容 */
        memset(receivebuff,0, sizeof(receivebuff)/sizeof(receivebuff[0])); /* 清空receivebuff数组 */
        LPUART1_printf("OVERSIZE!\r\n"); later_ms(500);
        return RE_OVERSIZE;
    }
    return RE_OK;
}

static YmFrame_t Ym_st; /* 申请结构体实例 */
static volatile uint32_t bits_remain = 1024u; /* 记录剩余多少字节的数据未接收 */
volatile int32_t packets_index = -1;
/* 通过Ymodem协议接收一个文件 */
ret_t Ymodem_Receive_File(uint32_t FlashDestination, uint32_t timeout) {
    uint8_t file_done=0;
    int32_t last_index, j;
    ret_t ret = RE_ERROR;
	status_t flash_status;
	YmFrame_t *Ym_F = &Ym_st;   /* 结构体地址赋值给指针 */
    Clear_YM(Ym_F);/* 清空Ym_F所有内容 */
    memset(receivebuff,0, sizeof(receivebuff)/sizeof(receivebuff[0])); /* 清空receivebuff数组 */
    packets_index=-1; /* 用于记录接收了帧数据 */
    last_index = packets_index;
    data_c = 0;
    for (;;){
    	for(j=0;j<20;++j){
    		if(j>18) {
    			LPUART1_printf("Time Out!\r\n"); later_ms(500);
    			Clear_YM(Ym_F);/* 清空Ym_F所有内容 */
    			return RE_TMOUT;
    		}
    		if(packets_index == -1 ){
    			Send_Byte(YM_C); /* 发送握手信号 */
    		}
			later_ms(200);   /* 延时，等待串口接收数据 */
            if(packets_index > last_index){/* 接受到一个新的数据帧 */
                ret = Receive_Packet(Ym_F); /* 处理接收的数据帧 */
                if(ret==RE_OK) break;
                else {packets_index--; Send_Byte(YM_NAK);}
            }else if(data_c!=0){
                if(Ym_Cmd_Process(data_c)==RE_OK) break;
            }
        }
        if(ret!=RE_OK){ /* 数据帧仍然有误 */
            Send_Byte(YM_CAN); Send_Byte(YM_CAN);  /* 取消传输 */
            Clear_YM(Ym_F);/* 清空Ym_F所有内容 */
            LPUART1_printf("Still ERROR:%d\r\n",ret);   later_ms(1000); /* 延时 */
            return ret; /* 结束传输 */
        }else if(ret==RE_OK){ /* 数据正常 */
            if(packets_index == 0 ) { /* 第0个为文件名数据帧 */
                if(File_Information_Storage(Ym_F)!=RE_OK) return RE_OVERSIZE; /* 文件信息存储 */
                bits_remain = Ym_F->file_size;
                Erase_Pages = Flash_Size_Calc(Ym_F->file_size); /* 计算需要擦除Flash的扇区大小 */
                flash_status = Flash_Erase_Sector(&flashConfig, FlashDestination, Erase_Pages);
                if(flash_status!=STATUS_SUCCESS) { /* 判断是否擦除 */
                	LPUART1_printf("Flash Erase error! \r\n"); later_ms(500);
                	Clear_YM(Ym_F);/* 清空Ym_F所有内容 */
                	return RE_FLASH_ERR;
                }
                Clear_YM(Ym_F);/* 清空Ym_F所有内容 */
                Send_Byte(YM_ACK); /* 发送确认 */
            }else if(Ym_F->Index == 0 && Ym_F->CRC16 == 0){ /* 结束帧 */
                Send_Byte(YM_ACK); file_done = 1;
            }else{ /* 内容数据帧 */
            	flash_status = Flash_Write_Data(&flashConfig, FlashDestination, Ym_F->data_length, Ym_F->data); /* 数据写到Flash中 */
                if (*(uint8_t*)FlashDestination != *(uint8_t*)(Ym_F->data)) { /* 判断是否正确写入 */
                    Send_Byte(YM_CAN); Send_Byte(YM_CAN);
                    Clear_YM(Ym_F);/* 清空Ym_F所有内容 */
                    LPUART1_printf("Flash Write error! \r\n");later_ms(500);
                    return RE_FLASH_ERR;
                }
                FlashDestination += Ym_F->data_length; /* 计算flash写入地址 */
                Clear_YM(Ym_F);/* 清空Ym_F所有内容 */
                Send_Byte(YM_ACK); /* 发送确认 */
            }
            last_index = packets_index;
        }
        if(file_done==1) {
        	Clear_YM(Ym_F);/* 清空Ym_F所有内容 */
        	LPUART1_printf("Ymodem file received success! \r\n");
        	later_ms(1000); /* 延时 */
        	return RE_OK;
        }
    }
}

/* 接收一个数据帧 */
ret_t Receive_Packet(YmFrame_t *Ym_F){
    uint32_t i=0;
    Ym_F->HEAD = receivebuff[0];
    switch (Ym_F->HEAD){ /* 判断帧头为何种命令 */
        case YM_SOH:Ym_F->data_length = PACKET_128_SIZE;  break;   /* 128字节数据包 */
        case YM_STX:Ym_F->data_length = PACKET_1024_SIZE; break;   /* 1024字节数据包 */
        default:Ym_Cmd_Process(Ym_F->HEAD); later_ms(500); return RE_ERROR;
    }
    Ym_F->Index = receivebuff[1];           /* 接收帧头 */
    Ym_F->Index_Inversion = receivebuff[2]; /* 接收帧头取反值 */
    if (Ym_F->Index != ((Ym_F->Index_Inversion ^ 0xff) & 0xff)){
    	 LPUART1_printf("Index ERROR\r\n"); later_ms(500);
        return RE_INDEX_ERR; /* 判断帧序与帧序取反的取反是否一致 */
    }

    memset(Ym_F->data,0, sizeof(Ym_F->data)/sizeof(Ym_F->data[0])); /* 接收之前清空data数组 */
    memcpy(Ym_F->data, receivebuff+PACKET_HEAD, Ym_F->data_length); /* 数据存进data数组 */

    Ym_F->CRC_H = receivebuff[Ym_F->data_length-2 + PACKET_HEAD + PACKET_TAIL];
    Ym_F->CRC_L = receivebuff[Ym_F->data_length-1 + PACKET_HEAD + PACKET_TAIL];
    /* 把CRC16高八位和低八位合并 */
    Ym_F->CRC16 = Ym_F->CRC_H;
    Ym_F->CRC16 <<= 8;
    Ym_F->CRC16 |= Ym_F->CRC_L;
    if (crc16(Ym_F->data, Ym_F->data_length) != Ym_F->CRC16){
    	LPUART1_printf("CRC ERROR\r\n"); later_ms(500);
        return RE_CRC_ERR; /* CRC校验 */
    }

    if(packets_index > 0){/* 从第一个数据帧开始 */
        /* 处理最后一个数据帧 */
        if(bits_remain < Ym_F->data_length){/* 把0X1A改为0xff */
        	for(i=bits_remain;i<Ym_F->data_length;++i){
        		Ym_F->data[i] = 0xff;
        	}
        }
    	if(bits_remain > Ym_F->data_length) bits_remain -= Ym_F->data_length;
    }

    memset(receivebuff,0, sizeof(receivebuff)/sizeof(receivebuff[0])); /* 接收之后清空数组 */
    return RE_OK;
}

/* CRC16计算函数 */
uint16_t crc16(uint8_t *data, uint32_t length) {
    const uint16_t polynomial = 0x1021;     // CRC16的多项式
    uint16_t crc = 0x0000;                  // 初始值
    uint8_t bit;
    for (; length>0; length--) {
        crc ^= (uint16_t)(*data++) << 8;    // 将数据字节与CRC左移8位后异或
        for (bit = 0; bit < 8; bit++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ polynomial; // 异或并左移1位
            } else {
                crc <<= 1;                  // 左移1位
            }
        }
    }
    return crc;
}

