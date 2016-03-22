/*
 * OMAP3 Clock init
 *
 * Copyright (C) 2013 Texas Instruments, Inc
 *     Tero Kristo (t-kristo@ti.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/clk/ti.h>

#include "clock.h"

/*
 * DPLL5_FREQ_FOR_USBHOST: USBHOST and USBTLL are the only clocks
 * that are sourced by DPLL5, and both of these require this clock
 * to be at 120 MHz for proper operation.
 */
#define DPLL5_FREQ_FOR_USBHOST		120000000

#define OMAP3430ES2_ST_DSS_IDLE_SHIFT			1
#define OMAP3430ES2_ST_HSOTGUSB_IDLE_SHIFT		5
#define OMAP3430ES2_ST_SSI_IDLE_SHIFT			8

#define OMAP34XX_CM_IDLEST_VAL				1

/*
 * In AM35xx IPSS, the {ICK,FCK} enable bits for modules are exported
 * in the same register at a bit offset of 0x8. The EN_ACK for ICK is
 * at an offset of 4 from ICK enable bit.
 */
#define AM35XX_IPSS_ICK_MASK			0xF
#define AM35XX_IPSS_ICK_EN_ACK_OFFSET		0x4
#define AM35XX_IPSS_ICK_FCK_OFFSET		0x8
#define AM35XX_IPSS_CLK_IDLEST_VAL		0

#define AM35XX_ST_IPSS_SHIFT			5

/**
 * omap3430es2_clk_ssi_find_idlest - return CM_IDLEST info for SSI
 * @clk: struct clk * being enabled
 * @idlest_reg: void __iomem ** to store CM_IDLEST reg address into
 * @idlest_bit: pointer to a u8 to store the CM_IDLEST bit shift into
 * @idlest_val: pointer to a u8 to store the CM_IDLEST indicator
 *
 * The OMAP3430ES2 SSI target CM_IDLEST bit is at a different shift
 * from the CM_{I,F}CLKEN bit.  Pass back the correct info via
 * @idlest_reg and @idlest_bit.  No return value.
 */
static void omap3430es2_clk_ssi_find_idlest(struct clk_hw_omap *clk,
					    void __iomem **idlest_reg,
					    u8 *idlest_bit,
					    u8 *idlest_val)
{
	u32 r;

	r = (((__force u32)clk->enable_reg & ~0xf0) | 0x20);
	*idlest_reg = (__force void __iomem *)r;
	*idlest_bit = OMAP3430ES2_ST_SSI_IDLE_SHIFT;
	*idlest_val = OMAP34XX_CM_IDLEST_VAL;
}

const struct clk_hw_omap_ops clkhwops_omap3430es2_iclk_ssi_wait = {
	.allow_idle	= omap2_clkt_iclk_allow_idle,
	.deny_idle	= omap2_clkt_iclk_deny_idle,
	.find_idlest	= omap3430es2_clk_ssi_find_idlest,
	.find_companion	= omap2_clk_dflt_find_companion,
};

/**
 * omap3430es2_clk_dss_usbhost_find_idlest - CM_IDLEST info for DSS, USBHOST
 * @clk: struct clk * being enabled
 * @idlest_reg: void __iomem ** to store CM_IDLEST reg address into
 * @idlest_bit: pointer to a u8 to store the CM_IDLEST bit shift into
 * @idlest_val: pointer to a u8 to store the CM_IDLEST indicator
 *
 * Some OMAP modules on OMAP3 ES2+ chips have both initiator and
 * target IDLEST bits.  For our purposes, we are concerned with the
 * target IDLEST bits, which exist at a different bit position than
 * the *CLKEN bit position for these modules (DSS and USBHOST) (The
 * default find_idlest code assumes that they are at the same
 * position.)  No return value.
 */
