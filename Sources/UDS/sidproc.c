/********************************************************************************
* FILE: sidproc.c
* DSCR: XXX
*
* Date: 2022/07/18 22:53
********************************************************************************/
#include "DcmSup.h"
#include "uds.h"
#include "ridproc.h"
#include "didproc.h"
#include "skgen.h"
#include "crc32.h"
#include "sidproc.h"


#define DOWNLOAD_LEN_ID     	0x20		//bit7_bit4:DOWNLOAD_BLOCK_LEN use bytes 20 mean use 2 bytes
#define DOWNLOAD_PAYLOAD_LEN   	1024		//download max length every 36 service
#define DOWNLOAD_BLOCK_LEN  	(DOWNLOAD_PAYLOAD_LEN+2) //34 RESPOND //SID[1B] + blockSequenceCounter[1B] + Payload[1024B]	

//#define SID_36_RESPOND_CRC

#define BOOT_PKT_CRC_OK     1
#define BOOT_ALL_PKT_CRC_OK 0
#define BOOT_PKT_CRC_ERR    2


static uint32_t _MemAddr = 0;
static uint32_t _MemSize = 0;
static uint32_t _MemRxNum = 0;
static uint8_t  _MemBlockCounter = 0;
static uint8_t  _MemStarted = 0;

static uint8_t  _RandomReload = 0;
static uint8_t  _FingerprintUnlock = 0;
/********************************************************************************
*  Date: 2022/07/18 22:53
*  Dscr:
********************************************************************************/
void SidInit(void)
{
	_MemAddr = 0;
	_MemSize = 0;
	_MemRxNum = 0;
	_MemBlockCounter = 0;
	_MemStarted = 0;
	_RandomReload = 0;
	_FingerprintUnlock = 0;
}

/********************************************************************************
*  Date: 2022/07/18 22:53
*  Dscr:
********************************************************************************/
void SidProcDiagSession(UdsCbT* pCb, uint8_t* pAdata, uint16_t Len,
	uint8_t* pResp, uint16_t* pRespLen)
{
	uint8_t SubFunc;

	if (Len != 1)
	{
		/* Message Len Incorrect */
		pResp[0] = UDS_SID_NACK;
		pResp[1] = UDS_SID_DIAG_SESS_CTRL;
		pResp[2] = UDS_NRC_MSG_LEN_FORMAT_INVALID;
		*pRespLen = 3;
		return;
	}

	/* Get Low 7-bit */
	SubFunc = pAdata[0] & ~UDS_SUB_SUPRESSPOSRESP_BITMASK;

	//01->02 or 02->03 or !=(01 02 03)
	if (((pCb->SessionType == UDS_SUB_DEFAULT_SESSION) &&
		(SubFunc == UDS_SUB_PROGRAM_SESSION)) ||
		((pCb->SessionType == UDS_SUB_PROGRAM_SESSION) &&
			(SubFunc == UDS_SUB_EXTENDED_SESSION)) ||
		((SubFunc != UDS_SUB_DEFAULT_SESSION) &&
			(SubFunc != UDS_SUB_PROGRAM_SESSION) &&
			(SubFunc != UDS_SUB_EXTENDED_SESSION)))
	{
		if (pCb->AddrType == DOCAN_ADDR_TYPE_FUNCTIONAL)
		{
			*pRespLen = 0;
		}
		else
		{
			/* Sub-Func Not Support */
			pResp[0] = UDS_SID_NACK;
			pResp[1] = UDS_SID_DIAG_SESS_CTRL;
			pResp[2] = UDS_NRC_SUBFUNC_N_SUPPORT; /*GAC need 12*/  //UDS_NRC_SUBFUNC_N_SUPPORT_SESS;
			*pRespLen = 3;
		}
		return;
	}
	//01 -> 01
	if ((SubFunc == UDS_SUB_DEFAULT_SESSION) &&
		(pCb->SessionType == UDS_SESSION_TYPE_DEFAULT))
	{
		/* Reset all active events */
		/* Todo */
		pCb->RidUnlock = FALSE;
		pCb->SecurityAccessEn = FALSE;
		pCb->SecurityAccessAttempts = 0;
		(void)memset(pCb->SecuritySeed, 0, 4);
		RidEventReset();
	}
	//03->03
	if ((SubFunc == UDS_SUB_EXTENDED_SESSION) &&
		(pCb->SessionType == UDS_SESSION_TYPE_EXTENDED))
	{
		pCb->RidUnlock = FALSE;
		pCb->SecurityAccessEn = FALSE;
		pCb->SecurityAccessAttempts = 0;
	}

	//02->02
	if ((SubFunc == UDS_SUB_PROGRAM_SESSION) &&
		(pCb->SessionType == UDS_SUB_PROGRAM_SESSION))
	{
		pCb->RidUnlock = FALSE;
		pCb->SecurityAccessEn = FALSE;
		pCb->SecurityAccessAttempts = 0;
	}

	//01->03
	if ((SubFunc == UDS_SUB_EXTENDED_SESSION) &&
		(pCb->SessionType == UDS_SESSION_TYPE_DEFAULT))
	{
		pCb->RidUnlock = FALSE;
		pCb->SecurityAccessEn = FALSE;
		pCb->SecurityAccessAttempts = 0;
		(void)memset(pCb->SecuritySeed, 0, 4);
		pCb->CommAppRxEn = TRUE;
		pCb->CommAppTxEn = TRUE;
		pCb->CommNmTxEn = TRUE;
		pCb->CommNmRxEn = TRUE;
		pCb->DtcEn = TRUE;

		/* Reset all active events */
		RidEventReset();
	}

	/* Save New Sess */
	pCb->SessionType = (UdsSessionT)SubFunc;

	if ((pAdata[0] & UDS_SUB_SUPRESSPOSRESP_BITMASK) == 0)
	{
		pResp[0] = UDS_SID_ACK(UDS_SID_DIAG_SESS_CTRL);
		pResp[1] = SubFunc;
		pResp[2] = (uint8_t)(UDS_T_P2_CAN_SERVER >> 8);
		pResp[3] = (uint8_t)(UDS_T_P2_CAN_SERVER & 0xFF);
		pResp[4] = (uint8_t)((UDS_T_P2STAR_CAN_SERVER / 10) >> 8);
		pResp[5] = (uint8_t)((UDS_T_P2STAR_CAN_SERVER / 10) & 0xFF);
		*pRespLen = 6;
	}
	else
	{
		*pRespLen = 0;
	}
}

