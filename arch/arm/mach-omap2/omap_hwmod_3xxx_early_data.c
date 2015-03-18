/*
 * omap_hwmod_3xxx_early_data.c - hardware modules present on the OMAP3xxx chips
 *
 * Copyright (C) 2009-2011 Nokia Corporation
 * Copyright (C) 2012 Texas Instruments, Inc.
 * Paul Walmsley
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * The data in this file should be completely autogeneratable from
 * the TI hardware database or other technical documentation.
 *
 * XXX these should be marked initdata for multi-OMAP kernels
 */

#include <linux/i2c-omap.h>
#include <linux/power/smartreflex.h>
#include <linux/platform_data/gpio-omap.h>
#include <linux/platform_data/hsmmc-omap.h>

#include <linux/omap-dma.h>
#include "l3_3xxx.h"
#include "l4_3xxx.h"
#include <linux/platform_data/asoc-ti-mcbsp.h>
#include <linux/platform_data/spi-omap2-mcspi.h>
#include <linux/platform_data/iommu-omap.h>
#include <linux/platform_data/mailbox-omap.h>
#include <plat/dmtimer.h>

#include "soc.h"
#include "omap_hwmod.h"
#include "omap_hwmod_common_data.h"
#include "prm-regbits-34xx.h"
#include "cm-regbits-34xx.h"

#include "i2c.h"
#include "wd_timer.h"
#include "serial.h"

/* L3 */
static struct omap_hwmod_irq_info omap3xxx_l3_main_irqs[] = {
	{ .irq = 9 + OMAP_INTC_START, },
	{ .irq = 10 + OMAP_INTC_START, },
	{ .irq = -1 },
};

static struct omap_hwmod omap3xxx_l3_main_hwmod = {
	.name		= "l3_main",
	.class		= &l3_hwmod_class,
	.mpu_irqs	= omap3xxx_l3_main_irqs,
	.flags		= HWMOD_NO_IDLEST,
};

/* L4 CORE */
static struct omap_hwmod omap3xxx_l4_core_hwmod = {
	.name		= "l4_core",
	.class		= &l4_hwmod_class,
	.flags		= HWMOD_NO_IDLEST,
};

/* L4 PER */
static struct omap_hwmod omap3xxx_l4_per_hwmod = {
	.name		= "l4_per",
	.class		= &l4_hwmod_class,
	.flags		= HWMOD_NO_IDLEST,
};

/* L4 WKUP */
static struct omap_hwmod omap3xxx_l4_wkup_hwmod = {
	.name		= "l4_wkup",
	.class		= &l4_hwmod_class,
	.flags		= HWMOD_NO_IDLEST,
};

/* L4 SEC */
static struct omap_hwmod omap3xxx_l4_sec_hwmod = {
	.name		= "l4_sec",
	.class		= &l4_hwmod_class,
	.flags		= HWMOD_NO_IDLEST,
};

/* MPU */
static struct omap_hwmod_irq_info omap3xxx_mpu_irqs[] = {
	{ .name = "pmu", .irq = 3 + OMAP_INTC_START },
	{ .irq = -1 }
};

static struct omap_hwmod omap3xxx_mpu_hwmod = {
	.name		= "mpu",
	.mpu_irqs	= omap3xxx_mpu_irqs,
	.class		= &mpu_hwmod_class,
	.main_clk	= "arm_fck",
};

/* timer class */
static struct omap_hwmod_class_sysconfig omap3xxx_timer_sysc = {
	.rev_offs	= 0x0000,
	.sysc_offs	= 0x0010,
	.syss_offs	= 0x0014,
	.sysc_flags	= (SYSC_HAS_SIDLEMODE | SYSC_HAS_CLOCKACTIVITY |
			   SYSC_HAS_ENAWAKEUP | SYSC_HAS_SOFTRESET |
			   SYSC_HAS_EMUFREE | SYSC_HAS_AUTOIDLE |
			   SYSS_HAS_RESET_STATUS),
	.idlemodes	= (SIDLE_FORCE | SIDLE_NO | SIDLE_SMART),
	.clockact	= CLOCKACT_TEST_ICLK,
	.sysc_fields	= &omap_hwmod_sysc_type1,
};

static struct omap_hwmod_class omap3xxx_timer_hwmod_class = {
	.name = "timer",
	.sysc = &omap3xxx_timer_sysc,
};

