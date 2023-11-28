
/********************************************************************************
* FILE: uds.h
* DSCR: XXX
*
* Date: 2022/07/18 23:00 
********************************************************************************/
#ifndef __UDS_H__
#define __UDS_H__

#include "docan.h"
#include "stdbool.h"
#include "S32K144.h" 
#include "Cpu.h"
/*
*       |A_PCI    |A_DATA[0]|
* |N_PCI|N_DATA[0]|N_DATA[1]| ... |
*/

#ifndef U8_TRUE
#define U8_TRUE 0x5A
#endif

#ifndef U32_TRUE
#define U32_TRUE 0x2379A55A
#endif

#define UDS_APCI_GET(pNetworkData)      (((uint8_t *)pNetworkData)[0])
#define UDS_ADATA_GET(pNetworkData)     (((uint8_t *)pNetworkData) + 1)
#define UDS_SUBFUNC_GET(pAppData)       (((uint8_t *)pAppData)[0])

/*  Services ID  SIDs  @see ISO-14229 */
#define UDS_SID_DIAG_SESS_CTRL          0x10	/* 外部设备切换会话模式 */
#define UDS_SID_ECU_RESET               0x11    /* ECU复位 */
#define UDS_SID_SECURITY_ACCESS         0x27    /* 解锁ECU */
#define UDS_SID_COMM_CTRL               0x28    /* 通信控制：打开或关闭CAN报文 */
#define UDS_SID_TESTER_PRESENT          0x3E    /* 保持会话 */
#define UDS_SID_ACCESS_TIME_PARA        0x83    /*  */
#define UDS_SID_SECURED_DATA_TX         0x84    /*  */
#define UDS_SID_CTRL_DTC_SETTING        0x85    /* 打开或关闭DTC */
#define UDS_SID_RESP_ON_EVENT           0x86    /*  */
#define UDS_SID_LINK_CTRL               0x87    /*  */
#define UDS_SID_READ_DATA               0x22    /* 数据读取 */
#define UDS_SID_READ_MEM                0x23
#define UDS_SID_READ_SCALDATA           0x24
#define UDS_SID_READ_DATA_PERIODIC      0x2A
#define UDS_SID_DYN_DEFINE_DID          0x2C
#define UDS_SID_WRITE_DATA              0x2E    /* 外部数据写入ECU */
#define UDS_SID_WRITE_MEM               0x3D    /*  */
#define UDS_SID_CLEAR_DIAG_INFO         0x14    /*  */
#define UDS_SID_READ_DTC_INFO           0x19    /*  */
#define UDS_SID_INPUT_OUTPUT_CTRL       0x2F    /*  */
#define UDS_SID_ROUTINE_CTRL            0x31    /*  */
#define UDS_SID_REQ_DOWNLOAD            0x34    /* 请求下载 */
#define UDS_SID_REQ_UPLOAD              0x35    /* 请求上传 */
#define UDS_SID_TRANSFER_DATA           0x36    /* 传输数据 */
#define UDS_SID_REQ_TRANSFER_EXIT       0x37    /* 请求退出传输 */
#define UDS_SID_REQ_FILE_TRANSFER       0x38

/* OBD */
#define UDS_SID_OBD_REQ_CURR_PWR        0x01
#define UDS_SID_OBD_REQ_PWR_FREEZE      0x02
#define UDS_SID_OBD_REQ_DIAG_CODE       0x03
#define UDS_SID_OBD_CLR_DIAG_INFO       0x04
#define UDS_SID_OBD_REQ_OXY_RSLT        0x05
#define UDS_SID_OBD_REQ_OBM_RSLT        0x06
#define UDS_SID_OBD_REQ_DTC             0x07
#define UDS_SID_OBD_REQ_CTRL            0x08
#define UDS_SID_OBD_REQ_VEH_INFO        0x09

/* Response */
#define UDS_SID_ACK(Sid)                ((Sid) + 0x40)
#define UDS_SID_NACK                    0x7F