/********************************************************************************
*  Date: 2022/07/18 22:53
*  Dscr:
********************************************************************************/
void SidProcEcuReset(UdsCbT* pCb, uint8_t* pAdata, uint16_t Len,
	uint8_t* pResp, uint16_t* pRespLen)
{
	uint8_t SubFunc;

	if (Len != 1)
	{
		/* Message Len Incorrect */
		pResp[0] = UDS_SID_NACK;
		pResp[1] = UDS_SID_ECU_RESET;
		pResp[2] = UDS_NRC_MSG_LEN_FORMAT_INVALID;
		*pRespLen = 3;
		return;
	}

	SubFunc = pAdata[0] & ~UDS_SUB_SUPRESSPOSRESP_BITMASK;

	if ((SubFunc != UDS_SUB_HARD_RESET)
		&& (SubFunc != UDS_SUB_KEY_OFF_ON_RESET)
		&& (SubFunc != UDS_SUB_SOFT_RESET))
	{
		if (pCb->AddrType == DOCAN_ADDR_TYPE_FUNCTIONAL)
		{
			/* Sub-Func Not Support */
			pResp[0] = UDS_SID_NACK;
			pResp[1] = UDS_SID_ECU_RESET;
			pResp[2] = UDS_NRC_SUBFUNC_N_SUPPORT;
			*pRespLen = 3;
		}
		else
		{
			/* Sub-Func Not Support */
			pResp[0] = UDS_SID_NACK;
			pResp[1] = UDS_SID_ECU_RESET;
			pResp[2] = UDS_NRC_SUBFUNC_N_SUPPORT;
			*pRespLen = 3;
		}
		return;
	}

	/* CPU Reset */
	if ((pAdata[0] & UDS_SUB_SUPRESSPOSRESP_BITMASK) == 0)
	{
		//goto reset
		SetJumpInfoPara(JUMP_REASON_SID_11_0x, SubFunc, U8_TRUE);/*reset and respond*/
#if 0 /*GAC Ask for a positive response before the reset*/
		pResp[0] = UDS_SID_NACK;
		pResp[1] = UDS_SID_ECU_RESET;
		pResp[2] = UDS_NRC_REQ_PEDING;
		*pRespLen = 3;
#else
		pResp[0] = UDS_SID_ACK(UDS_SID_ECU_RESET);;
		pResp[1] = SubFunc;
		*pRespLen = 2;
#endif
	}
	else
	{
		SetJumpInfoPara(JUMP_REASON_SID_11_0x, (UDS_SUB_SUPRESSPOSRESP_BITMASK | SubFunc), U8_TRUE);/*just reset*/
		*pRespLen = 0;
	}
}

/********************************************************************************
*  Date: 2022/07/18 22:53
*  Dscr:
********************************************************************************/
void SidProcSecurityAccess(UdsCbT* pCb, uint8_t* pAdata, uint16_t Len,
	uint8_t* pResp, uint16_t* pRespLen)
{
	uint8_t SubFunc;
	uint8_t SecretKey[4];
	uint32_t Random;

	if (Len < 1)
	{
		/* Message Len Incorrect */
		pResp[0] = UDS_SID_NACK;
		pResp[1] = UDS_SID_SECURITY_ACCESS;
		pResp[2] = UDS_NRC_MSG_LEN_FORMAT_INVALID;
		*pRespLen = 3;
		return;
	}

	/* Get Low 7-bit */
	SubFunc = pAdata[0] & ~UDS_SUB_SUPRESSPOSRESP_BITMASK;

	if (((SubFunc & 0x01) &&
		(Len != 1)) ||
		(((SubFunc > 1) && ((SubFunc - 1) & 0x01)) &&
			(Len != 5)))
	{
		_RandomReload = 0;
		/* Message Len Incorrect */
		pResp[0] = UDS_SID_NACK;
		pResp[1] = UDS_SID_SECURITY_ACCESS;
		pResp[2] = UDS_NRC_MSG_LEN_FORMAT_INVALID;
		*pRespLen = 3;
		return;
	}


	switch (SubFunc)
	{
	case UDS_SUB_REQ_SEED_BT:
	{
		if (pCb->SecurityAtttemptDelay)
		{
			_RandomReload = 0;
			/* Still in Delay */
			(void)memset(pCb->SecuritySeed, 0, 4);
			pResp[0] = UDS_SID_NACK;
			pResp[1] = UDS_SID_SECURITY_ACCESS;
			pResp[2] = UDS_NRC_REQ_TIME_DLY_N_EXP;
			*pRespLen = 3;
		}
		else
		{
			/* Send Seed */
			if (!pCb->SecurityAccessEn)
			{
				if (!_RandomReload)
				{
					_RandomReload = 1;
					srand(UDS_GET_SYS_TICK() + 0x80000023);
					Random = rand();
					if (!Random)
					{
						Random = 0x80000023;
					}

					(void)memcpy(pCb->SecuritySeed, (uint8_t*)&Random, 4);
				}
				else
				{
					/*If the tester sends a consecutive �DRequest Seed��,
					the request is ac-cepted and the same seed is returned,
					but the security access failure counter is incremented.*/
					pCb->SecurityAccessAttempts++;
					if (pCb->SecurityAccessAttempts >= UDS_SECURITY_ATTEM_MAX)
					{
						pCb->SecurityAtttemptDelay = UDS_SECURITY_ATTEM_DELAY;
					}
				}
			}
			else
			{
				//has already unlock but retry request seed,will clear it
				(void)memset(pCb->SecuritySeed, 0, 4);
			}

			pResp[0] = UDS_SID_ACK(UDS_SID_SECURITY_ACCESS);
			pResp[1] = SubFunc;
			pResp[2] = pCb->SecuritySeed[0];
			pResp[3] = pCb->SecuritySeed[1];
			pResp[4] = pCb->SecuritySeed[2];
			pResp[5] = pCb->SecuritySeed[3];
			*pRespLen = 6;
		}
	}
	break;

	case UDS_SUB_SEND_KEY_BT:
	{
		_RandomReload = 0;
		if (pCb->SecurityAccessAttempts < UDS_SECURITY_ATTEM_MAX)
		{
			if ((pCb->SecuritySeed[0] == 0) &&
				(pCb->SecuritySeed[1] == 0) &&
				(pCb->SecuritySeed[2] == 0) &&
				(pCb->SecuritySeed[3] == 0))
			{
				/* Seed has not been gen'd */
				pResp[0] = UDS_SID_NACK;
				pResp[1] = UDS_SID_SECURITY_ACCESS;
				pResp[2] = UDS_NRC_REQ_SEQ_ERROR;
				*pRespLen = 3;
			}
			else
			{
				SecrectKeyGen(pCb->SecuritySeed, SecretKey, UDS_SUB_REQ_SEED_BT);
				if (memcmp((const uint8_t*)SecretKey, (const uint8_t*)&pAdata[1], 4) == 0)
				{
					/* Match */
					pCb->SecurityAccessAttempts = 0;
					pCb->SecurityAccessEn = TRUE;
					pResp[0] = UDS_SID_ACK(UDS_SID_SECURITY_ACCESS);
					pResp[1] = SubFunc;
					*pRespLen = 2;
				}
				else
				{
					pCb->SecurityAccessEn = FALSE;
					pCb->SecurityAccessAttempts++;
					if (pCb->SecurityAccessAttempts >= UDS_SECURITY_ATTEM_MAX)
					{
						pCb->SecurityAtttemptDelay = UDS_SECURITY_ATTEM_DELAY;
						pResp[0] = UDS_SID_NACK;
						pResp[1] = UDS_SID_SECURITY_ACCESS;
						pResp[2] = UDS_NRC_EXCEED_NUM_OF_ATTEM;

						*pRespLen = 3;
					}
					else
					{
						pResp[0] = UDS_SID_NACK;
						pResp[1] = UDS_SID_SECURITY_ACCESS;
						pResp[2] = UDS_NRC_INVALID_KEY;
						*pRespLen = 3;
					}
				}
				//if  has already use this seed ,and clear it
				(void)memset(pCb->SecuritySeed, 0, 4);
			}

		}
		else
		{
			if ((pCb->SecuritySeed[0] == 0) &&
				(pCb->SecuritySeed[1] == 0) &&
				(pCb->SecuritySeed[2] == 0) &&
				(pCb->SecuritySeed[3] == 0))
			{
				/* Seed has not been gen'd */
				pResp[0] = UDS_SID_NACK;
				pResp[1] = UDS_SID_SECURITY_ACCESS;
				pResp[2] = UDS_NRC_REQ_SEQ_ERROR;
				*pRespLen = 3;
			}
			else
			{
				pResp[0] = UDS_SID_NACK;
				pResp[1] = UDS_SID_SECURITY_ACCESS;
				pResp[2] = UDS_NRC_EXCEED_NUM_OF_ATTEM;
				*pRespLen = 3;
			}
		}
	}
	break;

	default:
	{
		if (pCb->AddrType == DOCAN_ADDR_TYPE_FUNCTIONAL)
		{
			*pRespLen = 0;
		}
		else
		{
			/* Sub-Func Not Support */
			pResp[0] = UDS_SID_NACK;
			pResp[1] = UDS_SID_SECURITY_ACCESS;
			pResp[2] = UDS_NRC_SUBFUNC_N_SUPPORT;
			*pRespLen = 3;
		}
	}
	break;
	}
}

