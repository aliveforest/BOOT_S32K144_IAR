/********************************************************************************
* FILE: ridproc.c
* DSCR: XXX
*
* Date: 2022/07/18 22:50
********************************************************************************/
#include "DcmSup.h"
#include "crc32.h"
#include "uds.h"
#include "ridproc.h"


#define  correctCheckResul			0x00
#define  incorrectCheckResult		0x01


typedef enum
{
	IDX_RID_FLASH_ERASE = 0,
	IDX_RID_CRC_CHECK,
	IDX_RID_CONSITION_CHECK,
	IDX_RID_PROGRAMM_DP,
	IDX_RID_ROLLBACK,
	IDX_RID_SYNC2BACKAREA,

	RID_NUM,
}RidIndexT;
static uint8_t _RidRunning[RID_NUM] = { 0 };

/********************************************************************************
*  Date: 2022/07/18 22:49
*  Dscr:
********************************************************************************/
void RidEventReset(void)
{
	for (uint i = 0; i < RID_NUM; i++)
	{
		_RidRunning[i] = 0;
	}
}

/********************************************************************************
*  Date: 2022/07/18 22:49
*  Dscr:
********************************************************************************/
void RidStartProcFlashErase(UdsCbT* pCb, uint8_t* pReq,
	uint8_t* pResp, uint16_t* pRespLen, uint32_t addr, uint32_t size)
{

	//address valid is xxFFFFFF ,The upper 8 bits are invalid
	if (((addr & 0x00FFFFFF) + size > TOTAL_FALSH_ADDR_END)
		|| (size > TOTAL_FALSH_SIZE_MAX))
	{
		pResp[0] = UDS_SID_NACK;
		pResp[1] = UDS_SID_ROUTINE_CTRL;
		pResp[2] = UDS_NRC_REQ_OUT_OF_RANGE;
		*pRespLen = 3;
	}
	else
	{
		if (FlsDrvDownloadComplete())
		{
			_RidRunning[IDX_RID_FLASH_ERASE] = TRUE;

			pResp[0] = UDS_SID_ACK(UDS_SID_ROUTINE_CTRL);
			pResp[1] = pReq[0];
			pResp[2] = pReq[1];
			pResp[3] = pReq[2];


#if 0 
			if (FALSE == HasUpdateFingerprint(gFingerprintBufRx))
			{
				tTemp = TRUE;
			}
#endif
			UDS_FED_WDG();
			UdsNrc78Set_WhileLoopLocked(UDS_SID_ROUTINE_CTRL);
			UdsNrc78SendTri_WhileLoopLocked(1);
			if (FlashEraseHandle(addr, size))
			{
				pResp[4] = correctCheckResul;
				*pRespLen = 5;
			}
			else
			{
				pResp[4] = incorrectCheckResult;
				*pRespLen = 5;
				/*	pResp[0] = UDS_SID_NACK;
					pResp[1] = UDS_SID_ROUTINE_CTRL;
					pResp[2] = UDS_NRC_REQ_SEQ_ERROR;
					*pRespLen = 3;*/
			}
		}
		else
		{
			pResp[0] = UDS_SID_NACK;
			pResp[1] = UDS_SID_ROUTINE_CTRL;
			pResp[2] = UDS_NRC_REQ_SEQ_ERROR;
			*pRespLen = 3;
		}
	}

}

/********************************************************************************
*  Date: 2022/07/18 22:49
*  Dscr:
********************************************************************************/
void RidResultProcFlashErase(UdsCbT* pCb, uint8_t* pReq,
	uint8_t* pResp, uint16_t* pRespLen)
{
	if (_RidRunning[IDX_RID_FLASH_ERASE])
	{
		pResp[0] = UDS_SID_ACK(UDS_SID_ROUTINE_CTRL);
		pResp[1] = pReq[0];
		pResp[2] = pReq[1];
		pResp[3] = pReq[2];
		pResp[4] = 0;
		*pRespLen = 5;
		_RidRunning[IDX_RID_FLASH_ERASE] = 0;
	}
	else
	{
		pResp[0] = UDS_SID_NACK;
		pResp[1] = UDS_SID_ROUTINE_CTRL;
		pResp[2] = UDS_NRC_REQ_SEQ_ERROR;
		*pRespLen = 3;
	}
}