static void omap3430es2_clk_dss_usbhost_find_idlest(struct clk_hw_omap *clk,
						    void __iomem **idlest_reg,
						    u8 *idlest_bit,
						    u8 *idlest_val)
{
	u32 r;

	r = (((__force u32)clk->enable_reg & ~0xf0) | 0x20);
	*idlest_reg = (__force void __iomem *)r;
	/* USBHOST_IDLE has same shift */
	*idlest_bit = OMAP3430ES2_ST_DSS_IDLE_SHIFT;
	*idlest_val = OMAP34XX_CM_IDLEST_VAL;
}

const struct clk_hw_omap_ops clkhwops_omap3430es2_dss_usbhost_wait = {
	.find_idlest	= omap3430es2_clk_dss_usbhost_find_idlest,
	.find_companion	= omap2_clk_dflt_find_companion,
};

const struct clk_hw_omap_ops clkhwops_omap3430es2_iclk_dss_usbhost_wait = {
	.allow_idle	= omap2_clkt_iclk_allow_idle,
	.deny_idle	= omap2_clkt_iclk_deny_idle,
	.find_idlest	= omap3430es2_clk_dss_usbhost_find_idlest,
	.find_companion	= omap2_clk_dflt_find_companion,
};

/**
 * omap3430es2_clk_hsotgusb_find_idlest - return CM_IDLEST info for HSOTGUSB
 * @clk: struct clk * being enabled
 * @idlest_reg: void __iomem ** to store CM_IDLEST reg address into
 * @idlest_bit: pointer to a u8 to store the CM_IDLEST bit shift into
 * @idlest_val: pointer to a u8 to store the CM_IDLEST indicator
 *
 * The OMAP3430ES2 HSOTGUSB target CM_IDLEST bit is at a different
 * shift from the CM_{I,F}CLKEN bit.  Pass back the correct info via
 * @idlest_reg and @idlest_bit.  No return value.
 */
static void omap3430es2_clk_hsotgusb_find_idlest(struct clk_hw_omap *clk,
						 void __iomem **idlest_reg,
						 u8 *idlest_bit,
						 u8 *idlest_val)
{
	u32 r;

	r = (((__force u32)clk->enable_reg & ~0xf0) | 0x20);
	*idlest_reg = (__force void __iomem *)r;
	*idlest_bit = OMAP3430ES2_ST_HSOTGUSB_IDLE_SHIFT;
	*idlest_val = OMAP34XX_CM_IDLEST_VAL;
}

const struct clk_hw_omap_ops clkhwops_omap3430es2_iclk_hsotgusb_wait = {
	.allow_idle	= omap2_clkt_iclk_allow_idle,
	.deny_idle	= omap2_clkt_iclk_deny_idle,
	.find_idlest	= omap3430es2_clk_hsotgusb_find_idlest,
	.find_companion	= omap2_clk_dflt_find_companion,
};

/**
 * am35xx_clk_find_idlest - return clock ACK info for AM35XX IPSS
 * @clk: struct clk * being enabled
 * @idlest_reg: void __iomem ** to store CM_IDLEST reg address into
 * @idlest_bit: pointer to a u8 to store the CM_IDLEST bit shift into
 * @idlest_val: pointer to a u8 to store the CM_IDLEST indicator
 *
 * The interface clocks on AM35xx IPSS reflects the clock idle status
 * in the enable register itsel at a bit offset of 4 from the enable
 * bit. A value of 1 indicates that clock is enabled.
 */
static void am35xx_clk_find_idlest(struct clk_hw_omap *clk,
				   void __iomem **idlest_reg,
				   u8 *idlest_bit,
				   u8 *idlest_val)
{
	*idlest_reg = (__force void __iomem *)(clk->enable_reg);
	*idlest_bit = clk->enable_bit + AM35XX_IPSS_ICK_EN_ACK_OFFSET;
	*idlest_val = AM35XX_IPSS_CLK_IDLEST_VAL;
}

