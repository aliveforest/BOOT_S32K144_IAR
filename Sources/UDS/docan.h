
/********************************************************************************
* FILE: docan.h
* DSCR: XXX
*
* Date: 2022/07/18 22:47 
********************************************************************************/
#ifndef __DOCAN_H__
#define __DOCAN_H__


#define CanSendMessage(id,data,len)		CanMsgTx(id,data,len)

#define DOCAN_TIMER_PERIOD			1			/* ms */
#define DOCAN_BUFFER_SIZE			(1024+2+6)	//(DOWNLOAD_BLOCK_LEN+6)

#define DOCAN_TX_SUPPORT_CANFD		1
#if (defined DOCAN_TX_SUPPORT_CANFD)&&(1==DOCAN_TX_SUPPORT_CANFD)
#define DOCAN_TX_PRAME_MAX_SIZE		64	//CANFD
#else
#define DOCAN_TX_PRAME_MAX_SIZE		8	//CAN2.0
#endif
#define DOCAN_PADDING_VALUE			0xAA

/* Address Information */

#define DOCAN_ADDR_RESP     0x0000079F
#define DOCAN_ADDR_FUNC     0x000007DF
#define DOCAN_ADDR_PHYS     0x0000071F

/* Default Parameters */

#define DOCAN_N_WFT_MAX     0
#define DOCAN_BS            8
#define DOCAN_ST_MIN        0   /* ms */
#define DOCAN_N_ASAR        70  /* ms */
#define DOCAN_N_BS          150 /* ms */
#define DOCAN_N_BR          70  /* ms */
#define DOCAN_N_CS          70  /* ms */
#define DOCAN_N_CR          150 /* ms */

#define DOCAN_N_ST_MIN_EX   1   /* ms Extra Delay */

//CAN 2.0
#define DOCAN_DL_MAX_SF			7	//DLC<=8 (0000 SF_DL[xxxx]) B1-B7(7)
#define DOCAN_DL_MAX_FF			6	//DLC<=4095 (0001 FF_DL[xxxx xxxx xxxx]) B2-B7(6)
#define DOCAN_DL_MAX_CF			7	//(0010 SN[xxxx]) B1-B7(7)

#define DOCAN_DLC_MAX_FF		4095

//CANFD
#define DOCAN_DL_MAX_FD_SF				62	//DLC>8 (0000 0000 SF_DL[xxxx xxxx]) B2-B63(62)
#define DOCAN_DL_MAX_FD_FF_LESS_4095	62	//DLC<=4095 (0001 FF_DL[xxxx xxxx xxxx]) B2-B63(62)
#define DOCAN_DL_MAX_FD_FF_MORE_4095	58	//DLC>4095 (0001 0000 0000 0000 FF_DL[xxxx xxxx xxxx](2345)) B6-B63(58)
#define DOCAN_DL_MAX_FD_CF				63	//(0010 SN[xxxx]) B1-B63(63)


/* Protocol Control Information Spec */
#define DOCAN_PCITYPE_SF		0 /* Single Frame */
#define DOCAN_PCITYPE_FF		1 /* First Frame */
#define DOCAN_PCITYPE_CF		2 /* Consecutive Frame */
#define DOCAN_PCITYPE_FC		3 /* Flow Control */

#define DOCAN_FLOWSTATUS_CTS	0 /* Continue to Send */
#define DOCAN_FLOWSTATUS_WAIT	1 /* Continue to Wait */
#define DOCAN_FLOWSTATUS_OVF	2 /* Continue to Overflow */

/* Types Definition */
#define DOCAN_GET_PCITYPE(pData) \
    ((((uint8_t *)(pData))[0] >> 4) & 0x0F)

#define DOCAN_SET_PCITYPE(pData, Type) \
    ((uint8_t *)(pData))[0] = (((uint8_t *)(pData))[0] & 0x0F) | ((Type) << 4)

/* Single Frame */
#define DOCAN_SF_GET_DL(pData) \
    (((uint8_t *)(pData))[0] & 0x0F)

#define DOCAN_SF_SET_DL(pData, DataLen) \
    ((uint8_t *)(pData))[0] = (((uint8_t *)(pData))[0] & 0xF0) | (DataLen)

#define DOCAN_SF_GET_DL_O8(pData) \
    (((uint8_t *)(pData))[1] & 0xFF)

#define DOCAN_SF_SET_DL_O8(pData, DataLen)\
    ((uint8_t *)(pData))[1] = ((DataLen) & 0xFF)


/* First Frame */
#define DOCAN_FF_GET_DL(pData) \
    ( ((uint16)(((uint8_t *)(pData))[0] & 0x0F) << 8) | ((uint8_t *)(pData))[1] )

#define DOCAN_FF_SET_DL(pData, DataLen) { \
    ((uint8_t *)(pData))[0] = (((uint8_t *)(pData))[0] & 0xF0) | ((DataLen) >> 8); \
    ((uint8_t *)(pData))[1] = ((DataLen) & 0xFF); \
}

#define DOCAN_FF_GET_DL_O4095(pData) \
     (((uint32_t)(((uint8_t *)(pData))[2]) << 24)\
		|((uint32_t)(((uint8_t *)(pData))[3]) << 16)\
		|((uint32_t)(((uint8_t *)(pData))[4]) << 8)\
		|((uint32_t)(((uint8_t *)(pData))[5])))

#define DOCAN_FF_SET_DL_O4095(pData, DataLen) { \
    ((uint8_t *)(pData))[2] = (uint8_t)((DataLen) >> 24); \
    ((uint8_t *)(pData))[3] = (uint8_t)((DataLen) >> 16); \
	((uint8_t *)(pData))[4] = (uint8_t)((DataLen) >> 8); \
    ((uint8_t *)(pData))[5] = (uint8_t)((DataLen) & 0xFF); \
}

/* Consecutive Frame */
#define DOCAN_CF_GET_SN(pData) \
    (((uint8_t *)(pData))[0] & 0x0F)

#define DOCAN_CF_SET_SN(pData, SeqNum) \
    ((uint8_t *)(pData))[0] = (((uint8_t *)(pData))[0] & 0xF0) | (SeqNum)

/* Flow Control */
#define DOCAN_FC_GET_FS(pData) \
    (((uint8_t *)(pData))[0] & 0x0F)

#define DOCAN_FC_SET_FS(pData, FlowStatus) \
    ((uint8_t *)(pData))[0] = (((uint8_t *)(pData))[0] & 0xF0) | (FlowStatus)

#define DOCAN_FC_GET_BS(pData) \
    (((uint8_t *)(pData))[1])

#define DOCAN_FC_SET_BS(pData, BlockSize) \
    ((uint8_t *)(pData))[1] = (BlockSize)

#define DOCAN_FC_GET_STMIN(pData) \
    (((uint8_t *)(pData))[2])

#define DOCAN_FC_SET_STMIN(pData, StMin) \
    ((uint8_t *)(pData))[2] = (StMin)


typedef enum _DoCanAddrT {
	DOCAN_ADDR_TYPE_FUNCTIONAL = 0,
	DOCAN_ADDR_TYPE_PHYSICAL,
}DoCanAddrT;

/* API */
extern void DoCanInit(void);
extern void DoCanMsgProcess(uint32_t CanId, uint8_t *pData, uint8_t Num);
extern void DoCanTimerProcess(void);

#endif/*__DOCAN_H__*/




