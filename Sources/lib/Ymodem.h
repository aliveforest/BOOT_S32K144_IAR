/*
 * Ymodem.h
 *
 *  Created on: 2023年8月25日
 *      Author: dengtongbei
 */
#include <stdint.h>
#include "S32K144.h" 
#include "FlashWriteRead.h"
#include "latency.h"

#ifndef YMODEM_H_
#define YMODEM_H_

#define FILE_NAME_LENGTH (120)
#define FILE_SIZE_LENGTH (8)

#define PACKET_INDEX            (1) /* 帧序 */
#define PACKET_INDEX_INVERSION  (2) /* 帧序取反 */

#define PACKET_HEAD             (3) /* 帧头大小3 */
#define PACKET_128_SIZE         (128)
#define PACKET_1024_SIZE        (1024)
#define PACKET_TAIL             (2) /* 帧尾大小2 */

/* Ymodem协议命令 */
#define YM_SOH (0x01)   /* 128字节的数据包 */
#define YM_STX (0x02)   /* 1024字节的数据包 */
#define YM_EOT (0x04)   /* 结束传输 */
#define YM_ACK (0x06)   /* 正确接收回应 */
#define YM_NAK (0x15)   /* 错误接收回应，请求重传当前数据包 */
#define YM_CAN (0x18)   /* 取消传输，连续发送5次该命令 */
#define YM_C   (0x43)   /* 请求数据，'C'：0x43, 需要 16-bit CRC */

#define ABORT1 (0x41)   /* 'A'：0x41, 用户终止 */
#define ABORT2 (0x61)   /* 'a'：0x61, 用户终止 */

#define NAK_TIMEOUT (0x100000)
#define MAX_ERRORS (5)

/* 返回值信息 */
typedef enum {
    RE_OK = 0u,         /* 一切正常 */
    RE_ERROR = 1u,      /* 错误 */
    RE_OVERSIZE = 2u,   /* 文件过大 */
    RE_TMOUT = 3u,      /* 超时 */
    RE_NODATA  = 4u,    /* 无数据 */
    RE_CANSEL  = 5u,    /* 取消传输 */
    RE_CRC_ERR = 6u,    /* CRC错误 */
	RE_FLASH_ERR = 7u,  /* Flash写入错误 */
	RE_INDEX_ERR = 8u,  /* 帧序错误 */
	RE_UNSUPT  = 9u    /* 不支持 */
} ret_t; 
/* Ymodem协议帧结构体：包含file_name file_size data data_length */
typedef struct {
    uint8_t HEAD;
    uint8_t Index;
    uint8_t Index_Inversion;
    uint8_t data[PACKET_1024_SIZE]; /* 1024 取最大 */
    uint32_t file_size; /* 文件大小，字节 */
    uint32_t data_length;
    uint8_t CRC_H,CRC_L;
    uint16_t CRC16;
}YmFrame_t;

extern volatile int32_t packets_index;

ret_t Ymodem_Receive_File(uint32_t FlashDestination, uint32_t timeout) ;


#endif /* YMODEM_H_ */
