/********************************************************************************
* FILE: docan.c
* DSCR: XXX
*
* Date: 2022/07/18 22:46
********************************************************************************/
#include "DcmSup.h"
#include "docan.h"
#include "uds.h"
#include "docan.h"


typedef enum _DoCanStateT {
	DOCAN_STATE_IDLE = 0,
	DOCAN_STATE_RXCF,
	DOCAN_STATE_TXCF,
	DOCAN_STATE_WAIT_CTS,
}DoCanStateT;

typedef struct _DoCanCbT {
	uint32_t      AddressInfo;
	DoCanStateT State;
	uint16_t      TimeOutCntDown;
	uint8_t       RxSn : 4;
	uint8_t       res1 : 4;
	uint8_t       RxBlockFrameCnt;
	uint8_t       RxBlockSize;
	uint16_t      RxByteCnt;
	uint16_t      RxDataLength;
	uint8_t       RxBuffer[DOCAN_BUFFER_SIZE];
	uint8_t       TxSn : 4;
	uint8_t       res2 : 4;
	uint8_t       TxFsWaitCnt;
	uint8_t       TxStMin;
	uint8_t       TxBlockFrameCnt;
	uint8_t       TxBlockSize;
	uint16_t      TxByteCnt;
	uint16_t      TxDataLength;
	uint8_t       TxBuffer[DOCAN_BUFFER_SIZE];
}DoCanCbT;


static DoCanCbT _DoCanCb;
static uint8_t	RespBuf[DOCAN_TX_PRAME_MAX_SIZE] = { 0 };

static void _DoCanMsgResponse(void);
static void _DoCanStateIdleGetMsg(uint8_t* pData, uint8_t Num);
static void _DoCanStateRxCfGetMsg(uint8_t* pData, uint8_t Num);
static void _DoCanStateCtsCfGetMsg(uint8_t* pData, uint8_t Num);
static void _DoCanRxReset(void);
static void _DoCanTxReset(void);

static  void _DOCAN_SF_RESP(uint8_t* pData, uint16_t Dl);
static  uint8_t _DOCAN_FF_RESP(uint8_t* pData, uint16_t Dl);
static  void _DOCAN_CF_RESP(uint8_t* pData, uint8_t Sn, uint8_t Num);
static  void _DOCAN_FC_RESP(uint8_t Fc, uint8_t  Bs, uint8_t St);

/********************************************************************************
*  Date: 2022/07/18 22:46
*  Dscr:
********************************************************************************/
void DoCanInit(void)
{
	(void)memset(&_DoCanCb, 0, sizeof(_DoCanCb));

	_DoCanCb.RxBlockSize = DOCAN_BS; /* Default Block Size Request */
}

/********************************************************************************
*  Date: 2022/07/18 22:46
*  Dscr:
********************************************************************************/
void DoCanMsgProcess(uint32_t CanId, uint8_t* pData, uint8_t Num)
{
	_DoCanCb.AddressInfo = CanId;

	switch (_DoCanCb.State)
	{
	case DOCAN_STATE_IDLE:
		_DoCanStateIdleGetMsg(pData, Num);
		break;

	case DOCAN_STATE_RXCF:
		_DoCanStateRxCfGetMsg(pData, Num);
		break;

	case DOCAN_STATE_WAIT_CTS:
		_DoCanStateCtsCfGetMsg(pData, Num);
		break;

	case DOCAN_STATE_TXCF:
		break;

	default:
		_DoCanCb.State = DOCAN_STATE_IDLE;
		_DoCanRxReset();
		_DoCanTxReset();
		break;
	}
}