/* secure timers dev attribute */
static struct omap_timer_capability_dev_attr capability_secure_dev_attr = {
	.timer_capability	= OMAP_TIMER_ALWON | OMAP_TIMER_SECURE,
};

/* always-on timers dev attribute */
static struct omap_timer_capability_dev_attr capability_alwon_dev_attr = {
	.timer_capability	= OMAP_TIMER_ALWON,
};

/* timer1 */
static struct omap_hwmod omap3xxx_timer1_hwmod = {
	.name		= "timer1",
	.mpu_irqs	= omap2_timer1_mpu_irqs,
	.main_clk	= "gpt1_fck",
	.prcm		= {
		.omap2 = {
			.prcm_reg_id = 1,
			.module_bit = OMAP3430_EN_GPT1_SHIFT,
			.module_offs = WKUP_MOD,
			.idlest_reg_id = 1,
			.idlest_idle_bit = OMAP3430_ST_GPT1_SHIFT,
		},
	},
	.dev_attr	= &capability_alwon_dev_attr,
	.class		= &omap3xxx_timer_hwmod_class,
	.flags		= HWMOD_SET_DEFAULT_CLOCKACT,
};

/* timer12 */
static struct omap_hwmod_irq_info omap3xxx_timer12_mpu_irqs[] = {
	{ .irq = 95 + OMAP_INTC_START, },
	{ .irq = -1 },
};

static struct omap_hwmod omap3xxx_timer12_hwmod = {
	.name		= "timer12",
	.mpu_irqs	= omap3xxx_timer12_mpu_irqs,
	.main_clk	= "gpt12_fck",
	.prcm		= {
		.omap2 = {
			.prcm_reg_id = 1,
			.module_bit = OMAP3430_EN_GPT12_SHIFT,
			.module_offs = WKUP_MOD,
			.idlest_reg_id = 1,
			.idlest_idle_bit = OMAP3430_ST_GPT12_SHIFT,
		},
	},
	.dev_attr	= &capability_secure_dev_attr,
	.class		= &omap3xxx_timer_hwmod_class,
	.flags		= HWMOD_SET_DEFAULT_CLOCKACT,
};

/*
 * '32K sync counter' class
 * 32-bit ordinary counter, clocked by the falling edge of the 32 khz clock
 */
static struct omap_hwmod_class_sysconfig omap3xxx_counter_sysc = {
	.rev_offs	= 0x0000,
	.sysc_offs	= 0x0004,
	.sysc_flags	= SYSC_HAS_SIDLEMODE,
	.idlemodes	= (SIDLE_FORCE | SIDLE_NO),
	.sysc_fields	= &omap_hwmod_sysc_type1,
};

static struct omap_hwmod_class omap3xxx_counter_hwmod_class = {
	.name	= "counter",
	.sysc	= &omap3xxx_counter_sysc,
};

static struct omap_hwmod omap3xxx_counter_32k_hwmod = {
	.name		= "counter_32k",
	.class		= &omap3xxx_counter_hwmod_class,
	.clkdm_name	= "wkup_clkdm",
	.flags		= HWMOD_SWSUP_SIDLE,
	.main_clk	= "wkup_32k_fck",
	.prcm		= {
		.omap2	= {
			.module_offs = WKUP_MOD,
			.prcm_reg_id = 1,
			.module_bit = OMAP3430_ST_32KSYNC_SHIFT,
			.idlest_reg_id = 1,
			.idlest_idle_bit = OMAP3430_ST_32KSYNC_SHIFT,
		},
	},
};

/* UART1 */
static struct omap_hwmod omap3xxx_uart1_hwmod = {
	.name		= "uart1",
	.mpu_irqs	= omap2_uart1_mpu_irqs,
	.sdma_reqs	= omap2_uart1_sdma_reqs,
	.main_clk	= "uart1_fck",
	.flags		= DEBUG_TI81XXUART1_FLAGS | HWMOD_SWSUP_SIDLE,
	.prcm		= {
		.omap2 = {
			.module_offs = CORE_MOD,
			.prcm_reg_id = 1,
			.module_bit = OMAP3430_EN_UART1_SHIFT,
			.idlest_reg_id = 1,
			.idlest_idle_bit = OMAP3430_EN_UART1_SHIFT,
		},
	},
	.class		= &omap2_uart_class,
};

