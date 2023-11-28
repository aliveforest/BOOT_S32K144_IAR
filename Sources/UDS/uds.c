/********************************************************************************
* FILE: uds.c
* DSCR: XXX
*
* Date: 2022/07/18 23:00
********************************************************************************/

#include "docan.h"
#include "uds.h"
#include "sidproc.h"
#include "ridproc.h"

/* GLOBAL VARIABLES */
uint8_t gBootModelKeepExit;

typedef enum _UdsAccessT {
	UDS_ACCESS_TYPE_NONE = 0,
	UDS_ACCESS_TYPE_NORMAL = 1,
	UDS_ACCESS_TYPE_SECURITY = 2,
} UdsAccessT;

typedef void(*ApduProcFuncPtr) (UdsCbT* pCb, uint8_t* pAdata, uint16_t Len,
	uint8_t* pResp, uint16_t* pRespLen);

typedef struct _UdsProcTblT {
	ApduProcFuncPtr SidProcFunc;
	uint8_t FuncDefault;
	uint8_t FuncProgram;
	uint8_t FuncExtended;
	uint8_t PhysDefault;
	uint8_t PhysProgram;
	uint8_t PhysExtended;
} UdsProcTblT;

static UdsCbT _UdsCtrl;

#define SUPPORT_UDS_SID_NUM			12U		
const static UdsProcTblT _UdsProcTbl[SUPPORT_UDS_SID_NUM] = {
	/*--------------------------------Function-Physical
	----------------------------------De Pg Ex De Pg Ex */
	{ SidProcDiagSession,             1, 1, 1, 1, 1, 1 }, /* 0x10 */
	{ SidProcEcuReset,                1, 1, 1, 1, 1, 1 }, /* 0x11 */
	{ SidProcReadDataByIdentifier,    1, 0, 1, 1, 1, 1 }, /* 0x22 */
	{ SidProcSecurityAccess,          0, 0, 0, 0, 1, 0 }, /* 0x27 */
	{ SidProcCommCtrl,                1, 0, 1, 0, 0, 1 }, /* 0x28 */
	{ SidProcWriteDataByIdentifier,   0, 0, 0, 0, 2, 2 }, /* 0x2E */
	{ SidProcRoutineCtrl,             0, 0, 0, 1, 1, 1 }, /* 0x31 */
	{ SidProcRequestDownload,         0, 0, 0, 0, 2, 0 }, /* 0x34 */
	{ SidProcTransferData,            0, 0, 0, 0, 2, 0 }, /* 0x36 */
	{ SidProcRequestTransferExit,     0, 0, 0, 0, 2, 0 }, /* 0x37 */
	{ SidProcTesterPresent,           1, 1, 1, 1, 1, 1 }, /* 0x3E */
	{ SidProcDtcSetting,              1, 1, 1, 0, 0, 1 }, /* 0x85 */
};
const static uint8_t _UdsProcSidTbl[SUPPORT_UDS_SID_NUM] = {

	0x10,
	0x11,
	0x22,
	0x27,
	0x28,
	0x2E,
	0x31,
	0x34,
	0x36,
	0x37,
	0x3E,
	0x85,
};

static uint8_t _UdsAccessType(uint8_t Sid, DoCanAddrT AddrType, UdsSessionT SessType);


void SystemRest(void)
{
#ifdef  STAY_BOOT_MODEL_TEST
	//do nothing
#else
	DISABLE_INTERRUPTS();
	WDOG->CNT = 0;
	while (1);
#endif
}

