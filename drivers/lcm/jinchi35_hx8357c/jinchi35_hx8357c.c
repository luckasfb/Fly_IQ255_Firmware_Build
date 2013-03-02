/*****************************************************************************
 *  Copyright Statement:
 *  --------------------
 *  This software is protected by Copyright and the information contained
 *  herein is confidential. The software may not be copied and the information
 *  contained herein may not be used or disclosed except with the written
 *  permission of MediaTek Inc. (C) 2008
 *
 *  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 *  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 *  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
 *  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 *  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 *  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 *  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
 *  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
 *  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
 *  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
 *
 *  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
 *  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 *  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 *  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
 *  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 *  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
 *  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
 *  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
 *  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
 *  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
 *
 *****************************************************************************/

#include <linux/string.h>

#include "lcm_drv.h"

#if BUILD_UBOOT
#define	LCD_PRINT	printf
#else
#include <linux/kernel.h>

#define	LCD_PRINT	printk
#endif

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (320)
#define FRAME_HEIGHT (480)

#define LCMID_REG		(0xd0)
#define LCM_ID			(0x90)

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
static unsigned int get_lcd_id_r(unsigned int addr);
static unsigned int get_lcd_id_n(unsigned int addr, unsigned char count);

static __inline unsigned int HIGH_BYTE(unsigned int val)
{
    return (val >> 8) & 0xFF;
}

static __inline unsigned int LOW_BYTE(unsigned int val)
{
    return (val & 0xFF);
}

static __inline void send_ctrl_cmd(unsigned int cmd)
{
	lcm_util.send_cmd(cmd);
}

static __inline void send_data_cmd(unsigned int data)
{
	lcm_util.send_data(data);
}

static __inline unsigned int read_data_cmd()
{
    return lcm_util.read_data();
}

static __inline void set_lcm_register(unsigned int regIndex, unsigned int regData, unsigned int uiDelay)
{
	send_ctrl_cmd(regIndex);
	send_data_cmd(regData);

	if (uiDelay > 0)
		MDELAY(uiDelay);
}

static void  lcm_update_black(unsigned int x, unsigned int y,unsigned int width, unsigned int height, unsigned short data)
{
    unsigned int x0 = x;
    unsigned int y0 = y;
    unsigned int x1 = x0 + width;
    unsigned int y1 = y0 + height + 2;
    unsigned int k, i;
	
	send_ctrl_cmd(0x2A);
	send_data_cmd(HIGH_BYTE(x0));
	send_data_cmd(LOW_BYTE(x0));
	send_data_cmd(HIGH_BYTE(x1));
	send_data_cmd(LOW_BYTE(x1));

	send_ctrl_cmd(0x2B);
	send_data_cmd(HIGH_BYTE(y0));
	send_data_cmd(LOW_BYTE(y0));
	send_data_cmd(HIGH_BYTE(y1));
	send_data_cmd(LOW_BYTE(y1));
	send_ctrl_cmd(0x2C);
	
	for (i = x0; i < x1; i++)
	{
		for (k = y0; k < y1; k++)
		{
			send_data_cmd(data);
		}
	}
}