/* UART2 */
static struct omap_hwmod omap3xxx_uart2_hwmod = {
	.name		= "uart2",
	.mpu_irqs	= omap2_uart2_mpu_irqs,
	.sdma_reqs	= omap2_uart2_sdma_reqs,
	.main_clk	= "uart2_fck",
	.flags		= DEBUG_TI81XXUART2_FLAGS | HWMOD_SWSUP_SIDLE,
	.prcm		= {
		.omap2 = {
			.module_offs = CORE_MOD,
			.prcm_reg_id = 1,
			.module_bit = OMAP3430_EN_UART2_SHIFT,
			.idlest_reg_id = 1,
			.idlest_idle_bit = OMAP3430_EN_UART2_SHIFT,
		},
	},
	.class		= &omap2_uart_class,
};

/* UART3 */
static struct omap_hwmod omap3xxx_uart3_hwmod = {
	.name		= "uart3",
	.mpu_irqs	= omap2_uart3_mpu_irqs,
	.sdma_reqs	= omap2_uart3_sdma_reqs,
	.main_clk	= "uart3_fck",
	.flags		= DEBUG_OMAP3UART3_FLAGS | DEBUG_TI81XXUART3_FLAGS |
				HWMOD_SWSUP_SIDLE,
	.prcm		= {
		.omap2 = {
			.module_offs = OMAP3430_PER_MOD,
			.prcm_reg_id = 1,
			.module_bit = OMAP3430_EN_UART3_SHIFT,
			.idlest_reg_id = 1,
			.idlest_idle_bit = OMAP3430_EN_UART3_SHIFT,
		},
	},
	.class		= &omap2_uart_class,
};

/* UART4 */
static struct omap_hwmod_irq_info uart4_mpu_irqs[] = {
	{ .irq = 80 + OMAP_INTC_START, },
	{ .irq = -1 },
};

static struct omap_hwmod_dma_info uart4_sdma_reqs[] = {
	{ .name = "rx", .dma_req = 82, },
	{ .name = "tx", .dma_req = 81, },
	{ .dma_req = -1 }
};

static struct omap_hwmod omap36xx_uart4_hwmod = {
	.name		= "uart4",
	.mpu_irqs	= uart4_mpu_irqs,
	.sdma_reqs	= uart4_sdma_reqs,
	.main_clk	= "uart4_fck",
	.flags		= DEBUG_OMAP3UART4_FLAGS | HWMOD_SWSUP_SIDLE,
	.prcm		= {
		.omap2 = {
			.module_offs = OMAP3430_PER_MOD,
			.prcm_reg_id = 1,
			.module_bit = OMAP3630_EN_UART4_SHIFT,
			.idlest_reg_id = 1,
			.idlest_idle_bit = OMAP3630_EN_UART4_SHIFT,
		},
	},
	.class		= &omap2_uart_class,
};

static struct omap_hwmod_irq_info am35xx_uart4_mpu_irqs[] = {
	{ .irq = 84 + OMAP_INTC_START, },
	{ .irq = -1 },
};

static struct omap_hwmod_dma_info am35xx_uart4_sdma_reqs[] = {
	{ .name = "rx", .dma_req = 55, },
	{ .name = "tx", .dma_req = 54, },
	{ .dma_req = -1 }
};

/*
 * XXX AM35xx UART4 cannot complete its softreset without uart1_fck or
 * uart2_fck being enabled.  So we add uart1_fck as an optional clock,
 * below, and set the HWMOD_CONTROL_OPT_CLKS_IN_RESET.  This really
 * should not be needed.  The functional clock structure of the AM35xx
 * UART4 is extremely unclear and opaque; it is unclear what the role
 * of uart1/2_fck is for the UART4.  Any clarification from either
 * empirical testing or the AM3505/3517 hardware designers would be
 * most welcome.
 */
static struct omap_hwmod_opt_clk am35xx_uart4_opt_clks[] = {
	{ .role = "softreset_uart1_fck", .clk = "uart1_fck" },
};

