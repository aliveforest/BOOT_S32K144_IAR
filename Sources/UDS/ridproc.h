
/********************************************************************************
* FILE: ridproc.h
* DSCR: XXX
*
* Date: 2022/07/18 22:52
********************************************************************************/
#ifndef __RIDPROC_H__
#define __RIDPROC_H__


#define UDS_RID_FLASH_ERASE         0xFF00
#define UDS_RID_CRC_CHECK           0x0202
#define UDS_RID_CONSITION_CHECK		0x0203
#define UDS_RID_PROGRAMM_DP         0xFF01
#define UDS_RID_ROLLBACK            0x2110
#define UDS_RID_SYNC2BACKAREA       0x2111



void RidEventReset(void);

void RidStartProcFlashErase(UdsCbT* pCb, uint8_t* pReq, uint8_t* pResp, uint16_t* pRespLen, uint32_t addr, uint32_t size);
void RidResultProcFlashErase(UdsCbT* pCb, uint8_t* pReq, uint8_t* pResp, uint16_t* pRespLen);
void RidStartProcCheckCrc(UdsCbT* pCb, uint8_t* pReq, uint8_t* pResp, uint16_t* pRespLen);
void RidStartCheckProgrammingDependency(UdsCbT* pCb, uint8_t* pReq, uint8_t* pResp, uint16_t* pRespLen);
void CheckProgrammingPreconditions(UdsCbT* pCb, uint8_t* pReq, uint8_t* pResp, uint16_t* pRespLen);

void RidStartRollback(UdsCbT* pCb, uint8_t* pReq, uint8_t* pResp, uint16_t* pRespLe);
void RidResultRollback(UdsCbT* pCb, uint8_t* pReq, uint8_t* pResp, uint16_t* pRespLen);

void RidStartSync2BackupArea(UdsCbT* pCb, uint8_t* pReq, uint8_t* pResp, uint16_t* pRespLen);
void RidResultSync2BackupArea(UdsCbT* pCb, uint8_t* pReq, uint8_t* pResp, uint16_t* pRespLen);
#endif/*__RIDPROC_H__*/