/********************************************************************************
*  Date: 2022/07/18 22:58
*  Dscr:
********************************************************************************/
void UdsInit(uint8_t BootFlag, uint8_t Unlock)
{
	(void)memset(&_UdsCtrl, 0, sizeof(_UdsCtrl));
	if (BootFlag || Unlock){
		_UdsCtrl.Active = true;
		_UdsCtrl.SessionType = UDS_SESSION_TYPE_PROGRAM;
		//_UdsCtrl.SecurityAccessEn = true;
	}
	else{
		_UdsCtrl.SessionType = UDS_SESSION_TYPE_DEFAULT;
	}
	if (Unlock){
		_UdsCtrl.SecurityAccessEn = true;
		_UdsCtrl.RidUnlock = true;
	}

	_UdsCtrl.CommAppRxEn = true;
	_UdsCtrl.CommAppTxEn = true;
	_UdsCtrl.CommNmTxEn = true;
	_UdsCtrl.CommNmRxEn = true;
	_UdsCtrl.DtcEn = true;

	SidInit();
	RidEventReset();
}

/********************************************************************************
*  Date: 2022/07/18 22:58
*  Dscr:
********************************************************************************/
void UdsApduDispatch(DoCanAddrT AddrType, uint8_t* pReq, uint16_t ReqLen,
	uint8_t* pResp, uint16_t* pRespLen)
{
	uint8_t sidIndex = 0;
	uint8_t Sid = UDS_APCI_GET(pReq);
	UdsAccessT Access;

	/* Address Type Update */
	_UdsCtrl.AddrType = AddrType;

	/* Timer Clear */
	_UdsCtrl.Active = true;
	_UdsCtrl.IdleTimeCnt = 0;
	gBootModelKeepExit++;
	BootModelTimeoutClear();
	IF_RollBackStop(U8_TRUE);
	
	for (sidIndex = 0; sidIndex < SUPPORT_UDS_SID_NUM; sidIndex++)
	{
		if (Sid == _UdsProcSidTbl[sidIndex])
		{
			break;
		}
	}

	/* Sid Supported Check */
	if ((sidIndex >= SUPPORT_UDS_SID_NUM) ||
		(_UdsProcTbl[sidIndex].SidProcFunc == NULL))
	{
		if (AddrType != DOCAN_ADDR_TYPE_FUNCTIONAL)
		{
			/* SID Not Supported */
			pResp[0] = UDS_SID_NACK;
			pResp[1] = Sid;
			pResp[2] = UDS_NRC_SERVICE_N_SUPPORT;
			*pRespLen = 3;
		}
		else
		{
			*pRespLen = 0;
		}
		return;
	}

	/* Active Session Supported Check */
	Access = (UdsAccessT)_UdsAccessType(sidIndex, _UdsCtrl.AddrType, _UdsCtrl.SessionType);
	if (Access == UDS_ACCESS_TYPE_NONE)
	{
		/* Session Not Support */
		pResp[0] = UDS_SID_NACK;
		pResp[1] = Sid;
		pResp[2] = UDS_NRC_SERVICE_N_SUPPORT_SESS;
		*pRespLen = 3;
		return;
	}

	/* Security Check */
	if ((Access == UDS_ACCESS_TYPE_SECURITY) && !_UdsCtrl.SecurityAccessEn)
	{
		/* Security Access Deny */
		pResp[0] = UDS_SID_NACK;
		pResp[1] = Sid;
		pResp[2] = UDS_NRC_SECURITY_ACCESS_DENY;
		*pRespLen = 3;
		return;
	}

	_UdsProcTbl[sidIndex].SidProcFunc(&_UdsCtrl, pReq + 1, ReqLen - 1,
		pResp, pRespLen);
}

