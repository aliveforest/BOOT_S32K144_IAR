/********************************************************************************
* FILE: didproc.c
* DSCR: XXX
*
* Date: 2022/07/18 22:44
********************************************************************************/
#include "DcmSup.h"
#include "didproc.h"
#include "uds.h"


//sid 22
uint8_t DidRead_GACECUHardwareVersionNumberDataIdentifier(uint8_t* pResp, uint16_t* pRespLen, uint16_t RespMax)
{
	if (RespMax < APP_HW_LEN)
	{
		return UDS_NRC_MSG_LEN_FORMAT_INVALID;
	}
	if (TRUE != ecual_eeprom_read(EEP_APP_HW_ADDR, pResp, APP_HW_LEN))
	{
		(void)FLS_ApiRead(APP_HW_ADDR, APP_HW_LEN, pResp);
	}
	*pRespLen = APP_HW_LEN;

	return 0;
}
uint8_t DidRead_bootSoftwareIdentification(uint8_t* pResp, uint16_t* pRespLen, uint16_t RespMax)
{
	if (RespMax < APP_BOOT_SW_LEN)
	{
		return UDS_NRC_MSG_LEN_FORMAT_INVALID;
	}
	if (TRUE != ecual_eeprom_read(EEP_APP_BOOT_SW_ADDR, pResp, APP_BOOT_SW_LEN))
	{
		(void)FLS_ApiRead(APP_BOOT_SW_ADDR, APP_BOOT_SW_LEN, pResp);
	}

#if 0 /*all use app space version@2023-03-16 14:00:09*/
	uint16_t buf = BOOT_SW_DATA;
	pResp[APP_BOOT_SW_LEN - 2] = (uint8_t)((buf >> 4) & 0x0f) + '0';
	pResp[APP_BOOT_SW_LEN - 1] = (uint8_t)(buf & 0x0f) + '0';
#endif
	* pRespLen = APP_BOOT_SW_LEN;

	return 0;
}
uint8_t DidRead_GACSparePartNumberDataIdentifier(uint8_t* pResp, uint16_t* pRespLen, uint16_t RespMax)
{
	if (RespMax < APP_SPARE_PN_LEN)
	{
		return UDS_NRC_MSG_LEN_FORMAT_INVALID;
	}
	if (TRUE != ecual_eeprom_read(EEP_APP_SPARE_PN_ADDR, pResp, APP_SPARE_PN_LEN))
	{
		(void)FLS_ApiRead(APP_SPARE_PN_ADDR, APP_SPARE_PN_LEN, pResp);
	}

	*pRespLen = APP_SPARE_PN_LEN;

	return 0;
}
uint8_t DidRead_GACECUSoftwareVersionNumberDataIdentifier(uint8_t* pResp, uint16_t* pRespLen, uint16_t RespMax)
{
	if (RespMax < APP_SW_LEN)
	{
		return UDS_NRC_MSG_LEN_FORMAT_INVALID;
	}
	if (TRUE != ecual_eeprom_read(EEP_APP_SW_ADDR, pResp, APP_SW_LEN))
	{
		(void)FLS_ApiRead(APP_SW_ADDR, APP_SW_LEN, pResp);
	}
	*pRespLen = APP_SW_LEN;

	return 0;
}

uint8_t DidRead_DiagSessionMode(uint8_t* pResp, uint16_t* pRespLen, uint16_t RespMax)
{
	if (RespMax < 1)
	{
		return UDS_NRC_MSG_LEN_FORMAT_INVALID;
	}
	pResp[0]=UdsGetactiveDiagSessionMode();
	*pRespLen = 1;
	return 0;
}