/********************************************************************************
*  Date: 2022/07/18 22:53
*  Dscr:
********************************************************************************/
void SidProcCommCtrl(UdsCbT* pCb, uint8_t* pAdata, uint16_t Len,
	uint8_t* pResp, uint16_t* pRespLen)
{
	uint8_t SubFunc;

	if (Len != 2)
	{
		/* Message Len Incorrect */
		pResp[0] = UDS_SID_NACK;
		pResp[1] = UDS_SID_COMM_CTRL;
		pResp[2] = UDS_NRC_MSG_LEN_FORMAT_INVALID;
		*pRespLen = 3;
		return;
	}

	/* Get Low 7-bit */
	SubFunc = pAdata[0] & ~UDS_SUB_SUPRESSPOSRESP_BITMASK;

	switch (SubFunc)
	{
	case UDS_SUB_EN_RX_EN_TX:
		switch (pAdata[1])
		{
		case UDS_COMM_TYPE_APP:
			pCb->CommAppRxEn = TRUE;
			pCb->CommAppTxEn = TRUE;
			if ((pAdata[0] & UDS_SUB_SUPRESSPOSRESP_BITMASK) == 0)
			{
				pResp[0] = UDS_SID_ACK(UDS_SID_COMM_CTRL);
				pResp[1] = SubFunc;
				*pRespLen = 2;
			}
			break;

		case UDS_COMM_TYPE_NM:
			pCb->CommNmRxEn = TRUE;
			pCb->CommNmTxEn = TRUE;
			if ((pAdata[0] & UDS_SUB_SUPRESSPOSRESP_BITMASK) == 0)
			{
				pResp[0] = UDS_SID_ACK(UDS_SID_COMM_CTRL);
				pResp[1] = SubFunc;
				*pRespLen = 2;
			}
			break;

		case UDS_COMM_TYPE_ALL:
			pCb->CommAppRxEn = TRUE;
			pCb->CommAppTxEn = TRUE;
			pCb->CommNmRxEn = TRUE;
			pCb->CommNmTxEn = TRUE;
			if ((pAdata[0] & UDS_SUB_SUPRESSPOSRESP_BITMASK) == 0)
			{
				pResp[0] = UDS_SID_ACK(UDS_SID_COMM_CTRL);
				pResp[1] = SubFunc;
				*pRespLen = 2;
			}
			break;

		default:
			/* Request Out Of Range */
			pResp[0] = UDS_SID_NACK;
			pResp[1] = UDS_SID_COMM_CTRL;
			pResp[2] = UDS_NRC_REQ_OUT_OF_RANGE;
			*pRespLen = 3;
			break;
		}
		break;

	case UDS_SUB_DIS_RX_DIS_TX:
		switch (pAdata[1])
		{
		case UDS_COMM_TYPE_APP:
			pCb->CommAppRxEn = FALSE;
			pCb->CommAppTxEn = FALSE;
			if ((pAdata[0] & UDS_SUB_SUPRESSPOSRESP_BITMASK) == 0)
			{
				pResp[0] = UDS_SID_ACK(UDS_SID_COMM_CTRL);
				pResp[1] = SubFunc;
				*pRespLen = 2;
			}
			break;

		case UDS_COMM_TYPE_NM:
			pCb->CommNmRxEn = FALSE;
			pCb->CommNmTxEn = FALSE;
			if ((pAdata[0] & UDS_SUB_SUPRESSPOSRESP_BITMASK) == 0)
			{
				pResp[0] = UDS_SID_ACK(UDS_SID_COMM_CTRL);
				pResp[1] = SubFunc;
				*pRespLen = 2;
			}
			break;

		case UDS_COMM_TYPE_ALL:
			pCb->CommAppRxEn = FALSE;
			pCb->CommAppTxEn = FALSE;
			pCb->CommNmRxEn = FALSE;
			pCb->CommNmTxEn = FALSE;
			if ((pAdata[0] & UDS_SUB_SUPRESSPOSRESP_BITMASK) == 0)
			{
				pResp[0] = UDS_SID_ACK(UDS_SID_COMM_CTRL);
				pResp[1] = SubFunc;
				*pRespLen = 2;
			}
			break;

		default:
			/* Request Out Of Range */
			pResp[0] = UDS_SID_NACK;
			pResp[1] = UDS_SID_COMM_CTRL;
			pResp[2] = UDS_NRC_REQ_OUT_OF_RANGE;
			*pRespLen = 3;
			break;
		}
		break;

	default:
		if (pCb->AddrType == DOCAN_ADDR_TYPE_FUNCTIONAL)
		{
			*pRespLen = 0;
		}
		else
		{
			/* Sub-Func Not Support */
			pResp[0] = UDS_SID_NACK;
			pResp[1] = UDS_SID_COMM_CTRL;
			pResp[2] = UDS_NRC_SUBFUNC_N_SUPPORT;
			*pRespLen = 3;
		}
		break;
	}
}

