/*******************************************************************************
* Copyright 2011 Broadcom Corporation.  All rights reserved.
*
*	@file	drivers/video/broadcom/dsi/lcd/panel/dsi_hx8396.h
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
*  dsi_hx8369a.h
*
*  PURPOSE:
*    This is the LCD-specific code for a hx8369a module.
*
*****************************************************************************/

#ifndef __LCD_HX8369A_H__
#define __LCD_HX8369A_H__

#define DSI_VC            (0)
#define DSI_CMND_IS_LP    TRUE  // display init comm LP or HS mode

#define LCD_HEIGHT		800
#define LCD_WIDTH		480

#define PANEL_WIDTH 480
#define PANEL_HEIGHT 800


#define LCD_BITS_PER_PIXEL 32
#define INPUT_BPP 			4
#define RESET_GPIO 		   45
#define DSI_VC             (0)
#define DSI_CMND_IS_LP    TRUE  // display init comm LP or HS m


//btirunagaru
#define PANEL_BOE           0x61bc11 // ????

#define RESET_SEQ 	{WR_CMND, 0x2A,0},\
	{WR_DATA, 0, (dev->col_start) >> 8},\
	{WR_DATA, 0, dev->col_start & 0xFF},\
	{WR_DATA, 0, (dev->col_end) >> 8},\
	{WR_DATA, 0, dev->col_end & 0xFF},\
	{WR_CMND, 0x2B,0},\
	{WR_DATA, 0, (dev->row_start) >> 8},\
	{WR_DATA, 0, dev->row_start & 0xFF},\
	{WR_DATA, 0, (dev->row_end) >> 8},\
	{WR_DATA, 0, dev->row_end & 0xFF},\
	{WR_CMND, 0x2C,0}


#define LCD_CMD(x) (x)
#define LCD_DATA(x) (x)

const char *LCD_panel_name = "HX8369 LCD";

int LCD_num_panels = 1;
LCD_Intf_t LCD_Intf = LCD_DSI;
LCD_Bus_t LCD_Bus = LCD_32BIT;


//Not used, just to satisfy the compilation
CSL_LCDC_PAR_SPEED_t timingReg_ns;
CSL_LCDC_PAR_SPEED_t timingMem_ns;


LCD_dev_info_t LCD_device[1] = {
	{
	 .panel		= LCD_main_panel,
	 .height	= LCD_HEIGHT,
	 .width		= LCD_WIDTH,
	 .bits_per_pixel= LCD_BITS_PER_PIXEL,
	 .te_supported	= true,
	}
};


//TE Info
CSL_DSI_TE_IN_CFG_t teInCfg =
{
    CSL_DSI_TE_MODE_VSYNC,      // te mode
    CSL_DSI_TE_ACT_POL_LO,      // sync polarity
    0,                          // vsync_width [tectl_clk_count]
    0,                          // hsync_line
};


// DSI Command Mode VC Configuration
CSL_DSI_CM_VC_t dsiVcCmCfg =
{
    DSI_VC,               			// VC
    DSI_DT_LG_DCS_WR,               // dsiCmnd
    MIPI_DCS_WRITE_MEMORY_START,    // dcsCmndStart
    MIPI_DCS_WRITE_MEMORY_CONTINUE, // dcsCmndCont
    FALSE,                          // isLP
    LCD_IF_CM_I_RGB888U,            // cm_in
    LCD_IF_CM_O_RGB888,             // cm_out
    // TE configuration
    {
        DSI_TE_CTRLR_INPUT_0,
        &teInCfg                    // DSI Te Input Config
    },
};

// DSI BUS CONFIGURATION
CSL_DSI_CFG_t dsiCfg = {
    0,             // bus             set on open
    1,             // dlCount
    DSI_DPHY_0_92, // DSI_DPHY_SPEC_T
    // ESC CLK Config
    {156,2},       // escClk          fixed   156[MHz], DIV by 2 =  78[MHz]

    // HS CLK Config
	{500,1},       // hsBitClk        PLL     300[MHz], DIV by 1 = 300[Mbps]
	// LP Speed
    5,             // lpBitRate_Mbps, Max 10[Mbps]
    FALSE,         // enaContClock
    TRUE,          // enaRxCrc
    TRUE,          // enaRxEcc
    TRUE,          // enaHsTxEotPkt
    FALSE,         // enaLpTxEotPkt
    FALSE,         // enaLpRxEotPkt
};


static uint8_t param1[] = {0xFF, 0x83, 0x69};

static uint8_t param2[] = {0x01,0x0F};  

