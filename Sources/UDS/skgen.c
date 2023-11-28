/********************************************************************************
* FILE: skgen.c
* DSCR: XXX
*
* Date: 2022/07/18 22:57
********************************************************************************/
#include "DcmSup.h"
#include "uds.h"
#include "skgen.h"

#define SKGEN_SECTORY_L1          0x4D5A16E3
#define SKGEN_SECTORY_L2          0x40000011

/********************************************************************************
*  Date: 2022/07/18 22:58
*  Dscr:
********************************************************************************/
void SecrectKeyGen(uint8_t* pSeed, uint8_t* pSK, uint8_t level)
{

	unsigned int pin = 0;
	unsigned int i = 0;

	if (UDS_SUB_REQ_SEED_BT == level)
	{
		pin = 0x17454833;
		for (i = 0; i < 4; i++)
		{
			pin ^= ((pin << 5) + pSeed[i] + (pin >> 4));
		}
		for (i = 0; i < 4; i++)
		{
			pSK[i] = (pin >> (i * 8)) & 0xFF;
		}

	}
	else
	{
		for (i = 0; i < 4; i++)
		{
			pSK[i] = 0xFF;
		}
	}

}

/*end :skgen.c*/



