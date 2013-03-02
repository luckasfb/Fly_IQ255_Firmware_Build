/*******************************************************************************
* Copyright 2010 Broadcom Corporation.  All rights reserved.
*
* 	@file	drivers/video/broadcom/dss/bcm215xx/lcdc.c
*
* Unless you and Broadcom execute a separate written software license agreement
* governing use of this software, this software is licensed to you under the
* terms of the GNU General Public License version 2, available at
* http://www.gnu.org/copyleft/gpl.html (the "GPL").
*
* Notwithstanding the above, under no circumstances may you combine this
* software in any way with any other Broadcom software provided under a license
* other than the GPL, without Broadcom's express prior written consent.
*******************************************************************************/

/****************************************************************************
*
*  lcd_dsic.c
*
*  PURPOSE:
*    This implements the code to use a Broadcom Panel serial interface.
*
*  NOTES:
*    Uses device minor number to select panel:  0==main 1==sub
*
*****************************************************************************/

/* ---- Include Files ---------------------------------------------------- */
#include "lcdc_common.h"
#include "display_drv.h"        // Disp Drv Commons
#include "dispdrv_mipi_dcs.h"   // MIPI DCS
#include "dispdrv_mipi_dsi.h"   // MIPI DSI

#if defined(CONFIG_BCM_LCD_HX8369)//for HX8369
#if defined(CONFIG_BCM_LCD_TFT48080063E)
#include "../../displays/lcd_hx8369a_TFT480800.h"
#else
#include "../../displays/lcd_hx8369a.h"
#endif
#define CONFIG_ARGB8888
#elif defined(CONFIG_BCM_LCD_HX8357)//for HX8357
#include "../../displays/lcd_hx8357c.h"
#define CONFIG_ARGB8888
#elif defined(CONFIG_BCM_LCD_HX8363)//for HX8363
#include "../../displays/lcd_hx8363b.h"
#define CONFIG_ARGB8888
#endif 

#include "lcdc_dsi.h"

/**************************************************************************/
//static CSL_DSI_TE_IN_CFG_t  dsi_te;
//static CSL_DSI_CM_VC_t dsi_cm_vc;
//static CSL_DSI_CFG_t dsi_bus_cfg;
#ifndef _BRCM_8BYTE_MSG_CONSTRAINT
LCD_FrameBuffer_t base_params_buffer;
#endif


typedef struct
{
    CSL_LCD_HANDLE      clientH;        // DSI Client Handle
    CSL_LCD_HANDLE      dsiCmVcHandle;  // DSI CM VC Handle
    UInt32              busId;
    UInt32              teIn;
    UInt32              teOut;
} dsic_panel_t;
static  dsic_panel_t  panel[2];
static DISPDRV_CB_T dispdrv_cb = NULL;
static struct semaphore gDmaSema;
static DISPDRV_HANDLE_T display_hdl = NULL;


/*----- Extern Declarations-----------------------------------------------*/
extern int lcd_reset_gpio;
extern void __iomem *lcdc_base;

#ifdef CONFIG_CPU_FREQ_GOV_BCM21553
extern struct cpufreq_client_desc *lcdc_client;
#endif

/* globals to: communicate with the update thread */
/*   control access to LCD registers */
/*  manage DMA channel */
extern int gInitialized;
#ifdef CONFIG_HAS_WAKELOCK
extern struct wake_lock glcdfb_wake_lock;
#endif

#ifdef CONFIG_REGULATOR
extern struct regulator *lcdc_regulator;
#endif
extern bool ap_crashed;

/* ---- Functions -------------------------------------------------------- */
// LOCAL FUNCTIONs
static int      dsi_ioctlrd(
                    DISPDRV_HANDLE_T        drvH,
                    DISPDRV_CTRL_RW_REG*   acc );

static void     dsi_ioctlwr(
                    DISPDRV_HANDLE_T        drvH,
                    DISPDRV_CTRL_RW_REG*   acc );

static void     dsi_wrcmndP0  (
                    DISPDRV_HANDLE_T        drvH,
                    UInt32                  reg );

static void     dsi_wrcmndP1 (
                    DISPDRV_HANDLE_T        drvH,
                    UInt32                  reg,
                    UInt32                  val );


/* ---- Functions -------------------------------------------------------- */
//DSI Specific Local Functions.
//*****************************************************************************
//
// Function Name: dsi_teon
//
// Description:   Configure TE Input Pin & Route it to DSI Controller Input
//
//*****************************************************************************
static int dsi_teon ( dsic_panel_t *pPanel )
{
    Int32       res = 0;
	board_sysconfig(SYSCFG_LCD, SYSCFG_ENABLE);
    return ( res );
}

//*****************************************************************************
//
// Function Name: dsi_teoff
//
// Description:   'Release' TE Input Pin Used
//
//*****************************************************************************
static int dsi_teoff ( dsic_panel_t *pPanel )
{
    Int32  res = 0;
    return ( res );
}


