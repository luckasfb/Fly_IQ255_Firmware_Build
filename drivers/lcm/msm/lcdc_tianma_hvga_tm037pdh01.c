/* Copyright (c) 2009-2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/delay.h>
#include <linux/module.h>
#include <mach/gpio.h>
#include <mach/pmic.h>
#include "msm_fb.h"

static int prev_bl = 17;

static int spi_cs;
static int spi_sclk;
static int spi_mosi;
static int gpio_backlight_en;
static int gpio_display_reset;

struct tianma_state_type{
	boolean disp_initialized;
	boolean display_on;
	boolean disp_powered_up;
};

static struct tianma_state_type tianma_state = { 0 };
static struct msm_panel_common_pdata *lcdc_tianma_pdata;

static __inline__ void dpi_spi_write_byte(char dc, uint8 data)
{
	uint32 bit;
	int bnum;

	gpio_set_value_cansleep(spi_sclk, 0); /* clk low */
	/* dc: 0 for command, 1 for parameter */
	gpio_set_value_cansleep(spi_mosi, dc);
	udelay(1);	/* at least 20 ns */
	gpio_set_value_cansleep(spi_sclk, 1); /* clk high */
	udelay(1);	/* at least 20 ns */
	bnum = 8;	/* 8 data bits */
	bit = 0x80;
	while (bnum) {
		gpio_set_value_cansleep(spi_sclk, 0); /* clk low */
		if (data & bit)
			gpio_set_value_cansleep(spi_mosi, 1);
		else
			gpio_set_value_cansleep(spi_mosi, 0);
		udelay(1);
		gpio_set_value_cansleep(spi_sclk, 1); /* clk high */
		udelay(1);
		bit >>= 1;
		bnum--;
	}
}

static __inline__ int dpi_spi_write(char cmd, char *data, int num)
{
	int i;

	gpio_set_value_cansleep(spi_cs, 0);	/* cs low */
	/* command byte first */
	dpi_spi_write_byte(0, cmd);
	/* followed by parameter bytes */
	for(i = 0; i < num; i++)
	{
		if(data)
			dpi_spi_write_byte(1, data[i]);
	}
	gpio_set_value_cansleep(spi_mosi, 1);	/* mosi high */
	gpio_set_value_cansleep(spi_cs, 1);	/* cs high */
	udelay(10);
	return 0;
}

static void spi_pin_assign(void)
{
	printk("%s E\n", __func__);

	/* Setting the Default GPIO's */
	spi_mosi	= *(lcdc_tianma_pdata->gpio_num);
	spi_sclk	= *(lcdc_tianma_pdata->gpio_num + 1);
	spi_cs		= *(lcdc_tianma_pdata->gpio_num + 2);
	gpio_backlight_en = *(lcdc_tianma_pdata->gpio_num + 3);
	gpio_display_reset = *(lcdc_tianma_pdata->gpio_num + 4);
	printk("spi_mosi:%d spi_sclk:%d spi_cs:%d backlight:%d reset:%d\n",
		spi_mosi, spi_sclk, spi_cs, gpio_backlight_en, gpio_display_reset);

	printk("%s X\n", __func__);
}

static void tianma_disp_powerup(void)
{
	printk("%s E\n", __func__);

	if (!tianma_state.disp_powered_up && !tianma_state.display_on) {
		/* Reset the hardware first */
		/* Include DAC power up implementation here */
	      tianma_state.disp_powered_up = TRUE;
	}

	printk("%s X\n", __func__);
}


static char init_item_v1[] = {
	0xff, 0x83, 0x57,
};
static char init_item_v2[] = {
	0x09,
};
static char init_item_v3[] = {
	0x00,0x24,0x26,0x22,
	0x83,0x48,
};
static char init_item_v4[] = {
	0x43, 0x06, 0x06, 0x06,
};
static char init_item_v5[] = {
	0x52,
};
static char init_item_v6[] = { 
	0x02, 0x40, 0x00, 0x2a,
	0x2a, 0x0d, 0x3f, 
};
static char init_item_v7[] = {
 	0x50,0x50,0x01,0x3C,
	0xC8,0x08,
 };
