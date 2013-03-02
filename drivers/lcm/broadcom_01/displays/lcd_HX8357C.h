/*******************************************************************************
* Copyright 2011 TCT Corporation.  All rights reserved.
*
*	@file	drivers/video/broadcom/displays/lcd_HX8357C.h
*
* Unless you and TCT execute a separate written software license agreement
* governing use of this software, this software is licensed to you under the
* terms of the GNU General Public License version 2, available at
* http://www.gnu.org/copyleft/gpl.html (the "GPL").
*
* Notwithstanding the above, under no circumstances may you combine this
* software in any way with any other TCT software provided under a license
* other than the GPL, without TCT's express prior written consent.
*******************************************************************************/

/****************************************************************************
*
*  lcd_HX8357C.h
*
*  PURPOSE:
*    This is the LCD-specific code for TDT module.
*
*****************************************************************************/

#ifndef __LCD_HX8357C_H__
#define __LCD_HX8357C_H__

#define MIPI_DCS_SET_COLUMN_ADDRESS  	    0x2A
#define MIPI_DCS_SET_PAGE_ADDRESS	  	    0x2B
#define MIPI_DCS_WRITE_MEMORY_START		    0x2C

#define RESET_SEQ {WR_CMND, MIPI_DCS_SET_COLUMN_ADDRESS, 0},\
		{WR_DATA, 0x00, (dev->col_start) >> 8},\
		{WR_DATA, 0x00, dev->col_start & 0xFF},\
		{WR_DATA, 0x00, (dev->col_end) >> 8},\
		{WR_DATA, 0x00, dev->col_end & 0xFF},\
		{WR_CMND, MIPI_DCS_SET_PAGE_ADDRESS, 0},\
		{WR_DATA, 0x00, (dev->row_start) >> 8},\
		{WR_DATA, 0x00, dev->row_start & 0xFF},\
		{WR_DATA, 0x00, (dev->row_end) >> 8},\
		{WR_DATA, 0x00, dev->row_end & 0xFF},\
		{WR_CMND, MIPI_DCS_WRITE_MEMORY_START, 0}

#define LCD_CMD(x) (x)
#define LCD_DATA(x) (x)

#ifdef CONFIG_ENABLE_QVGA
#define LCD_HEIGHT              	320
#define PANEL_HEIGHT					320	
#define LCD_WIDTH               	240
#define PANEL_WIDTH              240
#else
#define LCD_HEIGHT              	480
#define PANEL_HEIGHT            	480
#define LCD_WIDTH               	320
#define PANEL_WIDTH              320
#endif

#ifdef CONFIG_ARGB8888
#define LCD_BITS_PER_PIXEL	32
#else
#define LCD_BITS_PER_PIXEL	16
#endif

#define TEAR_SCANLINE	480

const char *LCD_panel_name = "Himax HVGA HX8357C Controller";

int LCD_num_panels = 1;
LCD_Intf_t LCD_Intf = LCD_Z80;
#ifdef CONFIG_ARGB8888
LCD_Bus_t LCD_Bus = LCD_16BIT;
#error test by matt
#else
LCD_Bus_t LCD_Bus = LCD_18BIT;
//changed by matt
//LCD_Bus_t LCD_Bus = LCD_16BIT;
#endif

//Not determined, it seems not requiremented by the LCD.
CSL_LCDC_PAR_SPEED_t timingReg = {31, 24, 0, 1, 2, 0};
CSL_LCDC_PAR_SPEED_t timingMem = {31, 24, 0, 1, 2, 0};

LCD_dev_info_t LCD_device[1] = {
	{
	 .panel		= LCD_main_panel,
	 .height	= LCD_HEIGHT,
	 .width		= LCD_WIDTH,
	 .bits_per_pixel = LCD_BITS_PER_PIXEL,
	 .te_supported	= false}
};