//*****************************************************************************
//
// Function Name:  dsi_wrcmndPN
//
// Parameters:     reg   = 08-bit register address (DCS command)
//                 value = 08-bit register data    (DCS command parm)
//
// Description:    Register Write - DCS command byte, 1 parm
//
//*****************************************************************************
static void dsi_wrcmndPN(
    DISPDRV_HANDLE_T    drvH,
    UInt32              reg,
    UInt32		datasize,
    UInt8*              dataptr
    )
{
#ifdef _BRCM_8BYTE_MSG_CONSTRAINT
	if(datasize <8)
	{//datasize <=7

	    dsic_panel_t *pPanel = (dsic_panel_t *)drvH;
	    CSL_DSI_CMND_t      msg;
	    UInt8               msgData [(datasize+1)];
		int index = 0;

	    msg.dsiCmnd    = DSI_DT_LG_DCS_WR;
	    msg.msg        = &msgData[0];
	    msg.msgLen     = datasize+1;
	    msg.vc         = DSI_VC;
	    msg.isLP       = DSI_CMND_IS_LP;
	    msg.isLong     = TRUE;
	    msg.endWithBta = FALSE;

	    msgData[0] = reg;
		for(index = 0; index < datasize; index++)
		{
			msgData[index+1] = dataptr[index] & 0x000000FF;
		}
	    CSL_DSI_SendPacket (pPanel->clientH, &msg);
	}
	else
	{//TODO:following codes is just for HX8396, it need to be replaced with DATA_CMD_FIFO+PIXEL_FIFO
		dsic_panel_t *pPanel = (dsic_panel_t *)drvH;
	    CSL_DSI_CMND_t      msg;
	    UInt8               msgData [8];
		int index = 0;

		int sendindex[10];
		int sendsize[10];
		int sendcounter = 0;
		int temp = 0;
		sendindex[0]=0;
		sendsize[0]=5;

		index += 5;
		sendcounter = 1;
		//calculate size
		while(index < datasize)
		{
			if((datasize-index)<6)
			{
				sendsize[sendcounter]=(datasize-index);
				sendcounter++;
				break;
			}
			else
			{
				sendsize[sendcounter]=6;
				index += 6;
			}

			sendcounter++;
		}

		//calculate index
		for(index = 1; index < sendcounter; index++)
		{
			temp += sendsize[index-1];
			sendindex[index]=temp-1;
		}

		for(temp = 0; temp < sendcounter; temp++)
		{
		    int size = (temp == 0) ? sendsize[temp]: (sendsize[temp]+1);
		    msg.dsiCmnd    = DSI_DT_LG_DCS_WR;
		    msg.msg        = &msgData[0];
			if(temp == 0)
			msg.msgLen     = sendsize[temp]+1;
			else
			msg.msgLen     = sendsize[temp]+2;
		msg.vc         = DSI_VC;
		msg.isLP       = DSI_CMND_IS_LP;
		msg.isLong     = TRUE;
		msg.endWithBta = FALSE;

			if(temp == 0)
			msgData[0] = reg;
			else
				msgData[0] = 0xFD;
			for(index = 0; index < size; index++)
			{
				msgData[index+1] = dataptr[sendindex[temp]+index] & 0x000000FF;
			}
		CSL_DSI_SendPacket (pPanel->clientH, &msg);
		}
	}
#else
	    dsic_panel_t *pPanel = (dsic_panel_t *)drvH;
	    CSL_DSI_CMND_t      msg;
	    UInt8               msgData[(datasize + 1)];
	    int index = 0;

	    msg.dsiCmnd    = DSI_DT_LG_DCS_WR;
	    msg.msg        = &msgData[0];
	    msg.msgLen     = datasize+1;
	    msg.vc         = DSI_VC;
	    msg.isLP       = DSI_CMND_IS_LP;
	    msg.isLong     = TRUE;
	    msg.endWithBta = FALSE;

	    //printk("[dsi_wrcmndPN]msgLen:%d\n",msg.msgLen);
	    msgData[0] = reg;
	    for(index = 0; index < datasize; index++)
	    {
		msgData[index+1] = dataptr[index] & 0x000000FF;
	    }

	    CSL_DSI_SendPacket (pPanel->clientH, &msg);
#endif	//_BRCM_8BYTE_MSG_CONSTRAINT
}



static void dsi_set_max_ret_pkt_size(
    DISPDRV_HANDLE_T    drvH,
    UInt32              reg
    )
{
    dsic_panel_t *pPanel = (dsic_panel_t *)drvH;
    CSL_DSI_CMND_t      msg;
    UInt8               msgData[4];
    msg.dsiCmnd    = DSI_DT_SH_MAX_RET_PKT_SIZE;
    msg.msg        = &msgData[0];
    msg.msgLen     = 2;
    msg.vc         = DSI_VC;
    msg.isLP       = DSI_CMND_IS_LP;
    msg.isLong     = FALSE;
    msg.endWithBta = FALSE;

    msgData[0] = 2;
    msgData[1] = 0;
    CSL_DSI_SendPacket (pPanel->clientH, &msg);
}


//*****************************************************************************
//
// Function Name:  dsi_wrcmndP1
//
// Parameters:     reg   = 08-bit register address (DCS command)
//                 value = 08-bit register data    (DCS command parm)
//
// Description:    Register Write - DCS command byte, 1 parm
//
//*****************************************************************************
static void dsi_wrcmndP1(
    DISPDRV_HANDLE_T    drvH,
    UInt32              reg,
    UInt32              value
    )
{
    dsic_panel_t *pPanel = (dsic_panel_t *)drvH;
    CSL_DSI_CMND_t      msg;
    UInt8               msgData[4];
    msg.dsiCmnd    = DSI_DT_SH_DCS_WR_P1;
    msg.msg        = &msgData[0];
    msg.msgLen     = 2;
    msg.vc         = DSI_VC;
    msg.isLP       = DSI_CMND_IS_LP;
    msg.isLong     = FALSE;
    msg.endWithBta = FALSE;

    msgData[0] = reg;
    msgData[1] = value & 0x000000FF;
    CSL_DSI_SendPacket (pPanel->clientH, &msg);
}

//*****************************************************************************
//
// Function Name:  dsi_wrcmndP0
//
// Parameters:     reg   = 08-bit register address (DCS command)
//
// Description:    Register Write - DCS command byte, 0 parm
//
//*****************************************************************************
static void dsi_wrcmndP0(
    DISPDRV_HANDLE_T    drvH,
    UInt32              reg
    )
{
    dsic_panel_t *pPanel = (dsic_panel_t *)drvH;
    CSL_DSI_CMND_t      msg;
    UInt8               msgData[4];
    msg.dsiCmnd    = DSI_DT_SH_DCS_WR_P0;
    msg.msg        = &msgData[0];
    msg.msgLen     = 2;
    msg.vc         = DSI_VC;
    msg.isLP       = DSI_CMND_IS_LP;
    msg.isLong     = FALSE;
    msg.endWithBta = FALSE;

    msgData[0] = reg;
    msgData[1] = 0;
    CSL_DSI_SendPacket (pPanel->clientH, &msg);
}

