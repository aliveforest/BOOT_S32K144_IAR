/******************************************************************
 * @file      : Sources\UDS\crc32.h 
 * @brief     : 
 * @author    : dengtongbei@yftech.com
 * @version   : v1.1
 * @date      : 2023-11-28 16:40, Created.
 * @copyright : Copyright (C) www.yftech.com 2023
 * @note      : from 2022/07/18 22:38
******************************************************************/

#ifndef CRC32_H 
#define CRC32_H 

#ifdef __cplusplus
extern "C"{
#endif



extern uint32_t Crc32Gen(const uint8_t *pBuf, const uint32_t len,  uint32_t InitVal,uint8_t newv);



#ifdef __cplusplus
}
#endif

#endif	/* CRC32_H */

/************************** End of File **************************/




