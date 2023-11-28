
/********************************************************************************
* FILE: sidproc.h
* DSCR: XXX
*
* Date: 2022/07/18 22:56 
********************************************************************************/
#ifndef __SIDPROC_H__
#define __SIDPROC_H__



void SidInit(void);
void SidProcDiagSession(UdsCbT *pCb, uint8_t *pAdata, uint16_t Len, uint8_t *pResp, uint16_t *pRespLen);
void SidProcEcuReset(UdsCbT *pCb, uint8_t *pAdata, uint16_t Len, uint8_t *pResp, uint16_t *pRespLen);
void SidProcSecurityAccess(UdsCbT *pCb, uint8_t *pAdata, uint16_t Len, uint8_t *pResp, uint16_t *pRespLen);
void SidProcCommCtrl(UdsCbT *pCb, uint8_t *pAdata, uint16_t Len, uint8_t *pResp, uint16_t *pRespLen);
void SidProcTesterPresent(UdsCbT *pCb, uint8_t *pAdata, uint16_t Len, uint8_t *pResp, uint16_t *pRespLen);
void SidProcDtcSetting(UdsCbT *pCb, uint8_t *pAdata, uint16_t Len, uint8_t *pResp, uint16_t *pRespLen);
void SidProcReadDataByIdentifier(UdsCbT *pCb, uint8_t *pAdata, uint16_t Len, uint8_t *pResp, uint16_t *pRespLen);
void SidProcWriteDataByIdentifier(UdsCbT *pCb, uint8_t *pAdata, uint16_t Len, uint8_t *pResp, uint16_t *pRespLen);
void SidProcRoutineCtrl(UdsCbT *pCb, uint8_t *pAdata, uint16_t Len, uint8_t *pResp, uint16_t *pRespLen);
void SidProcRequestDownload(UdsCbT *pCb, uint8_t *pAdata, uint16_t Len, uint8_t *pResp, uint16_t *pRespLen);
void SidProcTransferData(UdsCbT *pCb, uint8_t *pAdata, uint16_t Len, uint8_t *pResp, uint16_t *pRespLen);
void SidProcRequestTransferExit(UdsCbT *pCb, uint8_t *pAdata, uint16_t Len, uint8_t *pResp, uint16_t *pRespLen);

#endif/*__SIDPROC_H__*/