//*****************************************************************************
//
// Function Name:  dsi_ioctlwr
//
// Parameters:
//
// Description:    IOCTL WR Test Code - DCS Wr With P0(no parm) or P1(1 parm)
//
//*****************************************************************************
static void dsi_ioctlwr(
    DISPDRV_HANDLE_T       drvH,
    DISPDRV_CTRL_RW_REG*   acc
    )
{
    if( acc->parmCount == 1 )
    {
        dsi_wrcmndP0 ( drvH, acc->cmnd );
        LCD_DBG ( LCD_DBG_INIT_ID, "[DISPDRV] %s: DSC+P0 "
            "DCS[0x%08X]\n\r", __FUNCTION__, (unsigned int)acc->cmnd );
    }
    else if( acc->parmCount == 2 )
    {
        dsi_wrcmndP1 ( drvH, acc->cmnd, *((UInt8*)acc->pBuff) );
        LCD_DBG ( LCD_DBG_INIT_ID, "[DISPDRV] %s: DSC+P1 "
            "DCS[0x%08X] P[0x%08X]\n\r", __FUNCTION__,
            (unsigned int)acc->cmnd, (unsigned int)*((UInt8*)acc->pBuff) );
    }
    else
    {
        LCD_DBG ( LCD_DBG_ERR_ID, "[DISPDRV] dsi_ioctlwr: "
            "Only DCS with 0|1 Parm Supported\n" );
    }
} // dsi_ioctlwr

//*****************************************************************************
//
// Function Name:  dsi_ioctlrd
//
// Parameters:
//
// Description:    IOCTL RD Test Code - DCS Rd
//
//*****************************************************************************
static int dsi_ioctlrd(
    DISPDRV_HANDLE_T       drvH,
    DISPDRV_CTRL_RW_REG*   acc
    )
{
    dsic_panel_t  *pPanel = (dsic_panel_t *)drvH;
    CSL_DSI_CMND_t      msg;
    CSL_DSI_REPLY_t     rxMsg;
    UInt8               txData[1];  // DCS Rd Command
    UInt32              reg;
    UInt8 *             pRxBuff = (UInt8*)acc->pBuff;
    Int32               res = 0;
    CSL_LCD_RES_T       cslRes;
    memset( (void*)&rxMsg, 0, sizeof(CSL_DSI_REPLY_t) );
    rxMsg.pReadReply = pRxBuff;
    msg.dsiCmnd    = DSI_DT_SH_DCS_RD_P0;
    msg.msg        = &txData[0];
    msg.msgLen     = 1;
    msg.vc         = DSI_VC;
    msg.isLP       = DSI_CMND_IS_LP;
    msg.isLong     = FALSE;
    msg.endWithBta = TRUE;
    msg.reply      = &rxMsg;

    txData[0] = acc->cmnd;
    cslRes = CSL_DSI_SendPacket ( pPanel->clientH, &msg );
    if( cslRes != CSL_LCD_OK )
    {
        LCD_DBG ( LCD_DBG_ERR_ID, "[DISPDRV] %s: ERR"
            "Reading From Reg[0x%08X]\n\r", __FUNCTION__, (unsigned int)acc->cmnd );
        res = -1;
    }
    else
    {
        reg = pRxBuff[0];
        LCD_DBG ( LCD_DBG_INIT_ID, "[DISPDRV] %s: Reg[0x%08X] "
            "Value[0x%08X]\n\r", __FUNCTION__, (unsigned int)acc->cmnd, (unsigned int)reg );
#if 0
        LCD_DBG ( LCD_DBG_INIT_ID, "   TYPE    : %s\n"    ,
            DISPDRV_dsiCslRxT2text (rxMsg.type,dsiE) );
#endif

        if( rxMsg.type & DSI_RX_TYPE_TRIG )
        {
            LCD_DBG ( LCD_DBG_INIT_ID, "   TRIG    : 0x%08X\n", (unsigned int)rxMsg.trigger );
        }

        if( rxMsg.type & DSI_RX_TYPE_READ_REPLY )
        {
#if 0
            LCD_DBG ( LCD_DBG_INIT_ID, "   RD DT   : %s\n"    ,
                DISPDRV_dsiRxDt2text(rxMsg.readReplyDt,dsiE) );
#endif
            LCD_DBG ( LCD_DBG_INIT_ID, "   RD STAT : 0x%08X\n", (unsigned int)rxMsg.readReplyRxStat );
            LCD_DBG ( LCD_DBG_INIT_ID, "   RD SIZE : %d\n"    , rxMsg.readReplySize );
            LCD_DBG ( LCD_DBG_INIT_ID, "   RD BUFF : 0x%02X 0x%02X 0x%02X 0x%02X "
                                   "0x%02X 0x%02X 0x%02X 0x%02X\n",
                pRxBuff[0], pRxBuff[1],  pRxBuff[2], pRxBuff[3],
                pRxBuff[4], pRxBuff[5],  pRxBuff[6], pRxBuff[7] );
        }

        if( rxMsg.type & DSI_RX_TYPE_ERR_REPLY )
        {
#if 0
            LCD_DBG ( LCD_DBG_INIT_ID, "   ERR DT  : %s\n"    ,
                DISPDRV_dsiRxDt2text (rxMsg.errReportDt,dsiE) );
#endif
            LCD_DBG ( LCD_DBG_INIT_ID, "   ERR STAT: 0x%08X\n", (unsigned int)rxMsg.errReportRxStat );
#if 0
            LCD_DBG ( LCD_DBG_INIT_ID, "   ERR     : %s\n"    ,
                DISPDRV_dsiErr2text (rxMsg.errReport, dsiE) );
#endif
        }
    }
    return ( res );
}