static struct omap_hwmod am35xx_uart4_hwmod = {
	.name		= "uart4",
	.mpu_irqs	= am35xx_uart4_mpu_irqs,
	.sdma_reqs	= am35xx_uart4_sdma_reqs,
	.main_clk	= "uart4_fck",
	.prcm		= {
		.omap2 = {
			.module_offs = CORE_MOD,
			.prcm_reg_id = 1,
			.module_bit = AM35XX_EN_UART4_SHIFT,
			.idlest_reg_id = 1,
			.idlest_idle_bit = AM35XX_ST_UART4_SHIFT,
		},
	},
	.opt_clks	= am35xx_uart4_opt_clks,
	.opt_clks_cnt	= ARRAY_SIZE(am35xx_uart4_opt_clks),
	.flags		= HWMOD_CONTROL_OPT_CLKS_IN_RESET,
	.class		= &omap2_uart_class,
};

static struct omap_hwmod_addr_space omap3xxx_l3_main_addrs[] = {
	{
		.pa_start	= 0x68000000,
		.pa_end		= 0x6800ffff,
		.flags		= ADDR_TYPE_RT,
	},
	{ }
};

/* MPU -> L3 interface */
static struct omap_hwmod_ocp_if omap3xxx_mpu__l3_main = {
	.master		= &omap3xxx_mpu_hwmod,
	.slave		= &omap3xxx_l3_main_hwmod,
	.addr		= omap3xxx_l3_main_addrs,
	.user		= OCP_USER_MPU,
};

/* L4 CORE -> UART1 interface */
static struct omap_hwmod_addr_space omap3xxx_uart1_addr_space[] = {
	{
		.pa_start	= OMAP3_UART1_BASE,
		.pa_end		= OMAP3_UART1_BASE + SZ_8K - 1,
		.flags		= ADDR_MAP_ON_INIT | ADDR_TYPE_RT,
	},
	{ }
};