/********************************************************************************
*  Date: 2022/07/18 22:49
*  Dscr:
********************************************************************************/
void RidStartProcCheckCrc(UdsCbT* pCb, uint8_t* pReq,
	uint8_t* pResp, uint16_t* pRespLen)
{
	uint32_t CrcValueRx = 0;
	uint8_t tType = 0;

	pResp[0] = UDS_SID_ACK(UDS_SID_ROUTINE_CTRL);
	pResp[1] = pReq[0];
	pResp[2] = pReq[1];
	pResp[3] = pReq[2];

	UDS_FED_WDG();
	UdsNrc78Set_WhileLoopLocked(UDS_SID_ROUTINE_CTRL);
	UdsNrc78SendTri_WhileLoopLocked(1);

	CrcValueRx = (uint32_t)((uint32_t)(pReq[3] << 24) | (uint32_t)(pReq[4] << 16) | (uint32_t)(pReq[5] << 8) | pReq[6]);
	tType = Get_DowloadAddressType();
	if (FlashCrcHandle(tType, CrcValueRx))
	{
		if (FLASH_TYPE_APP_CODE == tType)
		{
			//goto app for OTA			
			*(uint8_t_t*)(BOOT_JUMP_RES_ADDR) = (uint8_t_t)(U8_TRUE);
			SetAppCodeCrcValid(U32_TRUE);
			LogOcPut(OC_GOTA);

			SetJumpInfoPara(JUMP_REASON_OTA_MODEL, JUMP_REASON_BOOT2APP_OTA, U8_TRUE);

			pResp[0] = UDS_SID_NACK;
			pResp[1] = UDS_SID_ROUTINE_CTRL;
			pResp[2] = UDS_NRC_REQ_PEDING;
			*pRespLen = 3;
		}
		else
		{
			pResp[4] = correctCheckResul;
			*pRespLen = 5;
		}
	}
	else
	{
		pResp[4] = incorrectCheckResult;
		*pRespLen = 5;
	}
}

/********************************************************************************
*  Date: 2022/07/18 22:49
*  Dscr:
********************************************************************************/
void RidStartCheckProgrammingDependency(UdsCbT* pCb, uint8_t* pReq,
	uint8_t* pResp, uint16_t* pRespLen)
{

	uint8_t  cnt[2] = { 0 };
	uint16_t value = 0;
	uint8_t result = correctCheckResul;

	if (U8_TRUE != *(uint8_t_t*)(BOOT_JUMP_RES_ADDR))
	{
		pResp[0] = UDS_SID_NACK;
		pResp[1] = UDS_SID_ROUTINE_CTRL;
		pResp[2] = UDS_NRC_REQ_SEQ_ERROR;
		*pRespLen = 3;
		return;
	}

#if 0
	uint8_t AppMark[APP_DEP_INFO_LEN] = { 0 };
	uint8_t AppMarkValid[APP_DEP_INFO_LEN] = { APP_DEP_INFO_DATA };
	/*read application information in flash.*/
	(void)memcpy((void*)AppMark, (const void*)APP_DEP_INFO_ADDR, APP_DEP_INFO_LEN);

	UDS_FED_WDG();
	/*do check programming dependency*/
	for (uint8_t i = 0; i < APP_DEP_INFO_LEN; i++)
	{
		if (AppMarkValid[i] != AppMark[i])
		{
			result = incorrectCheckResult;
			break;
		}
	}
#else
	uint8_t AppMarkValid[APP_DEP_INFO_LEN] = { APP_DEP_INFO_DATA };
	if (0 != memcmp(AppMarkValid, (const uint8_t*)APP_DEP_INFO_ADDR, APP_DEP_INFO_LEN))
	{
		result = incorrectCheckResult;
	}
#endif
	if (correctCheckResul == result)
	{
		if (TRUE == Read_Eep_ReGrammCnt(cnt, 2))
		{
			value = (uint16_t)(((uint16_t)cnt[0] << 8) | ((uint16_t)cnt[1]));
			value++;
			cnt[0] = (uint8_t)(value >> 8);
			cnt[1] = (uint8_t)(value);
			if (TRUE != Write_Eep_ReGrammCnt((const uint8_t*)&cnt[0], 2))
			{
				result = incorrectCheckResult;
			}
		}
		else
		{
			result = incorrectCheckResult;
		}
	}
	pResp[0] = UDS_SID_ACK(UDS_SID_ROUTINE_CTRL);
	pResp[1] = pReq[0];
	pResp[2] = pReq[1];
	pResp[3] = pReq[2];
	pResp[4] = result;
	*pRespLen = 5;

}

void CheckProgrammingPreconditions(UdsCbT* pCb, uint8_t* pReq, uint8_t* pResp, uint16_t* pRespLen)
{
	pResp[0] = UDS_SID_ACK(UDS_SID_ROUTINE_CTRL);
	pResp[1] = pReq[0];
	pResp[2] = pReq[1];
	pResp[3] = pReq[2];
	*pRespLen = 4;

}

/********************************************************************************
*  Para: None
*  Retn: None
*  Date: 2023-01-29 20:04:05 LiangGaoJian
*  Dscr: start rollback process
********************************************************************************/
void RidStartRollback(UdsCbT* pCb, uint8_t* pReq, uint8_t* pResp, uint16_t* pRespLen)
{

	if (IF_RollBackEanble(U32_TRUE, 0))
	{
		_RidRunning[IDX_RID_ROLLBACK] = TRUE;
		pResp[0] = UDS_SID_ACK(UDS_SID_ROUTINE_CTRL);
		pResp[1] = pReq[0];
		pResp[2] = pReq[1];
		pResp[3] = pReq[2];
		//pResp[4] = IF_RollBackResult() == RL_ROLLBACK_RUNING ? 0x02 : 0x01;/*runing*/
		*pRespLen = 4;
	}
	else
	{
		pResp[0] = UDS_SID_NACK;
		pResp[1] = UDS_SID_ROUTINE_CTRL;
		pResp[2] = UDS_NRC_REQ_SEQ_ERROR;
		*pRespLen = 3;
	}
}