Lcd_init_t power_on_seq[] = {
	{SLEEP_MS, 0, 20},
		
	{WR_CMND, 0xB9, 0},
	{WR_DATA, 0x00, 0xFF},
	{WR_DATA, 0x00, 0x83},	
	{WR_DATA, 0x00, 0x57},
	{SLEEP_MS, 0, 5}, 

	{WR_CMND, 0xB6, 0},
	{WR_DATA, 0x00, 0xC0},	

	{WR_CMND, 0x11, 0}, // Exit sleep mode
	{SLEEP_MS, 0, 150},

	{WR_CMND, 0x36, 0}, // Set address mode	
	{WR_DATA, 0x00, 0x00}, // Alvin changed 0x48->0x00

	{WR_CMND, 0x3A, 0}, // Set pixel format
	{WR_DATA, 0x00, 0x66}, // 0x55-16bits, 0x66-18bits

	{WR_CMND, 0xB0, 0},
	{WR_DATA, 0x00, 0x68},
	
	{WR_CMND, 0xCC, 0},
	{WR_DATA, 0x00, 0x09},
	
	{WR_CMND, 0xB1, 0},
	{WR_DATA, 0x00, 0x00},
	{WR_DATA, 0x00, 0x14},
	{WR_DATA, 0x00, 0x1C},
	{WR_DATA, 0x00, 0x1C},
	{WR_DATA, 0x00, 0x83},
	{WR_DATA, 0x00, 0x48},

	{WR_CMND, 0xC0, 0},
	{WR_DATA, 0x00, 0x50},
	{WR_DATA, 0x00, 0x50},
	{WR_DATA, 0x00, 0x01},
	{WR_DATA, 0x00, 0x3C},
	{WR_DATA, 0x00, 0x1E},
	{WR_DATA, 0x00, 0x08},
	
	{WR_CMND, 0xB4, 0},
	{WR_DATA, 0x00, 0x02}, // NW
	{WR_DATA, 0x00, 0x40}, // RTN
	{WR_DATA, 0x00, 0x00}, // DIV
	{WR_DATA, 0x00, 0x2A}, // DUM
	{WR_DATA, 0x00, 0x2A}, // DUM
	{WR_DATA, 0x00, 0x0D}, // GDON
	{WR_DATA, 0x00, 0x78}, // GDOFF

	{WR_CMND, 0xE0, 0},
	{WR_DATA, 0x00, 0x02}, // 1
	{WR_DATA, 0x00, 0x08}, // 2
	{WR_DATA, 0x00, 0x11}, // 3
	{WR_DATA, 0x00, 0x23}, // 4
	{WR_DATA, 0x00, 0x2C}, // 5
	{WR_DATA, 0x00, 0x40}, // 6
	{WR_DATA, 0x00, 0x4A}, // 7
	{WR_DATA, 0x00, 0x52}, // 8
	{WR_DATA, 0x00, 0x48}, // 9
	{WR_DATA, 0x00, 0x41}, // 10
	{WR_DATA, 0x00, 0x3C}, // 11
	{WR_DATA, 0x00, 0x33}, // 12
	{WR_DATA, 0x00, 0x2E}, // 13
	{WR_DATA, 0x00, 0x28}, // 14	
	{WR_DATA, 0x00, 0x27}, // 15
	{WR_DATA, 0x00, 0x1B}, // 16
	{WR_DATA, 0x00, 0x02}, // 17
	{WR_DATA, 0x00, 0x08}, // 18
	{WR_DATA, 0x00, 0x11}, // 19
	{WR_DATA, 0x00, 0x23}, // 20
	{WR_DATA, 0x00, 0x2C}, // 21
	{WR_DATA, 0x00, 0x40}, // 22
	{WR_DATA, 0x00, 0x4A}, // 23
	{WR_DATA, 0x00, 0x52}, // 24
	{WR_DATA, 0x00, 0x48}, // 25
	{WR_DATA, 0x00, 0x41}, // 26
	{WR_DATA, 0x00, 0x3C}, // 27
	{WR_DATA, 0x00, 0x33}, // 28	
	{WR_DATA, 0x00, 0x2E}, // 29
	{WR_DATA, 0x00, 0x28}, // 30
	{WR_DATA, 0x00, 0x27}, // 31
	{WR_DATA, 0x00, 0x1B}, // 32
	{WR_DATA, 0x00, 0x00}, // 33
	{WR_DATA, 0x00, 0x01}, // 34

	{WR_CMND, 0x2A, 0}, // Display area column setting
	{WR_DATA, 0x00, 0x00},
	{WR_DATA, 0x00, 0x00},	
	{WR_DATA, 0x00, 0x01},
	{WR_DATA, 0x00, 0x3F},	

	{WR_CMND, 0x2B, 0}, // Display area page setting
	{WR_DATA, 0x00, 0x00},
	{WR_DATA, 0x00, 0x00},	
	{WR_DATA, 0x00, 0x01},
	{WR_DATA, 0x00, 0xE0},	
		
	{WR_CMND, 0x29, 0}, // Display on
	{SLEEP_MS, 0, 25},	
	{WR_CMND, 0x2C, 0}, // memary write
 
//////////////////////////////////////////////////
//inital picture data write into LCD memory RAM///
//////////////////////////////////////////////////
//end
//LED light on
	{CTRL_END, 0, 0}
};

/* Alvin: Not the official sequence */
Lcd_init_t power_off_seq[] = {
	{WR_CMND, 0x28, 0},
	{SLEEP_MS, 0, 50},	
	{WR_CMND, 0x10, 0}, //sleep in
	{SLEEP_MS, 0, 120},	
	{SLEEP_MS, 0, 300},	
	{CTRL_END, 0, 0},
//
//shut down VDD of LCM.
//
};

#endif /* __LCD_HX8357C_H__ */