//*****************************************************************************
//
// Function Name: dsic_exit
//
// Description:
//
//*****************************************************************************
Int32 dsic_exit ( void )
{
    LCD_DBG ( LCD_DBG_ERR_ID, "[DISPDRV] %s: Not Implemented\n\r",
        __FUNCTION__ );
    return ( -1 );
}

//*****************************************************************************
//
// Function Name: dsic_open
//
// Description:   Open Drivers
//
//*****************************************************************************
#ifndef _BRCM_8BYTE_MSG_CONSTRAINT
Int32 dsic_open (
	pCSL_DSI_INIT_CFG 	pInitCfg,
    const void*         params,
    DISPDRV_HANDLE_T*   drvH,
    void		*ptr
    )
#else
Int32 dsic_open (
	pCSL_DSI_INIT_CFG   pInitCfg,
    const void*         params,
    DISPDRV_HANDLE_T*   drvH
    )
#endif
{
    Int32                         res = 0;
    UInt32                        busId;
    const DISPDRV_OPEN_PARM_T*    pOpenParm;
    dsic_panel_t         *pPanel;

    //busCh - NA to DSI interface
    pOpenParm = (DISPDRV_OPEN_PARM_T*) params;
    busId     = pOpenParm->busCh;

    #define BUS_ID_MAX  0

    if( busId > BUS_ID_MAX )
    {
        LCD_DBG ( LCD_DBG_ERR_ID, "[DISPDRV] %s: ERROR Invalid DSI Bus[%d]\n\r",
            __FUNCTION__, (unsigned int)busId );
        return ( -1 );
    }

    pPanel = &panel[busId];

    dsiCfg.bus = busId;
    //pPanel->pFb = pPanel->pFbA = (void*)pOpenParm->busId;

	if(dsiVcCmCfg.teCfg.teInType != DSI_TE_NONE)
	{
	if( dsi_teon( pPanel ) ==  -1 )
	{
		LCD_DBG ( LCD_DBG_ERR_ID, "[DISPDRV] %s: "
		"Failed To Configure TE Input\n", __FUNCTION__ );
		return ( -1 );
	}
	}

#ifndef _BRCM_8BYTE_MSG_CONSTRAINT
	if ( CSL_DSI_Init( pInitCfg, &dsiCfg, ptr ) != CSL_LCD_OK )
#else
	if ( CSL_DSI_Init( pInitCfg, &dsiCfg ) != CSL_LCD_OK )
#endif	//_BRCM_8BYTE_MSG_CONSTRAINT
    {
        LCD_DBG ( LCD_DBG_ERR_ID, "[DISPDRV] %s: ERROR, DSI CSL Init "
            "Failed\n\r", __FUNCTION__ );
        return ( -1 );
    }

    if ( CSL_DSI_OpenClient ( busId, &pPanel->clientH ) != CSL_LCD_OK )
    {
        LCD_DBG ( LCD_DBG_ERR_ID, "[DISPDRV] %s: ERROR, CSL_DSI_OpenClient "
            "Failed\n\r", __FUNCTION__);
        return ( -1 );
    }

    if ( CSL_DSI_OpenCmVc ( pPanel->clientH, &dsiVcCmCfg, &pPanel->dsiCmVcHandle )
            != CSL_LCD_OK )
    {
        LCD_DBG ( LCD_DBG_ERR_ID, "[DISPDRV] %s: CSL_DSI_OpenCmVc Failed\n\r",
            __FUNCTION__);
        return ( -1 );
    }

    pPanel->busId      = busId;

    LCD_device[LCD_main_panel].dirty_rect.left   = 0;
    LCD_device[LCD_main_panel].dirty_rect.right  = PANEL_WIDTH-1;
    LCD_device[LCD_main_panel].dirty_rect.top    = 0;
    LCD_device[LCD_main_panel].dirty_rect.bottom = PANEL_HEIGHT-1;

    *drvH = (DISPDRV_HANDLE_T) pPanel;
    LCD_DBG ( LCD_DBG_INIT_ID, "[DISPDRV] %s: OK\n\r", __FUNCTION__ );

    return ( res );
}

//*****************************************************************************
//
// Function Name: dsic_close
//
// Description:   Close The Driver
//
//*****************************************************************************
Int32 dsic_close ( DISPDRV_HANDLE_T drvH )
{
    Int32                   res = 0;
    dsic_panel_t   *pPanel = (dsic_panel_t *)drvH;

    //pPanel->pFb  = NULL;
    //pPanel->pFbA = NULL;

    if ( CSL_DSI_CloseCmVc ( pPanel->dsiCmVcHandle ) )
    {
        LCD_DBG ( LCD_DBG_ERR_ID, "[DISPDRV] %s: ERROR, "
            "Closing Command Mode Handle\n\r", __FUNCTION__);
        return ( -1 );
    }

    if ( CSL_DSI_CloseClient ( pPanel->clientH ) != CSL_LCD_OK )
    {
        LCD_DBG ( LCD_DBG_ERR_ID, "[DISPDRV] %s: ERROR, Closing DSI Client\n\r",
            __FUNCTION__);
        return ( -1 );
    }

    if ( CSL_DSI_Close( pPanel->busId ) != CSL_LCD_OK )
    {
        LCD_DBG ( LCD_DBG_ERR_ID, "[DISPDRV] %s: ERR Closing DSI Controller\n\r",
            __FUNCTION__ );
        return ( -1 );
    }

    LCD_DBG ( LCD_DBG_INIT_ID, "[DISPDRV] %s: OK\n\r", __FUNCTION__ );

    return ( res );
}