/********************************************************************************
*  Date: 2022/07/18 22:46
*  Dscr:
********************************************************************************/
void DoCanTimerProcess(void)
{
	uint8_t Num;

	/* Rx Timeout */
	if ((_DoCanCb.State == DOCAN_STATE_RXCF) ||
		(_DoCanCb.State == DOCAN_STATE_TXCF) ||
		(_DoCanCb.State == DOCAN_STATE_WAIT_CTS))
	{
		if (_DoCanCb.TimeOutCntDown >= DOCAN_TIMER_PERIOD)
		{
			_DoCanCb.TimeOutCntDown -= DOCAN_TIMER_PERIOD;
		}
		else
		{
			_DoCanCb.TimeOutCntDown = 0;
		}

		if (!_DoCanCb.TimeOutCntDown)
		{
			/* Timeout Occur */
			switch (_DoCanCb.State)
			{
			case DOCAN_STATE_RXCF:
				_DoCanCb.State = DOCAN_STATE_IDLE;
				_DoCanRxReset();
				LogOcPut(OC_DC_TP_RXCFTE);
				break;

			case DOCAN_STATE_TXCF:
				/* Send Next CF */
				_DoCanCb.TimeOutCntDown = _DoCanCb.TxStMin + DOCAN_N_ST_MIN_EX; /* Increase to Prevent Test Fail */
				_DoCanCb.TxSn++;
				_DoCanCb.TxBlockFrameCnt++;
				if ((_DoCanCb.TxBlockSize) &&
					(_DoCanCb.TxBlockFrameCnt >= _DoCanCb.TxBlockSize))
				{
					_DoCanCb.State = DOCAN_STATE_WAIT_CTS;
					_DoCanCb.TxBlockFrameCnt = 0;
				}
				if (1 == DOCAN_TX_SUPPORT_CANFD)
				{
					if ((_DoCanCb.TxByteCnt + DOCAN_DL_MAX_FD_CF) <= _DoCanCb.TxDataLength)
						Num = DOCAN_DL_MAX_FD_CF;
					else
						Num = _DoCanCb.TxDataLength - _DoCanCb.TxByteCnt;
				}
				else
				{
					if ((_DoCanCb.TxByteCnt + DOCAN_DL_MAX_CF) <= _DoCanCb.TxDataLength)
						Num = DOCAN_DL_MAX_CF;
					else
						Num = _DoCanCb.TxDataLength - _DoCanCb.TxByteCnt;
				}


				/* Consecutive Frame Send */
				_DOCAN_CF_RESP(&_DoCanCb.TxBuffer[_DoCanCb.TxByteCnt], _DoCanCb.TxSn, Num);
				_DoCanCb.TxByteCnt += Num;
				if (_DoCanCb.TxByteCnt >= _DoCanCb.TxDataLength)
				{
					/* Finish Sending */
					_DoCanCb.State = DOCAN_STATE_IDLE;
					_DoCanTxReset();
					LogOcPut(OC_DC_TP_TXCFTE);
				}
				break;

			case DOCAN_STATE_WAIT_CTS:
				_DoCanCb.State = DOCAN_STATE_IDLE;
				_DoCanTxReset();
				LogOcPut(OC_DC_TP_WCTSTE);
				break;

			default:
				break;
			}
		}
	}
}

/********************************************************************************
*  Date: 2022/07/18 22:46
*  Dscr:
********************************************************************************/
void _DoCanMsgResponse(void)
{
	if ((_DoCanCb.TxDataLength > DOCAN_BUFFER_SIZE) ||
		(_DoCanCb.TxDataLength == 0)/* Transport Layer Limit */)
	{
		if (_DoCanCb.TxDataLength > DOCAN_BUFFER_SIZE)
		{
			LogOcPut(OC_DC_MR_PE);
		}
		return;
	}

	if ((_DoCanCb.TxDataLength <= DOCAN_DL_MAX_SF)
		|| ((1 == DOCAN_TX_SUPPORT_CANFD) && (_DoCanCb.TxDataLength <= DOCAN_DL_MAX_FD_SF)))
	{
		/* Single Frame Send */
		_DOCAN_SF_RESP(_DoCanCb.TxBuffer, _DoCanCb.TxDataLength);
		_DoCanCb.State = DOCAN_STATE_IDLE;
	}
	else
	{
		if ((_DoCanCb.TxDataLength <= DOCAN_DLC_MAX_FF)
			|| ((1 == DOCAN_TX_SUPPORT_CANFD) && (_DoCanCb.TxDataLength > DOCAN_DLC_MAX_FF)))
		{
			/* First Frame + Consecutive Frame */
			_DoCanCb.State = DOCAN_STATE_WAIT_CTS;
			_DoCanCb.TimeOutCntDown = /* DOCAN_N_ASAR + */ DOCAN_N_BS; /* Reduce to pass test */
			_DoCanCb.TxSn = 0;
			_DoCanCb.TxBlockFrameCnt = 0;
			_DoCanCb.TxBlockSize = 0;

			//_DoCanCb.TxByteCnt = DOCAN_DL_MAX_FF;
			/* First Frame Send */
			_DoCanCb.TxByteCnt = _DOCAN_FF_RESP(_DoCanCb.TxBuffer, _DoCanCb.TxDataLength);
		}
		else
		{
			LogOcPut(OC_DC_MR_LE);
		}
	}
}

