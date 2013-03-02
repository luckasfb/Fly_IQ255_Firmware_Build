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

#ifndef __DSI_HX8357C_H__
#define __DSI_HX8357C_H__

#define DSI_VC            (0)
#define DSI_CMND_IS_LP    TRUE  // display init comm LP or HS mode

#define PANEL_WIDTH 320
#define PANEL_HEIGHT 480


#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 480
#define INPUT_BPP 4
#define RESET_GPIO 45

// Display Info
DISPDRV_INFO_T DSI_Display_Info =
{
    DISPLAY_TYPE_LCD_STD,         // DISPLAY_TYPE_T          type;          
    PANEL_WIDTH,                  // UInt32                  width;         
    PANEL_HEIGHT,                 // UInt32                  height;        
    DISPDRV_FB_FORMAT_RGB888_U,   // DISPDRV_FB_FORMAT_T     input_format;
    DISPLAY_BUS_DSI,              // DISPLAY_BUS_T           bus_type;
    0,                            // UInt32                  interlaced;    
    DISPDRV_DITHER_NONE,          // DISPDRV_DITHER_T        output_dither; 
    0,                            // UInt32                  pixel_freq;    
    0,                            // UInt32                  line_rate;     
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
CSL_DSI_CM_VC_t alexVcCmCfg = 
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
	  {400,1},       // hsBitClk        PLL     300[MHz], DIV by 1 = 300[Mbps]
    // LP Speed
    5,             // lpBitRate_Mbps, Max 10[Mbps]
    FALSE,         // enaContClock 
    TRUE,          // enaRxCrc                
    TRUE,          // enaRxEcc               
    TRUE,          // enaHsTxEotPkt           
    FALSE,         // enaLpTxEotPkt        
    FALSE,         // enaLpRxEotPkt        
};    


DISPCTRL_REC_T LCD_Init[] = {
    {DISPCTRL_WR_CMND_MULTIPLE_DATA     ,    0xB9    , 0     ,3, {0xFF, 0x83, 0x57}},// SET password
    {DISPCTRL_SLEEP_MS,  0, 5},
    
    {DISPCTRL_WR_CMND_DATA,	 0xB6, 	0x53}, //VCOMDC  

    //{DISPCTRL_WR_CMND,   0x11, 0},//Sleep Out      
    //{DISPCTRL_SLEEP_MS,  0,  150},
    
    {DISPCTRL_WR_CMND,   0x35, 0},//TE ON
    
    {DISPCTRL_WR_CMND_DATA,	 0x3A, 	0x06}, //262K
  
    {DISPCTRL_WR_CMND_DATA,	 0xB0, 	0x68}, //70Hz      
  
    {DISPCTRL_WR_CMND_DATA,	 0xCC, 	0x05}, //Set Panel
      
    {DISPCTRL_WR_CMND_MULTIPLE_DATA     ,    0xB1    , 0     ,6, 
    	{0x00,//DP
    	 0x15,//BT
    	 0x1C,//VSPR
    	 0x1C,//VSNR
    	 0x83,//AP
    	 0x44}},//FS
    	 
    {DISPCTRL_WR_CMND_MULTIPLE_DATA     ,    0xC0    , 0     ,6, 
    	{0x50,//OPON
    	 0x50,//OPON
    	 0x01,//STBA
    	 0x3C,//STBA
    	 0xC8,//STBA
    	 0x08}},//GEN

     {DISPCTRL_WR_CMND_MULTIPLE_DATA     ,    0xB4    , 0     ,7, 
    	{0x02,//Nw
    	 0x40,//RTN
    	 0x00,//DIV
    	 0x2A,//DUM
    	 0x2A,//DUM
    	 0x0D,//GDON
    	 0x78,//GDOFF
    	 }},
    	 
     {DISPCTRL_WR_CMND_MULTIPLE_DATA     ,    0xE0    , 0     ,69,   	 
 			{//GAMMA
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
    	 }},

    {DISPCTRL_WR_CMND,   0x11, 0},//Sleep Out      
    {DISPCTRL_SLEEP_MS,  0,  250},    
    	 
    {DISPCTRL_WR_CMND,   0x29, 0},// display on    
    {DISPCTRL_SLEEP_MS,  0,  25},  
    
    {DISPCTRL_WR_CMND,   0x2C, 0},  
    //--- END OF COMMAND LIST -----------------------
    {DISPCTRL_LIST_END        , 0}
};

DISPCTRL_REC_T LCD_EnterSleep[] = {
    	{DISPCTRL_WR_CMND,   MIPI_DCS_SET_DISPLAY_OFF, 0},
    	{DISPCTRL_SLEEP_MS,  0,  120},  
    	{DISPCTRL_WR_CMND,   MIPI_DCS_ENTER_SLEEP_MODE, 0},
    	{DISPCTRL_SLEEP_MS,  0,  120},  
};

DISPCTRL_REC_T LCD_ExitSleep[] = {
    	{DISPCTRL_WR_CMND,   MIPI_DCS_EXIT_SLEEP_MODE, 0},
    	{DISPCTRL_SLEEP_MS,  0,  120},  
    	{DISPCTRL_WR_CMND,   MIPI_DCS_SET_DISPLAY_ON, 0},
    	{DISPCTRL_SLEEP_MS,  0,  120},  
};

#endif