void exec_cmnd_list(
    DISPDRV_HANDLE_T     dispH,
    Boolean              useOs,
    Lcd_init_t*           cmnd_lst
    )
{
    UInt32  i = 0;

    while (cmnd_lst[i].type != CTRL_END)
    {

		if (cmnd_lst[i].type == WR_CMND_MULTIPLE_DATA)
        {
		dsi_wrcmndPN (dispH, cmnd_lst[i].cmd,
                cmnd_lst[i].datasize, cmnd_lst[i].dataptr);
        }
		else if (cmnd_lst[i].type == WR_CMND_DATA)
        {
            dsi_wrcmndP1 (dispH, cmnd_lst[i].cmd,
                cmnd_lst[i].data);
        }
        else if (cmnd_lst[i].type == WR_CMND)
        {
            dsi_wrcmndP0 (dispH, cmnd_lst[i].cmd);
        }
        else if (cmnd_lst[i].type == SLEEP_MS)
        {
            if ( useOs )
            {
                OSTASK_Sleep ( TICKS_IN_MILLISECONDS(cmnd_lst[i].data) );
            }
            else
            {
				mdelay( cmnd_lst[i].data );
            }
        }
        i++;
    }
} // execCmndList

//*****************************************************************************
//
// Function Name:   dsi_setwindow
//
// Description:
//
//*****************************************************************************
static Int32 dsi_setwindow ( DISPDRV_HANDLE_T drvH,LCD_dev_info_t* dev )
{
	dsic_panel_t *pPanel = (dsic_panel_t *)drvH;
	UInt8 dataptr[4];

	{//write Set_clumn_address(2A)
		dataptr[0]= (dev->col_start) >> 8;
		dataptr[1]= dev->col_start & 0xFF;
		dataptr[2]= (dev->col_end) >> 8;
		dataptr[3]= dev->col_end & 0xFF;
		dsi_wrcmndPN (drvH, 0x2A, 4, dataptr);
	}

	{//write Set_page_address(2B)
                dataptr[0]= (dev->row_start) >> 8;
                dataptr[1]= dev->row_start & 0xFF;
                dataptr[2]= (dev->row_end) >> 8;
                dataptr[3]= dev->row_end & 0xFF;
		dsi_wrcmndPN (drvH, 0x2B, 4, dataptr);
	}
	//Write_memory_start (2Ch)
     dsi_wrcmndP0 ( drvH, 0x2C );
     return 0;
} // dsi_setwindow


//*****************************************************************************
//
// Function Name: dsic_set_control
//
// Description:
//
//*****************************************************************************
Int32 dsic_set_control (
        DISPDRV_HANDLE_T    drvH,
        DISPDRV_CTRL_ID_T   ctrlID,
        void*               ctrlParams
        )
{
    Int32 res = -1;
    switch ( ctrlID )
    {
        case DISPDRV_CTRL_ID_SET_REG:
            dsi_ioctlwr( drvH, (DISPDRV_CTRL_RW_REG*)ctrlParams );
            res = 0;
            break;

        default:
            LCD_DBG ( LCD_DBG_ERR_ID, "[DISPDRV] %s: "
                "CtrlId[%d] Not Implemented\n\r", __FUNCTION__, ctrlID );
            break;
    }
    return ( res );
}

//*****************************************************************************
//
// Function Name: dsic_get_control
//
// Description:
//
//*****************************************************************************
Int32 dsic_get_control (
            DISPDRV_HANDLE_T    drvH,
            DISPDRV_CTRL_ID_T   ctrlID,
            void*               ctrlParams
            )
{
    dsic_panel_t *pPanel = (dsic_panel_t *)drvH;
    Int32 res = -1;

    switch ( ctrlID )
    {
        case DISPDRV_CTRL_ID_GET_FB_ADDR:
            //((DISPDRV_CTL_GET_FB_ADDR *)ctrlParams)->frame_buffer = pPanel->pFbA;
            ((DISPDRV_CTL_GET_FB_ADDR *)ctrlParams)->frame_buffer =
		LCD_device[LCD_main_panel].frame_buffer.physPtr;
            res = 0;
            break;

        case DISPDRV_CTRL_ID_GET_REG:
			    if(1)
				{//Kevin.Wen:FIXME
					UInt8 dataprt[] = {0xFF, 0x83, 0x69};
					dsi_wrcmndPN (drvH, 0xB9, 3, dataprt);

					dsi_set_max_ret_pkt_size(drvH, 0x02);
			    }
            res = dsi_ioctlrd( drvH, (DISPDRV_CTRL_RW_REG*)ctrlParams );
            break;

        default:
            LCD_DBG ( LCD_DBG_ERR_ID, "[DISPDRV] %s: CtrlId[%d] Not "
                "Implemented\n\r", __FUNCTION__, ctrlID );
            break;
    }

    return ( res );
}


//*****************************************************************************
//
// Function Name: dsic_start
//
// Description:   Configure For Updates
//
//*****************************************************************************
Int32 dsic_start ( DISPDRV_HANDLE_T drvH )
{
    Int32 res = 0;
    return ( res );
}

//*****************************************************************************
//
// Function Name: dsic_stop
//
// Description:
//
//*****************************************************************************
Int32 dsic_stop ( DISPDRV_HANDLE_T drvH )
{
    Int32 res = 0;
    return ( res );
}

//*****************************************************************************
//
// Function Name: dsic_get_info
//
// Description:
//
//*****************************************************************************
const LCD_dev_info_t* dsic_get_info ( DISPDRV_HANDLE_T drvH )
{
    return (&LCD_device[LCD_main_panel]);
}

//*****************************************************************************
//
// Function Name: dsic_info
//
// Description:
//
//*****************************************************************************
Int32 dsic_info ( UInt32 *screenWidth, UInt32 *screenHeight, UInt32 *bpp, UInt32 *resetGpio)
{
   Int32 res = 0;
	*screenWidth = LCD_WIDTH;
	*screenHeight = LCD_HEIGHT;
	*bpp = INPUT_BPP;
	*resetGpio = RESET_GPIO;

   return ( res );
}