/** 否定响应码(negative response codes)     @see ISO-14229 */
#define UDS_NRC_GENERAL                0x10
#define UDS_NRC_SERVICE_N_SUPPORT      0x11 /** 服务不支持 */
#define UDS_NRC_SUBFUNC_N_SUPPORT      0x12 /** 子功能不支持 */
#define UDS_NRC_MSG_LEN_FORMAT_INVALID 0x13 /** 长度错误或无效格式 */
#define UDS_NRC_BUSY                   0x21
#define UDS_NRC_COND_N_CORRECT         0x22 /** 服务器执行诊断服务的条件不满足 */
#define UDS_NRC_REQ_SEQ_ERROR          0x24 /** 服务器认为诊断服务的请求（或者执行）顺序错误 */
#define UDS_NRC_REQ_OUT_OF_RANGE       0x31 /** 服务器没有客户端请求的数据，此否定响应适用于支持数据读、写，或者根据数据调整功能的服务器 */
#define UDS_NRC_SECURITY_ACCESS_DENY   0x33 /** 服务器阻止客户端的受限诊断服务请求 */
#define UDS_NRC_INVALID_KEY            0x35 /** 无效密钥 */
#define UDS_NRC_EXCEED_NUM_OF_ATTEM    0x36 /** 超出访问次数 */
#define UDS_NRC_REQ_TIME_DLY_N_EXP     0x37 /** 延迟时间未到 */
#define UDS_NRC_UL_DL_N_ACCEPTED       0x70 /* 上传/下载受限 */
#define UDS_NRC_TRANSFER_DATA_SUS      0x71
#define UDS_NRC_GENERAL_PROG_FAILURE   0x72 /** 再擦除或者烧写非易失性内存的过程中，服务器由于发现错误而终止诊断服务 */
#define UDS_NRC_WRONG_BLOCK_SEQ_CNT    0x73
#define UDS_NRC_REQ_PEDING             0x78 /** 挂起 */
#define UDS_NRC_SUBFUNC_N_SUPPORT_SESS 0x7E /** 在当前诊断模式下，服务器不支持客户端请求服务的子功能 */
#define UDS_NRC_SERVICE_N_SUPPORT_SESS 0x7F /** 在当前诊断模式下，服务器不支持客户端请求的SID */


/* Timing */
#define UDS_T_P2_CAN_SERVER             50
#define UDS_T_P2STAR_CAN_SERVER         2000
#define UDS_T_S3_SERVER                 5000

/* Sub Function IDs */

#define UDS_SUB_SUPRESSPOSRESP_BITMASK  0x80

/* 0x10 Diagnostic Session Control */
#define UDS_SUB_DEFAULT_SESSION         0x01
#define UDS_SUB_PROGRAM_SESSION         0x02
#define UDS_SUB_EXTENDED_SESSION        0x03

/* 0x11 ECU Reset */
#define UDS_SUB_HARD_RESET              0x01
#define UDS_SUB_KEY_OFF_ON_RESET        0x02
#define UDS_SUB_SOFT_RESET              0x03
#define UDS_SUB_EN_RAPID_PWR_SHUTDOWN   0x04
#define UDS_SUB_DIS_RAPID_PWR_SHUTDOWN  0x05

/* 0x27 Security Access */
#define UDS_SUB_REQ_SEED_BT             0x11
#define UDS_SUB_SEND_KEY_BT             0x12

/* 0x28 Communication Control */
#define UDS_SUB_EN_RX_EN_TX             0x00 /** Enable Rx and Tx */
#define UDS_SUB_EN_RX_DIS_TX            0x01 /** Enable Rx, Disable Tx */
#define UDS_SUB_DIS_RX_EN_TX            0x02 /** Disable Rx, Enable Tx */
#define UDS_SUB_DIS_RX_DIS_TX           0x03 /** Disable Rx and Tx */

#define UDS_COMM_TYPE_APP               1
#define UDS_COMM_TYPE_NM                2
#define UDS_COMM_TYPE_ALL               3

/* 0x83 Access Timing Parameter */
/* Not Support Now */

/* 0x84 Secured Data Transmission */
/* Not Support Now */

/* 0x85 Control DTC Setting */
#define UDS_SUB_DTC_ON                  0x01
#define UDS_SUB_DTC_OFF                 0x02

/* 0x86 Response On Event */
/* Not Support Now */

/* 0x87 Link Control */
/* Not Support Now */

/* 0x22 Read Data By Identifier */
/* | DID MSB | DID LSB | ... */
/* DID */

/* 0x23 Read Memory By Address */
/* | Format (Size Bit 7-4 Addr Bit 3-0) | Addr MSB | Addr ... | Size MSB | Size ... | */

/* 0x24 Read Scaling Data By Identifier */
/* Not Support Now */