/********************************************************************************
*  Date: 2022/07/18 22:52
*  Dscr:
********************************************************************************/
void SidProcTesterPresent(UdsCbT* pCb, uint8_t* pAdata, uint16_t Len,
	uint8_t* pResp, uint16_t* pRespLen)
{
	uint8_t SubFunc;

	if (Len != 1)
	{
		/* Message Len Incorrect */
		pResp[0] = UDS_SID_NACK;
		pResp[1] = UDS_SID_TESTER_PRESENT;
		pResp[2] = UDS_NRC_MSG_LEN_FORMAT_INVALID;
		*pRespLen = 3;
		return;
	}

	/* Get Low 7-bit */
	SubFunc = pAdata[0] & ~UDS_SUB_SUPRESSPOSRESP_BITMASK;
	if (gBootModelKeepExit)
	{
		gBootModelKeepExit--;
	}

	if (SubFunc != 0)
	{
		if (pCb->AddrType == DOCAN_ADDR_TYPE_FUNCTIONAL)
		{
			*pRespLen = 0;
		}
		else
		{
			/* Sub-Func Not Support */
			pResp[0] = UDS_SID_NACK;
			pResp[1] = UDS_SID_TESTER_PRESENT;
			pResp[2] = UDS_NRC_SUBFUNC_N_SUPPORT;
			*pRespLen = 3;
		}
		return;
	}

	if ((pAdata[0] & UDS_SUB_SUPRESSPOSRESP_BITMASK) == 0)
	{
		pResp[0] = UDS_SID_ACK(UDS_SID_TESTER_PRESENT);
		pResp[1] = 0;
		*pRespLen = 2;
	}
	else
	{
		*pRespLen = 0;
	}
}

/********************************************************************************
*  Date: 2022/07/18 22:52
*  Dscr:
********************************************************************************/
void SidProcDtcSetting(UdsCbT* pCb, uint8_t* pAdata, uint16_t Len,
	uint8_t* pResp, uint16_t* pRespLen)
{
	uint8_t SubFunc;

	if (Len < 1)
	{
		/* Message Len Incorrect */
		pResp[0] = UDS_SID_NACK;
		pResp[1] = UDS_SID_CTRL_DTC_SETTING;
		pResp[2] = UDS_NRC_MSG_LEN_FORMAT_INVALID;
		*pRespLen = 3;
		return;
	}

	SubFunc = pAdata[0];

	if (((SubFunc == UDS_SUB_DTC_ON) ||
		(SubFunc == UDS_SUB_DTC_OFF)) &&
		(Len != 1))
	{
		/* Message Len Incorrect */
		pResp[0] = UDS_SID_NACK;
		pResp[1] = UDS_SID_CTRL_DTC_SETTING;
		pResp[2] = UDS_NRC_MSG_LEN_FORMAT_INVALID;
		*pRespLen = 3;
		return;
	}

	switch (SubFunc)
	{
	case UDS_SUB_DTC_ON:
		pCb->DtcEn = TRUE;
		pResp[0] = UDS_SID_ACK(UDS_SID_CTRL_DTC_SETTING);
		pResp[1] = SubFunc;
		*pRespLen = 2;
		break;

	case UDS_SUB_DTC_OFF:
		pCb->DtcEn = FALSE;
		pResp[0] = UDS_SID_ACK(UDS_SID_CTRL_DTC_SETTING);
		pResp[1] = SubFunc;
		*pRespLen = 2;
		break;

	default:
		if (pCb->AddrType == DOCAN_ADDR_TYPE_FUNCTIONAL)
		{
			*pRespLen = 0;
		}
		else
		{
			/* Sub-Func Not Support */
			pResp[0] = UDS_SID_NACK;
			pResp[1] = UDS_SID_TESTER_PRESENT;
			pResp[2] = UDS_NRC_SUBFUNC_N_SUPPORT;
			*pRespLen = 3;
		}
		break;
	}
}

