/*
 * DRA7 Clock init
 *
 * Copyright (C) 2013 Texas Instruments, Inc.
 *
 * Tero Kristo (t-kristo@ti.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/clk/ti.h>

#include "clock.h"

#define DRA7_DPLL_GMAC_DEFFREQ				1000000000
#define DRA7_DPLL_USB_DEFFREQ				960000000

static struct ti_dt_clk dra7xx_clks[] = {
	DT_CLK("omap_i2c.1", "ick", "dummy_ck"),
	DT_CLK("omap_i2c.2", "ick", "dummy_ck"),
	DT_CLK("omap_i2c.3", "ick", "dummy_ck"),
	DT_CLK("omap_i2c.4", "ick", "dummy_ck"),
	DT_CLK(NULL, "mailboxes_ick", "dummy_ck"),
	DT_CLK("omap_hsmmc.0", "ick", "dummy_ck"),
	DT_CLK("omap_hsmmc.1", "ick", "dummy_ck"),
	DT_CLK("omap_hsmmc.2", "ick", "dummy_ck"),
	DT_CLK("omap_hsmmc.3", "ick", "dummy_ck"),
	DT_CLK("omap_hsmmc.4", "ick", "dummy_ck"),
	DT_CLK("omap-mcbsp.1", "ick", "dummy_ck"),
	DT_CLK("omap-mcbsp.2", "ick", "dummy_ck"),
	DT_CLK("omap-mcbsp.3", "ick", "dummy_ck"),
	DT_CLK("omap-mcbsp.4", "ick", "dummy_ck"),
	DT_CLK("omap2_mcspi.1", "ick", "dummy_ck"),
	DT_CLK("omap2_mcspi.2", "ick", "dummy_ck"),
	DT_CLK("omap2_mcspi.3", "ick", "dummy_ck"),
	DT_CLK("omap2_mcspi.4", "ick", "dummy_ck"),
	DT_CLK(NULL, "uart1_ick", "dummy_ck"),
	DT_CLK(NULL, "uart2_ick", "dummy_ck"),
	DT_CLK(NULL, "uart3_ick", "dummy_ck"),
	DT_CLK(NULL, "uart4_ick", "dummy_ck"),
	DT_CLK("usbhs_omap", "usbhost_ick", "dummy_ck"),
	DT_CLK("usbhs_omap", "usbtll_fck", "dummy_ck"),
	DT_CLK("omap_wdt", "ick", "dummy_ck"),
	DT_CLK(NULL, "timer_32k_ck", "sys_32k_ck"),
	DT_CLK("4ae18000.timer", "timer_sys_ck", "timer_sys_clk_div"),
	DT_CLK("48032000.timer", "timer_sys_ck", "timer_sys_clk_div"),
	DT_CLK("48034000.timer", "timer_sys_ck", "timer_sys_clk_div"),
	DT_CLK("48036000.timer", "timer_sys_ck", "timer_sys_clk_div"),
	DT_CLK("4803e000.timer", "timer_sys_ck", "timer_sys_clk_div"),
	DT_CLK("48086000.timer", "timer_sys_ck", "timer_sys_clk_div"),
	DT_CLK("48088000.timer", "timer_sys_ck", "timer_sys_clk_div"),
	DT_CLK("48820000.timer", "timer_sys_ck", "timer_sys_clk_div"),
	DT_CLK("48822000.timer", "timer_sys_ck", "timer_sys_clk_div"),
	DT_CLK("48824000.timer", "timer_sys_ck", "timer_sys_clk_div"),
	DT_CLK("48826000.timer", "timer_sys_ck", "timer_sys_clk_div"),
	DT_CLK("48828000.timer", "timer_sys_ck", "timer_sys_clk_div"),
	DT_CLK("4882a000.timer", "timer_sys_ck", "timer_sys_clk_div"),
	DT_CLK("4882c000.timer", "timer_sys_ck", "timer_sys_clk_div"),
	DT_CLK("4882e000.timer", "timer_sys_ck", "timer_sys_clk_div"),
	DT_CLK(NULL, "sys_clkin", "sys_clkin1"),
	DT_CLK(NULL, "dss_deshdcp_clk", "dss_deshdcp_clk"),
	{ .node_name = NULL },
};

int __init dra7xx_dt_clk_init(void)
{
	int rc;
	struct clk *dpll_ck, *hdcp_ck;

	ti_dt_clocks_register(dra7xx_clks);

	omap2_clk_disable_autoidle_all();

	dpll_ck = ti_clk_get("dpll_gmac_ck");
	rc = clk_set_rate(dpll_ck, DRA7_DPLL_GMAC_DEFFREQ);
	if (rc)
		pr_err("%s: failed to configure GMAC DPLL!\n", __func__);

	dpll_ck = ti_clk_get("dpll_usb_ck");
	rc = clk_set_rate(dpll_ck, DRA7_DPLL_USB_DEFFREQ);
	if (rc)
		pr_err("%s: failed to configure USB DPLL!\n", __func__);

	dpll_ck = ti_clk_get("dpll_usb_m2_ck");
	rc = clk_set_rate(dpll_ck, DRA7_DPLL_USB_DEFFREQ/2);
	if (rc)
		pr_err("%s: failed to set USB_DPLL M2 OUT\n", __func__);

	hdcp_ck = ti_clk_get("dss_deshdcp_clk");
	rc = clk_prepare_enable(hdcp_ck);
	if (rc)
		pr_err("%s: failed to set dss_deshdcp_clk\n", __func__);

	return rc;
}