uint8_t DidRead_vF181(uint8_t* pResp, uint16_t* pRespLen, uint16_t RespMax)
{
	if (RespMax < APP_SW_F181_LEN)
	{
		return UDS_NRC_MSG_LEN_FORMAT_INVALID;
	}
	(void)FLS_ApiRead(APP_SW_F181_ADDR, APP_SW_F181_LEN, pResp);
	*pRespLen = APP_SW_F181_LEN;
	return 0;
}
uint8_t DidRead_ReprogrammingCounter(uint8_t* pResp, uint16_t* pRespLen, uint16_t RespMax)
{
	if (RespMax < (EEP_LEN_GRAMM_CNT - MEM_DID_FLAG_LEN))
	{
		return UDS_NRC_MSG_LEN_FORMAT_INVALID;
	}
	Read_Eep_ReGrammCnt(pResp, (EEP_LEN_GRAMM_CNT - MEM_DID_FLAG_LEN));
	*pRespLen = (EEP_LEN_GRAMM_CNT - MEM_DID_FLAG_LEN);

	return 0;
}
uint8_t DidRead_ReprogrammingAttemptCounter(uint8_t* pResp, uint16_t* pRespLen, uint16_t RespMax)
{
	if (RespMax < (EEP_LEN_GRAMM_RETTRY_CNT - MEM_DID_FLAG_LEN))
	{
		return UDS_NRC_MSG_LEN_FORMAT_INVALID;
	}
	Read_Eep_ReAttmGrammCnt(pResp, (EEP_LEN_GRAMM_RETTRY_CNT - MEM_DID_FLAG_LEN));
	*pRespLen = (EEP_LEN_GRAMM_RETTRY_CNT - MEM_DID_FLAG_LEN);

	return 0;
}
uint8_t DidRead_ApplicationSoftwareFingerprintDataIdentifier(uint8_t* pResp, uint16_t* pRespLen, uint16_t RespMax)
{
	if (RespMax < (EEP_LEN_APP_SW_FINGER - MEM_DID_FLAG_LEN))
	{
		return UDS_NRC_MSG_LEN_FORMAT_INVALID;
	}

	Read_Eep_Fingerprint(pResp, (EEP_LEN_APP_SW_FINGER - MEM_DID_FLAG_LEN));

	*pRespLen = (EEP_LEN_APP_SW_FINGER - MEM_DID_FLAG_LEN);

	return 0;
}
uint8_t DidRead_programmingDateDataIdentifier(uint8_t* pResp, uint16_t* pRespLen, uint16_t RespMax)
{
	if (RespMax < (EEP_LEN_GRAMM_DATE - MEM_DID_FLAG_LEN))
	{
		return UDS_NRC_MSG_LEN_FORMAT_INVALID;
	}
	Read_Eep_GrammDate(pResp, (EEP_LEN_GRAMM_DATE - MEM_DID_FLAG_LEN));
	*pRespLen = (EEP_LEN_GRAMM_DATE - MEM_DID_FLAG_LEN);

	return 0;
}
uint8_t DidRead_v0110(uint8_t* pResp, uint16_t* pRespLen, uint16_t RespMax)
{
	if (RespMax < 1)
	{
		return UDS_NRC_MSG_LEN_FORMAT_INVALID;
	}
	if (TRUE != ecual_eeprom_read(EEP_ADDR_MFM_CNT, pResp, EEP_LEN_MFM_CNT))
	{
		pResp[0] = 0xFF;
	}
	*pRespLen = 1;

	return 0;
}
//ram
uint8_t DidRead_Custom_LogData(uint8_t* pResp, uint16_t* pRespLen, uint16_t RespMax)
{
	if (RespMax < KEEP_RAM_TOTAL_LEN)
	{
		return UDS_NRC_MSG_LEN_FORMAT_INVALID;
	}
	(void)memcpy((uint8_t*)pResp, (void*)KEEP_RAM_START_ADDR, TRY_CATCH_BUF_SIZE);
	*pRespLen = KEEP_RAM_TOTAL_LEN;

	return 0;
}
//eeprom
#ifdef LOG_FUN_ENABLE
uint8_t DidRead_Custom_LogEnSt(uint8_t* pResp, uint16_t* pRespLen, uint16_t RespMax)
{
	if (RespMax < EEP_LEN_CAN_LOG_EN)
	{
		return UDS_NRC_MSG_LEN_FORMAT_INVALID;
	}
	Read_Eep_CanLogEn(pResp, EEP_LEN_CAN_LOG_EN);
	*pRespLen = 2;

	return 0;
}
#endif