/********************************************************************************
*  Date: 2022/07/18 22:52
*  Dscr:
********************************************************************************/
void SidProcReadDataByIdentifier(UdsCbT* pCb, uint8_t* pAdata, uint16_t Len,
	uint8_t* pResp, uint16_t* pRespLen)
{
	uint16_t Did;
	uint16_t ReqRdIdx = 0, RespTot = 0, RespLen;
	uint8_t RetVal;


	if ((Len < 2) || ((Len & 1) == 1) || (Len > 10))/*read 22 limit 5 DID every time,so len <=10*/
	{
		/* Message Len Incorrect */
		pResp[0] = UDS_SID_NACK;
		pResp[1] = UDS_SID_READ_DATA;
		pResp[2] = UDS_NRC_MSG_LEN_FORMAT_INVALID;
		*pRespLen = 3;
		return;
	}

	pResp[0] = UDS_SID_ACK(UDS_SID_READ_DATA);
	RespTot = 1;

	while (Len)
	{
		/* Get DID */
		Did = ((uint16_t)pAdata[ReqRdIdx] << 8) | pAdata[ReqRdIdx + 1];
		pResp[RespTot] = pAdata[ReqRdIdx];
		pResp[RespTot + 1] = pAdata[ReqRdIdx + 1];
		RespTot += 2;
		ReqRdIdx += 2;
		switch (Did)
		{
		case UDS_DID_READ_ECU_HW:
			RetVal = DidRead_GACECUHardwareVersionNumberDataIdentifier(&pResp[RespTot], &RespLen,
				DOCAN_BUFFER_SIZE - RespTot);
			break;
		case UDS_DID_READ_BT_SW:
			RetVal = DidRead_bootSoftwareIdentification(&pResp[RespTot], &RespLen,
				DOCAN_BUFFER_SIZE - RespTot);
			break;
		case UDS_DID_READ_PN:
			RetVal = DidRead_GACSparePartNumberDataIdentifier(&pResp[RespTot], &RespLen,
				DOCAN_BUFFER_SIZE - RespTot);
			break;
		case UDS_DID_READ_ECU_SW:
			RetVal = DidRead_GACECUSoftwareVersionNumberDataIdentifier(&pResp[RespTot], &RespLen,
				DOCAN_BUFFER_SIZE - RespTot);
			break;
		case UDS_DID_READ_F186:
			RetVal = DidRead_DiagSessionMode(&pResp[RespTot], &RespLen,
				DOCAN_BUFFER_SIZE - RespTot);
			break;
		case UDS_DID_READ_vF181:
			RetVal = DidRead_vF181(&pResp[RespTot], &RespLen,
				DOCAN_BUFFER_SIZE - RespTot);
			break;

		case UDS_DID_READ_GRAMM_CNT:
			RetVal = DidRead_ReprogrammingCounter(&pResp[RespTot], &RespLen,
				DOCAN_BUFFER_SIZE - RespTot);
			break;
		case UDS_DID_READ_GRAMM_RETTRY_CNT:
			RetVal = DidRead_ReprogrammingAttemptCounter(&pResp[RespTot], &RespLen,
				DOCAN_BUFFER_SIZE - RespTot);
			break;
		case UDS_DID_READ_APP_SW_FINGER:
			RetVal = DidRead_ApplicationSoftwareFingerprintDataIdentifier(&pResp[RespTot], &RespLen,
				DOCAN_BUFFER_SIZE - RespTot);
			break;
		case UDS_DID_READ_GRAMM_DATE:
			RetVal = DidRead_programmingDateDataIdentifier(&pResp[RespTot], &RespLen,
				DOCAN_BUFFER_SIZE - RespTot);
			break;
		case UDS_DID_READ_v0110:
			RetVal = DidRead_v0110(&pResp[RespTot], &RespLen,
				DOCAN_BUFFER_SIZE - RespTot);
			break;
		case UDS_DID_READ_CUSTOM_BT_ST:
			RetVal = DidRead_Custom_LogData(&pResp[RespTot], &RespLen,
				DOCAN_BUFFER_SIZE - RespTot);
			break;
#ifdef LOG_FUN_ENABLE
		case UDS_DID_READ_CUSTOM_LOG_ST:
			RetVal = DidRead_Custom_LogEnSt(&pResp[RespTot], &RespLen,
				DOCAN_BUFFER_SIZE - RespTot);
			break;
#endif

#ifdef UDS_DID_TEST
		case  UDS_DID_TEST_INTERNAL:
			RetVal = DidRead_InternalTest(&pResp[RespTot], &RespLen,
				DOCAN_BUFFER_SIZE - RespTot);
			break;
#endif
#if 0
		case UDS_DID_READ_FINGER_PRINT:
			RetVal = DidReadFingerprintDataidentifier(&pResp[RespTot], &RespLen,
				DOCAN_BUFFER_SIZE - RespTot);
			break;
		case 0xF193:
			(void)memcpy((uint8_t*)&pResp[RespTot], (void*)(APP_SW_ADDR), (APP_SW_LEN));
			RespLen = APP_SW_LEN;
			RetVal = 0;
			break;
		case 0xF195:
			(void)memcpy((uint8_t*)&pResp[RespTot], (void*)(APP_HW_ADDR), (APP_HW_LEN));
			RespLen = APP_HW_LEN;
			RetVal = 0;
			break;
#endif
		default:
			RetVal = UDS_NRC_REQ_OUT_OF_RANGE;
			break;
		}

		if (RetVal == 0)
		{
			RespTot += RespLen;
		}
		else
		{
			if (pCb->AddrType == DOCAN_ADDR_TYPE_FUNCTIONAL)
			{
				*pRespLen = 0;
			}
			else
			{
				/* Out of Range */
				pResp[0] = UDS_SID_NACK;
				pResp[1] = UDS_SID_READ_DATA;
				pResp[2] = RetVal;
				*pRespLen = 3;
			}
			return;
		}
		Len -= 2;
	}

	*pRespLen = RespTot;
}