/********************************************************************************
*  Date: 2022/07/18 22:46
*  Dscr:
********************************************************************************/
void _DoCanStateIdleGetMsg(uint8_t* pData, uint8_t Num)
{
	DoCanAddrT AddrType;

	switch (DOCAN_GET_PCITYPE(pData))
	{
	case DOCAN_PCITYPE_SF: /* Single Frame */
		_DoCanCb.RxDataLength = DOCAN_SF_GET_DL(pData);
		if ((0 == _DoCanCb.RxDataLength)
			&& (1 == DOCAN_TX_SUPPORT_CANFD))
		{
			_DoCanCb.RxDataLength = DOCAN_SF_GET_DL_O8(pData);
		}
		/* Response Only when Data Len is Correct */
		if ((_DoCanCb.RxDataLength > 0)
			&& (((_DoCanCb.RxDataLength < 8) /*CAN2.0*/
				&& ((_DoCanCb.RxDataLength + 1) <= Num)) ||
				((_DoCanCb.RxDataLength < 64)/*CANFD*/
					&& ((_DoCanCb.RxDataLength + 2) <= Num))))
		{
			if (_DoCanCb.RxDataLength > DOCAN_DL_MAX_SF)
			{
				(void)memcpy(_DoCanCb.RxBuffer, pData + 2, _DoCanCb.RxDataLength);
			}
			else
			{
				(void)memcpy(_DoCanCb.RxBuffer, pData + 1, _DoCanCb.RxDataLength);
			}

			/* Call UDS API */
			if (_DoCanCb.AddressInfo == DOCAN_ADDR_PHYS)
			{
				AddrType = DOCAN_ADDR_TYPE_PHYSICAL;
			}
			else
			{
				AddrType = DOCAN_ADDR_TYPE_FUNCTIONAL;
			}

			UdsApduDispatch(AddrType, _DoCanCb.RxBuffer, _DoCanCb.RxDataLength,
				_DoCanCb.TxBuffer, &_DoCanCb.TxDataLength);
			_DoCanMsgResponse();
			_DoCanRxReset();
		}
		else
		{
			/* Incorrect DLC or Data Len */
			_DoCanCb.RxDataLength = 0;
			LogOcPut(OC_DC_IGM_DDLE);
		}
		break;

	case DOCAN_PCITYPE_FF: /* First Frame */
		if (_DoCanCb.AddressInfo == DOCAN_ADDR_PHYS)
		{
			_DoCanCb.RxDataLength = DOCAN_FF_GET_DL(pData);
			if ((0 == _DoCanCb.RxDataLength)
				&& (1 == DOCAN_TX_SUPPORT_CANFD))
			{
				_DoCanCb.RxDataLength = (uint16_t)DOCAN_FF_GET_DL_O4095(pData);
			}
			if (_DoCanCb.RxDataLength > 7)
			{
				if (_DoCanCb.RxDataLength <= DOCAN_DLC_MAX_FF)
				{
					if (Num > DOCAN_DL_MAX_FF)
					{
						(void)memcpy(_DoCanCb.RxBuffer, pData + 2, (Num - 2));
						_DoCanCb.RxByteCnt = (Num - 2);
					}
					else
					{
						(void)memcpy(_DoCanCb.RxBuffer, pData + 2, DOCAN_DL_MAX_FF);
						_DoCanCb.RxByteCnt = DOCAN_DL_MAX_FF;
					}
				}
				else
				{
					(void)memcpy(_DoCanCb.RxBuffer, pData + 6, DOCAN_DL_MAX_FD_FF_MORE_4095);
					_DoCanCb.RxByteCnt = DOCAN_DL_MAX_FD_FF_MORE_4095;
				}
				_DoCanCb.TimeOutCntDown = /* DOCAN_N_ASAR + */ DOCAN_N_CR; /* Reduce to pass test */
				/* Send CTS and Wait for CF */
				_DOCAN_FC_RESP(DOCAN_FLOWSTATUS_CTS, DOCAN_BS, DOCAN_ST_MIN);
				_DoCanCb.State = DOCAN_STATE_RXCF;
			}
			else
			{
				/* Exceed Buffer Limit */
				_DoCanCb.RxDataLength = 0;
				LogOcPut(OC_DC_IGM_FFOLE);
			}
		}
		break;

	default: /* Ignore */
		_DoCanRxReset();
		break;
	}
}