static uint8_t param3[] = {0x01, 0x00, 0x34, 0x07, 0x00
              ,0x0E, 0x0E, 0x1A, 0x22, 0x3F
              ,0x3F, 0x07, 0x23, 0x01, 0xE6
              ,0xE6, 0xE6, 0xE6, 0xE6}; 

static uint8_t param4[] =   {0x00, 0x20, 0x0A, 0x0A, 0x70
		,0x00, 0xFF, 0x00, 0x00, 0x00
		,0x00, 0x03, 0x03, 0x00, 0x01};

static uint8_t param5[] = {0x00, 0x0C, 0x84, 0x0C, 0x01 };

static uint8_t param6[] = {0x27,0x27};

static uint8_t param7[] = {0x00, 0x05, 0x03, 0x00, 0x01
		,0x09, 0x10, 0x80, 0x37, 0x37
		,0x20, 0x31, 0x46, 0x8A, 0x57
		,0x9B, 0x20, 0x31, 0x46, 0x8A
		,0x57, 0x9B, 0x07, 0x0F, 0x07
		,0x00};
 

static uint8_t param8[] =     {0x00, 0x02, 0x05, 0x31, 0x39
		,0x3F, 0x0E, 0x31, 0x06, 0x0C
		,0x0D, 0x10, 0x12, 0x10, 0x13
		,0x10, 0x18, 0x00, 0x02, 0x05
		,0x31, 0x39, 0x3F, 0x0E, 0x31
		,0x06, 0x0C, 0x0D, 0x10, 0x12
		,0x10, 0x13, 0x10, 0x18};
 

static uint8_t param9[] =   {0x00, 0xA0, 0xC6, 0x00, 0x0A
		,0x00, 0x10, 0x30, 0x6C, 0x02
		,0x10//DSI_CMD_1Lane
		,0x18
		,0x40};


Lcd_init_t power_on_seq[] =
{
    {WR_CMND_MULTIPLE_DATA     ,    0xB9    , 0     ,3, param1},// SET password
    {SLEEP_MS,  0, 1},

    {WR_CMND_MULTIPLE_DATA     ,    0xB0    , 0     ,2, param2},// SET OSC Clock
    {SLEEP_MS,  0, 1},

    {WR_CMND_MULTIPLE_DATA     ,    0xB1    , 0     ,19, param3},
    {SLEEP_MS,  0, 1},

    {WR_CMND_MULTIPLE_DATA     ,    0xB2    , 0     ,15, // SET Display 480x800
    param4},
    {SLEEP_MS,  0, 1},

    {WR_CMND_MULTIPLE_DATA     ,     0xB4,  0,  5, // SET Display 480x800
     param5},
    {SLEEP_MS,  0, 1},

    {WR_CMND_MULTIPLE_DATA,     0xB6,  0,   2, // SET VCOM
    param6},
    {SLEEP_MS,  0, 1},

    {WR_CMND_MULTIPLE_DATA,     0xD5,  0,    26,
     param7},
    {SLEEP_MS,  0, 1},

    {WR_CMND_MULTIPLE_DATA,     0xE0,   0,   34,
     param8},
    {SLEEP_MS,  0, 1},

    {WR_CMND_DATA,	 0x3A, 	0x77},
    {SLEEP_MS,  0, 1},

    {WR_CMND_MULTIPLE_DATA,     0xBA,   0,  13,// Set MIPI
     param9},
    {SLEEP_MS,  0, 1},

    {WR_CMND_DATA,	 0x35, 	0x00},   //Set TE ON
    {SLEEP_MS,  0, 1},

    {WR_CMND,   0x11, 0},//Sleep Out
    {SLEEP_MS,  0,  250},

    {WR_CMND,   0x29,  0},//Display On
    {SLEEP_MS,  0, 10},

    //--- END OF COMMAND LIST -----------------------
    {CTRL_END        , 0}

};

Lcd_init_t  enter_sleep_seq[] = {
    	{DISPCTRL_WR_CMND,   MIPI_DCS_SET_DISPLAY_OFF, 0},
    	{DISPCTRL_SLEEP_MS,  0,  120},  
    	{DISPCTRL_WR_CMND,   MIPI_DCS_ENTER_SLEEP_MODE, 0},
    	{DISPCTRL_SLEEP_MS,  0,  120},  
       {CTRL_END        , 0}
};

Lcd_init_t  exit_sleep_seq[] = {
    	{DISPCTRL_WR_CMND,   MIPI_DCS_EXIT_SLEEP_MODE, 0},
    	{DISPCTRL_SLEEP_MS,  0,  120},  
    	{DISPCTRL_WR_CMND,   MIPI_DCS_SET_DISPLAY_ON, 0},
    	{DISPCTRL_SLEEP_MS,  0,  120},  
       {CTRL_END        , 0}
};

#endif