#if 0
//*****************************************************************************
//
// Function Name: dsic_update
//
// Description:   DMA/OS Update using INT frame buffer
//                Call this only when the full frame update is required.
//
//*****************************************************************************
static Int32 dsic_update (
    DISPDRV_HANDLE_T  drvH,
    LCD_dev_info_t  * dev,
    int				fb_idx
    )
{
    dsic_panel_t *pPanel = (dsic_panel_t *)drvH;
    CSL_LCD_UPD_REQ_T   req;
    Int32               res  = 0;
	bool                multiLLI_Set = true;
	OSDAL_Dma_Buffer_List *buffer_list=NULL, *temp_list=NULL;

    //LCD_DBG ( LCD_DBG_ID, "[DISPDRV] +%s\r\n", __FUNCTION__ );

    if (down_killable(&gDmaSema))
		return -1;

#ifdef CONFIG_HAS_WAKELOCK
		wake_lock(&glcdfb_wake_lock);
#endif

	if(multiLLI_Set == true)
	{
		void* src_buff = NULL;
		int i=0;
		buffer_list = kzalloc((dev->height) * sizeof(OSDAL_Dma_Buffer_List), GFP_KERNEL);
		if (!buffer_list) {
	        LCD_DBG ( LCD_DBG_ERR_ID, "[DISPDRV] +%s: Could not allocate memory\r\n",
	            __FUNCTION__ );
			res = -1;
			goto done;
		}

		if (0 == fb_idx)
           src_buff = dev->frame_buffer.physPtr;//pPanel->pFbA;
	    else
           src_buff = (void *)((UInt32)dev->frame_buffer.physPtr + dev->width * dev->height * INPUT_BPP);
           //(void *)((UInt32)pPanel->pFbA + dev->width * dev->height * INPUT_BPP);

		temp_list = buffer_list;
		for (i = 0; i < dev->height; i++) {
			temp_list->buffers[0].srcAddr  = (UInt32)src_buff + (i * dev->width * INPUT_BPP);
			temp_list->buffers[0].destAddr = 0x084a0140;
			temp_list->buffers[0].length   = dev->width * INPUT_BPP;
			temp_list->buffers[0].bRepeat  = 0;
			temp_list++;
		}
		temp_list--;		/* Go back to the last list item to set interrupt = 1 */
		temp_list->buffers[0].interrupt = 1;

		req.buff = (void *)buffer_list;
		req.multiLLI = true;
	}
	else if( multiLLI_Set == false)
	{
	    if (0 == fb_idx)
           req.buff = dev->frame_buffer.physPtr;//pPanel->pFbA;
	    else
		    req.buff = (void *)((UInt32)dev->frame_buffer.physPtr + dev->width * dev->height * INPUT_BPP);
           //(void *)((UInt32)pPanel->pFbA + dev->width * dev->height * INPUT_BPP);

        req.multiLLI = false;
	}

    req.lineLenP       = dev->width;
    req.lineCount      = dev->height;
    req.buffBpp        = INPUT_BPP;
    req.timeOut_ms     = 100;
    //printk(KERN_ERR "buf=%08x, linelenp = %d, linecnt =%d\n", (u32)req.buff, req.lineLenP, req.lineCount);
    req.cslLcdCbRef 	= NULL;
    req.cslLcdCb 		= NULL;
	dev->col_start = 0;
	dev->col_end   = (dev->width -1);
	dev->row_start = 0;
	dev->row_end   = (dev->height-1);
    dsi_setwindow(drvH,dev);

   if ( CSL_DSI_UpdateCmVc ( pPanel->dsiCmVcHandle, &req ) != CSL_LCD_OK )
   {
        LCD_DBG ( LCD_DBG_ERR_ID, "[DISPDRV] %s: ERROR ret by "
            "CSL_DSI_UpdateCmVc\n\r", __FUNCTION__ );
        res = -1;
   }
fail:
   if(buffer_list)
	 kfree(buffer_list);

done:
   //LCD_DBG ( LCD_DBG_ID, "[DISPDRV] -%s\r\n", __FUNCTION__ );
   up(&gDmaSema);

#ifdef CONFIG_HAS_WAKELOCK
	wake_unlock(&glcdfb_wake_lock);
#endif

   return ( res );
}
#endif
/****************************************************************************
*
*  dsi_update_column
*
*  Update one column of LCD in non-DMA mode within dirty region.
*  DMA mode is also implemented
*
*
***************************************************************************/
static int update_column(DISPDRV_HANDLE_T  drvH,
    LCD_dev_info_t * dev,
	unsigned int column
    )
{
	int i, stride;
	uint32_t  *source;
	Int32     res  = 0;
	CSL_LCD_UPD_REQ_T   req;
	uint32_t  count;
	dsic_panel_t *pPanel = (dsic_panel_t *)drvH;

#ifdef LCD_COLUMN_UPDATE_CPU
	UInt32 *data_buff = NULL,*buff = NULL;
#endif
#ifdef LCD_COLUMN_UPDATE_DMA
	OSDAL_Dma_Buffer_List *buffer_list, *temp_list;
#endif


	stride = dev->width * INPUT_BPP;
	source = (UInt32)(((unsigned long)dev->frame_buffer.virtPtr)) + stride * dev->dirty_rect.top +
					column * INPUT_BPP;

	//source = (UInt32)(phys_to_virt((unsigned long)pPanel->pFbA)) + stride * dev->dirty_rect.top +
	//			column * INPUT_BPP;

	if (4 == INPUT_BPP)  {
		count = (dev->dirty_rect.bottom - dev->dirty_rect.top + 1);
		count &= ~1; //Ignore one pixel in case count is odd
#ifdef LCD_COLUMN_UPDATE_DMA
		buffer_list = kzalloc((dev->dirty_rect.bottom - dev->dirty_rect.top + 1) * sizeof(OSDAL_Dma_Buffer_List), GFP_KERNEL);
        if (!buffer_list) {
			pr_info("Couldn't allocate memory for dma buffer list\n");
			goto done;
		}
		temp_list = buffer_list;
        for (i = 0; i < count; i++) {
                temp_list->buffers[0].srcAddr = source;
                temp_list->buffers[0].destAddr = 0x084a0140;
                temp_list->buffers[0].length = 1 * INPUT_BPP;
                temp_list->buffers[0].bRepeat = 0;
                temp_list++;
                source += stride;
		}
        temp_list--;            /* Go back to the last list item to set interrupt = 1 */
        temp_list->buffers[0].interrupt = 1;

    req.buff           = (void *)buffer_list;
	req.lineLenP       = 1;
	req.lineCount      = count;
	req.buffBpp        = INPUT_BPP;
	req.timeOut_ms     = 100;
	req.multiLLI       = true;
	req.cslLcdCbRef = NULL;
	req.cslLcdCb = NULL;
	if ( CSL_DSI_UpdateCmVc ( pPanel->dsiCmVcHandle, &req ) != CSL_LCD_OK )
		{
			LCD_DBG ( LCD_DBG_ERR_ID, "[DISPDRV] %s: ERROR ret by "
				"CSL_DSI_UpdateCmVc\n\r", __FUNCTION__ );
			res = -1;
		}
#endif
#ifdef LCD_COLUMN_UPDATE_CPU
		data_buff = kzalloc((LCD_HEIGHT) * sizeof(unsigned int), GFP_KERNEL);
        if (!data_buff) {
			pr_info("Couldn't allocate memory for data buffer \n");
			goto done;
		}
		buff = data_buff;
        for (i = 0; i < count; i++) {
			*buff++ = *source;
			source += stride;
		}
		req.lineLenP    =1;
		req.lineCount   =count;
		req.cslLcdCbRef = NULL;
		//dispdrv_cb = apiCb;
		/*if( apiCb != NULL )
			req.cslLcdCb = dsi_cb;
		else*/
		req.cslLcdCb    =NULL;
		if( CSL_DSI_CPU_UpdateCmVc(pPanel->dsiCmVcHandle, &req,(unsigned int*)data_buff) != CSL_LCD_OK )
		{
			LCD_DBG ( LCD_DBG_ERR_ID, "[DISPDRV] %s: ERROR ret by "
				"CSL_DSI_UpdateCmVc\n\r", __FUNCTION__ );
			res = -1;
		}
#endif

	} else {
		pr_info("bpp=%d is not supported\n", INPUT_BPP);
	}

done :
#ifdef LCD_COLUMN_UPDATE_DMA
	kfree(buffer_list);
#endif
	return ( res );
}