/**
 * am35xx_clk_find_companion - find companion clock to @clk
 * @clk: struct clk * to find the companion clock of
 * @other_reg: void __iomem ** to return the companion clock CM_*CLKEN va in
 * @other_bit: u8 ** to return the companion clock bit shift in
 *
 * Some clocks don't have companion clocks.  For example, modules with
 * only an interface clock (such as HECC) don't have a companion
 * clock.  Right now, this code relies on the hardware exporting a bit
 * in the correct companion register that indicates that the
 * nonexistent 'companion clock' is active.  Future patches will
 * associate this type of code with per-module data structures to
 * avoid this issue, and remove the casts.  No return value.
 */
static void am35xx_clk_find_companion(struct clk_hw_omap *clk,
				      void __iomem **other_reg,
				      u8 *other_bit)
{
	*other_reg = (__force void __iomem *)(clk->enable_reg);
	if (clk->enable_bit & AM35XX_IPSS_ICK_MASK)
		*other_bit = clk->enable_bit + AM35XX_IPSS_ICK_FCK_OFFSET;
	else
	*other_bit = clk->enable_bit - AM35XX_IPSS_ICK_FCK_OFFSET;
}

const struct clk_hw_omap_ops clkhwops_am35xx_ipss_module_wait = {
	.find_idlest	= am35xx_clk_find_idlest,
	.find_companion	= am35xx_clk_find_companion,
};

/**
 * am35xx_clk_ipss_find_idlest - return CM_IDLEST info for IPSS
 * @clk: struct clk * being enabled
 * @idlest_reg: void __iomem ** to store CM_IDLEST reg address into
 * @idlest_bit: pointer to a u8 to store the CM_IDLEST bit shift into
 * @idlest_val: pointer to a u8 to store the CM_IDLEST indicator
 *
 * The IPSS target CM_IDLEST bit is at a different shift from the
 * CM_{I,F}CLKEN bit.  Pass back the correct info via @idlest_reg
 * and @idlest_bit.  No return value.
 */
static void am35xx_clk_ipss_find_idlest(struct clk_hw_omap *clk,
					void __iomem **idlest_reg,
					u8 *idlest_bit,
					u8 *idlest_val)
{
	u32 r;

	r = (((__force u32)clk->enable_reg & ~0xf0) | 0x20);
	*idlest_reg = (__force void __iomem *)r;
	*idlest_bit = AM35XX_ST_IPSS_SHIFT;
	*idlest_val = OMAP34XX_CM_IDLEST_VAL;
}

const struct clk_hw_omap_ops clkhwops_am35xx_ipss_wait = {
	.allow_idle	= omap2_clkt_iclk_allow_idle,
	.deny_idle	= omap2_clkt_iclk_deny_idle,
	.find_idlest	= am35xx_clk_ipss_find_idlest,
	.find_companion	= omap2_clk_dflt_find_companion,
};

static struct ti_dt_clk omap3xxx_clks[] = {
	DT_CLK(NULL, "apb_pclk", "dummy_apb_pclk"),
	DT_CLK("twl", "fck", "osc_sys_ck"),
	DT_CLK("etb", "emu_core_alwon_ck", "emu_core_alwon_ck"),
	DT_CLK("etb", "emu_per_alwon_ck", "emu_per_alwon_ck"),
	DT_CLK("etb", "emu_mpu_alwon_ck", "emu_mpu_alwon_ck"),
	DT_CLK("omap_hdq.0", "fck", "hdq_fck"),
	DT_CLK("omap_hsmmc.1", "ick", "mmchs2_ick"),
	DT_CLK("omap_hsmmc.0", "ick", "mmchs1_ick"),
	DT_CLK("omap_hdq.0", "ick", "hdq_ick"),
	DT_CLK("omap2_mcspi.4", "ick", "mcspi4_ick"),
	DT_CLK("omap2_mcspi.3", "ick", "mcspi3_ick"),
	DT_CLK("omap2_mcspi.2", "ick", "mcspi2_ick"),
	DT_CLK("omap2_mcspi.1", "ick", "mcspi1_ick"),
	DT_CLK("omap_i2c.3", "ick", "i2c3_ick"),
	DT_CLK("omap_i2c.2", "ick", "i2c2_ick"),
	DT_CLK("omap_i2c.1", "ick", "i2c1_ick"),
	DT_CLK(NULL, "init_60m_fclk", "dummy_ck"),
	DT_CLK("omap_wdt", "ick", "wdt2_ick"),
	DT_CLK("etb", "emu_src_ck", "emu_src_ck"),
	DT_CLK(NULL, "timer_32k_ck", "omap_32k_fck"),
	DT_CLK(NULL, "timer_sys_ck", "sys_ck"),
	DT_CLK(NULL, "cpufreq_ck", "dpll1_ck"),
	{ .node_name = NULL },
};