static char init_item_v8[] = {
	0x17, 0x00,
};
static char init_item_v9[] = {
	0x66
};
static char init_item_v10[] = {
	0x00,0x05,0x0E,0x29,
	0x2C,0x41,0x49,0x52,
	0x48,0x42,0x3D,0x33,
	0x2F,0x27,0x24,0x00,
	0x00,0x05,0x0E,0x29,
	0x2C,0x41,0x49,0x52,
	0x48,0x42,0x3D,0x33,
	0x2F,0x27,0x24,0x00,
	0x00,0x01,
};
static char init_item_v15[2] = {
	0x03,0x03,
};

static void tianma_disp_reginit(void)
{
	printk("%s Edisp_powered_up:%d display_on:%d\n", __func__,tianma_state.disp_powered_up, tianma_state.display_on);
	if (tianma_state.disp_powered_up && !tianma_state.display_on) {
		gpio_set_value_cansleep(spi_cs, 1);	/* cs high */

		dpi_spi_write(0x11, NULL, 0);//sleep out
		mdelay(150);
		dpi_spi_write(0xb9, init_item_v1, sizeof(init_item_v1));
		mdelay(5);
		dpi_spi_write(0xcc, init_item_v2, sizeof(init_item_v2));
		dpi_spi_write(0xb1, init_item_v3, sizeof(init_item_v3));
		dpi_spi_write(0xb3, init_item_v4, sizeof(init_item_v4));
		dpi_spi_write(0xb6, init_item_v5, sizeof(init_item_v5));
		dpi_spi_write(0xb4, init_item_v6, sizeof(init_item_v6));
		dpi_spi_write(0xB5, init_item_v15, sizeof(init_item_v15));
		dpi_spi_write(0xc0, init_item_v7, sizeof(init_item_v7));
		dpi_spi_write(0xe3, init_item_v8, sizeof(init_item_v8));
		dpi_spi_write(0x3a, init_item_v9, sizeof(init_item_v9));
		dpi_spi_write(0xe0, init_item_v10, sizeof(init_item_v10));
		dpi_spi_write(0x29, NULL, 0);
		mdelay(20);
		dpi_spi_write(0x2c, NULL, 0);

		tianma_state.display_on = TRUE;
	}
	printk("%s X\n", __func__);
}

static int lcdc_tianma_panel_on(struct platform_device *pdev)
{
	printk("%s E\n", __func__);
	/* Configure reset GPIO that drives DAC */
	if (lcdc_tianma_pdata->panel_config_gpio)
		lcdc_tianma_pdata->panel_config_gpio(1);
	gpio_set_value_cansleep(gpio_display_reset, 1);
	tianma_disp_powerup();
	tianma_disp_reginit();
	tianma_state.disp_initialized = TRUE;
	printk("%s X\n", __func__);
	return 0;
}

static int lcdc_tianma_panel_off(struct platform_device *pdev)
{
	printk("%s E\n", __func__);
	if (tianma_state.disp_powered_up && tianma_state.display_on) {
		/* Main panel power off (Pull down reset) */
		gpio_set_value_cansleep(gpio_display_reset, 0);
		tianma_state.display_on = FALSE;
		tianma_state.disp_initialized = FALSE;
	}
	printk("%s X\n", __func__);
	return 0;
}