static void dsi_get_interface_info(uint32_t* phy_addr)
{
	*phy_addr = REG_DSI_DATR_PADDR;
}

static void dsi_init_panels(void)
{
	exec_cmnd_list(display_hdl, TRUE, power_on_seq);
}

static void dsi_poweroff_panels(void)
{

	exec_cmnd_list(display_hdl, TRUE, enter_sleep_seq);
}


static void  dsi_update_column(LCD_dev_info_t * dev, unsigned int column)
{
    	dsic_panel_t *pPanel = &panel[LCD_main_panel];
    	dsic_panel_t *drvH   = pPanel;

        dsi_setwindow(drvH,dev);
        update_column(drvH, dev, column); 
}

static int  dsi_update_rect_dma(LCD_dev_info_t * dev, void*  req)
{
	CSL_LCD_RES_T ret;
	int err = -EINVAL;

    	dsic_panel_t *pPanel = &panel[LCD_main_panel];
    	dsic_panel_t *drvH   = pPanel;

      	dsi_setwindow(drvH,dev);
    	if ( CSL_DSI_UpdateCmVc ( pPanel->dsiCmVcHandle, (CSL_LCD_UPD_REQ_T*)req ) != CSL_LCD_OK )
    	{
        	LCD_DBG ( LCD_DBG_ERR_ID, "[DISPDRV] %s: ERROR ret by "
            	"CSL_DSI_UpdateCmVc\n\r", __FUNCTION__ );
        	return -1;
    	}

	return 0;

}

static void dsi_display_black_background(void)
{//Kevin.Wen:FIXME

}

/****************************************************************************/


//#ifdef CONFIG_BRCM_KPANIC_UI_IND
static void dsi_lock_csl_handle(void)
{//Kevin.Wen:FIXME

}
static void dsi_unlock_csl_handle(void)
{//Kevin.Wen:FIXME
}