static struct ti_dt_clk omap34xx_omap36xx_clks[] = {
	DT_CLK("omap_rng", "ick", "rng_ick"),
	DT_CLK("omap3-rom-rng", "ick", "rng_ick"),
	DT_CLK("omap-aes", "ick", "aes2_ick"),
	DT_CLK("omap-sham", "ick", "sha12_ick"),
	{ .node_name = NULL },
};

static struct ti_dt_clk omap36xx_omap3430es2plus_clks[] = {
	DT_CLK(NULL, "ssi_ssr_fck", "ssi_ssr_fck_3430es2"),
	DT_CLK(NULL, "ssi_sst_fck", "ssi_sst_fck_3430es2"),
	DT_CLK("musb-omap2430", "ick", "hsotgusb_ick_3430es2"),
	DT_CLK(NULL, "hsotgusb_ick", "hsotgusb_ick_3430es2"),
	DT_CLK(NULL, "ssi_ick", "ssi_ick_3430es2"),
	{ .node_name = NULL },
};

static struct ti_dt_clk omap3430es1_clks[] = {
	DT_CLK(NULL, "ssi_ssr_fck", "ssi_ssr_fck_3430es1"),
	DT_CLK(NULL, "ssi_sst_fck", "ssi_sst_fck_3430es1"),
	DT_CLK("musb-omap2430", "ick", "hsotgusb_ick_3430es1"),
	DT_CLK(NULL, "hsotgusb_ick", "hsotgusb_ick_3430es1"),
	DT_CLK(NULL, "ssi_ick", "ssi_ick_3430es1"),
	DT_CLK(NULL, "dss1_alwon_fck", "dss1_alwon_fck_3430es1"),
	DT_CLK("omapdss_dss", "ick", "dss_ick_3430es1"),
	DT_CLK(NULL, "dss_ick", "dss_ick_3430es1"),
	{ .node_name = NULL },
};

static struct ti_dt_clk omap36xx_am35xx_omap3430es2plus_clks[] = {
	DT_CLK("omap_hsmmc.2", "ick", "mmchs3_ick"),
	DT_CLK(NULL, "dss1_alwon_fck", "dss1_alwon_fck_3430es2"),
	DT_CLK("omapdss_dss", "ick", "dss_ick_3430es2"),
	DT_CLK(NULL, "dss_ick", "dss_ick_3430es2"),
	{ .node_name = NULL },
};

static struct ti_dt_clk am35xx_clks[] = {
	DT_CLK("davinci_emac.0", NULL, "emac_ick"),
	DT_CLK("davinci_mdio.0", NULL, "emac_fck"),
	DT_CLK("vpfe-capture", "master", "vpfe_ick"),
	DT_CLK("vpfe-capture", "slave", "vpfe_fck"),
	DT_CLK(NULL, "hsotgusb_ick", "hsotgusb_ick_am35xx"),
	DT_CLK(NULL, "hsotgusb_fck", "hsotgusb_fck_am35xx"),
	DT_CLK(NULL, "uart4_ick", "uart4_ick_am35xx"),
	DT_CLK(NULL, "uart4_fck", "uart4_fck_am35xx"),
	{ .node_name = NULL },
};

static struct ti_dt_clk omap36xx_clks[] = {
	{ .node_name = NULL },
};

static const char *enable_init_clks[] = {
	"sdrc_ick",
	"gpmc_fck",
	"omapctrl_ick",
};