/********************************************************************************
*  Para: None
*  Retn: None
*  Date: 2023-01-29 20:04:23 LiangGaoJian
*  Dscr:
********************************************************************************/
void RidResultRollback(UdsCbT* pCb, uint8_t* pReq, uint8_t* pResp, uint16_t* pRespLen)
{
	RollbackResultT tRet = IF_RollBackResult();
	if ((_RidRunning[IDX_RID_ROLLBACK])
		&& (tRet > RL_NONE))
	{
		pResp[0] = UDS_SID_ACK(UDS_SID_ROUTINE_CTRL);
		pResp[1] = pReq[0];
		pResp[2] = pReq[1];
		pResp[3] = pReq[2];
		if ((tRet == RL_ROLLBACK_RUNING)
			|| (tRet == RL_ROLLBACK_BUSY))
		{
			pResp[4] = 0x02;/*runing*/
		}
		else if (tRet == RL_SUCCESS)
		{
			_RidRunning[IDX_RID_ROLLBACK] = 0;
			pResp[4] = 0x00;/*success*/
		}
		else if (tRet >= RL_FAIL)
		{
			_RidRunning[IDX_RID_ROLLBACK] = 0;
			pResp[4] = 0x01;/*fail*/
		}
		else
		{
			pResp[4] = 0xFF;
		}
		*pRespLen = 5;
	}
	else
	{
		pResp[0] = UDS_SID_NACK;
		pResp[1] = UDS_SID_ROUTINE_CTRL;
		pResp[2] = UDS_NRC_REQ_SEQ_ERROR;
		*pRespLen = 3;
	}
}

/********************************************************************************
*  Para: None
*  Retn: None
*  Date: 2023-01-29 20:04:30 LiangGaoJian
*  Dscr:
********************************************************************************/
void RidStartSync2BackupArea(UdsCbT* pCb, uint8_t* pReq, uint8_t* pResp, uint16_t* pRespLen)
{

	if (IF_Sync2BackupEanble(U32_TRUE))
	{
		_RidRunning[IDX_RID_SYNC2BACKAREA] = TRUE;
		pResp[0] = UDS_SID_ACK(UDS_SID_ROUTINE_CTRL);
		pResp[1] = pReq[0];
		pResp[2] = pReq[1];
		pResp[3] = pReq[2];
		//	pResp[4] = IF_RollBackResult() == RL_ROLLBACK_RUNING ? 0x02 : 0x01;/*runing*/
		*pRespLen = 4;
	}
	else
	{
		pResp[0] = UDS_SID_NACK;
		pResp[1] = UDS_SID_ROUTINE_CTRL;
		pResp[2] = UDS_NRC_REQ_SEQ_ERROR;
		*pRespLen = 3;
	}

}

/********************************************************************************
*  Para: None
*  Retn: None
*  Date: 2023-01-29 20:04:36 LiangGaoJian
*  Dscr:
********************************************************************************/
void RidResultSync2BackupArea(UdsCbT* pCb, uint8_t* pReq, uint8_t* pResp, uint16_t* pRespLen)
{
	RollbackResultT tRet = IF_RollBackResult();

	if ((_RidRunning[IDX_RID_SYNC2BACKAREA])
		&& (tRet > RL_NONE))
	{
		pResp[0] = UDS_SID_ACK(UDS_SID_ROUTINE_CTRL);
		pResp[1] = pReq[0];
		pResp[2] = pReq[1];
		pResp[3] = pReq[2];
		if ((tRet == RL_ROLLBACK_RUNING)
			|| (tRet == RL_ROLLBACK_BUSY))
		{
			pResp[4] = 0x02;/*runing*/
		}
		else if (tRet == RL_SUCCESS)
		{
			_RidRunning[IDX_RID_SYNC2BACKAREA] = 0;
			pResp[4] = 0x00;/*success*/
		}
		else if (tRet >= RL_FAIL)
		{
			_RidRunning[IDX_RID_SYNC2BACKAREA] = 0;
			pResp[4] = 0x01;/*fail*/
		}
		else
		{
			pResp[4] = 0xFF;
		}
		*pRespLen = 5;
	}
	else
	{
		pResp[0] = UDS_SID_NACK;
		pResp[1] = UDS_SID_ROUTINE_CTRL;
		pResp[2] = UDS_NRC_REQ_SEQ_ERROR;
		*pRespLen = 3;
	}
}
/*end :ridproc.c*/