static void init_lcm_registers(void)
{
	send_ctrl_cmd(0xB9);
	send_data_cmd(0xFF);
	send_data_cmd(0x83);
	send_data_cmd(0x57);

	MDELAY(50);

	send_ctrl_cmd(0xB6); //
	send_data_cmd(0x25); //VCOMDC  2c 

	send_ctrl_cmd(0x35); // TE ON 

	send_ctrl_cmd(0xB0);
	send_data_cmd(0x68); //70Hz

	send_ctrl_cmd(0xCC); //Set Panel
	send_data_cmd(0x09); //

	send_ctrl_cmd(0xB1); //
	send_data_cmd(0x00); //DP
	send_data_cmd(0x15); //BT
	send_data_cmd(0x1C); //VSPR1C3F
	send_data_cmd(0x1C); //VSNR1C3F
	send_data_cmd(0x83); //AP
	send_data_cmd(0x48); //FS 

	send_ctrl_cmd(0xC0); //
	send_data_cmd(0x50); //OPON
	send_data_cmd(0x50); //OPON
	send_data_cmd(0x01); //STBA
	send_data_cmd(0x3C); //STBA
	send_data_cmd(0x1E); //STBA
	send_data_cmd(0x08); //GEN

	send_ctrl_cmd(0xB4); //
	send_data_cmd(0x02); //NW
	send_data_cmd(0x40); //RTN
	send_data_cmd(0x00); //DIV
	send_data_cmd(0x2A); //DUM
	send_data_cmd(0x2A); //DUM
	send_data_cmd(0x0D); //GDON
	send_data_cmd(0x78); //GDOFF

	send_ctrl_cmd(0xE0); //
	send_data_cmd(0x02); // //  1
	send_data_cmd(0x08); // // 2
	send_data_cmd(0x11); //3
	send_data_cmd(0x23); //4
	send_data_cmd(0x2C); //5
	send_data_cmd(0x40); //6
	send_data_cmd(0x4A); //7
	send_data_cmd(0x52); //8
	send_data_cmd(0x48); //9
	send_data_cmd(0x41); //10
	send_data_cmd(0x3C); //11
	send_data_cmd(0x33); //12
	send_data_cmd(0x2E); //13
	send_data_cmd(0x28); //14
	send_data_cmd(0x27); //15
	send_data_cmd(0x1B); //16
	send_data_cmd(0x02); //17 v1
	send_data_cmd(0x08); //18
	send_data_cmd(0x11); //19
	send_data_cmd(0x23); //20
	send_data_cmd(0x2C); //21
	send_data_cmd(0x40); //22
	send_data_cmd(0x4A); //23
	send_data_cmd(0x52); //24
	send_data_cmd(0x48); //25
	send_data_cmd(0x41); //26
	send_data_cmd(0x3C); //27
	send_data_cmd(0x33); //28
	send_data_cmd(0x2E); //29
	send_data_cmd(0x28); //30
	send_data_cmd(0x27); //31
	send_data_cmd(0x1B); //32
	send_data_cmd(0x00); //33
	send_data_cmd(0x01); //34

	send_ctrl_cmd(0x3A);   
	send_data_cmd(0x55);   

	send_ctrl_cmd(0x36);                 
	send_data_cmd(0x00);

	send_ctrl_cmd(0x2A);
	send_data_cmd(0x00);
	send_data_cmd(0x00);
	send_data_cmd(0x01);
	send_data_cmd(0x3F);

	send_ctrl_cmd(0x2B);
	send_data_cmd(0x00);
	send_data_cmd(0x00);
	send_data_cmd(0x01);
	send_data_cmd(0xDF);

	send_ctrl_cmd(0x11);                // SLPOUT 
	MDELAY(120);   

	send_ctrl_cmd(0x29); // display on
	MDELAY(50);

	send_ctrl_cmd(0x2C);      	
}

// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

/*
 * ATTENTIONS: There are a few important differences under the MT6575/15 compared with the MT6573/13
 * 
 *  type: if set LCM_TYPE_DBI, cpu interface.
 *  io_select_mode: if set 0, selects buss composition: LPA0, LWRB, LRDB control bus and NLD[0:15] data bus
 *                  if set 1, selects buss composition: DPIVSYNC, DPIDE, DPIHSYNC control bus and DPIRGB data bus.
 *  dbi.port:  0 -- select parallel port 0; 1 -- select parallel port 1
 *
 * Author: chu, zewei 
 * Date:  2012/09/05
 */
static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type   = LCM_TYPE_DBI;
	params->ctrl   = LCM_CTRL_PARALLEL_DBI;
	params->width  = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	params->io_select_mode = 1;	
	
	params->dbi.port                    = 0;
	params->dbi.clock_freq              = LCM_DBI_CLOCK_FREQ_52M;
	params->dbi.data_width              = LCM_DBI_DATA_WIDTH_16BITS;
	params->dbi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dbi.data_format.trans_seq   = LCM_DBI_TRANS_SEQ_MSB_FIRST;
	params->dbi.data_format.padding     = LCM_DBI_PADDING_ON_MSB;
	params->dbi.data_format.format      = LCM_DBI_FORMAT_RGB565;
	params->dbi.data_format.width       = LCM_DBI_DATA_WIDTH_16BITS;
	params->dbi.cpu_write_bits          = LCM_DBI_CPU_WRITE_16_BITS;
	params->dbi.io_driving_current      = LCM_DRIVING_CURRENT_6575_8MA; //LCM_DRIVING_CURRENT_8MA;

	params->dbi.parallel.write_setup    = 1; // 2;
	params->dbi.parallel.write_hold     = 1; // 3; // 2, 4
	params->dbi.parallel.write_wait     = 6; //6;
	
	params->dbi.parallel.read_setup     = 3; // 3;
	params->dbi.parallel.read_hold      = 0;
    params->dbi.parallel.read_latency   = 18;
	
	params->dbi.parallel.wait_period    = 1;
	params->dbi.parallel.cs_high_width  = 0; //cycles of cs high level between each transfer
	//params->dbi.parallel.read_latency   = 20;  //40
	//params->dbi.parallel.wait_period    = 10; // 0

	// enable tearing-free
    //params->dbi.te_mode                 = LCM_DBI_TE_MODE_VSYNC_ONLY;
    //params->dbi.te_edge_polarity        = LCM_POLARITY_FALLING;
}