/********************************************************************************
*  Date: 2022/07/18 22:58
*  Dscr:
********************************************************************************/
void UdsTimerProcess(uint8_t TimePass)
{
	if (_UdsCtrl.Active)
	{
		if (UDS_SESSION_TYPE_DEFAULT != _UdsCtrl.SessionType)
			_UdsCtrl.IdleTimeCnt += TimePass;
	}

	/* Dialog Timeout */
	if (_UdsCtrl.IdleTimeCnt >= UDS_T_S3_SERVER)
	{
		//LogOcPut(OC_S3TE);
		if (UDS_SESSION_TYPE_PROGRAM == _UdsCtrl.SessionType)
		{
			//just doing at bootloader project
			SystemRest();
		}
		_UdsCtrl.Active = false;
		_UdsCtrl.IdleTimeCnt = 0;
		_UdsCtrl.RidUnlock = false;
		_UdsCtrl.SecurityAccessEn = false;
		_UdsCtrl.SessionType = UDS_SESSION_TYPE_DEFAULT;
		_UdsCtrl.CommAppTxEn = true;
		_UdsCtrl.CommAppRxEn = true;
		_UdsCtrl.CommNmTxEn = true;
		_UdsCtrl.CommNmRxEn = true;
		RidEventReset();
	}

	if (_UdsCtrl.SecurityAtttemptDelay >= TimePass)
	{
		_UdsCtrl.SecurityAtttemptDelay -= TimePass;
		if (_UdsCtrl.SecurityAtttemptDelay < TimePass)
		{
			_UdsCtrl.SecurityAtttemptDelay = 0;
			if (_UdsCtrl.SecurityAccessAttempts)
			{
				_UdsCtrl.SecurityAccessAttempts--;
			}
		}
	}
	else
	{
		_UdsCtrl.SecurityAtttemptDelay = 0;
	}
}

/********************************************************************************
*  Date: 2022/07/18 22:58
*  Dscr:
********************************************************************************/
bool UdsGetDtcSetting(void)
{
	return _UdsCtrl.DtcEn;
}

/********************************************************************************
*  Date: 2022/07/18 22:58
*  Dscr:
********************************************************************************/
bool UdsGetCommAppTxSetting(void)
{
	return _UdsCtrl.CommAppTxEn;
}

/********************************************************************************
*  Date: 2022/07/18 22:58
*  Dscr:
********************************************************************************/
bool UdsGetCommAppRxSetting(void)
{
	return _UdsCtrl.CommAppRxEn;
}

/********************************************************************************
*  Date: 2022/07/18 22:58
*  Dscr:
********************************************************************************/
bool UdsGetCommNmTxSetting(void)
{
	return _UdsCtrl.CommNmTxEn;
}

/********************************************************************************
*  Date: 2022/07/18 22:58
*  Dscr:
********************************************************************************/
bool UdsGetCommNmRxSetting(void)
{
	return _UdsCtrl.CommNmRxEn;
}

/********************************************************************************
*  Date: 2022/07/18 22:58
*  Dscr:
********************************************************************************/
uint8_t   UdsGetactiveDiagSessionMode(void)
{
	return (uint8_t)_UdsCtrl.SessionType;
}
/********************************************************************************
*  Date: 2022/07/18 22:58
*  Dscr:
********************************************************************************/
uint8_t _UdsAccessType(uint8_t Sid, DoCanAddrT AddrType, UdsSessionT SessType)
{
	uint8_t RetVal = 0;

	if (AddrType == DOCAN_ADDR_TYPE_FUNCTIONAL)
	{
		switch (SessType)
		{
		case UDS_SESSION_TYPE_DEFAULT:
			RetVal = _UdsProcTbl[Sid].FuncDefault;
			break;

		case UDS_SESSION_TYPE_PROGRAM:
			RetVal = _UdsProcTbl[Sid].FuncProgram;
			break;

		case UDS_SESSION_TYPE_EXTENDED:
			RetVal = _UdsProcTbl[Sid].FuncExtended;
			break;
		}
	}
	else
	{
		switch (SessType)
		{
		case UDS_SESSION_TYPE_DEFAULT:
			RetVal = _UdsProcTbl[Sid].PhysDefault;
			break;

		case UDS_SESSION_TYPE_PROGRAM:
			RetVal = _UdsProcTbl[Sid].PhysProgram;
			break;

		case UDS_SESSION_TYPE_EXTENDED:
			RetVal = _UdsProcTbl[Sid].PhysExtended;
			break;
		}
	}

	return RetVal;
}

/********************************************************************************
*  Date: 2022/07/18 22:58
*  Dscr:
********************************************************************************/
void UdsClearS3Time(void)
{
	_UdsCtrl.IdleTimeCnt = 0;
}
/*end :uds.c*/