//sid 2E
uint8_t DidWrite_ReprogrammingCounte(uint8_t* pReq, uint16_t ReqLen, uint8_t* pResp, uint16_t* pRespLen, uint16_t RespMax)
{
	if (ReqLen != (EEP_LEN_GRAMM_CNT - MEM_DID_FLAG_LEN))
	{
		return UDS_NRC_MSG_LEN_FORMAT_INVALID;
	}
	*pRespLen = 0;
	return (TRUE == Write_Eep_ReGrammCnt((const uint8_t*)pReq, ReqLen)) ? 0 : 1;
}
uint8_t DidWrite_ReprogrammingAttemptCounter(uint8_t* pReq, uint16_t ReqLen, uint8_t* pResp, uint16_t* pRespLen, uint16_t RespMax)
{
	if (ReqLen != (EEP_LEN_GRAMM_RETTRY_CNT - MEM_DID_FLAG_LEN))
	{
		return UDS_NRC_MSG_LEN_FORMAT_INVALID;
	}
	*pRespLen = 0;

	return (TRUE == Write_Eep_ReAttmGrammCnt((const uint8_t*)pReq, ReqLen)) ? 0 : 1;
}
uint8_t DidWrite_ApplicationSoftwareFingerprintDataIdentifier(uint8_t* pReq, uint16_t ReqLen, uint8_t* pResp, uint16_t* pRespLen, uint16_t RespMax)
{
	if (ReqLen != (EEP_LEN_APP_SW_FINGER - MEM_DID_FLAG_LEN))
	{
		return UDS_NRC_MSG_LEN_FORMAT_INVALID;
	}
	*pRespLen = 0;

	return (TRUE == Write_Eep_Fingerprint((const uint8_t*)pReq, ReqLen)) ? 0 : 1;

}
uint8_t DidWrite_programmingDateDataIdentifier(uint8_t* pReq, uint16_t ReqLen, uint8_t* pResp, uint16_t* pRespLen, uint16_t RespMax)
{
	if (ReqLen != (EEP_LEN_GRAMM_DATE - MEM_DID_FLAG_LEN))
	{
		return UDS_NRC_MSG_LEN_FORMAT_INVALID;
	}
	*pRespLen = 0;

	return (TRUE == Write_Eep_GrammDate((const uint8_t*)pReq, ReqLen)) ? 0 : 1;

}

#ifdef LOG_FUN_ENABLE
uint8_t DidWrite_Custom_LogEnSt(uint8_t* pReq, uint16_t ReqLen, uint8_t* pResp, uint16_t* pRespLen, uint16_t RespMax)
{
	if (ReqLen != EEP_LEN_CAN_LOG_EN)
	{
		return UDS_NRC_MSG_LEN_FORMAT_INVALID;
	}
	*pRespLen = 0;

	return (TRUE == Write_Eep_CanLogEn((const uint8_t*)pReq, ReqLen)) ? 0 : 1;
}
#endif

#ifdef UDS_DID_TEST
static uint16_t  _InternalDidTestMaxLen = 8;
static uint8_t   _InternalDidTestBuf[DOCAN_BUFFER_SIZE] = { 0 };
uint8_t DidWrite_InternalTest(uint8_t* pReq, uint16_t ReqLen, uint8_t* pResp, uint16_t* pRespLen, uint16_t RespMax)
{
	if (ReqLen >= DOCAN_BUFFER_SIZE)
	{
		return UDS_NRC_MSG_LEN_FORMAT_INVALID;
	}
	_InternalDidTestMaxLen = (ReqLen % DOCAN_BUFFER_SIZE);
	if (0 == _InternalDidTestMaxLen)
	{
		_InternalDidTestMaxLen = 8;
	}
	for (uint16_t i = 0; i < ReqLen; i++)
	{
		_InternalDidTestBuf[i] = pReq[i];
	}
	*pRespLen = 0;
	return 0;

}

uint8_t DidRead_InternalTest(uint8_t* pResp, uint16_t* pRespLen, uint16_t RespMax)
{
	if (RespMax < _InternalDidTestMaxLen)
	{
		return UDS_NRC_MSG_LEN_FORMAT_INVALID;
	}
	(void)memcpy((uint8_t*)pResp, _InternalDidTestBuf, _InternalDidTestMaxLen);
	*pRespLen = _InternalDidTestMaxLen;
	return 0;
}
#endif
/*end :didproc.c*/



