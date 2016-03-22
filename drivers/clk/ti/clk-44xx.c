/*
 * OMAP4 Clock init
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
#include <dt-bindings/clock/ti,omap44xx.h>

#include "clock.h"

/*
 * OMAP4 ABE DPLL default frequency. In OMAP4460 TRM version V, section
 * "3.6.3.2.3 CM1_ABE Clock Generator" states that the "DPLL_ABE_X2_CLK
 * must be set to 196.608 MHz" and hence, the DPLL locked frequency is
 * half of this value.
 */
#define OMAP4_DPLL_ABE_DEFFREQ				98304000

/*
 * OMAP4 USB DPLL default frequency. In OMAP4430 TRM version V, section
 * "3.6.3.9.5 DPLL_USB Preferred Settings" shows that the preferred
 * locked frequency for the USB DPLL is 960MHz.
 */
#define OMAP4_DPLL_USB_DEFFREQ				960000000

static struct ti_clk_hwmod mpu_mod_ck_data = {
	.reg = OMAP44XX_MPUSS_MPU,
	.parent = "dpll_mpu_m2_ck",
};

static struct ti_clk mpu_mod_ck = {
	.name = "mpu_mod_ck",
	.clkdm_name = "mpuss_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &mpu_mod_ck_data,
};

static struct ti_clk_hwmod dsp_mod_ck_data = {
	.reg = OMAP44XX_TESLA_DSP,
	.parent = "dpll_iva_m4x2_ck",
	.flags = CLKF_HW_SUP,
};

static struct ti_clk dsp_mod_ck = {
	.name = "dsp_mod_ck",
	.clkdm_name = "tesla_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &dsp_mod_ck_data,
};

static struct ti_clk_hwmod mmu_dsp_mod_ck_data = {
	.reg = OMAP44XX_TESLA_MMU_DSP,
	.parent = "dpll_iva_m4x2_ck",
	.flags = CLKF_HW_SUP,
};

static struct ti_clk mmu_dsp_mod_ck = {
	.name = "mmu_dsp_mod_ck",
	.clkdm_name = "tesla_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &mmu_dsp_mod_ck_data,
};

static struct ti_clk_hwmod l4_abe_mod_ck_data = {
	.reg = OMAP44XX_ABE_L4_ABE,
	.parent = "ocp_abe_iclk",
};

static struct ti_clk l4_abe_mod_ck = {
	.name = "l4_abe_mod_ck",
	.clkdm_name = "abe_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &l4_abe_mod_ck_data,
};