static void lcdc_tianma_set_backlight(struct msm_fb_data_type *mfd)
{
	int step = 0, i = 0;
	unsigned long flags;
	int bl_level = mfd->bl_level;

	/* real backlight level, 1 - max, 16 - min, 17 - off */
	bl_level = 17 - bl_level;

	//printk("%s: prev_bl = %d, bl_level = %d\n", __func__, prev_bl, bl_level);

	if (bl_level > prev_bl) {
		step = bl_level - prev_bl;
		if (bl_level == 17) {
			step--;
		}
	} else if (bl_level < prev_bl) {
		step = bl_level + 16 - prev_bl;
	} else {
		printk("%s: no change\n", __func__);
		return;
	}

	//printk("%s: step = %d\n", __func__, step);

	if (bl_level == 17) {
		/* turn off backlight */
		//printk("%s: turn off backlight\n", __func__);
		gpio_set_value(gpio_backlight_en, 0);
	} else {
		local_irq_save(flags);

		if (prev_bl == 17) {
			/* turn on backlight */
			//printk("%s: turn on backlight\n", __func__);
			gpio_set_value(gpio_backlight_en, 1);
			udelay(30);
		}

		/* adjust backlight level */
		for (i = 0; i < step; i++) {
			gpio_set_value(gpio_backlight_en, 0);
			udelay(1);
			gpio_set_value(gpio_backlight_en, 1);
			udelay(1);
		}

		local_irq_restore(flags);
	}
	msleep(1);
	prev_bl = bl_level;

	return;
}

static int __devinit tianma_probe(struct platform_device *pdev)
{
	printk("%s E\n", __func__);

	if (pdev->id == 0) {
		lcdc_tianma_pdata = pdev->dev.platform_data;
		spi_pin_assign();
		return 0;
	}
	msm_fb_add_device(pdev);

	printk("%s X\n", __func__);
	return 0;
}

static struct platform_driver this_driver = {
	.probe  = tianma_probe,
	.driver = {
		.name	= "lcdc_tianma_tm037pdh01_pt",
	},
};

static struct msm_fb_panel_data tianma_panel_data = {
	.on = lcdc_tianma_panel_on,
	.off = lcdc_tianma_panel_off,
	.set_backlight = lcdc_tianma_set_backlight,
};

static struct platform_device this_device = {
	.name	= "lcdc_tianma_tm037pdh01_pt",
	.id	= 1,
	.dev	= {
		.platform_data = &tianma_panel_data,
	}
};

static int __init lcdc_tianma_panel_init(void)
{
	int ret;
	struct msm_panel_info *pinfo;

	printk("%s E\n", __func__);

	ret = platform_driver_register(&this_driver);
	if (ret)
		return ret;
	pinfo = &tianma_panel_data.panel_info;
	pinfo->xres = 320;
	pinfo->yres = 480;
	MSM_FB_SINGLE_MODE_PANEL(pinfo);
	pinfo->type = LCDC_PANEL;
	pinfo->pdest = DISPLAY_1;
	pinfo->wait_cycle = 0;
	pinfo->bpp = 18;
	pinfo->fb_num = 2;
	/* 10Mhz mdp_lcdc_pclk and mdp_lcdc_pad_pcl */
	pinfo->clk_rate = 10240000;
	pinfo->bl_max = 16;
	pinfo->bl_min = 1;

	pinfo->lcdc.h_back_porch = 16;		/* hsw = 8 + hbp=16 */
	pinfo->lcdc.h_front_porch = 4;
	pinfo->lcdc.h_pulse_width = 8;
	pinfo->lcdc.v_back_porch = 7;		/* vsw=1 + vbp = 7 */
	pinfo->lcdc.v_front_porch = 3;
	pinfo->lcdc.v_pulse_width = 1;
	pinfo->lcdc.border_clr = 0;			/* blk */
	pinfo->lcdc.underflow_clr = 0xff;	/* blue */
	pinfo->lcdc.hsync_skew = 0;

	ret = platform_device_register(&this_device);
	if (ret) {
		printk(KERN_ERR "%s not able to register the device\n",
			 __func__);
		goto fail_driver;
	}

	printk("%s X\n", __func__);
	return ret;

fail_driver:
	platform_driver_unregister(&this_driver);
	return ret;
}

device_initcall(lcdc_tianma_panel_init);