/********************************************************************************
*  Date: 2022/07/18 22:52
*  Dscr:
********************************************************************************/
void SidProcWriteDataByIdentifier(UdsCbT* pCb, uint8_t* pAdata, uint16_t Len,
	uint8_t* pResp, uint16_t* pRespLen)
{
	uint16_t Did;
	uint16_t RespTot = 0, RespLen;
	uint8_t RetVal;

	if (Len < 3)
	{
		/* Message Len Incorrect */
		pResp[0] = UDS_SID_NACK;
		pResp[1] = UDS_SID_WRITE_DATA;
		pResp[2] = UDS_NRC_MSG_LEN_FORMAT_INVALID;
		*pRespLen = 3;
		return;
	}

	pResp[0] = UDS_SID_ACK(UDS_SID_WRITE_DATA);
	RespTot = 1;

	/* Get DID */
	Did = ((uint16_t)pAdata[0] << 8) | pAdata[1];
	pResp[RespTot] = pAdata[0];
	pResp[RespTot + 1] = pAdata[1];
	RespTot += 2;

	switch (Did)
	{
	case UDS_DID_READ_APP_SW_FINGER:
		RetVal = DidWrite_ApplicationSoftwareFingerprintDataIdentifier(&pAdata[2], Len - 2,
			&pResp[RespTot], &RespLen, DOCAN_BUFFER_SIZE - RespTot);
		if (0 == RetVal)
		{
			_FingerprintUnlock = U8_TRUE;
		}
		break;

	case  UDS_DID_READ_GRAMM_DATE:
		RetVal = DidWrite_programmingDateDataIdentifier(&pAdata[2], Len - 2,
			&pResp[RespTot], &RespLen, DOCAN_BUFFER_SIZE - RespTot);
		break;

		// case  UDS_DID_READ_GRAMM_CNT:
		// 	RetVal = DidWrite_ReprogrammingCounte(&pAdata[2], Len - 2,
		// 		&pResp[RespTot], &RespLen, DOCAN_BUFFER_SIZE - RespTot);
		// 	break;

		// case  UDS_DID_READ_GRAMM_RETTRY_CNT:
		// 	RetVal = DidWrite_ReprogrammingAttemptCounter(&pAdata[2], Len - 2,
		// 		&pResp[RespTot], &RespLen, DOCAN_BUFFER_SIZE - RespTot);
		// 	break;

#ifdef LOG_FUN_ENABLE	
	case UDS_DID_READ_CUSTOM_LOG_ST:
		RetVal = DidWrite_Custom_LogEnSt(&pAdata[2], Len - 2,
			&pResp[RespTot], &RespLen, DOCAN_BUFFER_SIZE - RespTot);
		break;
#endif

#ifdef UDS_DID_TEST
	case  UDS_DID_TEST_INTERNAL:
		RetVal = DidWrite_InternalTest(&pAdata[2], Len - 2,
			&pResp[RespTot], &RespLen, DOCAN_BUFFER_SIZE - RespTot);
		break;
#endif
	default:
		/* Out of Range */
		pResp[0] = UDS_SID_NACK;
		pResp[1] = UDS_SID_WRITE_DATA;
		pResp[2] = UDS_NRC_REQ_OUT_OF_RANGE;
		*pRespLen = 3;
		return;
		break;
	}

	if (RetVal == 0)
	{
		RespTot += RespLen;
	}
	else
	{
		if (pCb->AddrType == DOCAN_ADDR_TYPE_FUNCTIONAL)
		{
			*pRespLen = 0;
		}
		else
		{
			/* Message Len Incorrect */
			pResp[0] = UDS_SID_NACK;
			pResp[1] = UDS_SID_WRITE_DATA;
			pResp[2] = RetVal;
			*pRespLen = 3;
		}
		return;
	}

	*pRespLen = RespTot;
}

/********************************************************************************
*  Date: 2022/07/18 22:52
*  Dscr:
********************************************************************************/
void SidProcRoutineCtrl(UdsCbT* pCb, uint8_t* pAdata, uint16_t Len,
	uint8_t* pResp, uint16_t* pRespLen)
{
	uint16_t Rid;
	uint8_t AddressLen = 0, SizeLen = 0, i = 0;
	uint32_t Address = 0, Size = 0;

	if (Len < 3)
	{
		/* Message Len Incorrect */
		pResp[0] = UDS_SID_NACK;
		pResp[1] = UDS_SID_ROUTINE_CTRL;
		pResp[2] = UDS_NRC_MSG_LEN_FORMAT_INVALID;
		*pRespLen = 3;
		return;
	}

	Rid = ((uint16_t)pAdata[1] << 8) | pAdata[2];

	switch (pAdata[0]) /* Sub Function */
	{
	case UDS_SUB_START_ROUTINE:
		switch (Rid)
		{
		case UDS_RID_FLASH_ERASE:
			if (Len > 4)
			{
				if (pCb->SecurityAccessEn)
				{
					//if has address info data=31 01 FF 00 44 XX XX XX XX YY YY YY YY
					AddressLen = pAdata[3] & 0xF;
					SizeLen = (pAdata[3] >> 4);

					if (Len < (2 + AddressLen + SizeLen))
					{
						/* Message Len Incorrect */
						pResp[0] = UDS_SID_NACK;
						pResp[1] = UDS_SID_REQ_DOWNLOAD;
						pResp[2] = UDS_NRC_MSG_LEN_FORMAT_INVALID;
						*pRespLen = 3;
						return;
					}

					if ((AddressLen > 4) /* Invalid Address Len */ ||
						(SizeLen > 4) /* Invalid Size Len */)
					{
						pResp[0] = UDS_SID_NACK;
						pResp[1] = UDS_SID_REQ_DOWNLOAD;
						pResp[2] = UDS_NRC_REQ_OUT_OF_RANGE;
						*pRespLen = 3;
						return;
					}

					Address = 0;
					for (i = 0; i < AddressLen; i++)
					{
						Address |= ((uint32_t)pAdata[4 + i] << (8 * (AddressLen - 1 - i)));
					}

					Size = 0;
					for (i = 0; i < SizeLen; i++)
					{
						Size |= ((uint32_t)pAdata[4 + AddressLen + i] << (8 * (SizeLen - 1 - i)));
					}

					//erase runing
					RidStartProcFlashErase(pCb, pAdata, pResp, pRespLen, Address, Size);
				}
				else
				{
					/* Security Access Deny */
					pResp[0] = UDS_SID_NACK;
					pResp[1] = UDS_SID_ROUTINE_CTRL;
					pResp[2] = UDS_NRC_SECURITY_ACCESS_DENY;
					*pRespLen = 3;
					return;
				}
			}
			else
			{
				/* Message Len Incorrect */
				pResp[0] = UDS_SID_NACK;
				pResp[1] = UDS_SID_ROUTINE_CTRL;
				pResp[2] = UDS_NRC_MSG_LEN_FORMAT_INVALID;
				*pRespLen = 3;
				return;
			}
			break;

		case UDS_RID_CRC_CHECK:
			if (_MemStarted)
			{
				/*hasn't exit transport*/
				pResp[0] = UDS_SID_NACK;
				pResp[1] = UDS_SID_ROUTINE_CTRL;
				pResp[2] = UDS_NRC_REQ_SEQ_ERROR;
				*pRespLen = 3;
			}
			else
			{
				RidStartProcCheckCrc(pCb, pAdata, pResp, pRespLen);
			}
			break;

		case UDS_RID_PROGRAMM_DP:
			RidStartCheckProgrammingDependency(pCb, pAdata, pResp, pRespLen);
			break;

		case UDS_RID_CONSITION_CHECK:
			CheckProgrammingPreconditions(pCb, pAdata, pResp, pRespLen);
			break;

		case UDS_RID_ROLLBACK:
			if (pCb->SecurityAccessEn)
			{
				RidStartRollback(pCb, pAdata, pResp, pRespLen);
			}
			else
			{
				/* Security Access Deny */
				pResp[0] = UDS_SID_NACK;
				pResp[1] = UDS_SID_ROUTINE_CTRL;
				pResp[2] = UDS_NRC_SECURITY_ACCESS_DENY;
				*pRespLen = 3;
				return;
			}
			break;

		case UDS_RID_SYNC2BACKAREA:
			if (pCb->SecurityAccessEn)
			{
				RidStartSync2BackupArea(pCb, pAdata, pResp, pRespLen);
			}
			else
			{
				/* Security Access Deny */
				pResp[0] = UDS_SID_NACK;
				pResp[1] = UDS_SID_ROUTINE_CTRL;
				pResp[2] = UDS_NRC_SECURITY_ACCESS_DENY;
				*pRespLen = 3;
				return;
			}
			break;

		default:
			/* Out of Range */
			pResp[0] = UDS_SID_NACK;
			pResp[1] = UDS_SID_ROUTINE_CTRL;
			pResp[2] = UDS_NRC_REQ_OUT_OF_RANGE;
			*pRespLen = 3;
			break;
		}
		break;

	case UDS_SUB_REQUEST_ROUTINE_RESULTS:
		switch (Rid)
		{
		case UDS_RID_FLASH_ERASE:
			RidResultProcFlashErase(pCb, pAdata, pResp, pRespLen);
			break;
		case UDS_RID_ROLLBACK:
			RidResultRollback(pCb, pAdata, pResp, pRespLen);
			break;
		case UDS_RID_SYNC2BACKAREA:
			RidResultSync2BackupArea(pCb, pAdata, pResp, pRespLen);
			break;
		default:
			/* Out of Range */
			pResp[0] = UDS_SID_NACK;
			pResp[1] = UDS_SID_ROUTINE_CTRL;
			pResp[2] = UDS_NRC_REQ_OUT_OF_RANGE;
			*pRespLen = 3;
			break;
		}
		break;

	case UDS_SUB_STOP_ROUTINE:
	default:
		/* Sub-Func Not Support */
		pResp[0] = UDS_SID_NACK;
		pResp[1] = UDS_SID_ROUTINE_CTRL;
		pResp[2] = UDS_NRC_SUBFUNC_N_SUPPORT;
		*pRespLen = 3;
		break;
	}

}