/********************************************************************************
*  Date: 2022/07/18 22:46
*  Dscr:
********************************************************************************/
void _DoCanStateRxCfGetMsg(uint8_t* pData, uint8_t Num)
{
	uint8_t Sn, ByteNum;
	DoCanAddrT AddrType;
	uint8_t rxLen = 0;
	uint8_t rxbuf[8] = { 0 };
	uint16_t txLen = 0;
	uint8_t txbuf[8] = { 0 };

	switch (DOCAN_GET_PCITYPE(pData))
	{
	case DOCAN_PCITYPE_CF: /* Consecutive Frame */
		Sn = DOCAN_CF_GET_SN(pData);
		if (((Sn - _DoCanCb.RxSn) & 0x0F) == 1)
		{
			/* Correct Sequence Number */
			if (Num > 8)
			{
				//CANFD
				ByteNum = ((_DoCanCb.RxDataLength - _DoCanCb.RxByteCnt) > DOCAN_DL_MAX_FD_CF) ?
					DOCAN_DL_MAX_FD_CF : (uint8_t)(_DoCanCb.RxDataLength - _DoCanCb.RxByteCnt);
			}
			else
			{
				//CAN2.0
				ByteNum = ((_DoCanCb.RxDataLength - _DoCanCb.RxByteCnt) > DOCAN_DL_MAX_CF) ?
					DOCAN_DL_MAX_CF : (uint8_t)(_DoCanCb.RxDataLength - _DoCanCb.RxByteCnt);
			}

			if ((ByteNum + 1) <= Num)
			{
				if ((_DoCanCb.RxByteCnt + ByteNum) > DOCAN_BUFFER_SIZE)
				{
					if (_DoCanCb.RxByteCnt < DOCAN_BUFFER_SIZE)
					{
						(void)memcpy(_DoCanCb.RxBuffer + _DoCanCb.RxByteCnt, pData + 1,
							(DOCAN_BUFFER_SIZE - _DoCanCb.RxByteCnt));
					}
					else
					{
						/*buffer length overflow,ignore this data*/
					}
				}
				{
					(void)memcpy(_DoCanCb.RxBuffer + _DoCanCb.RxByteCnt, pData + 1, ByteNum);
				}

				_DoCanCb.RxByteCnt += ByteNum;
				if (_DoCanCb.RxByteCnt >= _DoCanCb.RxDataLength)
				{
					/* All Bytes Received, Info UDS Layer */
					if (_DoCanCb.AddressInfo == DOCAN_ADDR_PHYS)
					{
						AddrType = DOCAN_ADDR_TYPE_PHYSICAL;
					}
					else
					{
						AddrType = DOCAN_ADDR_TYPE_FUNCTIONAL;
					}

					UdsApduDispatch(AddrType, _DoCanCb.RxBuffer, _DoCanCb.RxDataLength,
						_DoCanCb.TxBuffer, &_DoCanCb.TxDataLength);
					_DoCanMsgResponse();
					_DoCanRxReset();
				}
				else
				{
					_DoCanCb.RxSn++;
					_DoCanCb.RxBlockFrameCnt++;
					if (_DoCanCb.RxBlockFrameCnt >= _DoCanCb.RxBlockSize)
					{
						/* Send CTS */
						_DOCAN_FC_RESP(DOCAN_FLOWSTATUS_CTS, DOCAN_BS, DOCAN_ST_MIN);
						_DoCanCb.RxBlockFrameCnt = 0;
						_DoCanCb.TimeOutCntDown = DOCAN_N_BS + DOCAN_N_CR;
					}
					else
					{
						_DoCanCb.TimeOutCntDown = DOCAN_N_CR;
					}
				}
			}
			else
			{
				/* Incorrect DLC */
				_DoCanCb.State = DOCAN_STATE_IDLE;
				_DoCanRxReset();
				LogOcPut(OC_DC_RGM_DDLE);
			}
		}
		else
		{
			/* Wrong Sequence Number */
			_DoCanCb.State = DOCAN_STATE_IDLE;
			_DoCanRxReset();
			LogOcPut(OC_DC_RGM_CFWSNE);
		}
		break;

	case DOCAN_PCITYPE_SF: /* Single Frame */
#if 0
		_DoCanCb.RxDataLength = DOCAN_SF_GET_DL(pData);
		/* Response Only when Data Len is Correct */
		if ((_DoCanCb.RxDataLength > 0) && (_DoCanCb.RxDataLength < 8) &&
			((_DoCanCb.RxDataLength + 1) <= Num))
		{
			(void)memcpy(_DoCanCb.RxBuffer, pData + 1, _DoCanCb.RxDataLength);
			/* Call UDS API */
			if (_DoCanCb.AddressInfo == DOCAN_ADDR_PHYS)
			{
				AddrType = DOCAN_ADDR_TYPE_PHYSICAL;
			}
			else
			{
				AddrType = DOCAN_ADDR_TYPE_FUNCTIONAL;
			}
			UdsApduDispatch(AddrType, _DoCanCb.RxBuffer, _DoCanCb.RxDataLength,
				_DoCanCb.TxBuffer, &_DoCanCb.TxDataLength);
			_DoCanMsgResponse();
			_DoCanRxReset();

		}
#else
		rxLen = DOCAN_SF_GET_DL(pData);
		/* Response Only when Data Len is Correct */
		if ((rxLen > 0) && (rxLen < 8) &&
			((rxLen + 1) <= Num))
		{
			(void)memcpy(rxbuf, pData + 1, rxLen);
			/* Call UDS API */
			if (_DoCanCb.AddressInfo == DOCAN_ADDR_PHYS)
			{
				AddrType = DOCAN_ADDR_TYPE_PHYSICAL;
			}
			else
			{
				AddrType = DOCAN_ADDR_TYPE_FUNCTIONAL;
			}
			UdsApduDispatch(AddrType, rxbuf, rxLen, txbuf, &txLen);
			if ((txLen > 0) && (txbuf[1] != 0x3E))
			{
				_DoCanCb.TxDataLength = txLen;
				(void)memcpy(_DoCanCb.TxBuffer, txbuf, txLen);
				_DoCanMsgResponse();
				_DoCanRxReset();
			}
		}
#endif
		else
		{
			/* Incorrect DLC or Data Len */
			_DoCanCb.RxDataLength = 0;
			LogOcPut(OC_DC_RGM_SFLE);
		}
		break;

	case DOCAN_PCITYPE_FF: /* First Frame */
		if (_DoCanCb.AddressInfo == DOCAN_ADDR_PHYS)
		{
			_DoCanCb.RxDataLength = DOCAN_FF_GET_DL(pData);
			if ((_DoCanCb.RxDataLength) > 7 &&
				(_DoCanCb.RxDataLength <= DOCAN_BUFFER_SIZE))
			{
				(void)memcpy(_DoCanCb.RxBuffer, pData + 2, DOCAN_DL_MAX_FF);
				_DoCanCb.RxByteCnt = DOCAN_DL_MAX_FF;
				_DoCanCb.TimeOutCntDown = /* DOCAN_N_ASAR + */ DOCAN_N_CR; /* Reduce to pass test */
				/* Send CTS and Wait for CF */
				_DOCAN_FC_RESP(DOCAN_FLOWSTATUS_CTS, DOCAN_BS, DOCAN_ST_MIN);
				_DoCanCb.State = DOCAN_STATE_RXCF;
			}
			else
			{
				/* Exceed Buffer Limit */
				_DoCanCb.RxDataLength = 0;
				LogOcPut(OC_DC_RGM_FFOLE);
			}
		}
		break;

	case DOCAN_PCITYPE_FC: /* Flow Control */
	default:
		/* Just Ignore the Unknown Frame */
		break;
	}
}