//*****************************************************************************
//
// Function Name: dsic_send_data
//
// Description:   DMA/OS Update using INT frame buffer
//
//*****************************************************************************
static void dsi_send_data (
	UInt16*		panic_data_ptr,
	int			img_width,
	int			img_height,
	bool               rle	
	)
{
	int			fb_idx = 0;
	LCD_dev_info_t 		*dev = &LCD_device[LCD_main_panel];
	dsic_panel_t *pPanel = &panel[LCD_main_panel];
        dsic_panel_t *drvH = pPanel;
	CSL_LCD_UPD_REQ_T   req;
	Int32   			res  = 0;
	UInt32 *fb_ptr = NULL;
	UInt16 *data_ptr = panic_data_ptr;
	int i,rle_count;
	UInt32 pixel_data;

	//LCD_DBG ( LCD_DBG_ID, "[DISPDRV] +%s\r\n", __FUNCTION__ );

	if(data_ptr == NULL)
	{
		LCD_DBG ( LCD_DBG_ERR_ID, "[DISPDRV] +%s: data_ptr is NULL\r\n",
			__FUNCTION__ );
		return ;
	}

    fb_ptr = (UInt32*)dev->frame_buffer.virtPtr;

    if(fb_ptr == NULL)
	{
		LCD_DEBUG("Invalid fb_ptr\n");
		return;
	}

	for (i = 0, rle_count = 0; i <(img_width*img_height); i++) {
       if (0 >= rle_count) {
          rle_count = *data_ptr++;
          rle_count |= (*data_ptr++ << 16);
          pixel_data = *data_ptr++;
          pixel_data |= (*data_ptr++ << 16);
         }
         --rle_count;
         *fb_ptr++ = pixel_data;
	}

	if (0 == fb_idx)
		req.buff		   = dev->frame_buffer.physPtr;//pPanel->pFbA;
	 else
		req.buff		   = (void *)((UInt32)dev->frame_buffer.physPtr +
                                      dev->width * dev->height * INPUT_BPP);

		//req.buff		   = (void *)((UInt32)pPanel->pFbA +
        //                              dev->width * dev->height * INPUT_BPP);

	req.lineLenP	   = img_width;
	req.lineCount      = img_height;
	req.buffBpp 	   = INPUT_BPP;
	req.timeOut_ms     = 100;
	req.multiLLI	   = false;
	//printk(KERN_ERR "SSSSK buf=%08x, linelenp = %d, linecnt =%d\n", (u32)req.buff, req.lineLenP, req.lineCount);
	req.cslLcdCbRef = NULL;
	req.cslLcdCb = NULL;

	dsi_setwindow(drvH,dev);

	if ( CSL_DSI_UpdateCmVc ( pPanel->dsiCmVcHandle, &req ) != CSL_LCD_OK )
	{
		LCD_DBG ( LCD_DBG_ERR_ID, "[DISPDRV] %s: ERROR ret by "
			"CSL_DSI_UpdateCmVc\n\r", __FUNCTION__ );
		return;
	}
	//LCD_DBG ( LCD_DBG_ID, "[DISPDRV] -%s\r\n", __FUNCTION__ );
	return;
}

//#endif //CONFIG_BRCM_KPANIC_UI_IND

static int dsi_probe(struct platform_device *pdev)
{
	int rc, i;
	struct resource *res;
	struct lcdc_platform_data_t *pdata;
	CSL_LCD_RES_T ret = 0;
	CSL_DSI_INIT_CFG_t initCfg;

	DISPDRV_OPEN_PARM_T local_DISPDRV_OPEN_PARM_T;


	/* Hack
	* Since the display driver is not using this field,
	* we use it to pass the dma addr.
	*/
	//dsic_reset_display(gpio);

	local_DISPDRV_OPEN_PARM_T.busId = LCD_device[LCD_main_panel].frame_buffer.physPtr;
	local_DISPDRV_OPEN_PARM_T.busCh = LCD_main_panel;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		dev_err(&pdev->dev, "can't get device irq resources\n");
		return -ENOENT;
	}

	initCfg.dsi_base_address = lcdc_base;
	initCfg.interruptId = res->start;
	if (!initCfg.interruptId) {
		dev_err(&pdev->dev, "couldn't get interrupt Id!\n");
		return -ENOENT;
	}else{
		printk("Base Address = [%u]!!!!\n",initCfg.dsi_base_address);
		printk("Interrupt Id = [%u]!!!!\n",initCfg.interruptId);
	}


#ifndef _BRCM_8BYTE_MSG_CONSTRAINT
	base_params_buffer.virtPtr = NULL;
	base_params_buffer.sizeInBytes = 256;
	base_params_buffer.physPtr = NULL;

	base_params_buffer.virtPtr = dma_alloc_writecombine(&pdev->dev,
							base_params_buffer.sizeInBytes,
							&base_params_buffer.physPtr,
							GFP_KERNEL);

	if(base_params_buffer.virtPtr == NULL) {
		ret = -ENOMEM;
		dev_err(&pdev->dev,"Unable to allocate DMA buffer for dcs params memory\n");
		goto fail_to_init;
	}
	ret = dsic_open(&initCfg,(void *)&local_DISPDRV_OPEN_PARM_T, (DISPDRV_HANDLE_T*)&display_hdl,base_params_buffer.virtPtr);
#else
	ret = dsic_open(&initCfg,(void *)&local_DISPDRV_OPEN_PARM_T, (DISPDRV_HANDLE_T*)&display_hdl);
#endif
	if (ret != 0) {
		dev_err(&pdev->dev,"Failed to open this display device!\n");
		goto fail_to_open;
	}

	ret = dsic_start(display_hdl);
	if (ret != 0) {
		dev_err(&pdev->dev,"Failed to start this display device!\n");
		goto fail_to_start;
	}

	dev_err(&pdev->dev,"dsi display is enabled successfully\n");

/*
	//Display Black BackGround.
	ret = dsic_update(display_hdl, &LCD_device[LCD_main_panel], 0);
	if (ret != 0) {
		dev_err(&pdev->dev,"Failed to update the black screen on this display device!\n");
		goto fail_to_power_control;
	}
*/
	return 0;

fail_to_power_control:
	dsic_stop(&panel[local_DISPDRV_OPEN_PARM_T.busCh]);
fail_to_start:
	dsic_close(&panel[local_DISPDRV_OPEN_PARM_T.busCh]);
fail_to_open:
	dsic_exit();
fail_to_init:
	return ret;
}


static LCD_Interface_Drv_t dsi_interface = 
{
		.lcd_get_interface_info = dsi_get_interface_info,
		.lcd_init_panels = dsi_init_panels,
		.lcd_poweroff_panels = dsi_poweroff_panels,
		.lcd_display_black_background = dsi_display_black_background,
		.lcd_update_column = dsi_update_column,
		.lcd_update_rect_dma = dsi_update_rect_dma,
		.lcd_probe = dsi_probe,
		.lcd_send_data = dsi_send_data,
		.lcd_lock_csl_handle = dsi_lock_csl_handle,
		.lcd_unlock_csl_handle = dsi_unlock_csl_handle
};

LCD_Interface_Drv_t* get_dsic_interface()
{
	return &dsi_interface;
}
