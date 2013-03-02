/*******************************************************************************
* Copyright 2011 Broadcom Corporation.  All rights reserved.
*
*	@file	drivers/video/broadcom/dsi/lcd/panel/dsi_hx8357c.h
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
*  dsi_hx8357c.h
*
*  PURPOSE:
*    This is the LCD-specific code for a hx8357c module.
*
*****************************************************************************/

#ifndef __LCD_HX8357C_H__
#define __LCD_HX8357C_H__

#define DSI_VC            (0)
#define DSI_CMND_IS_LP    TRUE  // display init comm LP or HS mode

#define LCD_WIDTH 320
#define LCD_HEIGHT 480


#define PANEL_WIDTH 320
#define PANEL_HEIGHT 480

#define LCD_BITS_PER_PIXEL 32
#define INPUT_BPP 4
#define RESET_GPIO 45
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

const char *LCD_panel_name = "HX8357c LCD";

int LCD_num_panels = 1;
LCD_Intf_t LCD_Intf = LCD_DSI;
LCD_Bus_t LCD_Bus = LCD_32BIT;


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
    LCD_IF_CM_O_RGB666,             // cm_out        
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
	  {200,1},       // hsBitClk        PLL     300[MHz], DIV by 1 = 300[Mbps]
    // LP Speed
    5,             // lpBitRate_Mbps, Max 10[Mbps]
    FALSE,         // enaContClock 
    TRUE,          // enaRxCrc                
    TRUE,          // enaRxEcc               
    TRUE,          // enaHsTxEotPkt           
    FALSE,         // enaLpTxEotPkt        
    FALSE,         // enaLpRxEotPkt        
};    

static uint8_t param1[] = {0xFF, 0x83, 0x57};
static uint8_t param2[] = {0x00,//DP
    	 0x15,//BT
    	 0x1C,//VSPR
    	 0x1C,//VSNR
    	 0x83,//AP
    	 0x44};//FS

static uint8_t param3[] =  {0x50,//OPON
    	 0x50,//OPON
    	 0x01,//STBA
    	 0x3C,//STBA
    	 0xC8,//STBA
    	 0x08};

static uint8_t param4[] =  {0x02,//Nw
    	 0x40,//RTN
    	 0x00,//DIV
    	 0x2A,//DUM
    	 0x2A,//DUM
    	 0x0D,//GDON
    	 0x78,//GDOFF
    	 };  	

static uint8_t param5[] =  {//GAMMA
			 0x00, //1
			 0x00, //1
			 0x30, //1
			 0x00, //1
			 0x37, //2
			 0x00, //1
			 0x3B, //3
			 0x00, //1
			 0x3F, //4
			 0x00, //1
			 0x44, //5
			 0x00, //1
			 0x52, //6
			 0x00, //1
			 0x59, //7
			 0x00, //1
			 0x60, //8
			 0x00, //1
			 0x3A, //9
			 0x00, //1
			 0x35, //10
			 0x00, //1
			 0x2F, //11
			 0x00, //1
			 0x29, //12
			 0x00, //1
			 0x22, //13
			 0x00, //1
			 0x1C, //14
			 0x00, //1
			 0x19, //15
			 0x00, //1
			 0x03, //16
			 0x00, //1
			 0x30, //17
			 0x00, //1
			 0x37, //18
			 0x00, //1
			 0x3B, //19
			 0x00, //1
			 0x3F, //20
			 0x00, //1
			 0x44, //21
			 0x00, //1
			 0x52, //22
			 0x00, //1
			 0x59, //23
			 0x00, //1
			 0x60, //24
			 0x00, //1
			 0x3A, //25
			 0x00, //1
			 0x35, //26
			 0x00, //1
			 0x2F, //27
			 0x00, //1
			 0x29, //28
			 0x00, //1
			 0x22, //29
			 0x00, //1
			 0x1C, //30
			 0x00, //1
			 0x19, //31
			 0x00, //1
			 0x03, //32
			 0x00, //1
			 0x00, //33
			 0x00, //1
			 0x01, //34
    	 };
  
Lcd_init_t power_on_seq[] = {
    {WR_CMND_MULTIPLE_DATA     ,    0xB9    , 0     ,3,  param1},// SET password
    {SLEEP_MS,  0, 5},
    
    {WR_CMND_DATA,	 0xB6, 	0x53}, //VCOMDC  

    //{DISPCTRL_WR_CMND,   0x11, 0},//Sleep Out      
    //{DISPCTRL_SLEEP_MS,  0,  150},
    
    {WR_CMND,   0x35, 0},//TE ON
    
    {WR_CMND_DATA,	 0x3A, 	0x06}, //262K
  
    {WR_CMND_DATA,	 0xB0, 	0x68}, //70Hz      
  
    {WR_CMND_DATA,	 0xCC, 	0x05}, //Set Panel
      
    {WR_CMND_MULTIPLE_DATA     ,    0xB1    , 0     ,6, 
     param2},//FS
    	 
    {WR_CMND_MULTIPLE_DATA     ,    0xC0    , 0     ,6, 
     param3},

     {WR_CMND_MULTIPLE_DATA     ,    0xB4    , 0     ,7, 
     param4},
    	 
     {WR_CMND_MULTIPLE_DATA     ,    0xE0    , 0     ,69,   	 
     param5},
    {WR_CMND,   0x11, 0},//Sleep Out      
    {SLEEP_MS,  0,  250},
   	 
    {WR_CMND,   0x29, 0},// display on    
    {SLEEP_MS,  0,  25},  
    
    {WR_CMND,   0x2C, 0},  
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
