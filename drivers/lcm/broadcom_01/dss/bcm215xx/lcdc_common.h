/*******************************************************************************
* Copyright 2010 Broadcom Corporation.  All rights reserved.
*
* 	@file	drivers/video/broadcom/dss/bcm215xx/lcdc.h
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

#ifndef __BCM_LCDC_H
#define __BCM_LCDC_H

#include <linux/string.h>
#include <linux/module.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/interrupt.h>

#include <linux/version.h>
#include <linux/types.h>
#include <linux/param.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/kernel_stat.h>
#include <linux/broadcom/bcm_sysctl.h>
#include <linux/dma-mapping.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#endif
#include <linux/suspend.h>
#include <linux/kthread.h>
#include <plat/dma.h>
#include <linux/platform_device.h>
#include <plat/bcm_lcdc.h>

#include <asm/byteorder.h>
#include <asm/irq.h>
#include <linux/gpio.h>
#include <linux/broadcom/regaccess.h>
#if defined(CONFIG_BCM_IDLE_PROFILER_SUPPORT)
#include <linux/broadcom/idle_prof.h>
#endif
#include <linux/broadcom/cpu_sleep.h>
#include <asm/mach/irq.h>
#include <asm/io.h>

#include <linux/broadcom/bcm_major.h>
#include <linux/broadcom/hw.h>
#include <linux/broadcom/lcd.h>
#include <linux/broadcom/PowerManager.h>
#include <cfg_global.h>
#include <plat/syscfg.h>
#include <linux/regulator/consumer.h>

#include "lcd.h"
#include "reg_lcdc.h"
#include <plat/types.h>
#include <plat/osdal_os_driver.h>
#include <plat/csl/csl_lcd.h>
#include <linux/clk.h>

#include <plat/csl/csl_lcdc.h>
#include <plat/csl/csl_dsi.h>

#ifdef CONFIG_CPU_FREQ_GOV_BCM21553
#include <mach/bcm21553_cpufreq_gov.h>
#endif

#endif /* __BCM_LCDC_H */
