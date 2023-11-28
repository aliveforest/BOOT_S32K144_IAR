
/********************************************************************************
* FILE: didproc.h
* DSCR: XXX
*
* Date: 2022/07/18 22:45
********************************************************************************/
#ifndef __DIDPROC_H__
#define __DIDPROC_H__

#include "sys_cfg.h"
//SID 22
#define UDS_DID_READ_ECU_HW				0xF17F	//GACECUHardwareVersionNumberDataIdentifier
#define UDS_DID_READ_BT_SW				0xF180	//bootSoftwareIdentification
#define UDS_DID_READ_PN					0xF187	//GAC SparePartNumberDataIdentifier
#define UDS_DID_READ_ECU_SW				0xF189	//GACECUSoftwareVersionNumberDataIdentifier
#define UDS_DID_READ_vF181				0xF181
#define UDS_DID_READ_F186				0xF186  //activeDiagSessionMode

#define UDS_DID_READ_APP_SW_FINGER		0xF184	//[base+0xBA]ApplicationSoftwareFingerprintDataIdentifier
#define UDS_DID_READ_GRAMM_DATE			0xF199	//[base+0xC6]programmingDateDataIdentifier
#define UDS_DID_READ_GRAMM_CNT			0x0200	//[base+0xCC]Reprogramming Counter
#define UDS_DID_READ_GRAMM_RETTRY_CNT	0x0201	//[base+0xD0]Reprogramming Attempt Counter
#define UDS_DID_READ_v0110				0x0110

#define UDS_DID_READ_CUSTOM_BT_ST		0x0E10

#ifdef LOG_FUN_ENABLE
#define UDS_DID_READ_CUSTOM_LOG_ST		0x0E11
#endif

#ifdef UDS_DID_TEST
#define UDS_DID_TEST_INTERNAL		    0x0E20
#endif

/* DID Processes */
//SID 22 const
extern uint8_t DidRead_GACECUHardwareVersionNumberDataIdentifier(uint8_t *pResp, uint16_t *pRespLen, uint16_t RespMax);//LEN:17
extern uint8_t DidRead_bootSoftwareIdentification(uint8_t *pResp, uint16_t *pRespLen, uint16_t RespMax);//LEN:17
extern uint8_t DidRead_GACSparePartNumberDataIdentifier(uint8_t *pResp, uint16_t *pRespLen, uint16_t RespMax);//LEN:14
extern uint8_t DidRead_GACECUSoftwareVersionNumberDataIdentifier(uint8_t *pResp, uint16_t *pRespLen, uint16_t RespMax);//LEN:17
extern uint8_t DidRead_vF181(uint8_t *pResp, uint16_t *pRespLen, uint16_t RespMax);//LEN:17
extern uint8_t DidRead_DiagSessionMode(uint8_t* pResp, uint16_t* pRespLen, uint16_t RespMax);

//DID EEPROM
extern uint8_t DidRead_ReprogrammingCounter(uint8_t *pResp, uint16_t *pRespLen, uint16_t RespMax);
extern uint8_t DidRead_ReprogrammingAttemptCounter(uint8_t *pResp, uint16_t *pRespLen, uint16_t RespMax);
extern uint8_t DidRead_ApplicationSoftwareFingerprintDataIdentifier(uint8_t *pResp, uint16_t *pRespLen, uint16_t RespMax);
extern uint8_t DidRead_programmingDateDataIdentifier(uint8_t *pResp, uint16_t *pRespLen, uint16_t RespMax);
extern uint8_t DidRead_v0110(uint8_t *pResp, uint16_t *pRespLen, uint16_t RespMax);

//DID RAM
extern uint8_t DidRead_Custom_LogData(uint8_t *pResp, uint16_t *pRespLen, uint16_t RespMax);

#ifdef LOG_FUN_ENABLE
extern uint8_t DidRead_Custom_LogEnSt(uint8_t* pResp, uint16_t* pRespLen, uint16_t RespMax);
#endif

//SID 2E
extern uint8_t DidWrite_ReprogrammingCounte(uint8_t *pReq, uint16_t ReqLen, uint8_t *pResp, uint16_t *pRespLen, uint16_t RespMax);
extern uint8_t DidWrite_ReprogrammingAttemptCounter(uint8_t *pReq, uint16_t ReqLen, uint8_t *pResp, uint16_t *pRespLen, uint16_t RespMax);
extern uint8_t DidWrite_ApplicationSoftwareFingerprintDataIdentifier(uint8_t *pReq, uint16_t ReqLen, uint8_t *pResp, uint16_t *pRespLen, uint16_t RespMax);
extern uint8_t DidWrite_programmingDateDataIdentifier(uint8_t *pReq, uint16_t ReqLen, uint8_t *pResp, uint16_t *pRespLen, uint16_t RespMax);

#ifdef LOG_FUN_ENABLE
extern uint8_t DidWrite_Custom_LogEnSt(uint8_t *pReq, uint16_t ReqLen, uint8_t *pResp, uint16_t *pRespLen, uint16_t RespMax);
#endif

#ifdef UDS_DID_TEST
extern uint8_t DidWrite_InternalTest(uint8_t *pReq, uint16_t ReqLen, uint8_t *pResp, uint16_t *pRespLen, uint16_t RespMax);
extern uint8_t DidRead_InternalTest(uint8_t *pResp, uint16_t *pRespLen, uint16_t RespMax);
#endif

#endif/*__DIDPROC_H__*/




