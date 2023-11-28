/******************************************************************
 * @file      : Sources\UDS\crc32.c 
 * @brief     : 
 * @author    : dengtongbei@yftech.com
 * @version   : v1.1
 * @date      : 2023-11-28 16:35, Created.
 * @copyright : Copyright (C) www.yftech.com 2023
 * @note      : from 2022/07/18 22:38
******************************************************************/

#include "S32K144.h" 
#include "Cpu.h"
#include "crc32.h"

uint32_t CRC32_HwCalculate(const uint8_t* data, uint32_t size, uint32_t InitVal, uint8_t newv)
{
	/*! CRC-32 Algorithm:
	 * polynomial = 0x04C11DB7
	 * seed = 0xFFFFFFFF
	 * Bits in a byte are transposed for writes.
	 * Both bits in bytes and bytes are transposed for read.
	 * XOR on reading.
	*/
	PCC->PCCn[PCC_CRC_INDEX] |= PCC_PCCn_CGC_MASK;	/* enable CRC clock */

	CRC->CTRL |= CRC_CTRL_TCRC(1);	/* enable 32-bit CRC protocol */
	CRC->CTRL |= CRC_CTRL_TOT(2);	/* RefIn:Only bytes are transposed; no bits in a byte are transposed */
	CRC->CTRL |= CRC_CTRL_TOTR(2);	/* RefOut:Only bytes are transposed; no bits in a byte are transposed */
	CRC->CTRL |= CRC_CTRL_FXOR(1);	/* XorOut:XOR on reading. */

	CRC->GPOLY = 0x04C11DB7;
	if (newv)
	{
		CRC->CTRL |= CRC_CTRL_WAS_MASK;	/* Set CRC_CTRL[WAS] to program the seed value. */
		CRC->DATAu.DATA = InitVal;	/* seed value */
	}

	CRC->CTRL &= ~CRC_CTRL_WAS_MASK;	/* Clear CRC_CTRL[WAS] to start writing data values. */
	for (uint32_t i = 0;i < size;i++)
	{
		CRC->DATAu.DATA_8.LL = *data++;/* write data values */
	}
	return (uint32_t)(CRC->DATAu.DATA);
}

/**
 * @brief  : CRC32 Calculation
 * @param  : void
 * @return : uint32_t result
**/
uint32_t Crc32Gen(const uint8_t* pBuf, const uint32_t len, uint32_t InitVal, uint8_t newv)
{
	uint32_t Rtv = 0;
	Rtv = CRC32_HwCalculate(pBuf, len, InitVal, newv);
	return Rtv;
}



/************************** End of File **************************/