static void lcm_init(void)
{
	//unsigned int lcd_id = 0;
	
	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(100);
	SET_RESET_PIN(1);

    // Advises that should hold high level for about 120 ms as for HX... series lcd ic
    MDELAY(120);
	//MDELAY(50);

	//lcd_id = get_lcd_id_n(LCMID_REG, 2);
	//LCD_PRINT("[XXD35_ILI9486L]lcm_init: lcd_id = 0x%x\n", lcd_id);

	init_lcm_registers();
	lcm_update_black(0, 0, FRAME_WIDTH, FRAME_HEIGHT, 0x00);

    #if 0
    //Set TE register
	send_ctrl_cmd(0x35);
	send_data_cmd(0x00);

    send_ctrl_cmd(0X0044);  // Set TE signal delay scanline
    send_data_cmd(0X0000);  // Set as 0-th scanline
    send_data_cmd(0X0000);
	//sw_clear_panel(0);
    #endif
}


static void lcm_suspend(void)
{
	send_ctrl_cmd(0x28);
	MDELAY(50);
	send_ctrl_cmd(0x10);
	MDELAY(120);
}


#if 0//ndef  BUILD_UBOOT
#if 0
typedef struct
{
    unsigned int				STATUS;				// 1000
//    UINT16                    	rsv_0002;           // 1002
    unsigned int         	INT_ENABLE;         // 1004
//    UINT16                    	rsv_0006;           // 1006
    unsigned int         	INT_STATUS;         // 1008
//    UINT16                    	rsv_000A;           // 100A
    unsigned int             	START;              // 100C
//    UINT16                    	rsv_000E;           // 100E
    unsigned int                    	RESET;              // 1010
    unsigned int                    	rsv_0014[2];        // 1014..1018
	unsigned int		  	SIF_TIMING[2];	  	// 101C..1020
    unsigned int                    	rsv_0024;        	// 1024
	unsigned int			  	SERIAL_CFG;		  	// 1028
	unsigned int			  	SIF_CS;			  	// 102C
    unsigned int              	PARALLEL_CFG[3];    // 1030..1038
    unsigned int            	PARALLEL_DW;        // 103C
    unsigned int            	GAMCON;             // 1040
	unsigned int			CALC_HTT;			// 1044
	unsigned int		SYNC_LCM_SIZE;		// 1048
	unsigned int			SYNC_CNT;			// 104C
    unsigned int             	TEARING_CFG;        // 1050
    unsigned int            	GMC_CON;            // 1054
    unsigned int                    	rsv_0054[2];        // 1058..105C
    unsigned int                    	WROI_W2M_ADDR[3];   // 1060..1068
    unsigned int                    	rsv_006c;           // 106C
    unsigned int                    	W2M_PITCH;          // 1070
    unsigned int                   	rsv_0072;           // 1072    
    unsigned int        	WROI_W2M_OFFSET;    // 1074
    unsigned int       	WROI_W2M_CONTROL;   // 1078
    unsigned int                    	rsv_007C;	        // 107C
    unsigned int          	WROI_CONTROL;       // 1080
    unsigned int             	WROI_OFFSET;        // 1084
    unsigned int           	WROI_CMD_ADDR;      // 1088
//    UINT16                      rsv_008A;
    unsigned int           	WROI_DATA_ADDR;     // 108c
//    UINT16                      rsv_008E;
    unsigned int              	WROI_SIZE;          // 1090
    unsigned int            	WROI_HW_REFRESH;    // 1094
    unsigned int           	WROI_DC;            // 1098
    unsigned int                    	WROI_BG_COLOR;      // 109C
    unsigned int            	DS_DSI_CON;         // 10A0
    unsigned int                    	rsv_00A4[3];        // 10A4..10AC
    unsigned int             	LAYER[6];           // 10B0..11CC
    unsigned int                    	rsv_00D0[4];        // 11D0..11DC
    unsigned int                    	WROI_HWREF_BLK;     // 11E0
    unsigned int                    	WROI_HWREF_DLY;     // 11E4
    unsigned int                    	rsv_01E8[2];        // 11E8..11EC
    unsigned int        	DITHER_CON;         // 11F0
    unsigned int                    	rsv_01F4[3];        // 11F4..11FC
    unsigned int                    	LGMA_CON[20];       // 1200..124C
    unsigned int          	COEF_ROW[6];        // 1250..1264
//	#warning "need to implement the debug/control register"
    unsigned int                    	rsv_0268[358];      // 1268..17FC
	unsigned int                    	GAMMA[256];			// 1800..1BFF
	unsigned int						CMDQ[64];		    // 1C00..1CFC
	unsigned int						rsv_1D00[128];		// 1D00..1EFC
    unsigned int                    	PCMD0;             // 1F00
    unsigned int                    	rsv_1F04[7];	    // 1F04..1F1C
    unsigned int                    	PCMD1;             // 1F20 
    unsigned int                    	rsv_1F24[7];  		// 1F24..1F3C
    unsigned int                    	PCMD2;             // 1F40 
    unsigned int                    	rsv_1F44[15];	   	// 1F44..1F7C
    unsigned int                    	SCMD0;             // 1F80
    unsigned int                    	rsv_1F84[7];   		// 1F84..1F9C
    unsigned int                    	SCMD1;             // 1FA0
    unsigned int                    	rsv_1FA4[7];    	// 1FA4..1FBC
} volatile LCD_REGS, *PLCD_REGS;

