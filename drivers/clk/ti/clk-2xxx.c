/*
 * OMAP2 Clock init
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
#include <linux/clk/ti.h>

#include "clock.h"

static struct ti_dt_clk omap2xxx_clks[] = {
	DT_CLK("omapdss_dss", "ick", "dss_ick"),
	DT_CLK(NULL, "ssi_fck", "ssi_ssr_sst_fck"),
	DT_CLK("omap-mcbsp.1", "ick", "mcbsp1_ick"),
	DT_CLK("omap-mcbsp.2", "ick", "mcbsp2_ick"),
	DT_CLK("omap2_mcspi.1", "ick", "mcspi1_ick"),
	DT_CLK("omap2_mcspi.2", "ick", "mcspi2_ick"),
	DT_CLK("omap_wdt", "ick", "mpu_wdt_ick"),
	DT_CLK("omap24xxcam", "fck", "cam_fck"),
	DT_CLK("omap24xxcam", "ick", "cam_ick"),
	DT_CLK("omap_hdq.0", "ick", "hdq_ick"),
	DT_CLK("omap_hdq.0", "fck", "hdq_fck"),
	DT_CLK("omap_i2c.1", "ick", "i2c1_ick"),
	DT_CLK("omap_i2c.2", "ick", "i2c2_ick"),
	DT_CLK("omap-sham", "ick", "sha_ick"),
	DT_CLK("omap_rng", "ick", "rng_ick"),
	DT_CLK("omap-aes", "ick", "aes_ick"),
	DT_CLK(NULL, "timer_32k_ck", "func_32k_ck"),
	DT_CLK(NULL, "timer_sys_ck", "sys_ck"),
	DT_CLK(NULL, "timer_ext_ck", "alt_ck"),
	{ .node_name = NULL },
};

static struct ti_dt_clk omap2420_clks[] = {
	DT_CLK("mmci-omap.0", "ick", "mmc_ick"),
	DT_CLK("mmci-omap.0", "fck", "mmc_fck"),
	DT_CLK("musb-hdrc", "fck", "osc_ck"),
	{ .node_name = NULL },
};

static struct ti_dt_clk omap2430_clks[] = {
	DT_CLK("twl", "fck", "osc_ck"),
	DT_CLK("omap-mcbsp.3", "ick", "mcbsp3_ick"),
	DT_CLK("omap-mcbsp.4", "ick", "mcbsp4_ick"),
	DT_CLK("omap-mcbsp.5", "ick", "mcbsp5_ick"),
	DT_CLK("omap2_mcspi.3", "ick", "mcspi3_ick"),
	DT_CLK("musb-omap2430", "ick", "usbhs_ick"),
	DT_CLK("omap_hsmmc.0", "ick", "mmchs1_ick"),
	DT_CLK("omap_hsmmc.1", "ick", "mmchs2_ick"),
	DT_CLK("omap_hsmmc.0", "mmchsdb_fck", "mmchsdb1_fck"),
	DT_CLK("omap_hsmmc.1", "mmchsdb_fck", "mmchsdb2_fck"),
	{ .node_name = NULL },
};

static const char *enable_init_clks[] = {
	"apll96_ck",
	"apll54_ck",
	"sync_32k_ick",
	"omapctrl_ick",
	"gpmc_fck",
	"sdrc_ick",
};

enum {
	OMAP2_SOC_OMAP2420,
	OMAP2_SOC_OMAP2430,
};

static int __init omap2xxx_dt_clk_init(int soc_type)
{
	ti_dt_clocks_register(omap2xxx_clks);

	if (soc_type == OMAP2_SOC_OMAP2420)
		ti_dt_clocks_register(omap2420_clks);
	else
		ti_dt_clocks_register(omap2430_clks);

	omap2xxx_clkt_vps_init();

	omap2_clk_disable_autoidle_all();

	omap2_clk_enable_init_clocks(enable_init_clks,
				     ARRAY_SIZE(enable_init_clks));

	pr_info("Clocking rate (Crystal/DPLL/MPU): %ld.%01ld/%ld/%ld MHz\n",
		(clk_get_rate(ti_clk_get("sys_ck")) / 1000000),
		(clk_get_rate(ti_clk_get("sys_ck")) / 100000) % 10,
		(clk_get_rate(ti_clk_get("dpll_ck")) / 1000000),
		(clk_get_rate(ti_clk_get("mpu_ck")) / 1000000));

	return 0;
}

int __init omap2420_dt_clk_init(void)
{
	return omap2xxx_dt_clk_init(OMAP2_SOC_OMAP2420);
}

int __init omap2430_dt_clk_init(void)
{
	return omap2xxx_dt_clk_init(OMAP2_SOC_OMAP2430);
}