static struct omap_hwmod_ocp_if omap3_l4_core__uart1 = {
	.master		= &omap3xxx_l4_core_hwmod,
	.slave		= &omap3xxx_uart1_hwmod,
	.clk		= "uart1_ick",
	.addr		= omap3xxx_uart1_addr_space,
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

/* L4 CORE -> UART2 interface */
static struct omap_hwmod_addr_space omap3xxx_uart2_addr_space[] = {
	{
		.pa_start	= OMAP3_UART2_BASE,
		.pa_end		= OMAP3_UART2_BASE + SZ_1K - 1,
		.flags		= ADDR_MAP_ON_INIT | ADDR_TYPE_RT,
	},
	{ }
};

static struct omap_hwmod_ocp_if omap3_l4_core__uart2 = {
	.master		= &omap3xxx_l4_core_hwmod,
	.slave		= &omap3xxx_uart2_hwmod,
	.clk		= "uart2_ick",
	.addr		= omap3xxx_uart2_addr_space,
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

/* L4 PER -> UART3 interface */
static struct omap_hwmod_addr_space omap3xxx_uart3_addr_space[] = {
	{
		.pa_start	= OMAP3_UART3_BASE,
		.pa_end		= OMAP3_UART3_BASE + SZ_1K - 1,
		.flags		= ADDR_MAP_ON_INIT | ADDR_TYPE_RT,
	},
	{ }
};

static struct omap_hwmod_ocp_if omap3_l4_per__uart3 = {
	.master		= &omap3xxx_l4_per_hwmod,
	.slave		= &omap3xxx_uart3_hwmod,
	.clk		= "uart3_ick",
	.addr		= omap3xxx_uart3_addr_space,
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

/* L4 PER -> UART4 interface */
static struct omap_hwmod_addr_space omap36xx_uart4_addr_space[] = {
	{
		.pa_start	= OMAP3_UART4_BASE,
		.pa_end		= OMAP3_UART4_BASE + SZ_1K - 1,
		.flags		= ADDR_MAP_ON_INIT | ADDR_TYPE_RT,
	},
	{ }
};

static struct omap_hwmod_ocp_if omap36xx_l4_per__uart4 = {
	.master		= &omap3xxx_l4_per_hwmod,
	.slave		= &omap36xx_uart4_hwmod,
	.clk		= "uart4_ick",
	.addr		= omap36xx_uart4_addr_space,
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

/* AM35xx: L4 CORE -> UART4 interface */
static struct omap_hwmod_addr_space am35xx_uart4_addr_space[] = {
	{
		.pa_start	= OMAP3_UART4_AM35XX_BASE,
		.pa_end		= OMAP3_UART4_AM35XX_BASE + SZ_1K - 1,
		.flags		= ADDR_MAP_ON_INIT | ADDR_TYPE_RT,
	},
	{ }
};

static struct omap_hwmod_ocp_if am35xx_l4_core__uart4 = {
	.master		= &omap3xxx_l4_core_hwmod,
	.slave		= &am35xx_uart4_hwmod,
	.clk		= "uart4_ick",
	.addr		= am35xx_uart4_addr_space,
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

static struct omap_hwmod_addr_space omap3xxx_timer1_addrs[] = {
	{
		.pa_start	= 0x48318000,
		.pa_end		= 0x48318000 + SZ_1K - 1,
		.flags		= ADDR_TYPE_RT
	},
	{ }
};

/* l4_wkup -> timer1 */
static struct omap_hwmod_ocp_if omap3xxx_l4_wkup__timer1 = {
	.master		= &omap3xxx_l4_wkup_hwmod,
	.slave		= &omap3xxx_timer1_hwmod,
	.clk		= "gpt1_ick",
	.addr		= omap3xxx_timer1_addrs,
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

static struct omap_hwmod_addr_space omap3xxx_timer12_addrs[] = {
	{
		.pa_start	= 0x48304000,
		.pa_end		= 0x48304000 + SZ_1K - 1,
		.flags		= ADDR_TYPE_RT
	},
	{ }
};

/* l4_core -> timer12 */
static struct omap_hwmod_ocp_if omap3xxx_l4_sec__timer12 = {
	.master		= &omap3xxx_l4_sec_hwmod,
	.slave		= &omap3xxx_timer12_hwmod,
	.clk		= "gpt12_ick",
	.addr		= omap3xxx_timer12_addrs,
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

/* l4_wkup -> 32ksync_counter */
static struct omap_hwmod_addr_space omap3xxx_counter_32k_addrs[] = {
	{
		.pa_start	= 0x48320000,
		.pa_end		= 0x4832001f,
		.flags		= ADDR_TYPE_RT
	},
	{ }
};

static struct omap_hwmod_ocp_if omap3xxx_l4_wkup__counter_32k = {
	.master		= &omap3xxx_l4_wkup_hwmod,
	.slave		= &omap3xxx_counter_32k_hwmod,
	.clk		= "omap_32ksync_ick",
	.addr		= omap3xxx_counter_32k_addrs,
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

static struct omap_hwmod_ocp_if *omap3_early_hwmod_ocp_ifs[] __initdata = {
	&omap3xxx_mpu__l3_main,
	&omap3xxx_l4_wkup__timer1,
	&omap3xxx_l4_wkup__counter_32k,
	&omap3_l4_core__uart1,
	&omap3_l4_core__uart2,
	&omap3_l4_per__uart3,
	NULL,
};

static struct omap_hwmod_ocp_if *omap3_gp_early_hwmod_ocp_ifs[] __initdata = {
	&omap3xxx_l4_sec__timer12,
	NULL,
};

static struct omap_hwmod_ocp_if *omap36xx_early_hwmod_ocp_ifs[] __initdata = {
	&omap36xx_l4_per__uart4,
	NULL,
};

static struct omap_hwmod_ocp_if *am35xx_early_hwmod_ocp_ifs[] __initdata = {
	&am35xx_l4_core__uart4,
	NULL,
};

int __init omap3xxx_hwmod_early_init(void)
{
	int r;
	unsigned int rev;

	omap_hwmod_init();

	rev = omap_rev();

	r = omap_hwmod_register_links(omap3_early_hwmod_ocp_ifs);
	if (r < 0)
		return r;

	if (omap_type() == OMAP2_DEVICE_TYPE_GP) {
		r = omap_hwmod_register_links(omap3_gp_early_hwmod_ocp_ifs);

		if (r < 0)
			return r;
	}

	if (rev == AM35XX_REV_ES1_0 || rev == AM35XX_REV_ES1_1)
		r = omap_hwmod_register_links(am35xx_early_hwmod_ocp_ifs);
	else if (rev == OMAP3630_REV_ES1_0 || rev == OMAP3630_REV_ES1_1 ||
		 rev == OMAP3630_REV_ES1_2)
		r = omap_hwmod_register_links(omap36xx_early_hwmod_ocp_ifs);

	return r;
}