#endif

// mt6575_reg_base.h
//PLCD_REGS const LCD_REG = (PLCD_REGS)0xF20A1000;

void print_lcm_registers(void)
{
	unsigned int i = 0;
	unsigned int *p = 0xF20A1000;

	for (i = 0x0000; i <= 0x01F0; i += 0x0004)
	{
		LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", (p + i), *(p + i));
	}
/*	
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->STATUS, LCD_REG->STATUS);
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->INT_ENABLE, LCD_REG->INT_ENABLE);
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->INT_STATUS, LCD_REG->INT_STATUS);
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->START, LCD_REG->START);
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->RESET, LCD_REG->RESET);

	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->SIF_TIMING[0], LCD_REG->SIF_TIMING[0]);
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->SERIAL_CFG, LCD_REG->SERIAL_CFG);
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->SIF_CS, LCD_REG->SIF_CS);
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->PARALLEL_CFG[0], LCD_REG->PARALLEL_CFG[0]);
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->PARALLEL_DW, LCD_REG->PARALLEL_DW);

	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->GAMCON, LCD_REG->GAMCON);
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->WROI_W2M_ADDR[0], LCD_REG->WROI_W2M_ADDR[0]);
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->W2M_PITCH, LCD_REG->W2M_PITCH);
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->WROI_W2M_OFFSET, LCD_REG->WROI_W2M_OFFSET);
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->WROI_W2M_CONTROL, LCD_REG->WROI_W2M_CONTROL);

	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->WROI_CONTROL, LCD_REG->WROI_CONTROL);
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->WROI_OFFSET, LCD_REG->WROI_OFFSET);
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->WROI_CMD_ADDR, LCD_REG->WROI_CMD_ADDR);
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->WROI_DATA_ADDR, LCD_REG->WROI_DATA_ADDR);
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->WROI_SIZE, LCD_REG->WROI_SIZE);

	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->WROI_HW_REFRESH, LCD_REG->WROI_HW_REFRESH);
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->WROI_DC, LCD_REG->WROI_DC);
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->WROI_BG_COLOR, LCD_REG->WROI_BG_COLOR);
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->DS_DSI_CON, LCD_REG->DS_DSI_CON);
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->LAYER[0], LCD_REG->LAYER[0]);

	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->WROI_HWREF_BLK, LCD_REG->WROI_HWREF_BLK);
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->WROI_HWREF_DLY, LCD_REG->WROI_HWREF_DLY);
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->DITHER_CON, LCD_REG->DITHER_CON);
*/
	/*
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->STATUS, LCD_REG->STATUS);
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->STATUS, LCD_REG->STATUS);

	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->STATUS, LCD_REG->STATUS);
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->STATUS, LCD_REG->STATUS);
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->STATUS, LCD_REG->STATUS);
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->STATUS, LCD_REG->STATUS);
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->STATUS, LCD_REG->STATUS);

	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->STATUS, LCD_REG->STATUS);
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->STATUS, LCD_REG->STATUS);
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->STATUS, LCD_REG->STATUS);
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->STATUS, LCD_REG->STATUS);
	LCD_PRINT("[JINCHI35_HX8357C] LCD_REG (%p) = 0x%x\n", &LCD_REG->STATUS, LCD_REG->STATUS);
	*/
}
#endif