/********************************************************************************
*  Date: 2022/07/18 22:45
*  Dscr:
********************************************************************************/
void _DoCanStateCtsCfGetMsg(uint8_t* pData, uint8_t Num)
{
	switch (DOCAN_GET_PCITYPE(pData))
	{
	case DOCAN_PCITYPE_FC: /* Flow Control Frame */
		switch (DOCAN_FC_GET_FS(pData))
		{
		case DOCAN_FLOWSTATUS_CTS:
			if ((Num < 8) || (_DoCanCb.AddressInfo != DOCAN_ADDR_PHYS))
			{
				/* Stop Transmit */
				_DoCanCb.State = DOCAN_STATE_IDLE;
				_DoCanTxReset();
				LogOcPut(OC_DC_CGM_CTSLE);
			}
			else
			{
				_DoCanCb.TxBlockSize = DOCAN_FC_GET_BS(pData);
				_DoCanCb.TxStMin = DOCAN_FC_GET_STMIN(pData);
				if (_DoCanCb.TxStMin > 127)
				{
					_DoCanCb.TxStMin = 1; /* Now We Only Support Resolution that Greater than 1ms */
				}
				_DoCanCb.TimeOutCntDown = _DoCanCb.TxStMin + DOCAN_N_ST_MIN_EX; /* Increase to Prevent Test Fail */
				_DoCanCb.State = DOCAN_STATE_TXCF;
			}
			break;

		case DOCAN_FLOWSTATUS_WAIT:
			_DoCanCb.TxFsWaitCnt++;
			if (_DoCanCb.TxFsWaitCnt > DOCAN_N_WFT_MAX)
			{
				//(" TxFsWaitCnt > DOCAN_N_WFT_MAX Stop Transmit !\r\n");
				/* Stop Transmit */
				_DoCanCb.State = DOCAN_STATE_IDLE;
				_DoCanTxReset();
				LogOcPut(OC_DC_CGM_WCOE);
			}
			else
			{
				/* Reset Timer for Another Flow Control Frame */
				_DoCanCb.TimeOutCntDown = DOCAN_N_BS;
				LogOcPut(OC_DC_CGM_WTE);
			}
			break;

		default:
		case DOCAN_FLOWSTATUS_OVF:
			_DoCanCb.State = DOCAN_STATE_IDLE;
			_DoCanTxReset();
			//" DOCAN_FLOWSTATUS_OVF !
			LogOcPut(OC_DC_CGM_FOE);
			break;
		}
		break;

	default:
		_DoCanCb.State = DOCAN_STATE_IDLE;
		_DoCanTxReset();
		break;
	}
}