enum {
	OMAP3_SOC_AM35XX,
	OMAP3_SOC_OMAP3430_ES1,
	OMAP3_SOC_OMAP3430_ES2_PLUS,
	OMAP3_SOC_OMAP3630,
};

/**
 * omap3_clk_lock_dpll5 - locks DPLL5
 *
 * Locks DPLL5 to a pre-defined frequency. This is required for proper
 * operation of USB.
 */
void __init omap3_clk_lock_dpll5(void)
{
	struct clk *dpll5_clk;
	struct clk *dpll5_m2_clk;

	dpll5_clk = ti_clk_get("dpll5_ck");
	clk_set_rate(dpll5_clk, DPLL5_FREQ_FOR_USBHOST);
	clk_prepare_enable(dpll5_clk);

	/* Program dpll5_m2_clk divider for no division */
	dpll5_m2_clk = ti_clk_get("dpll5_m2_ck");
	clk_prepare_enable(dpll5_m2_clk);
	clk_set_rate(dpll5_m2_clk, DPLL5_FREQ_FOR_USBHOST);

	clk_disable_unprepare(dpll5_m2_clk);
	clk_disable_unprepare(dpll5_clk);
}

static int __init omap3xxx_dt_clk_init(int soc_type)
{
	if (soc_type == OMAP3_SOC_AM35XX || soc_type == OMAP3_SOC_OMAP3630 ||
	    soc_type == OMAP3_SOC_OMAP3430_ES1 ||
	    soc_type == OMAP3_SOC_OMAP3430_ES2_PLUS)
		ti_dt_clocks_register(omap3xxx_clks);

	if (soc_type == OMAP3_SOC_AM35XX)
		ti_dt_clocks_register(am35xx_clks);

	if (soc_type == OMAP3_SOC_OMAP3630 || soc_type == OMAP3_SOC_AM35XX ||
	    soc_type == OMAP3_SOC_OMAP3430_ES2_PLUS)
		ti_dt_clocks_register(omap36xx_am35xx_omap3430es2plus_clks);

	if (soc_type == OMAP3_SOC_OMAP3430_ES1)
		ti_dt_clocks_register(omap3430es1_clks);

	if (soc_type == OMAP3_SOC_OMAP3430_ES2_PLUS ||
	    soc_type == OMAP3_SOC_OMAP3630)
		ti_dt_clocks_register(omap36xx_omap3430es2plus_clks);

	if (soc_type == OMAP3_SOC_OMAP3430_ES1 ||
	    soc_type == OMAP3_SOC_OMAP3430_ES2_PLUS ||
	    soc_type == OMAP3_SOC_OMAP3630)
		ti_dt_clocks_register(omap34xx_omap36xx_clks);

	if (soc_type == OMAP3_SOC_OMAP3630)
		ti_dt_clocks_register(omap36xx_clks);

	omap2_clk_disable_autoidle_all();

	omap2_clk_enable_init_clocks(enable_init_clks,
				     ARRAY_SIZE(enable_init_clks));

	pr_info("Clocking rate (Crystal/Core/MPU): %ld.%01ld/%ld/%ld MHz\n",
		(clk_get_rate(ti_clk_get("osc_sys_ck")) / 1000000),
		(clk_get_rate(ti_clk_get("osc_sys_ck")) / 100000) % 10,
		(clk_get_rate(ti_clk_get("core_ck")) / 1000000),
		(clk_get_rate(ti_clk_get("arm_fck")) / 1000000));

	if (soc_type != OMAP3_SOC_OMAP3430_ES1)
		omap3_clk_lock_dpll5();

	return 0;
}

int __init omap3430_dt_clk_init(void)
{
	return omap3xxx_dt_clk_init(OMAP3_SOC_OMAP3430_ES2_PLUS);
}

int __init omap3630_dt_clk_init(void)
{
	return omap3xxx_dt_clk_init(OMAP3_SOC_OMAP3630);
}

int __init am35xx_dt_clk_init(void)
{
	return omap3xxx_dt_clk_init(OMAP3_SOC_AM35XX);
}