static struct ti_clk_hwmod aess_mod_ck_data = {
	.reg = OMAP44XX_ABE_AESS,
	.parent = "aess_fclk",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk aess_mod_ck = {
	.name = "aess_mod_ck",
	.clkdm_name = "abe_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &aess_mod_ck_data,
};

static struct ti_clk_hwmod mcpdm_mod_ck_data = {
	.reg = OMAP44XX_ABE_MCPDM,
	.parent = "pad_clks_ck",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk mcpdm_mod_ck = {
	.name = "mcpdm_mod_ck",
	.clkdm_name = "abe_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &mcpdm_mod_ck_data,
};

static struct ti_clk_hwmod dmic_mod_ck_data = {
	.reg = OMAP44XX_ABE_DMIC,
	.parent = "func_dmic_abe_gfclk",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk dmic_mod_ck = {
	.name = "dmic_mod_ck",
	.clkdm_name = "abe_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &dmic_mod_ck_data,
};

static struct ti_clk_hwmod mcasp_mod_ck_data = {
	.reg = OMAP44XX_ABE_MCASP,
	.parent = "func_mcasp_abe_gfclk",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk mcasp_mod_ck = {
	.name = "mcasp_mod_ck",
	.clkdm_name = "abe_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &mcasp_mod_ck_data,
};

static struct ti_clk_hwmod mcbsp1_mod_ck_data = {
	.reg = OMAP44XX_ABE_MCBSP1,
	.parent = "func_mcbsp1_gfclk",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk mcbsp1_mod_ck = {
	.name = "mcbsp1_mod_ck",
	.clkdm_name = "abe_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &mcbsp1_mod_ck_data,
};

static struct ti_clk_hwmod mcbsp2_mod_ck_data = {
	.reg = OMAP44XX_ABE_MCBSP2,
	.parent = "func_mcbsp2_gfclk",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk mcbsp2_mod_ck = {
	.name = "mcbsp2_mod_ck",
	.clkdm_name = "abe_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &mcbsp2_mod_ck_data,
};

static struct ti_clk_hwmod mcbsp3_mod_ck_data = {
	.reg = OMAP44XX_ABE_MCBSP3,
	.parent = "func_mcbsp3_gfclk",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk mcbsp3_mod_ck = {
	.name = "mcbsp3_mod_ck",
	.clkdm_name = "abe_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &mcbsp3_mod_ck_data,
};

static struct ti_clk_hwmod slimbus1_mod_ck_data = {
	.reg = OMAP44XX_ABE_SLIMBUS1,
	.parent = "slimbus1_fclk_0",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk slimbus1_mod_ck = {
	.name = "slimbus1_mod_ck",
	.clkdm_name = "abe_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &slimbus1_mod_ck_data,
};

static const char * const timer5_mod_ck_parents[] = {
	"syc_clk_div_ck",
	"sys_32k_ck",
};

static struct ti_clk_hwmod_mux timer5_mod_ck_data = {
	.mod_reg = OMAP44XX_ABE_TIMER5,
	.mux_reg = OMAP44XX_ABE_TIMER5,
	.shift = 24,
	.parents = timer5_mod_ck_parents,
	.num_parents = ARRAY_SIZE(timer5_mod_ck_parents),
	.flags = CLKF_SW_SUP,
};

static struct ti_clk timer5_mod_ck = {
	.name = "timer5_mod_ck",
	.clkdm_name = "abe_clkdm",
	.type = TI_CLK_HWMOD_MUX,
	.data = &timer5_mod_ck_data,
};

static const char * const timer6_mod_ck_parents[] = {
	"syc_clk_div_ck",
	"sys_32k_ck",
};

static struct ti_clk_hwmod_mux timer6_mod_ck_data = {
	.mod_reg = OMAP44XX_ABE_TIMER6,
	.mux_reg = OMAP44XX_ABE_TIMER6,
	.shift = 24,
	.parents = timer6_mod_ck_parents,
	.num_parents = ARRAY_SIZE(timer6_mod_ck_parents),
	.flags = CLKF_SW_SUP,
};

static struct ti_clk timer6_mod_ck = {
	.name = "timer6_mod_ck",
	.clkdm_name = "abe_clkdm",
	.type = TI_CLK_HWMOD_MUX,
	.data = &timer6_mod_ck_data,
};

static const char * const timer7_mod_ck_parents[] = {
	"syc_clk_div_ck",
	"sys_32k_ck",
};

static struct ti_clk_hwmod_mux timer7_mod_ck_data = {
	.mod_reg = OMAP44XX_ABE_TIMER7,
	.mux_reg = OMAP44XX_ABE_TIMER7,
	.shift = 24,
	.parents = timer7_mod_ck_parents,
	.num_parents = ARRAY_SIZE(timer7_mod_ck_parents),
	.flags = CLKF_SW_SUP,
};

static struct ti_clk timer7_mod_ck = {
	.name = "timer7_mod_ck",
	.clkdm_name = "abe_clkdm",
	.type = TI_CLK_HWMOD_MUX,
	.data = &timer7_mod_ck_data,
};

static const char * const timer8_mod_ck_parents[] = {
	"syc_clk_div_ck",
	"sys_32k_ck",
};

static struct ti_clk_hwmod_mux timer8_mod_ck_data = {
	.mod_reg = OMAP44XX_ABE_TIMER8,
	.mux_reg = OMAP44XX_ABE_TIMER8,
	.shift = 24,
	.parents = timer8_mod_ck_parents,
	.num_parents = ARRAY_SIZE(timer8_mod_ck_parents),
	.flags = CLKF_SW_SUP,
};

static struct ti_clk timer8_mod_ck = {
	.name = "timer8_mod_ck",
	.clkdm_name = "abe_clkdm",
	.type = TI_CLK_HWMOD_MUX,
	.data = &timer8_mod_ck_data,
};

static struct ti_clk_hwmod wd_timer3_mod_ck_data = {
	.reg = OMAP44XX_ABE_WD_TIMER3,
	.parent = "sys_32k_ck",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk wd_timer3_mod_ck = {
	.name = "wd_timer3_mod_ck",
	.clkdm_name = "abe_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &wd_timer3_mod_ck_data,
};

static struct ti_clk_hwmod l4_wkup_mod_ck_data = {
	.reg = OMAP44XX_L4_WKUP_L4_WKUP,
	.parent = "l4_wkup_clk_mux_ck",
};

static struct ti_clk l4_wkup_mod_ck = {
	.name = "l4_wkup_mod_ck",
	.clkdm_name = "l4_wkup_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &l4_wkup_mod_ck_data,
};

static struct ti_clk_hwmod wd_timer2_mod_ck_data = {
	.reg = OMAP44XX_L4_WKUP_WD_TIMER2,
	.parent = "sys_32k_ck",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk wd_timer2_mod_ck = {
	.name = "wd_timer2_mod_ck",
	.clkdm_name = "l4_wkup_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &wd_timer2_mod_ck_data,
};

static struct ti_clk_hwmod gpio1_mod_ck_data = {
	.reg = OMAP44XX_L4_WKUP_GPIO1,
	.parent = "l4_wkup_clk_mux_ck",
	.flags = CLKF_HW_SUP,
};

static struct ti_clk gpio1_mod_ck = {
	.name = "gpio1_mod_ck",
	.clkdm_name = "l4_wkup_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &gpio1_mod_ck_data,
};

static const char * const timer1_mod_ck_parents[] = {
	"sys_clkin_ck",
	"sys_32k_ck",
};

static struct ti_clk_hwmod_mux timer1_mod_ck_data = {
	.mod_reg = OMAP44XX_L4_WKUP_TIMER1,
	.mux_reg = OMAP44XX_L4_WKUP_TIMER1,
	.shift = 24,
	.parents = timer1_mod_ck_parents,
	.num_parents = ARRAY_SIZE(timer1_mod_ck_parents),
	.flags = CLKF_SW_SUP,
};

static struct ti_clk timer1_mod_ck = {
	.name = "timer1_mod_ck",
	.clkdm_name = "l4_wkup_clkdm",
	.type = TI_CLK_HWMOD_MUX,
	.data = &timer1_mod_ck_data,
};

static struct ti_clk_hwmod counter_32k_mod_ck_data = {
	.reg = OMAP44XX_L4_WKUP_COUNTER_32K,
	.parent = "sys_32k_ck",
};

static struct ti_clk counter_32k_mod_ck = {
	.name = "counter_32k_mod_ck",
	.clkdm_name = "l4_wkup_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &counter_32k_mod_ck_data,
};

static struct ti_clk_hwmod kbd_mod_ck_data = {
	.reg = OMAP44XX_L4_WKUP_KBD,
	.parent = "sys_32k_ck",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk kbd_mod_ck = {
	.name = "kbd_mod_ck",
	.clkdm_name = "l4_wkup_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &kbd_mod_ck_data,
};

static struct ti_clk_hwmod debugss_mod_ck_data = {
	.reg = OMAP44XX_EMU_SYS_DEBUGSS,
	.parent = "trace_clk_div_ck",
};

static struct ti_clk debugss_mod_ck = {
	.name = "debugss_mod_ck",
	.clkdm_name = "emu_sys_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &debugss_mod_ck_data,
};

static struct ti_clk_hwmod dss_core_mod_ck_data = {
	.reg = OMAP44XX_L3_DSS_DSS_CORE,
	.parent = "dss_dss_clk",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk dss_core_mod_ck = {
	.name = "dss_core_mod_ck",
	.clkdm_name = "l3_dss_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &dss_core_mod_ck_data,
};

static struct ti_clk_hwmod dss_venc_mod_ck_data = {
	.reg = OMAP44XX_L3_DSS_DSS_VENC,
	.parent = "dss_tv_clk",
};

static struct ti_clk dss_venc_mod_ck = {
	.name = "dss_venc_mod_ck",
	.clkdm_name = "l3_dss_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &dss_venc_mod_ck_data,
};

static struct ti_clk_hwmod dss_dispc_mod_ck_data = {
	.reg = OMAP44XX_L3_DSS_DSS_DISPC,
	.parent = "dss_dss_clk",
};

static struct ti_clk dss_dispc_mod_ck = {
	.name = "dss_dispc_mod_ck",
	.clkdm_name = "l3_dss_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &dss_dispc_mod_ck_data,
};

static struct ti_clk_hwmod dss_dsi1_mod_ck_data = {
	.reg = OMAP44XX_L3_DSS_DSS_DSI1,
	.parent = "dss_dss_clk",
};

static struct ti_clk dss_dsi1_mod_ck = {
	.name = "dss_dsi1_mod_ck",
	.clkdm_name = "l3_dss_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &dss_dsi1_mod_ck_data,
};

static struct ti_clk_hwmod dss_rfbi_mod_ck_data = {
	.reg = OMAP44XX_L3_DSS_DSS_RFBI,
	.parent = "dss_dss_clk",
};

static struct ti_clk dss_rfbi_mod_ck = {
	.name = "dss_rfbi_mod_ck",
	.clkdm_name = "l3_dss_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &dss_rfbi_mod_ck_data,
};

static struct ti_clk_hwmod dss_dsi2_mod_ck_data = {
	.reg = OMAP44XX_L3_DSS_DSS_DSI2,
	.parent = "dss_dss_clk",
};

static struct ti_clk dss_dsi2_mod_ck = {
	.name = "dss_dsi2_mod_ck",
	.clkdm_name = "l3_dss_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &dss_dsi2_mod_ck_data,
};

static struct ti_clk_hwmod dss_hdmi_mod_ck_data = {
	.reg = OMAP44XX_L3_DSS_DSS_HDMI,
	.parent = "dss_48mhz_clk",
};

static struct ti_clk dss_hdmi_mod_ck = {
	.name = "dss_hdmi_mod_ck",
	.clkdm_name = "l3_dss_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &dss_hdmi_mod_ck_data,
};

static struct ti_clk_hwmod fdif_mod_ck_data = {
	.reg = OMAP44XX_ISS_FDIF,
	.parent = "fdif_fck",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk fdif_mod_ck = {
	.name = "fdif_mod_ck",
	.clkdm_name = "iss_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &fdif_mod_ck_data,
};

static struct ti_clk_hwmod gpio2_mod_ck_data = {
	.reg = OMAP44XX_L4_PER_GPIO2,
	.parent = "l4_div_ck",
	.flags = CLKF_HW_SUP,
};

static struct ti_clk gpio2_mod_ck = {
	.name = "gpio2_mod_ck",
	.clkdm_name = "l4_per_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &gpio2_mod_ck_data,
};

static struct ti_clk_hwmod gpio3_mod_ck_data = {
	.reg = OMAP44XX_L4_PER_GPIO3,
	.parent = "l4_div_ck",
	.flags = CLKF_HW_SUP,
};

static struct ti_clk gpio3_mod_ck = {
	.name = "gpio3_mod_ck",
	.clkdm_name = "l4_per_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &gpio3_mod_ck_data,
};

static struct ti_clk_hwmod gpio4_mod_ck_data = {
	.reg = OMAP44XX_L4_PER_GPIO4,
	.parent = "l4_div_ck",
	.flags = CLKF_HW_SUP,
};

static struct ti_clk gpio4_mod_ck = {
	.name = "gpio4_mod_ck",
	.clkdm_name = "l4_per_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &gpio4_mod_ck_data,
};

static struct ti_clk_hwmod gpio5_mod_ck_data = {
	.reg = OMAP44XX_L4_PER_GPIO5,
	.parent = "l4_div_ck",
	.flags = CLKF_HW_SUP,
};

static struct ti_clk gpio5_mod_ck = {
	.name = "gpio5_mod_ck",
	.clkdm_name = "l4_per_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &gpio5_mod_ck_data,
};

static struct ti_clk_hwmod gpio6_mod_ck_data = {
	.reg = OMAP44XX_L4_PER_GPIO6,
	.parent = "l4_div_ck",
	.flags = CLKF_HW_SUP,
};

static struct ti_clk gpio6_mod_ck = {
	.name = "gpio6_mod_ck",
	.clkdm_name = "l4_per_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &gpio6_mod_ck_data,
};

static struct ti_clk_hwmod hdq1w_mod_ck_data = {
	.reg = OMAP44XX_L4_PER_HDQ1W,
	.parent = "func_12m_fclk",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk hdq1w_mod_ck = {
	.name = "hdq1w_mod_ck",
	.clkdm_name = "l4_per_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &hdq1w_mod_ck_data,
};

static struct ti_clk_hwmod i2c1_mod_ck_data = {
	.reg = OMAP44XX_L4_PER_I2C1,
	.parent = "func_96m_fclk",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk i2c1_mod_ck = {
	.name = "i2c1_mod_ck",
	.clkdm_name = "l4_per_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &i2c1_mod_ck_data,
};

static struct ti_clk_hwmod i2c2_mod_ck_data = {
	.reg = OMAP44XX_L4_PER_I2C2,
	.parent = "func_96m_fclk",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk i2c2_mod_ck = {
	.name = "i2c2_mod_ck",
	.clkdm_name = "l4_per_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &i2c2_mod_ck_data,
};

static struct ti_clk_hwmod i2c3_mod_ck_data = {
	.reg = OMAP44XX_L4_PER_I2C3,
	.parent = "func_96m_fclk",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk i2c3_mod_ck = {
	.name = "i2c3_mod_ck",
	.clkdm_name = "l4_per_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &i2c3_mod_ck_data,
};

static struct ti_clk_hwmod i2c4_mod_ck_data = {
	.reg = OMAP44XX_L4_PER_I2C4,
	.parent = "func_96m_fclk",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk i2c4_mod_ck = {
	.name = "i2c4_mod_ck",
	.clkdm_name = "l4_per_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &i2c4_mod_ck_data,
};

static struct ti_clk_hwmod l4_per_mod_ck_data = {
	.reg = OMAP44XX_L4_PER_L4_PER,
	.parent = "l4_div_ck",
};

static struct ti_clk l4_per_mod_ck = {
	.name = "l4_per_mod_ck",
	.clkdm_name = "l4_per_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &l4_per_mod_ck_data,
};

static struct ti_clk_hwmod gpu_mod_ck_data = {
	.reg = OMAP44XX_L3_GFX_GPU,
	.parent = "sgx_clk_mux",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk gpu_mod_ck = {
	.name = "gpu_mod_ck",
	.clkdm_name = "l3_gfx_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &gpu_mod_ck_data,
};

static struct ti_clk_hwmod hsi_mod_ck_data = {
	.reg = OMAP44XX_L3_INIT_HSI,
	.parent = "hsi_fck",
	.flags = CLKF_HW_SUP,
};

static struct ti_clk hsi_mod_ck = {
	.name = "hsi_mod_ck",
	.clkdm_name = "l3_init_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &hsi_mod_ck_data,
};

static struct ti_clk_hwmod iss_mod_ck_data = {
	.reg = OMAP44XX_ISS_ISS,
	.parent = "ducati_clk_mux_ck",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk iss_mod_ck = {
	.name = "iss_mod_ck",
	.clkdm_name = "iss_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &iss_mod_ck_data,
};

static struct ti_clk_hwmod mcbsp4_mod_ck_data = {
	.reg = OMAP44XX_L4_PER_MCBSP4,
	.parent = "per_mcbsp4_gfclk",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk mcbsp4_mod_ck = {
	.name = "mcbsp4_mod_ck",
	.clkdm_name = "l4_per_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &mcbsp4_mod_ck_data,
};

static struct ti_clk_hwmod mcspi1_mod_ck_data = {
	.reg = OMAP44XX_L4_PER_MCSPI1,
	.parent = "func_48m_fclk",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk mcspi1_mod_ck = {
	.name = "mcspi1_mod_ck",
	.clkdm_name = "l4_per_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &mcspi1_mod_ck_data,
};

static struct ti_clk_hwmod mcspi2_mod_ck_data = {
	.reg = OMAP44XX_L4_PER_MCSPI2,
	.parent = "func_48m_fclk",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk mcspi2_mod_ck = {
	.name = "mcspi2_mod_ck",
	.clkdm_name = "l4_per_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &mcspi2_mod_ck_data,
};

static struct ti_clk_hwmod mcspi3_mod_ck_data = {
	.reg = OMAP44XX_L4_PER_MCSPI3,
	.parent = "func_48m_fclk",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk mcspi3_mod_ck = {
	.name = "mcspi3_mod_ck",
	.clkdm_name = "l4_per_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &mcspi3_mod_ck_data,
};

static struct ti_clk_hwmod mcspi4_mod_ck_data = {
	.reg = OMAP44XX_L4_PER_MCSPI4,
	.parent = "func_48m_fclk",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk mcspi4_mod_ck = {
	.name = "mcspi4_mod_ck",
	.clkdm_name = "l4_per_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &mcspi4_mod_ck_data,
};

static struct ti_clk_hwmod mmc3_mod_ck_data = {
	.reg = OMAP44XX_L4_PER_MMC3,
	.parent = "func_48m_fclk",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk mmc3_mod_ck = {
	.name = "mmc3_mod_ck",
	.clkdm_name = "l4_per_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &mmc3_mod_ck_data,
};

static struct ti_clk_hwmod mmc4_mod_ck_data = {
	.reg = OMAP44XX_L4_PER_MMC4,
	.parent = "func_48m_fclk",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk mmc4_mod_ck = {
	.name = "mmc4_mod_ck",
	.clkdm_name = "l4_per_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &mmc4_mod_ck_data,
};

static struct ti_clk_hwmod mmc1_mod_ck_data = {
	.reg = OMAP44XX_L3_INIT_MMC1,
	.parent = "hsmmc1_fclk",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk mmc1_mod_ck = {
	.name = "mmc1_mod_ck",
	.clkdm_name = "l3_init_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &mmc1_mod_ck_data,
};

static struct ti_clk_hwmod mmc2_mod_ck_data = {
	.reg = OMAP44XX_L3_INIT_MMC2,
	.parent = "hsmmc2_fclk",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk mmc2_mod_ck = {
	.name = "mmc2_mod_ck",
	.clkdm_name = "l3_init_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &mmc2_mod_ck_data,
};

static struct ti_clk_hwmod ocp2scp_usb_phy_mod_ck_data = {
	.reg = OMAP44XX_L3_INIT_OCP2SCP_USB_PHY,
	.parent = "ocp2scp_usb_phy_phy_48m",
	.flags = CLKF_HW_SUP,
};

static struct ti_clk ocp2scp_usb_phy_mod_ck = {
	.name = "ocp2scp_usb_phy_mod_ck",
	.clkdm_name = "l3_init_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &ocp2scp_usb_phy_mod_ck_data,
};

static const char * const timer10_mod_ck_parents[] = {
	"sys_clkin_ck",
	"sys_32k_ck",
};

static struct ti_clk_hwmod_mux timer10_mod_ck_data = {
	.mod_reg = OMAP44XX_L4_PER_TIMER10,
	.mux_reg = OMAP44XX_L4_PER_TIMER10,
	.shift = 24,
	.parents = timer10_mod_ck_parents,
	.num_parents = ARRAY_SIZE(timer10_mod_ck_parents),
	.flags = CLKF_SW_SUP,
};

static struct ti_clk timer10_mod_ck = {
	.name = "timer10_mod_ck",
	.clkdm_name = "l4_per_clkdm",
	.type = TI_CLK_HWMOD_MUX,
	.data = &timer10_mod_ck_data,
};

static const char * const timer11_mod_ck_parents[] = {
	"sys_clkin_ck",
	"sys_32k_ck",
};

static struct ti_clk_hwmod_mux timer11_mod_ck_data = {
	.mod_reg = OMAP44XX_L4_PER_TIMER11,
	.mux_reg = OMAP44XX_L4_PER_TIMER11,
	.shift = 24,
	.parents = timer11_mod_ck_parents,
	.num_parents = ARRAY_SIZE(timer11_mod_ck_parents),
	.flags = CLKF_SW_SUP,
};

static struct ti_clk timer11_mod_ck = {
	.name = "timer11_mod_ck",
	.clkdm_name = "l4_per_clkdm",
	.type = TI_CLK_HWMOD_MUX,
	.data = &timer11_mod_ck_data,
};

static const char * const timer2_mod_ck_parents[] = {
	"sys_clkin_ck",
	"sys_32k_ck",
};

static struct ti_clk_hwmod_mux timer2_mod_ck_data = {
	.mod_reg = OMAP44XX_L4_PER_TIMER2,
	.mux_reg = OMAP44XX_L4_PER_TIMER2,
	.shift = 24,
	.parents = timer2_mod_ck_parents,
	.num_parents = ARRAY_SIZE(timer2_mod_ck_parents),
	.flags = CLKF_SW_SUP,
};

static struct ti_clk timer2_mod_ck = {
	.name = "timer2_mod_ck",
	.clkdm_name = "l4_per_clkdm",
	.type = TI_CLK_HWMOD_MUX,
	.data = &timer2_mod_ck_data,
};

static const char * const timer3_mod_ck_parents[] = {
	"sys_clkin_ck",
	"sys_32k_ck",
};

static struct ti_clk_hwmod_mux timer3_mod_ck_data = {
	.mod_reg = OMAP44XX_L4_PER_TIMER3,
	.mux_reg = OMAP44XX_L4_PER_TIMER3,
	.shift = 24,
	.parents = timer3_mod_ck_parents,
	.num_parents = ARRAY_SIZE(timer3_mod_ck_parents),
	.flags = CLKF_SW_SUP,
};

static struct ti_clk timer3_mod_ck = {
	.name = "timer3_mod_ck",
	.clkdm_name = "l4_per_clkdm",
	.type = TI_CLK_HWMOD_MUX,
	.data = &timer3_mod_ck_data,
};

static const char * const timer4_mod_ck_parents[] = {
	"sys_clkin_ck",
	"sys_32k_ck",
};

static struct ti_clk_hwmod_mux timer4_mod_ck_data = {
	.mod_reg = OMAP44XX_L4_PER_TIMER4,
	.mux_reg = OMAP44XX_L4_PER_TIMER4,
	.shift = 24,
	.parents = timer4_mod_ck_parents,
	.num_parents = ARRAY_SIZE(timer4_mod_ck_parents),
	.flags = CLKF_SW_SUP,
};

static struct ti_clk timer4_mod_ck = {
	.name = "timer4_mod_ck",
	.clkdm_name = "l4_per_clkdm",
	.type = TI_CLK_HWMOD_MUX,
	.data = &timer4_mod_ck_data,
};

static const char * const timer9_mod_ck_parents[] = {
	"sys_clkin_ck",
	"sys_32k_ck",
};

static struct ti_clk_hwmod_mux timer9_mod_ck_data = {
	.mod_reg = OMAP44XX_L4_PER_TIMER9,
	.mux_reg = OMAP44XX_L4_PER_TIMER9,
	.shift = 24,
	.parents = timer9_mod_ck_parents,
	.num_parents = ARRAY_SIZE(timer9_mod_ck_parents),
	.flags = CLKF_SW_SUP,
};

static struct ti_clk timer9_mod_ck = {
	.name = "timer9_mod_ck",
	.clkdm_name = "l4_per_clkdm",
	.type = TI_CLK_HWMOD_MUX,
	.data = &timer9_mod_ck_data,
};

static struct ti_clk_hwmod elm_mod_ck_data = {
	.reg = OMAP44XX_L4_PER_ELM,
	.parent = "l4_div_ck",
};

static struct ti_clk elm_mod_ck = {
	.name = "elm_mod_ck",
	.clkdm_name = "l4_per_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &elm_mod_ck_data,
};

static struct ti_clk_hwmod slimbus2_mod_ck_data = {
	.reg = OMAP44XX_L4_PER_SLIMBUS2,
	.parent = "slimbus2_fclk_0",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk slimbus2_mod_ck = {
	.name = "slimbus2_mod_ck",
	.clkdm_name = "l4_per_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &slimbus2_mod_ck_data,
};

static struct ti_clk_hwmod uart1_mod_ck_data = {
	.reg = OMAP44XX_L4_PER_UART1,
	.parent = "func_48m_fclk",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk uart1_mod_ck = {
	.name = "uart1_mod_ck",
	.clkdm_name = "l4_per_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &uart1_mod_ck_data,
};

static struct ti_clk_hwmod uart2_mod_ck_data = {
	.reg = OMAP44XX_L4_PER_UART2,
	.parent = "func_48m_fclk",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk uart2_mod_ck = {
	.name = "uart2_mod_ck",
	.clkdm_name = "l4_per_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &uart2_mod_ck_data,
};

static struct ti_clk_hwmod uart3_mod_ck_data = {
	.reg = OMAP44XX_L4_PER_UART3,
	.parent = "func_48m_fclk",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk uart3_mod_ck = {
	.name = "uart3_mod_ck",
	.clkdm_name = "l4_per_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &uart3_mod_ck_data,
};

static struct ti_clk_hwmod uart4_mod_ck_data = {
	.reg = OMAP44XX_L4_PER_UART4,
	.parent = "func_48m_fclk",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk uart4_mod_ck = {
	.name = "uart4_mod_ck",
	.clkdm_name = "l4_per_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &uart4_mod_ck_data,
};

static struct ti_clk_hwmod mmc5_mod_ck_data = {
	.reg = OMAP44XX_L4_PER_MMC5,
	.parent = "func_48m_fclk",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk mmc5_mod_ck = {
	.name = "mmc5_mod_ck",
	.clkdm_name = "l4_per_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &mmc5_mod_ck_data,
};

static struct ti_clk_hwmod smartreflex_core_mod_ck_data = {
	.reg = OMAP44XX_L4_AO_SMARTREFLEX_CORE,
	.parent = "smartreflex_core_fck",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk smartreflex_core_mod_ck = {
	.name = "smartreflex_core_mod_ck",
	.clkdm_name = "l4_ao_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &smartreflex_core_mod_ck_data,
};

static struct ti_clk_hwmod smartreflex_iva_mod_ck_data = {
	.reg = OMAP44XX_L4_AO_SMARTREFLEX_IVA,
	.parent = "smartreflex_iva_fck",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk smartreflex_iva_mod_ck = {
	.name = "smartreflex_iva_mod_ck",
	.clkdm_name = "l4_ao_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &smartreflex_iva_mod_ck_data,
};

static struct ti_clk_hwmod smartreflex_mpu_mod_ck_data = {
	.reg = OMAP44XX_L4_AO_SMARTREFLEX_MPU,
	.parent = "smartreflex_mpu_fck",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk smartreflex_mpu_mod_ck = {
	.name = "smartreflex_mpu_mod_ck",
	.clkdm_name = "l4_ao_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &smartreflex_mpu_mod_ck_data,
};

static struct ti_clk_hwmod usb_host_fs_mod_ck_data = {
	.reg = OMAP44XX_L3_INIT_USB_HOST_FS,
	.parent = "usb_host_fs_fck",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk usb_host_fs_mod_ck = {
	.name = "usb_host_fs_mod_ck",
	.clkdm_name = "l3_init_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &usb_host_fs_mod_ck_data,
};

static struct ti_clk_hwmod usb_host_hs_mod_ck_data = {
	.reg = OMAP44XX_L3_INIT_USB_HOST_HS,
	.parent = "usb_host_hs_fck",
	.flags = CLKF_SW_SUP,
};

static struct ti_clk usb_host_hs_mod_ck = {
	.name = "usb_host_hs_mod_ck",
	.clkdm_name = "l3_init_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &usb_host_hs_mod_ck_data,
};

static struct ti_clk_hwmod usb_otg_hs_mod_ck_data = {
	.reg = OMAP44XX_L3_INIT_USB_OTG_HS,
	.parent = "usb_otg_hs_ick",
	.flags = CLKF_HW_SUP,
};

static struct ti_clk usb_otg_hs_mod_ck = {
	.name = "usb_otg_hs_mod_ck",
	.clkdm_name = "l3_init_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &usb_otg_hs_mod_ck_data,
};

static struct ti_clk_hwmod l3_main_1_mod_ck_data = {
	.reg = OMAP44XX_L3_1_L3_MAIN_1,
	.parent = "l3_div_ck",
};

static struct ti_clk l3_main_1_mod_ck = {
	.name = "l3_main_1_mod_ck",
	.clkdm_name = "l3_1_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &l3_main_1_mod_ck_data,
};

static struct ti_clk_hwmod l3_main_2_mod_ck_data = {
	.reg = OMAP44XX_L3_2_L3_MAIN_2,
	.parent = "l3_div_ck",
};

static struct ti_clk l3_main_2_mod_ck = {
	.name = "l3_main_2_mod_ck",
	.clkdm_name = "l3_2_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &l3_main_2_mod_ck_data,
};

static struct ti_clk_hwmod gpmc_mod_ck_data = {
	.reg = OMAP44XX_L3_2_GPMC,
	.parent = "l3_div_ck",
	.flags = CLKF_HW_SUP,
};

static struct ti_clk gpmc_mod_ck = {
	.name = "gpmc_mod_ck",
	.clkdm_name = "l3_2_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &gpmc_mod_ck_data,
};

static struct ti_clk_hwmod ocmc_ram_mod_ck_data = {
	.reg = OMAP44XX_L3_2_OCMC_RAM,
	.parent = "l3_div_ck",
};

static struct ti_clk ocmc_ram_mod_ck = {
	.name = "ocmc_ram_mod_ck",
	.clkdm_name = "l3_2_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &ocmc_ram_mod_ck_data,
};

static struct ti_clk_hwmod ipu_mod_ck_data = {
	.reg = OMAP44XX_DUCATI_IPU,
	.parent = "ducati_clk_mux_ck",
	.flags = CLKF_HW_SUP,
};

static struct ti_clk ipu_mod_ck = {
	.name = "ipu_mod_ck",
	.clkdm_name = "ducati_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &ipu_mod_ck_data,
};

static struct ti_clk_hwmod mmu_ipu_mod_ck_data = {
	.reg = OMAP44XX_DUCATI_MMU_IPU,
	.parent = "ducati_clk_mux_ck",
	.flags = CLKF_HW_SUP,
};

static struct ti_clk mmu_ipu_mod_ck = {
	.name = "mmu_ipu_mod_ck",
	.clkdm_name = "ducati_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &mmu_ipu_mod_ck_data,
};

static struct ti_clk_hwmod dma_system_mod_ck_data = {
	.reg = OMAP44XX_L3_DMA_DMA_SYSTEM,
	.parent = "l3_div_ck",
};

static struct ti_clk dma_system_mod_ck = {
	.name = "dma_system_mod_ck",
	.clkdm_name = "l3_dma_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &dma_system_mod_ck_data,
};

static struct ti_clk_hwmod dmm_mod_ck_data = {
	.reg = OMAP44XX_L3_EMIF_DMM,
	.parent = "l3_div_ck",
};

static struct ti_clk dmm_mod_ck = {
	.name = "dmm_mod_ck",
	.clkdm_name = "l3_emif_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &dmm_mod_ck_data,
};

static struct ti_clk_hwmod emif1_mod_ck_data = {
	.reg = OMAP44XX_L3_EMIF_EMIF1,
	.parent = "ddrphy_ck",
	.flags = CLKF_HW_SUP,
};

static struct ti_clk emif1_mod_ck = {
	.name = "emif1_mod_ck",
	.clkdm_name = "l3_emif_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &emif1_mod_ck_data,
};

static struct ti_clk_hwmod emif2_mod_ck_data = {
	.reg = OMAP44XX_L3_EMIF_EMIF2,
	.parent = "ddrphy_ck",
	.flags = CLKF_HW_SUP,
};

static struct ti_clk emif2_mod_ck = {
	.name = "emif2_mod_ck",
	.clkdm_name = "l3_emif_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &emif2_mod_ck_data,
};

static struct ti_clk_hwmod c2c_mod_ck_data = {
	.reg = OMAP44XX_D2D_C2C,
	.parent = "div_core_ck",
};

static struct ti_clk c2c_mod_ck = {
	.name = "c2c_mod_ck",
	.clkdm_name = "d2d_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &c2c_mod_ck_data,
};

static struct ti_clk_hwmod l4_cfg_mod_ck_data = {
	.reg = OMAP44XX_L4_CFG_L4_CFG,
	.parent = "l4_div_ck",
};

static struct ti_clk l4_cfg_mod_ck = {
	.name = "l4_cfg_mod_ck",
	.clkdm_name = "l4_cfg_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &l4_cfg_mod_ck_data,
};

static struct ti_clk_hwmod spinlock_mod_ck_data = {
	.reg = OMAP44XX_L4_CFG_SPINLOCK,
	.parent = "l4_div_ck",
};

static struct ti_clk spinlock_mod_ck = {
	.name = "spinlock_mod_ck",
	.clkdm_name = "l4_cfg_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &spinlock_mod_ck_data,
};

static struct ti_clk_hwmod mailbox_mod_ck_data = {
	.reg = OMAP44XX_L4_CFG_MAILBOX,
	.parent = "l4_div_ck",
};

static struct ti_clk mailbox_mod_ck = {
	.name = "mailbox_mod_ck",
	.clkdm_name = "l4_cfg_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &mailbox_mod_ck_data,
};

static struct ti_clk_hwmod l3_main_3_mod_ck_data = {
	.reg = OMAP44XX_L3_INSTR_L3_MAIN_3,
	.parent = "l3_div_ck",
	.flags = CLKF_HW_SUP,
};

static struct ti_clk l3_main_3_mod_ck = {
	.name = "l3_main_3_mod_ck",
	.clkdm_name = "l3_instr_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &l3_main_3_mod_ck_data,
};

static struct ti_clk_hwmod l3_instr_mod_ck_data = {
	.reg = OMAP44XX_L3_INSTR_L3_INSTR,
	.parent = "l3_div_ck",
	.flags = CLKF_HW_SUP,
};

static struct ti_clk l3_instr_mod_ck = {
	.name = "l3_instr_mod_ck",
	.clkdm_name = "l3_instr_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &l3_instr_mod_ck_data,
};

static struct ti_clk_hwmod ocp_wp_noc_mod_ck_data = {
	.reg = OMAP44XX_L3_INSTR_OCP_WP_NOC,
	.parent = "l3_div_ck",
	.flags = CLKF_HW_SUP,
};

static struct ti_clk ocp_wp_noc_mod_ck = {
	.name = "ocp_wp_noc_mod_ck",
	.clkdm_name = "l3_instr_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &ocp_wp_noc_mod_ck_data,
};

static struct ti_clk_hwmod iva_mod_ck_data = {
	.reg = OMAP44XX_IVAHD_IVA,
	.parent = "dpll_iva_m5x2_ck",
	.flags = CLKF_HW_SUP,
};

static struct ti_clk iva_mod_ck = {
	.name = "iva_mod_ck",
	.clkdm_name = "ivahd_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &iva_mod_ck_data,
};

static struct ti_clk_hwmod sl2if_mod_ck_data = {
	.reg = OMAP44XX_IVAHD_SL2IF,
	.parent = "dpll_iva_m5x2_ck",
	.flags = CLKF_HW_SUP,
};

static struct ti_clk sl2if_mod_ck = {
	.name = "sl2if_mod_ck",
	.clkdm_name = "ivahd_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &sl2if_mod_ck_data,
};

static struct ti_clk_hwmod usb_tll_hs_mod_ck_data = {
	.reg = OMAP44XX_L3_INIT_USB_TLL_HS,
	.parent = "usb_tll_hs_ick",
	.flags = CLKF_HW_SUP,
};

static struct ti_clk usb_tll_hs_mod_ck = {
	.name = "usb_tll_hs_mod_ck",
	.clkdm_name = "l3_init_clkdm",
	.type = TI_CLK_HWMOD,
	.data = &usb_tll_hs_mod_ck_data,
};

static struct ti_clk_alias omap44xx_hwmod_clks[] = {
	CLK(NULL, "mpu_mod_ck", &mpu_mod_ck),
	CLK(NULL, "dsp_mod_ck", &dsp_mod_ck),
	CLK(NULL, "mmu_dsp_mod_ck", &mmu_dsp_mod_ck),
	CLK(NULL, "l4_abe_mod_ck", &l4_abe_mod_ck),
	CLK(NULL, "aess_mod_ck", &aess_mod_ck),
	CLK(NULL, "mcpdm_mod_ck", &mcpdm_mod_ck),
	CLK(NULL, "dmic_mod_ck", &dmic_mod_ck),
	CLK(NULL, "mcasp_mod_ck", &mcasp_mod_ck),
	CLK(NULL, "mcbsp1_mod_ck", &mcbsp1_mod_ck),
	CLK(NULL, "mcbsp2_mod_ck", &mcbsp2_mod_ck),
	CLK(NULL, "mcbsp3_mod_ck", &mcbsp3_mod_ck),
	CLK(NULL, "slimbus1_mod_ck", &slimbus1_mod_ck),
	CLK(NULL, "timer5_mod_ck", &timer5_mod_ck),
	CLK(NULL, "timer6_mod_ck", &timer6_mod_ck),
	CLK(NULL, "timer7_mod_ck", &timer7_mod_ck),
	CLK(NULL, "timer8_mod_ck", &timer8_mod_ck),
	CLK(NULL, "wd_timer3_mod_ck", &wd_timer3_mod_ck),
	CLK(NULL, "l4_wkup_mod_ck", &l4_wkup_mod_ck),
	CLK(NULL, "wd_timer2_mod_ck", &wd_timer2_mod_ck),
	CLK(NULL, "gpio1_mod_ck", &gpio1_mod_ck),
	CLK(NULL, "timer1_mod_ck", &timer1_mod_ck),
	CLK(NULL, "counter_32k_mod_ck", &counter_32k_mod_ck),
	CLK(NULL, "kbd_mod_ck", &kbd_mod_ck),
	CLK(NULL, "debugss_mod_ck", &debugss_mod_ck),
	CLK(NULL, "dss_core_mod_ck", &dss_core_mod_ck),
	CLK(NULL, "dss_venc_mod_ck", &dss_venc_mod_ck),
	CLK(NULL, "dss_dispc_mod_ck", &dss_dispc_mod_ck),
	CLK(NULL, "dss_dsi1_mod_ck", &dss_dsi1_mod_ck),
	CLK(NULL, "dss_rfbi_mod_ck", &dss_rfbi_mod_ck),
	CLK(NULL, "dss_dsi2_mod_ck", &dss_dsi2_mod_ck),
	CLK(NULL, "dss_hdmi_mod_ck", &dss_hdmi_mod_ck),
	CLK(NULL, "fdif_mod_ck", &fdif_mod_ck),
	CLK(NULL, "gpio2_mod_ck", &gpio2_mod_ck),
	CLK(NULL, "gpio3_mod_ck", &gpio3_mod_ck),
	CLK(NULL, "gpio4_mod_ck", &gpio4_mod_ck),
	CLK(NULL, "gpio5_mod_ck", &gpio5_mod_ck),
	CLK(NULL, "gpio6_mod_ck", &gpio6_mod_ck),
	CLK(NULL, "hdq1w_mod_ck", &hdq1w_mod_ck),
	CLK(NULL, "i2c1_mod_ck", &i2c1_mod_ck),
	CLK(NULL, "i2c2_mod_ck", &i2c2_mod_ck),
	CLK(NULL, "i2c3_mod_ck", &i2c3_mod_ck),
	CLK(NULL, "i2c4_mod_ck", &i2c4_mod_ck),
	CLK(NULL, "l4_per_mod_ck", &l4_per_mod_ck),
	CLK(NULL, "gpu_mod_ck", &gpu_mod_ck),
	CLK(NULL, "hsi_mod_ck", &hsi_mod_ck),
	CLK(NULL, "iss_mod_ck", &iss_mod_ck),
	CLK(NULL, "mcbsp4_mod_ck", &mcbsp4_mod_ck),
	CLK(NULL, "mcspi1_mod_ck", &mcspi1_mod_ck),
	CLK(NULL, "mcspi2_mod_ck", &mcspi2_mod_ck),
	CLK(NULL, "mcspi3_mod_ck", &mcspi3_mod_ck),
	CLK(NULL, "mcspi4_mod_ck", &mcspi4_mod_ck),
	CLK(NULL, "mmc3_mod_ck", &mmc3_mod_ck),
	CLK(NULL, "mmc4_mod_ck", &mmc4_mod_ck),
	CLK(NULL, "mmc1_mod_ck", &mmc1_mod_ck),
	CLK(NULL, "mmc2_mod_ck", &mmc2_mod_ck),
	CLK(NULL, "ocp2scp_usb_phy_mod_ck", &ocp2scp_usb_phy_mod_ck),
	CLK(NULL, "timer10_mod_ck", &timer10_mod_ck),
	CLK(NULL, "timer11_mod_ck", &timer11_mod_ck),
	CLK(NULL, "timer2_mod_ck", &timer2_mod_ck),
	CLK(NULL, "timer3_mod_ck", &timer3_mod_ck),
	CLK(NULL, "timer4_mod_ck", &timer4_mod_ck),
	CLK(NULL, "timer9_mod_ck", &timer9_mod_ck),
	CLK(NULL, "elm_mod_ck", &elm_mod_ck),
	CLK(NULL, "slimbus2_mod_ck", &slimbus2_mod_ck),
	CLK(NULL, "uart1_mod_ck", &uart1_mod_ck),
	CLK(NULL, "uart2_mod_ck", &uart2_mod_ck),
	CLK(NULL, "uart3_mod_ck", &uart3_mod_ck),
	CLK(NULL, "uart4_mod_ck", &uart4_mod_ck),
	CLK(NULL, "mmc5_mod_ck", &mmc5_mod_ck),
	CLK(NULL, "smartreflex_core_mod_ck", &smartreflex_core_mod_ck),
	CLK(NULL, "smartreflex_iva_mod_ck", &smartreflex_iva_mod_ck),
	CLK(NULL, "smartreflex_mpu_mod_ck", &smartreflex_mpu_mod_ck),
	CLK(NULL, "usb_host_fs_mod_ck", &usb_host_fs_mod_ck),
	CLK(NULL, "usb_host_hs_mod_ck", &usb_host_hs_mod_ck),
	CLK(NULL, "usb_otg_hs_mod_ck", &usb_otg_hs_mod_ck),
	CLK(NULL, "l3_main_1_mod_ck", &l3_main_1_mod_ck),
	CLK(NULL, "l3_main_2_mod_ck", &l3_main_2_mod_ck),
	CLK(NULL, "gpmc_mod_ck", &gpmc_mod_ck),
	CLK(NULL, "ocmc_ram_mod_ck", &ocmc_ram_mod_ck),
	CLK(NULL, "ipu_mod_ck", &ipu_mod_ck),
	CLK(NULL, "mmu_ipu_mod_ck", &mmu_ipu_mod_ck),
	CLK(NULL, "dma_system_mod_ck", &dma_system_mod_ck),
	CLK(NULL, "dmm_mod_ck", &dmm_mod_ck),
	CLK(NULL, "emif1_mod_ck", &emif1_mod_ck),
	CLK(NULL, "emif2_mod_ck", &emif2_mod_ck),
	CLK(NULL, "c2c_mod_ck", &c2c_mod_ck),
	CLK(NULL, "l4_cfg_mod_ck", &l4_cfg_mod_ck),
	CLK(NULL, "spinlock_mod_ck", &spinlock_mod_ck),
	CLK(NULL, "mailbox_mod_ck", &mailbox_mod_ck),
	CLK(NULL, "l3_main_3_mod_ck", &l3_main_3_mod_ck),
	CLK(NULL, "l3_instr_mod_ck", &l3_instr_mod_ck),
	CLK(NULL, "ocp_wp_noc_mod_ck", &ocp_wp_noc_mod_ck),
	CLK(NULL, "iva_mod_ck", &iva_mod_ck),
	CLK(NULL, "sl2if_mod_ck", &sl2if_mod_ck),
	CLK(NULL, "usb_tll_hs_mod_ck", &usb_tll_hs_mod_ck),
	{ NULL },
};

static struct ti_dt_clk omap44xx_clks[] = {
	DT_CLK("smp_twd", NULL, "mpu_periphclk"),
	DT_CLK("omapdss_dss", "ick", "dss_fck"),
	DT_CLK("usbhs_omap", "fs_fck", "usb_host_fs_fck"),
	DT_CLK("usbhs_omap", "hs_fck", "usb_host_hs_fck"),
	DT_CLK("musb-omap2430", "ick", "usb_otg_hs_ick"),
	DT_CLK("usbhs_omap", "usbtll_ick", "usb_tll_hs_ick"),
	DT_CLK("usbhs_tll", "usbtll_ick", "usb_tll_hs_ick"),
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
	DT_CLK("usbhs_tll", "usbtll_fck", "dummy_ck"),
	DT_CLK("omap_wdt", "ick", "dummy_ck"),
	DT_CLK(NULL, "timer_32k_ck", "sys_32k_ck"),
	DT_CLK("4a318000.timer", "timer_sys_ck", "sys_clkin_ck"),
	DT_CLK("48032000.timer", "timer_sys_ck", "sys_clkin_ck"),
	DT_CLK("48034000.timer", "timer_sys_ck", "sys_clkin_ck"),
	DT_CLK("48036000.timer", "timer_sys_ck", "sys_clkin_ck"),
	DT_CLK("4803e000.timer", "timer_sys_ck", "sys_clkin_ck"),
	DT_CLK("48086000.timer", "timer_sys_ck", "sys_clkin_ck"),
	DT_CLK("48088000.timer", "timer_sys_ck", "sys_clkin_ck"),
	DT_CLK("40138000.timer", "timer_sys_ck", "syc_clk_div_ck"),
	DT_CLK("4013a000.timer", "timer_sys_ck", "syc_clk_div_ck"),
	DT_CLK("4013c000.timer", "timer_sys_ck", "syc_clk_div_ck"),
	DT_CLK("4013e000.timer", "timer_sys_ck", "syc_clk_div_ck"),
	DT_CLK(NULL, "cpufreq_ck", "dpll_mpu_ck"),
	{ .node_name = NULL },
};

int __init omap4xxx_dt_clk_init(void)
{
	int rc;
	struct clk *abe_dpll_ref, *abe_dpll, *sys_32k_ck, *usb_dpll;

	ti_dt_clocks_register(omap44xx_clks);

	ti_clk_register_clks(omap44xx_hwmod_clks);

	omap2_clk_disable_autoidle_all();

	/*
	 * Lock USB DPLL on OMAP4 devices so that the L3INIT power
	 * domain can transition to retention state when not in use.
	 */
	usb_dpll = clk_get_sys(NULL, "dpll_usb_ck");
	rc = clk_set_rate(usb_dpll, OMAP4_DPLL_USB_DEFFREQ);
	if (rc)
		pr_err("%s: failed to configure USB DPLL!\n", __func__);

	/*
	 * On OMAP4460 the ABE DPLL fails to turn on if in idle low-power
	 * state when turning the ABE clock domain. Workaround this by
	 * locking the ABE DPLL on boot.
	 * Lock the ABE DPLL in any case to avoid issues with audio.
	 */
	abe_dpll_ref = clk_get_sys(NULL, "abe_dpll_refclk_mux_ck");
	sys_32k_ck = clk_get_sys(NULL, "sys_32k_ck");
	rc = clk_set_parent(abe_dpll_ref, sys_32k_ck);
	abe_dpll = clk_get_sys(NULL, "dpll_abe_ck");
	if (!rc)
		rc = clk_set_rate(abe_dpll, OMAP4_DPLL_ABE_DEFFREQ);
	if (rc)
		pr_err("%s: failed to configure ABE DPLL!\n", __func__);

	return 0;
}