/********************************************************************************
*  Date: 2022/07/18 22:45
*  Dscr:
********************************************************************************/
void _DoCanRxReset(void)
{
	//_DoCanCb.TimeOutCntDown = 0;
	_DoCanCb.RxSn = 0;
	_DoCanCb.RxBlockFrameCnt = 0;
	_DoCanCb.RxByteCnt = 0;
	_DoCanCb.RxDataLength = 0;
}

/********************************************************************************
*  Date: 2022/07/18 22:45
*  Dscr:
********************************************************************************/
void _DoCanTxReset(void)
{
	//_DoCanCb.TimeOutCntDown = 0;
	_DoCanCb.TxSn = 0;
	_DoCanCb.TxFsWaitCnt = 0;
	_DoCanCb.TxStMin = 0;
	_DoCanCb.TxBlockFrameCnt = 0;
	_DoCanCb.TxBlockSize = 0;
	_DoCanCb.TxByteCnt = 0;
	_DoCanCb.TxDataLength = 0;
}

/********************************************************************************
*  Date: 2022/07/18 22:45
*  Dscr:
********************************************************************************/
#if 1
uint8_t _DlcMix(uint8_t size)
{
	if (size > 48)
		return 64;
	else if (size > 32)
		return 48;
	else if (size > 24)
		return 32;
	else if (size > 20)
		return 24;
	else if (size > 16)
		return 20;
	else if (size > 12)
		return 16;
	else if (size > 8)
		return 12;
	else
		return size;
}


/* Single Frame Construct */
void _DOCAN_SF_RESP(uint8_t* pData, uint16_t Dl)
{

	uint8_t offset = 0;
	uint8_t paddingLen = 0;
	uint8_t dlc = 0;

	(void)memset(RespBuf, 0, DOCAN_TX_PRAME_MAX_SIZE);
	DOCAN_SET_PCITYPE(RespBuf, DOCAN_PCITYPE_SF);

	if (Dl <= DOCAN_DL_MAX_SF)
	{
		dlc = 8;
		paddingLen = DOCAN_DL_MAX_SF - (Dl);
		DOCAN_SF_SET_DL(RespBuf, (Dl));
		offset = 1;
	}
	else if (Dl <= DOCAN_DL_MAX_FD_SF)
	{
		dlc = _DlcMix(Dl + 2);
		//paddingLen = (dlc - Dl);
		DOCAN_SF_SET_DL_O8(RespBuf, (Dl));
		offset = 2;
		paddingLen = (uint8_t)((dlc > (Dl + 2)) ? (uint8_t)(dlc - (Dl + 2)) : 0);
	}
	else
	{
		LogOcPut(OC_DC_SFR_LE);
		return;
	}

	(void)memcpy((uint8_t*)&RespBuf[offset], (pData), (Dl));

	if (paddingLen)
	{
		(void)memset((uint8_t*)&RespBuf[offset + Dl], DOCAN_PADDING_VALUE, paddingLen);
	}
	(void)CanSendMessage(DOCAN_ADDR_RESP, RespBuf, dlc);
}