/********************************************************************************
*  Date: 2022/07/18 22:52
*  Dscr:
********************************************************************************/
void SidProcRequestDownload(UdsCbT* pCb, uint8_t* pAdata, uint16_t Len,
	uint8_t* pResp, uint16_t* pRespLen)
{
	uint8_t AddressLen, SizeLen, i;

	if (Len < 4)
	{
		/* Message Len Incorrect */
		pResp[0] = UDS_SID_NACK;
		pResp[1] = UDS_SID_REQ_DOWNLOAD;
		pResp[2] = UDS_NRC_MSG_LEN_FORMAT_INVALID;
		*pRespLen = 3;
		return;
	}

	AddressLen = pAdata[1] & 0xF;
	SizeLen = (pAdata[1] >> 4);

	if (Len < (2 + AddressLen + SizeLen))
	{
		/* Message Len Incorrect */
		pResp[0] = UDS_SID_NACK;
		pResp[1] = UDS_SID_REQ_DOWNLOAD;
		pResp[2] = UDS_NRC_MSG_LEN_FORMAT_INVALID;
		*pRespLen = 3;
		return;
	}

	if ((pAdata[0] != 0) /* Invalid Format */ ||
		(AddressLen > 4) /* Invalid Address Len */ ||
		(SizeLen > 4) /* Invalid Size Len */)
	{
		pResp[0] = UDS_SID_NACK;
		pResp[1] = UDS_SID_REQ_DOWNLOAD;
		pResp[2] = UDS_NRC_REQ_OUT_OF_RANGE;
		*pRespLen = 3;
		return;
	}

	_MemAddr = 0;
	for (i = 0; i < AddressLen; i++)
	{
		_MemAddr |= ((uint32_t)pAdata[2 + i] << (8 * (AddressLen - 1 - i)));
	}

	_MemSize = 0;
	for (i = 0; i < SizeLen; i++)
	{
		_MemSize |= ((uint32_t)pAdata[2 + AddressLen + i] << (8 * (SizeLen - 1 - i)));
	}

	if (isDrvCodeFls(_MemAddr, _MemSize))
	{
		if (U8_TRUE != _FingerprintUnlock)
		{
			pResp[0] = UDS_SID_NACK;
			pResp[1] = UDS_SID_REQ_DOWNLOAD;
			pResp[2] = UDS_NRC_UL_DL_N_ACCEPTED;
			*pRespLen = 3;
			return;
		}
		else
		{
			Set_DowloadAddressType(FLASH_TYPE_DRIVER_CODE);
		}
	}
	else if (isAppCodeFls(_MemAddr, _MemSize))
	{
		if (U8_TRUE != FlsEraseComplete())
		{
			pResp[0] = UDS_SID_NACK;
			pResp[1] = UDS_SID_REQ_DOWNLOAD;
			pResp[2] = UDS_NRC_UL_DL_N_ACCEPTED;
			*pRespLen = 3;
			return;
		}
		else
		{
			Set_DowloadAddressType(FLASH_TYPE_APP_CODE);
		}
	}
	else
	{
		//address invalid
		pResp[0] = UDS_SID_NACK;
		pResp[1] = UDS_SID_REQ_DOWNLOAD;
		pResp[2] = UDS_NRC_REQ_OUT_OF_RANGE;
		*pRespLen = 3;
		return;
	}

	_MemBlockCounter = 0;
	_MemStarted = 1;
	_MemRxNum = 0;
	/* Reply Block Len */
	pResp[0] = UDS_SID_ACK(UDS_SID_REQ_DOWNLOAD);
	pResp[1] = DOWNLOAD_LEN_ID;
#if 1
	pResp[2] = (DOWNLOAD_BLOCK_LEN >> 8) & 0xFF;
	pResp[3] = DOWNLOAD_BLOCK_LEN & 0xFF;
	*pRespLen = 4;
#else
	pResp[2] = (DOWNLOAD_BLOCK_LEN >> 24) & 0xFF;
	pResp[3] = (DOWNLOAD_BLOCK_LEN >> 16) & 0xFF;
	pResp[4] = (DOWNLOAD_BLOCK_LEN >> 8) & 0xFF;
	pResp[5] = DOWNLOAD_BLOCK_LEN & 0xFF;
	*pRespLen = 6;
#endif
}