/* 0x2A Read Data by Periodic Data */
/* Not Support Now */

/* 0x2C Dynamically Define Data Identifier */
#define UDS_SUB_DEF_BY_ID               0x01
#define UDS_SUB_DEF_BY_ADDR             0x02
#define UDS_SUB_DEF_CLR                 0x03

/* 0x2E Write Data By Identifier */
/* | DID MSB | DID LSB | Data 1 ... m | */

/* 0x3D Write Memory By Address */
/* | Format (Size Bit 7-4 Addr Bit 3-0) | Addr MSB | Addr ... | Size MSB | Size ... | Data ... | */

/* 0x14 Clear Diagnostic Information */
/* | Group 0 | Group 1 | Group 2 | */

/* 0x19 Read DTC Information */
#define UDS_SUB_REPORT_NUM_DTC_BY_MASK              0x01
#define UDS_SUB_REPORT_DTC_BY_MASK                  0x02
#define UDS_SUB_REPORT_DTC_DATA_BY_NUM              0x06
#define UDS_SUB_REPORT_MIRROR_MEM_DTC_DATA_BY_NUM   0x10
#define UDS_SUB_REPORT_NUM_OF_MIRROR_MEM_BY_MASK    0x11
#define UDS_SUB_REPORT_MIRROR_MEM_DTC_BY_MASK       0x0F
#define UDS_SUB_SUPPORTED_DTC                       0x0A

/* 0x2F Input Output Control By Identifier */
/* | DID MSB | DID LSB | Record | Mask | */
#define UDS_IOC_PARA_RETURN_CONTROL_TO_ECU          0x00
#define UDS_IOC_PARA_RESET_TO_DEFAULT               0x01
#define UDS_IOC_PARA_FREEZE_CURR_STATE              0x02
#define UDS_IOC_PARA_SHORT_TERM_ADJUSTMENT          0x03

/* 0x31 Routine Control */
/* | RID | SUB | RID MSB | RID LSB | Record * | */
#define UDS_SUB_START_ROUTINE                       0x01
#define UDS_SUB_STOP_ROUTINE                        0x02
#define UDS_SUB_REQUEST_ROUTINE_RESULTS             0x03

/* Security Access Parameters */
#define UDS_SECURITY_ATTEM_MAX                      3
#define UDS_SECURITY_ATTEM_DELAY                    10000   /* ms */

/* Data Types */

typedef enum _UdsSessionT {
	UDS_SESSION_TYPE_DEFAULT = UDS_SUB_DEFAULT_SESSION,
	UDS_SESSION_TYPE_PROGRAM = UDS_SUB_PROGRAM_SESSION,
	UDS_SESSION_TYPE_EXTENDED = UDS_SUB_EXTENDED_SESSION,
} UdsSessionT;

typedef struct _UdsCbT {
	bool		    Active;
	uint16_t		IdleTimeCnt;
	DoCanAddrT		AddrType;
	UdsSessionT		SessionType;
	bool			SecurityAccessEn;
	uint8_t			SecurityAccessAttempts;
	uint16_t		SecurityAtttemptDelay;
	uint8_t			SecuritySeed[4];
	bool			RidUnlock;
	uint8_t			OutCode[4];             /* OutCode[0] not Used, Fill with 0 */
	bool			CommAppRxEn;               /* Comm Control for Non-Diagnose Msg */
	bool			CommAppTxEn;
	bool			CommNmRxEn;               /* Comm Control for Non-Diagnose Msg */
	bool			CommNmTxEn;
	bool			DtcEn;
} UdsCbT;


/* GLOBAL VARIABLES */
extern uint8_t gBootModelKeepExit;

/* API */

extern void UdsInit(uint8_t BootFlag,uint8_t Unlock);
extern void UdsApduDispatch(DoCanAddrT AddrType, uint8_t *pReq, uint16_t ReqLen,
	uint8_t *pResp, uint16_t *pRespLen);
extern void UdsTimerProcess(uint8_t TimePass);
extern int UdsGetDtcSetting(void);
extern int UdsGetCommAppTxSetting(void);
extern int UdsGetCommAppRxSetting(void);
extern int UdsGetCommNmTxSetting(void);
extern int UdsGetCommNmRxSetting(void);
extern uint8_t   UdsGetactiveDiagSessionMode(void);
extern void UdsClearS3Time(void);
#endif/*__UDS_H__*/