/* First Frame Construct */
uint8_t _DOCAN_FF_RESP(uint8_t* pData, uint16_t Dl)
{
	uint8_t offset = 0;
	uint8_t paddingLen = 0;
	uint8_t dlc = 0;
	uint8_t Len = 0;

	(void)memset(RespBuf, 0, DOCAN_TX_PRAME_MAX_SIZE);
	DOCAN_SET_PCITYPE(RespBuf, DOCAN_PCITYPE_FF);
	if (Dl > DOCAN_DLC_MAX_FF)
	{
		Len = DOCAN_DL_MAX_FD_FF_MORE_4095;
		DOCAN_FF_SET_DL_O4095(RespBuf, (Dl));
		offset = 6;
		dlc = 64;
	}
	else
	{
		if (Dl <= DOCAN_DL_MAX_FF)
		{
			//CAN 2.0
			dlc = 8;
			paddingLen = DOCAN_DL_MAX_FF - Dl;
			Len = Dl;
		}
		else
		{
			//CAN FD
			Len = (uint8_t)((Dl > DOCAN_DL_MAX_FD_FF_LESS_4095) ? DOCAN_DL_MAX_FD_FF_LESS_4095 : Len);
			dlc = _DlcMix((Len + 2));
			paddingLen = (uint8_t)((dlc > (Len + 2)) ? (uint8_t)(dlc - (Len + 2)) : 0);
		}
		DOCAN_FF_SET_DL(RespBuf, (Dl));
		offset = 2; // offset 2 bytes
	}
	(void)memcpy((uint8_t*)&RespBuf[offset], (pData), Len);

	if (paddingLen)
	{
		(void)memset((uint8_t*)&RespBuf[offset + Len], DOCAN_PADDING_VALUE, paddingLen);
	}
	(void)CanSendMessage(DOCAN_ADDR_RESP, RespBuf, dlc);
	return Len;
}

/* Consecutive Frame Construct */
void _DOCAN_CF_RESP(uint8_t* pData, uint8_t Sn, uint8_t Num)
{
	uint8_t dlc = 0;

	(void)memset(RespBuf, 0, DOCAN_TX_PRAME_MAX_SIZE);
	DOCAN_SET_PCITYPE(RespBuf, DOCAN_PCITYPE_CF);
	DOCAN_CF_SET_SN(RespBuf, (Sn));

	if ((Num) <= DOCAN_DL_MAX_CF)
	{
		(void)memcpy((uint8_t*)&RespBuf[1], (pData), (Num));
		if ((Num) < DOCAN_DL_MAX_CF)
		{
			(void)memset((uint8_t*)&RespBuf[1 + (Num)], DOCAN_PADDING_VALUE, DOCAN_DL_MAX_CF - (Num));
		}
		dlc = 8;
	}
	else if ((Num) <= DOCAN_DL_MAX_FD_CF)
	{
		dlc = _DlcMix(Num + 1);
		(void)memcpy((uint8_t*)&RespBuf[1], (pData), (Num));
		if ((dlc - Num) > 0)
		{
			(void)memset((uint8_t*)&RespBuf[1 + (Num)], DOCAN_PADDING_VALUE, (dlc - Num));
		}
	}
	else
	{
		LogOcPut(OC_DC_CFR_LE);
		return;
	}
	(void)CanSendMessage(DOCAN_ADDR_RESP, RespBuf, dlc);
}

/* Flow Control Construct */
void _DOCAN_FC_RESP(uint8_t Fc, uint8_t  Bs, uint8_t St)
{
	(void)memset(RespBuf, DOCAN_PADDING_VALUE, 8);
	DOCAN_SET_PCITYPE(RespBuf, DOCAN_PCITYPE_FC);
	DOCAN_FC_SET_FS(RespBuf, (Fc));
	DOCAN_FC_SET_BS(RespBuf, (Bs));
	DOCAN_FC_SET_STMIN(RespBuf, (St));
	(void)CanSendMessage(DOCAN_ADDR_RESP, RespBuf, 8);
}
#endif
/*end :docan.c*/