static void lcm_resume(void)
{
	#if 1
	send_ctrl_cmd(0x11);
	MDELAY(120);
	send_ctrl_cmd(0x29);
	MDELAY(50);
	#else
	unsigned int lcd_id = 0;
	
	SET_RESET_PIN(1);
	MDELAY(5);
    SET_RESET_PIN(0);
    MDELAY(20);  
    SET_RESET_PIN(1);
    MDELAY(20);  // 400

	send_ctrl_cmd(0xB9);
	send_data_cmd(0xFF);
	send_data_cmd(0x83);
	send_data_cmd(0x57);

	UDELAY(10);

	lcd_id = get_lcd_id_n(LCMID_REG, 2);
	LCD_PRINT("[JINCHI35_HX8357C]lcm_resume: lcd_id = 0x%x\n", lcd_id);

#if 0//ndef BUILD_UBOOT
	print_lcm_registers();
#endif

	init_lcm_registers();	
	#endif
}

static void lcm_update(unsigned int x, unsigned int y,	unsigned int width, unsigned int height)
{
    unsigned int x0 = x;
    unsigned int y0 = y;
    unsigned int x1 = x0 + width - 1;
    unsigned int y1 = y0 + height - 1;

	send_ctrl_cmd(0x2A);
	send_data_cmd(HIGH_BYTE(x0));
	send_data_cmd(LOW_BYTE(x0));
	send_data_cmd(HIGH_BYTE(x1));
	send_data_cmd(LOW_BYTE(x1));

	send_ctrl_cmd(0x2B);
	send_data_cmd(HIGH_BYTE(y0));
	send_data_cmd(LOW_BYTE(y0));
	send_data_cmd(HIGH_BYTE(y1));
	send_data_cmd(LOW_BYTE(y1));

	// Write To GRAM
	send_ctrl_cmd(0x2C);
}

static unsigned int get_lcd_id_r(unsigned int addr)
{
	unsigned short id = 0;
	
	send_ctrl_cmd(addr);
    id = read_data_cmd();
	//id = (id << 8) | (read_data_cmd() & 0xFF);

	LCD_PRINT("[JINCHI35_HX8357C]get_lcd_id_r: id = 0x%x\n", id);
	
	return id;
}

static unsigned int get_lcd_id_n(unsigned int addr, unsigned char count)
{
	volatile unsigned int id = 0;
	unsigned char k = 0;
	

	send_ctrl_cmd(addr);
    UDELAY(10);

	while (k < count)
	{
		id = read_data_cmd();
		k++;
	}
	LCD_PRINT("[JINCHI35_HX8357C]get_lcd_id_n: id = 0x%x\n", id);
	
    return id;
}


static void lcm_setbacklight(unsigned int level)
{
	/*
    // Tearing effect
	if(level > 255) level = 255;
	send_ctrl_cmd(0x51);
	send_data_cmd(level);	
	*/
}

static void lcm_setpwm(unsigned int divider)
{
}

static unsigned int lcm_getpwm(unsigned int divider)
{
	unsigned int pwm_clk = 23706 / (1<<divider);	
	return pwm_clk;
}

static unsigned int lcm_compare_id(void)
{	
	unsigned int lcd_id = 0x0;

	SET_RESET_PIN(1);
	MDELAY(5);
    SET_RESET_PIN(0);
    MDELAY(50);  
    SET_RESET_PIN(1);
    MDELAY(120);

	send_ctrl_cmd(0xB9);
	send_data_cmd(0xFF);
	send_data_cmd(0x83);
	send_data_cmd(0x57);

	UDELAY(10);
	
	lcd_id = get_lcd_id_n(LCMID_REG, 2);
	LCD_PRINT("[JINCHI35_HX8357C]lcm_compare_id: lcd_id = 0x%x\n", lcd_id);
	
    return (LCM_ID == lcd_id)?1:0;
}

LCM_DRIVER jinchi35_hx8357c_lcm_drv = 
{
    .name			= "jinchi35_hx8357c",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.update         = lcm_update,
	
	.compare_id 	= lcm_compare_id,
	.set_backlight 	= lcm_setbacklight,
	.set_pwm		= lcm_setpwm,
	.get_pwm		= lcm_getpwm,
};