/********************************************************************************
*  Date: 2022/07/18 22:52
*  Dscr:
********************************************************************************/
void SidProcTransferData(UdsCbT* pCb, uint8_t* pAdata, uint16_t Len,
	uint8_t* pResp, uint16_t* pRespLen)
{
	uint16_t DownloadSize;
	uint8_t RetVal;
	uint32_t CrcValue = 0;

	//pAdata:byte0[sn] byte1~(Len-1)[data]
	DownloadSize = Len - 1;//payload
	if ((Len < 2) || (DownloadSize > DOWNLOAD_PAYLOAD_LEN) ||
		(_MemRxNum >= _MemSize))
	{
		/* Message Len Incorrect */
		pResp[0] = UDS_SID_NACK;
		pResp[1] = UDS_SID_TRANSFER_DATA;
		pResp[2] = UDS_NRC_MSG_LEN_FORMAT_INVALID;
		*pRespLen = 3;
		return;
	}

	if (!(0x01 & _MemStarted))
	{
		pResp[0] = UDS_SID_NACK;
		pResp[1] = UDS_SID_TRANSFER_DATA;
		pResp[2] = UDS_NRC_REQ_SEQ_ERROR;
		*pRespLen = 3;
		return;
	}

	if (_MemBlockCounter == pAdata[0])
	{
		pResp[0] = UDS_SID_ACK(UDS_SID_TRANSFER_DATA);
		pResp[1] = _MemBlockCounter;
		*pRespLen = 2;
		return;
	}

	_MemBlockCounter++;
	if (_MemBlockCounter != pAdata[0])
	{
		pResp[0] = UDS_SID_NACK;
		pResp[1] = UDS_SID_TRANSFER_DATA;
		pResp[2] = UDS_NRC_WRONG_BLOCK_SEQ_CNT;
		*pRespLen = 3;
		return;
	}


	if ((U8_TRUE != FlsDrvDownloadComplete())
		&& (isDrvCodeFls((_MemAddr + _MemRxNum), DownloadSize)))
	{
		UDS_FED_WDG();
		RetVal = FlashWriteHandle(FLASH_TYPE_DRIVER_CODE, (_MemAddr + _MemRxNum), (uint8_t*)&pAdata[1], DownloadSize, (uint32_t*)&CrcValue);
		if (!RetVal)
		{
			/* Download Not Accepted */
			pResp[0] = UDS_SID_NACK;
			pResp[1] = UDS_SID_TRANSFER_DATA;
			pResp[2] = UDS_NRC_GENERAL_PROG_FAILURE;
			*pRespLen = 3;
			return;
		}
		_MemRxNum += DownloadSize;
		//respond
		pResp[0] = UDS_SID_ACK(UDS_SID_TRANSFER_DATA);
		pResp[1] = _MemBlockCounter;
#ifdef SID_36_RESPOND_CRC  //If the 36 service needs to feedback the result of the crc
		pResp[2] = (CrcValue >> 24);
		pResp[3] = (CrcValue >> 16);
		pResp[4] = (CrcValue >> 8);
		pResp[5] = (CrcValue & 0xFF);
		*pRespLen = 6;
#else
		* pRespLen = 2;
#endif		
	}
	else if ((U8_TRUE == FlsDrvDownloadComplete())
		&& (isAppCodeFls((_MemAddr + _MemRxNum), DownloadSize)))
	{
		//if  flash driver has download complete
		UDS_FED_WDG();
		UdsNrc78Set_WhileLoopLocked(UDS_SID_TRANSFER_DATA);
		RetVal = FlashWriteHandle(FLASH_TYPE_APP_CODE, (_MemAddr + _MemRxNum), (uint8_t*)&pAdata[1], DownloadSize, (uint32_t*)&CrcValue);
		if (!RetVal)
		{
			/* Download Not Accepted */
			pResp[0] = UDS_SID_NACK;
			pResp[1] = UDS_SID_TRANSFER_DATA;
			pResp[2] = UDS_NRC_GENERAL_PROG_FAILURE;
			*pRespLen = 3;
			return;
		}
		_MemRxNum += DownloadSize;

		//respond 
		pResp[0] = UDS_SID_ACK(UDS_SID_TRANSFER_DATA);
		pResp[1] = _MemBlockCounter;
#ifdef SID_36_RESPOND_CRC	//If the 36 service needs to feedback the result of the crc			
		pResp[2] = (CrcValue >> 24);
		pResp[3] = (CrcValue >> 16);
		pResp[4] = (CrcValue >> 8);
		pResp[5] = (CrcValue & 0xFF);
		*pRespLen = 6;
#else
		* pRespLen = 2;
#endif
	}
	else
	{
		/* Download Not Accepted ,address or leng error ,or when write app but flash driver hasn't download*/
		pResp[0] = UDS_SID_NACK;
		pResp[1] = UDS_SID_TRANSFER_DATA;
		pResp[2] = UDS_NRC_GENERAL_PROG_FAILURE;
		*pRespLen = 3;
		return;
	}
	_MemStarted |= 0x02;
}

/********************************************************************************
*  Date: 2022/07/18 22:52
*  Dscr:
********************************************************************************/
void SidProcRequestTransferExit(UdsCbT* pCb, uint8_t* pAdata, uint16_t Len,
	uint8_t* pResp, uint16_t* pRespLen)
{
	if (!(0x02 & _MemStarted))
	{
		/* Request Seq Error */
		pResp[0] = UDS_SID_NACK;
		pResp[1] = UDS_SID_REQ_TRANSFER_EXIT;
		pResp[2] = UDS_NRC_REQ_SEQ_ERROR;
		*pRespLen = 3;
		return;
	}

	_MemStarted = 0;
	pResp[0] = UDS_SID_ACK(UDS_SID_REQ_TRANSFER_EXIT);
	*pRespLen = 1;
}


/*end :sidproc.c*/



