/*
 * OMAP5 Clock init
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
#include <linux/io.h>
#include <linux/clk/ti.h>
#include <dt-bindings/clock/omap5.h>

#include "clock.h"

#define OMAP5_DPLL_ABE_DEFFREQ				98304000

/*
 * OMAP543x TRM, section "3.6.3.9.5 DPLL_USB Preferred Settings"
 * states it must be at 960MHz
 */
#define OMAP5_DPLL_USB_DEFFREQ				960000000

static const struct omap_clkctrl_reg_data omap5_mpu_clkctrl_regs[] __initconst = {
	{ OMAP5_MPU_CLKCTRL, NULL, 0, "dpll_mpu_m2_ck" },
	{ 0 },
};

static const struct omap_clkctrl_reg_data omap5_dsp_clkctrl_regs[] __initconst = {
	{ OMAP5_MMU_DSP_CLKCTRL, NULL, CLKF_HW_SUP, "dpll_iva_h11x2_ck" },
	{ 0 },
};

static const char * const omap5_dmic_gfclk_parents[] __initconst = {
	"dmic_sync_mux_ck",
	"pad_clks_ck",
	"slimbus_clk",
	NULL,
};

static const char * const omap5_dmic_sync_mux_ck_parents[] __initconst = {
	"abe_24m_fclk",
	"dss_syc_gfclk_div",
	"func_24m_clk",
	NULL,
};

static const struct omap_clkctrl_bit_data omap5_dmic_bit_data[] __initconst = {
	{ 24, TI_CLK_MUX, omap5_dmic_gfclk_parents, NULL },
	{ 26, TI_CLK_MUX, omap5_dmic_sync_mux_ck_parents, NULL },
	{ 0 },
};

static const char * const omap5_mcbsp1_gfclk_parents[] __initconst = {
	"mcbsp1_sync_mux_ck",
	"pad_clks_ck",
	"slimbus_clk",
	NULL,
};

static const struct omap_clkctrl_bit_data omap5_mcbsp1_bit_data[] __initconst = {
	{ 24, TI_CLK_MUX, omap5_mcbsp1_gfclk_parents, NULL },
	{ 26, TI_CLK_MUX, omap5_dmic_sync_mux_ck_parents, NULL },
	{ 0 },
};

static const char * const omap5_mcbsp2_gfclk_parents[] __initconst = {
	"mcbsp2_sync_mux_ck",
	"pad_clks_ck",
	"slimbus_clk",
	NULL,
};

static const struct omap_clkctrl_bit_data omap5_mcbsp2_bit_data[] __initconst = {
	{ 24, TI_CLK_MUX, omap5_mcbsp2_gfclk_parents, NULL },
	{ 26, TI_CLK_MUX, omap5_dmic_sync_mux_ck_parents, NULL },
	{ 0 },
};

static const char * const omap5_mcbsp3_gfclk_parents[] __initconst = {
	"mcbsp3_sync_mux_ck",
	"pad_clks_ck",
	"slimbus_clk",
	NULL,
};

static const struct omap_clkctrl_bit_data omap5_mcbsp3_bit_data[] __initconst = {
	{ 24, TI_CLK_MUX, omap5_mcbsp3_gfclk_parents, NULL },
	{ 26, TI_CLK_MUX, omap5_dmic_sync_mux_ck_parents, NULL },
	{ 0 },
};

static const char * const omap5_timer5_gfclk_mux_parents[] __initconst = {
	"dss_syc_gfclk_div",
	"sys_32k_ck",
	NULL,
};

static const struct omap_clkctrl_bit_data omap5_timer5_bit_data[] __initconst = {
	{ 24, TI_CLK_MUX, omap5_timer5_gfclk_mux_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_bit_data omap5_timer6_bit_data[] __initconst = {
	{ 24, TI_CLK_MUX, omap5_timer5_gfclk_mux_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_bit_data omap5_timer7_bit_data[] __initconst = {
	{ 24, TI_CLK_MUX, omap5_timer5_gfclk_mux_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_bit_data omap5_timer8_bit_data[] __initconst = {
	{ 24, TI_CLK_MUX, omap5_timer5_gfclk_mux_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_reg_data omap5_abe_clkctrl_regs[] __initconst = {
	{ OMAP5_L4_ABE_CLKCTRL, NULL, 0, "abe_iclk" },
	{ OMAP5_MCPDM_CLKCTRL, NULL, CLKF_SW_SUP, "pad_clks_ck" },
	{ OMAP5_DMIC_CLKCTRL, omap5_dmic_bit_data, CLKF_SW_SUP, "dmic_gfclk" },
	{ OMAP5_MCBSP1_CLKCTRL, omap5_mcbsp1_bit_data, CLKF_SW_SUP, "mcbsp1_gfclk" },
	{ OMAP5_MCBSP2_CLKCTRL, omap5_mcbsp2_bit_data, CLKF_SW_SUP, "mcbsp2_gfclk" },
	{ OMAP5_MCBSP3_CLKCTRL, omap5_mcbsp3_bit_data, CLKF_SW_SUP, "mcbsp3_gfclk" },
	{ OMAP5_TIMER5_CLKCTRL, omap5_timer5_bit_data, CLKF_SW_SUP, "timer5_gfclk_mux" },
	{ OMAP5_TIMER6_CLKCTRL, omap5_timer6_bit_data, CLKF_SW_SUP, "timer6_gfclk_mux" },
	{ OMAP5_TIMER7_CLKCTRL, omap5_timer7_bit_data, CLKF_SW_SUP, "timer7_gfclk_mux" },
	{ OMAP5_TIMER8_CLKCTRL, omap5_timer8_bit_data, CLKF_SW_SUP, "timer8_gfclk_mux" },
	{ 0 },
};

static const struct omap_clkctrl_reg_data omap5_l3main1_clkctrl_regs[] __initconst = {
	{ OMAP5_L3_MAIN_1_CLKCTRL, NULL, 0, "l3_iclk_div" },
	{ 0 },
};

static const struct omap_clkctrl_reg_data omap5_l3main2_clkctrl_regs[] __initconst = {
	{ OMAP5_L3_MAIN_2_CLKCTRL, NULL, 0, "l3_iclk_div" },
	{ 0 },
};

static const struct omap_clkctrl_reg_data omap5_ipu_clkctrl_regs[] __initconst = {
	{ OMAP5_MMU_IPU_CLKCTRL, NULL, CLKF_HW_SUP, "dpll_core_h22x2_ck" },
	{ 0 },
};

static const struct omap_clkctrl_reg_data omap5_dma_clkctrl_regs[] __initconst = {
	{ OMAP5_DMA_SYSTEM_CLKCTRL, NULL, 0, "l3_iclk_div" },
	{ 0 },
};

static const struct omap_clkctrl_reg_data omap5_emif_clkctrl_regs[] __initconst = {
	{ OMAP5_DMM_CLKCTRL, NULL, 0, "l3_iclk_div" },
	{ OMAP5_EMIF1_CLKCTRL, NULL, CLKF_HW_SUP, "dpll_core_h11x2_ck" },
	{ OMAP5_EMIF2_CLKCTRL, NULL, CLKF_HW_SUP, "dpll_core_h11x2_ck" },
	{ 0 },
};

static const struct omap_clkctrl_reg_data omap5_l4cfg_clkctrl_regs[] __initconst = {
	{ OMAP5_L4_CFG_CLKCTRL, NULL, 0, "l4_root_clk_div" },
	{ OMAP5_SPINLOCK_CLKCTRL, NULL, 0, "l4_root_clk_div" },
	{ OMAP5_MAILBOX_CLKCTRL, NULL, 0, "l4_root_clk_div" },
	{ 0 },
};

static const struct omap_clkctrl_reg_data omap5_l3instr_clkctrl_regs[] __initconst = {
	{ OMAP5_L3_MAIN_3_CLKCTRL, NULL, CLKF_HW_SUP, "l3_iclk_div" },
	{ OMAP5_L3_INSTR_CLKCTRL, NULL, CLKF_HW_SUP, "l3_iclk_div" },
	{ 0 },
};

static const char * const omap5_timer10_gfclk_mux_parents[] __initconst = {
	"sys_clkin",
	"sys_32k_ck",
	NULL,
};

static const struct omap_clkctrl_bit_data omap5_timer10_bit_data[] __initconst = {
	{ 24, TI_CLK_MUX, omap5_timer10_gfclk_mux_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_bit_data omap5_timer11_bit_data[] __initconst = {
	{ 24, TI_CLK_MUX, omap5_timer10_gfclk_mux_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_bit_data omap5_timer2_bit_data[] __initconst = {
	{ 24, TI_CLK_MUX, omap5_timer10_gfclk_mux_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_bit_data omap5_timer3_bit_data[] __initconst = {
	{ 24, TI_CLK_MUX, omap5_timer10_gfclk_mux_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_bit_data omap5_timer4_bit_data[] __initconst = {
	{ 24, TI_CLK_MUX, omap5_timer10_gfclk_mux_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_bit_data omap5_timer9_bit_data[] __initconst = {
	{ 24, TI_CLK_MUX, omap5_timer10_gfclk_mux_parents, NULL },
	{ 0 },
};

static const char * const omap5_gpio2_dbclk_parents[] __initconst = {
	"sys_32k_ck",
	NULL,
};

static const struct omap_clkctrl_bit_data omap5_gpio2_bit_data[] __initconst = {
	{ 8, TI_CLK_GATE, omap5_gpio2_dbclk_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_bit_data omap5_gpio3_bit_data[] __initconst = {
	{ 8, TI_CLK_GATE, omap5_gpio2_dbclk_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_bit_data omap5_gpio4_bit_data[] __initconst = {
	{ 8, TI_CLK_GATE, omap5_gpio2_dbclk_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_bit_data omap5_gpio5_bit_data[] __initconst = {
	{ 8, TI_CLK_GATE, omap5_gpio2_dbclk_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_bit_data omap5_gpio6_bit_data[] __initconst = {
	{ 8, TI_CLK_GATE, omap5_gpio2_dbclk_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_bit_data omap5_gpio7_bit_data[] __initconst = {
	{ 8, TI_CLK_GATE, omap5_gpio2_dbclk_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_bit_data omap5_gpio8_bit_data[] __initconst = {
	{ 8, TI_CLK_GATE, omap5_gpio2_dbclk_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_reg_data omap5_l4per_clkctrl_regs[] __initconst = {
	{ OMAP5_TIMER10_CLKCTRL, omap5_timer10_bit_data, CLKF_SW_SUP, "timer10_gfclk_mux" },
	{ OMAP5_TIMER11_CLKCTRL, omap5_timer11_bit_data, CLKF_SW_SUP, "timer11_gfclk_mux" },
	{ OMAP5_TIMER2_CLKCTRL, omap5_timer2_bit_data, CLKF_SW_SUP, "timer2_gfclk_mux" },
	{ OMAP5_TIMER3_CLKCTRL, omap5_timer3_bit_data, CLKF_SW_SUP, "timer3_gfclk_mux" },
	{ OMAP5_TIMER4_CLKCTRL, omap5_timer4_bit_data, CLKF_SW_SUP, "timer4_gfclk_mux" },
	{ OMAP5_TIMER9_CLKCTRL, omap5_timer9_bit_data, CLKF_SW_SUP, "timer9_gfclk_mux" },
	{ OMAP5_GPIO2_CLKCTRL, omap5_gpio2_bit_data, CLKF_HW_SUP, "l4_root_clk_div" },
	{ OMAP5_GPIO3_CLKCTRL, omap5_gpio3_bit_data, CLKF_HW_SUP, "l4_root_clk_div" },
	{ OMAP5_GPIO4_CLKCTRL, omap5_gpio4_bit_data, CLKF_HW_SUP, "l4_root_clk_div" },
	{ OMAP5_GPIO5_CLKCTRL, omap5_gpio5_bit_data, CLKF_HW_SUP, "l4_root_clk_div" },
	{ OMAP5_GPIO6_CLKCTRL, omap5_gpio6_bit_data, CLKF_HW_SUP, "l4_root_clk_div" },
	{ OMAP5_I2C1_CLKCTRL, NULL, CLKF_SW_SUP, "func_96m_fclk" },
	{ OMAP5_I2C2_CLKCTRL, NULL, CLKF_SW_SUP, "func_96m_fclk" },
	{ OMAP5_I2C3_CLKCTRL, NULL, CLKF_SW_SUP, "func_96m_fclk" },
	{ OMAP5_I2C4_CLKCTRL, NULL, CLKF_SW_SUP, "func_96m_fclk" },
	{ OMAP5_L4_PER_CLKCTRL, NULL, 0, "l4_root_clk_div" },
	{ OMAP5_MCSPI1_CLKCTRL, NULL, CLKF_SW_SUP, "func_48m_fclk" },
	{ OMAP5_MCSPI2_CLKCTRL, NULL, CLKF_SW_SUP, "func_48m_fclk" },
	{ OMAP5_MCSPI3_CLKCTRL, NULL, CLKF_SW_SUP, "func_48m_fclk" },
	{ OMAP5_MCSPI4_CLKCTRL, NULL, CLKF_SW_SUP, "func_48m_fclk" },
	{ OMAP5_GPIO7_CLKCTRL, omap5_gpio7_bit_data, CLKF_HW_SUP, "l4_root_clk_div" },
	{ OMAP5_GPIO8_CLKCTRL, omap5_gpio8_bit_data, CLKF_HW_SUP, "l4_root_clk_div" },
	{ OMAP5_MMC3_CLKCTRL, NULL, CLKF_SW_SUP, "func_48m_fclk" },
	{ OMAP5_MMC4_CLKCTRL, NULL, CLKF_SW_SUP, "func_48m_fclk" },
	{ OMAP5_UART1_CLKCTRL, NULL, CLKF_SW_SUP, "func_48m_fclk" },
	{ OMAP5_UART2_CLKCTRL, NULL, CLKF_SW_SUP, "func_48m_fclk" },
	{ OMAP5_UART3_CLKCTRL, NULL, CLKF_SW_SUP, "func_48m_fclk" },
	{ OMAP5_UART4_CLKCTRL, NULL, CLKF_SW_SUP, "func_48m_fclk" },
	{ OMAP5_MMC5_CLKCTRL, NULL, CLKF_SW_SUP, "func_96m_fclk" },
	{ OMAP5_I2C5_CLKCTRL, NULL, CLKF_SW_SUP, "func_96m_fclk" },
	{ OMAP5_UART5_CLKCTRL, NULL, CLKF_SW_SUP, "func_48m_fclk" },
	{ OMAP5_UART6_CLKCTRL, NULL, CLKF_SW_SUP, "func_48m_fclk" },
	{ 0 },
};

static const char * const omap5_dss_dss_clk_parents[] __initconst = {
	"dpll_per_h12x2_ck",
	NULL,
};

static const char * const omap5_dss_48mhz_clk_parents[] __initconst = {
	"func_48m_fclk",
	NULL,
};

static const char * const omap5_dss_sys_clk_parents[] __initconst = {
	"dss_syc_gfclk_div",
	NULL,
};

static const struct omap_clkctrl_bit_data omap5_dss_core_bit_data[] __initconst = {
	{ 8, TI_CLK_GATE, omap5_dss_dss_clk_parents, NULL },
	{ 9, TI_CLK_GATE, omap5_dss_48mhz_clk_parents, NULL },
	{ 10, TI_CLK_GATE, omap5_dss_sys_clk_parents, NULL },
	{ 11, TI_CLK_GATE, omap5_gpio2_dbclk_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_reg_data omap5_dss_clkctrl_regs[] __initconst = {
	{ OMAP5_DSS_CORE_CLKCTRL, omap5_dss_core_bit_data, CLKF_SW_SUP, "dss_dss_clk" },
	{ 0 },
};

static const char * const omap5_mmc1_fclk_mux_parents[] __initconst = {
	"func_128m_clk",
	"dpll_per_m2x2_ck",
	NULL,
};

static const char * const omap5_mmc1_fclk_parents[] __initconst = {
	"mmc1_fclk_mux",
	NULL,
};

static const struct omap_clkctrl_div_data omap5_mmc1_fclk_data __initconst = {
	.max_div = 2,
};

static const struct omap_clkctrl_bit_data omap5_mmc1_bit_data[] __initconst = {
	{ 8, TI_CLK_GATE, omap5_gpio2_dbclk_parents, NULL },
	{ 24, TI_CLK_MUX, omap5_mmc1_fclk_mux_parents, NULL },
	{ 25, TI_CLK_DIVIDER, omap5_mmc1_fclk_parents, &omap5_mmc1_fclk_data },
	{ 0 },
};

static const char * const omap5_mmc2_fclk_parents[] __initconst = {
	"mmc2_fclk_mux",
	NULL,
};

static const struct omap_clkctrl_div_data omap5_mmc2_fclk_data __initconst = {
	.max_div = 2,
};

static const struct omap_clkctrl_bit_data omap5_mmc2_bit_data[] __initconst = {
	{ 24, TI_CLK_MUX, omap5_mmc1_fclk_mux_parents, NULL },
	{ 25, TI_CLK_DIVIDER, omap5_mmc2_fclk_parents, &omap5_mmc2_fclk_data },
	{ 0 },
};

static const char * const omap5_usb_host_hs_hsic60m_p3_clk_parents[] __initconst = {
	"l3init_60m_fclk",
	NULL,
};

static const char * const omap5_usb_host_hs_hsic480m_p3_clk_parents[] __initconst = {
	"dpll_usb_m2_ck",
	NULL,
};

static const char * const omap5_usb_host_hs_utmi_p1_clk_parents[] __initconst = {
	"utmi_p1_gfclk",
	NULL,
};

static const char * const omap5_usb_host_hs_utmi_p2_clk_parents[] __initconst = {
	"utmi_p2_gfclk",
	NULL,
};

static const char * const omap5_utmi_p1_gfclk_parents[] __initconst = {
	"l3init_60m_fclk",
	"xclk60mhsp1_ck",
	NULL,
};

static const char * const omap5_utmi_p2_gfclk_parents[] __initconst = {
	"l3init_60m_fclk",
	"xclk60mhsp2_ck",
	NULL,
};

static const struct omap_clkctrl_bit_data omap5_usb_host_hs_bit_data[] __initconst = {
	{ 6, TI_CLK_GATE, omap5_usb_host_hs_hsic60m_p3_clk_parents, NULL },
	{ 7, TI_CLK_GATE, omap5_usb_host_hs_hsic480m_p3_clk_parents, NULL },
	{ 8, TI_CLK_GATE, omap5_usb_host_hs_utmi_p1_clk_parents, NULL },
	{ 9, TI_CLK_GATE, omap5_usb_host_hs_utmi_p2_clk_parents, NULL },
	{ 10, TI_CLK_GATE, omap5_usb_host_hs_hsic60m_p3_clk_parents, NULL },
	{ 11, TI_CLK_GATE, omap5_usb_host_hs_hsic60m_p3_clk_parents, NULL },
	{ 12, TI_CLK_GATE, omap5_usb_host_hs_hsic60m_p3_clk_parents, NULL },
	{ 13, TI_CLK_GATE, omap5_usb_host_hs_hsic480m_p3_clk_parents, NULL },
	{ 14, TI_CLK_GATE, omap5_usb_host_hs_hsic480m_p3_clk_parents, NULL },
	{ 24, TI_CLK_MUX, omap5_utmi_p1_gfclk_parents, NULL },
	{ 25, TI_CLK_MUX, omap5_utmi_p2_gfclk_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_bit_data omap5_usb_tll_hs_bit_data[] __initconst = {
	{ 8, TI_CLK_GATE, omap5_usb_host_hs_hsic60m_p3_clk_parents, NULL },
	{ 9, TI_CLK_GATE, omap5_usb_host_hs_hsic60m_p3_clk_parents, NULL },
	{ 10, TI_CLK_GATE, omap5_usb_host_hs_hsic60m_p3_clk_parents, NULL },
	{ 0 },
};

static const char * const omap5_sata_ref_clk_parents[] __initconst = {
	"sys_clkin",
	NULL,
};

static const struct omap_clkctrl_bit_data omap5_sata_bit_data[] __initconst = {
	{ 8, TI_CLK_GATE, omap5_sata_ref_clk_parents, NULL },
	{ 0 },
};

static const char * const omap5_usb_otg_ss_refclk960m_parents[] __initconst = {
	"dpll_usb_clkdcoldo",
	NULL,
};

static const struct omap_clkctrl_bit_data omap5_usb_otg_ss_bit_data[] __initconst = {
	{ 8, TI_CLK_GATE, omap5_usb_otg_ss_refclk960m_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_reg_data omap5_l3init_clkctrl_regs[] __initconst = {
	{ OMAP5_MMC1_CLKCTRL, omap5_mmc1_bit_data, CLKF_SW_SUP, "mmc1_fclk" },
	{ OMAP5_MMC2_CLKCTRL, omap5_mmc2_bit_data, CLKF_SW_SUP, "mmc2_fclk" },
	{ OMAP5_USB_HOST_HS_CLKCTRL, omap5_usb_host_hs_bit_data, CLKF_SW_SUP, "l3init_60m_fclk" },
	{ OMAP5_USB_TLL_HS_CLKCTRL, omap5_usb_tll_hs_bit_data, CLKF_HW_SUP, "l4_root_clk_div" },
	{ OMAP5_SATA_CLKCTRL, omap5_sata_bit_data, CLKF_SW_SUP, "func_48m_fclk" },
	{ OMAP5_OCP2SCP1_CLKCTRL, NULL, CLKF_HW_SUP, "l4_root_clk_div" },
	{ OMAP5_OCP2SCP3_CLKCTRL, NULL, CLKF_HW_SUP, "l4_root_clk_div" },
	{ OMAP5_USB_OTG_SS_CLKCTRL, omap5_usb_otg_ss_bit_data, CLKF_HW_SUP, "dpll_core_h13x2_ck" },
	{ 0 },
};

static const struct omap_clkctrl_bit_data omap5_gpio1_bit_data[] __initconst = {
	{ 8, TI_CLK_GATE, omap5_gpio2_dbclk_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_bit_data omap5_timer1_bit_data[] __initconst = {
	{ 24, TI_CLK_MUX, omap5_timer10_gfclk_mux_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_reg_data omap5_wkupaon_clkctrl_regs[] __initconst = {
	{ OMAP5_L4_WKUP_CLKCTRL, NULL, 0, "wkupaon_iclk_mux" },
	{ OMAP5_WD_TIMER2_CLKCTRL, NULL, CLKF_SW_SUP, "sys_32k_ck" },
	{ OMAP5_GPIO1_CLKCTRL, omap5_gpio1_bit_data, CLKF_HW_SUP, "wkupaon_iclk_mux" },
	{ OMAP5_TIMER1_CLKCTRL, omap5_timer1_bit_data, CLKF_SW_SUP, "timer1_gfclk_mux" },
	{ OMAP5_COUNTER_32K_CLKCTRL, NULL, 0, "wkupaon_iclk_mux" },
	{ OMAP5_KBD_CLKCTRL, NULL, CLKF_SW_SUP, "sys_32k_ck" },
	{ 0 },
};

const struct omap_clkctrl_data omap5_clkctrl_data[] __initconst = {
	{ 0x4a004320, omap5_mpu_clkctrl_regs },
	{ 0x4a004420, omap5_dsp_clkctrl_regs },
	{ 0x4a004520, omap5_abe_clkctrl_regs },
	{ 0x4a008720, omap5_l3main1_clkctrl_regs },
	{ 0x4a008820, omap5_l3main2_clkctrl_regs },
	{ 0x4a008920, omap5_ipu_clkctrl_regs },
	{ 0x4a008a20, omap5_dma_clkctrl_regs },
	{ 0x4a008b20, omap5_emif_clkctrl_regs },
	{ 0x4a008d20, omap5_l4cfg_clkctrl_regs },
	{ 0x4a008e20, omap5_l3instr_clkctrl_regs },
	{ 0x4a009020, omap5_l4per_clkctrl_regs },
	{ 0x4a009420, omap5_dss_clkctrl_regs },
	{ 0x4a009620, omap5_l3init_clkctrl_regs },
	{ 0x4ae07920, omap5_wkupaon_clkctrl_regs },
	{ 0 },
};

static struct ti_dt_clk omap54xx_clks[] = {
	DT_CLK(NULL, "pad_clks_src_ck", "pad_clks_src_ck"),
	DT_CLK(NULL, "pad_clks_ck", "pad_clks_ck"),
	DT_CLK(NULL, "secure_32k_clk_src_ck", "secure_32k_clk_src_ck"),
	DT_CLK(NULL, "slimbus_src_clk", "slimbus_src_clk"),
	DT_CLK(NULL, "slimbus_clk", "slimbus_clk"),
	DT_CLK(NULL, "sys_32k_ck", "sys_32k_ck"),
	DT_CLK(NULL, "virt_12000000_ck", "virt_12000000_ck"),
	DT_CLK(NULL, "virt_13000000_ck", "virt_13000000_ck"),
	DT_CLK(NULL, "virt_16800000_ck", "virt_16800000_ck"),
	DT_CLK(NULL, "virt_19200000_ck", "virt_19200000_ck"),
	DT_CLK(NULL, "virt_26000000_ck", "virt_26000000_ck"),
	DT_CLK(NULL, "virt_27000000_ck", "virt_27000000_ck"),
	DT_CLK(NULL, "virt_38400000_ck", "virt_38400000_ck"),
	DT_CLK(NULL, "sys_clkin", "sys_clkin"),
	DT_CLK(NULL, "xclk60mhsp1_ck", "xclk60mhsp1_ck"),
	DT_CLK(NULL, "xclk60mhsp2_ck", "xclk60mhsp2_ck"),
	DT_CLK(NULL, "abe_dpll_bypass_clk_mux", "abe_dpll_bypass_clk_mux"),
	DT_CLK(NULL, "abe_dpll_clk_mux", "abe_dpll_clk_mux"),
	DT_CLK(NULL, "dpll_abe_ck", "dpll_abe_ck"),
	DT_CLK(NULL, "dpll_abe_x2_ck", "dpll_abe_x2_ck"),
	DT_CLK(NULL, "dpll_abe_m2x2_ck", "dpll_abe_m2x2_ck"),
	DT_CLK(NULL, "abe_24m_fclk", "abe_24m_fclk"),
	DT_CLK(NULL, "abe_clk", "abe_clk"),
	DT_CLK(NULL, "abe_iclk", "abe_iclk"),
	DT_CLK(NULL, "abe_lp_clk_div", "abe_lp_clk_div"),
	DT_CLK(NULL, "dpll_abe_m3x2_ck", "dpll_abe_m3x2_ck"),
	DT_CLK(NULL, "dpll_core_ck", "dpll_core_ck"),
	DT_CLK(NULL, "dpll_core_x2_ck", "dpll_core_x2_ck"),
	DT_CLK(NULL, "dpll_core_h21x2_ck", "dpll_core_h21x2_ck"),
	DT_CLK(NULL, "c2c_fclk", "c2c_fclk"),
	DT_CLK(NULL, "c2c_iclk", "c2c_iclk"),
	DT_CLK(NULL, "custefuse_sys_gfclk_div", "custefuse_sys_gfclk_div"),
	DT_CLK(NULL, "dpll_core_h11x2_ck", "dpll_core_h11x2_ck"),
	DT_CLK(NULL, "dpll_core_h12x2_ck", "dpll_core_h12x2_ck"),
	DT_CLK(NULL, "dpll_core_h13x2_ck", "dpll_core_h13x2_ck"),
	DT_CLK(NULL, "dpll_core_h14x2_ck", "dpll_core_h14x2_ck"),
	DT_CLK(NULL, "dpll_core_h22x2_ck", "dpll_core_h22x2_ck"),
	DT_CLK(NULL, "dpll_core_h23x2_ck", "dpll_core_h23x2_ck"),
	DT_CLK(NULL, "dpll_core_h24x2_ck", "dpll_core_h24x2_ck"),
	DT_CLK(NULL, "dpll_core_m2_ck", "dpll_core_m2_ck"),
	DT_CLK(NULL, "dpll_core_m3x2_ck", "dpll_core_m3x2_ck"),
	DT_CLK(NULL, "iva_dpll_hs_clk_div", "iva_dpll_hs_clk_div"),
	DT_CLK(NULL, "dpll_iva_ck", "dpll_iva_ck"),
	DT_CLK(NULL, "dpll_iva_x2_ck", "dpll_iva_x2_ck"),
	DT_CLK(NULL, "dpll_iva_h11x2_ck", "dpll_iva_h11x2_ck"),
	DT_CLK(NULL, "dpll_iva_h12x2_ck", "dpll_iva_h12x2_ck"),
	DT_CLK(NULL, "mpu_dpll_hs_clk_div", "mpu_dpll_hs_clk_div"),
	DT_CLK(NULL, "dpll_mpu_ck", "dpll_mpu_ck"),
	DT_CLK(NULL, "dpll_mpu_m2_ck", "dpll_mpu_m2_ck"),
	DT_CLK(NULL, "per_dpll_hs_clk_div", "per_dpll_hs_clk_div"),
	DT_CLK(NULL, "dpll_per_ck", "dpll_per_ck"),
	DT_CLK(NULL, "dpll_per_x2_ck", "dpll_per_x2_ck"),
	DT_CLK(NULL, "dpll_per_h11x2_ck", "dpll_per_h11x2_ck"),
	DT_CLK(NULL, "dpll_per_h12x2_ck", "dpll_per_h12x2_ck"),
	DT_CLK(NULL, "dpll_per_h14x2_ck", "dpll_per_h14x2_ck"),
	DT_CLK(NULL, "dpll_per_m2_ck", "dpll_per_m2_ck"),
	DT_CLK(NULL, "dpll_per_m2x2_ck", "dpll_per_m2x2_ck"),
	DT_CLK(NULL, "dpll_per_m3x2_ck", "dpll_per_m3x2_ck"),
	DT_CLK(NULL, "dpll_unipro1_ck", "dpll_unipro1_ck"),
	DT_CLK(NULL, "dpll_unipro1_clkdcoldo", "dpll_unipro1_clkdcoldo"),
	DT_CLK(NULL, "dpll_unipro1_m2_ck", "dpll_unipro1_m2_ck"),
	DT_CLK(NULL, "dpll_unipro2_ck", "dpll_unipro2_ck"),
	DT_CLK(NULL, "dpll_unipro2_clkdcoldo", "dpll_unipro2_clkdcoldo"),
	DT_CLK(NULL, "dpll_unipro2_m2_ck", "dpll_unipro2_m2_ck"),
	DT_CLK(NULL, "usb_dpll_hs_clk_div", "usb_dpll_hs_clk_div"),
	DT_CLK(NULL, "dpll_usb_ck", "dpll_usb_ck"),
	DT_CLK(NULL, "dpll_usb_clkdcoldo", "dpll_usb_clkdcoldo"),
	DT_CLK(NULL, "dpll_usb_m2_ck", "dpll_usb_m2_ck"),
	DT_CLK(NULL, "dss_syc_gfclk_div", "dss_syc_gfclk_div"),
	DT_CLK(NULL, "func_128m_clk", "func_128m_clk"),
	DT_CLK(NULL, "func_12m_fclk", "func_12m_fclk"),
	DT_CLK(NULL, "func_24m_clk", "func_24m_clk"),
	DT_CLK(NULL, "func_48m_fclk", "func_48m_fclk"),
	DT_CLK(NULL, "func_96m_fclk", "func_96m_fclk"),
	DT_CLK(NULL, "l3_iclk_div", "l3_iclk_div"),
	DT_CLK(NULL, "gpu_l3_iclk", "gpu_l3_iclk"),
	DT_CLK(NULL, "l3init_60m_fclk", "l3init_60m_fclk"),
	DT_CLK(NULL, "wkupaon_iclk_mux", "wkupaon_iclk_mux"),
	DT_CLK(NULL, "l3instr_ts_gclk_div", "l3instr_ts_gclk_div"),
	DT_CLK(NULL, "l4_root_clk_div", "l4_root_clk_div"),
	DT_CLK(NULL, "dss_32khz_clk", "dss_32khz_clk"),
	DT_CLK(NULL, "dss_48mhz_clk", "dss_48mhz_clk"),
	DT_CLK(NULL, "dss_dss_clk", "dss_dss_clk"),
	DT_CLK(NULL, "dss_sys_clk", "dss_sys_clk"),
	DT_CLK(NULL, "gpio1_dbclk", "gpio1_dbclk"),
	DT_CLK(NULL, "gpio2_dbclk", "gpio2_dbclk"),
	DT_CLK(NULL, "gpio3_dbclk", "gpio3_dbclk"),
	DT_CLK(NULL, "gpio4_dbclk", "gpio4_dbclk"),
	DT_CLK(NULL, "gpio5_dbclk", "gpio5_dbclk"),
	DT_CLK(NULL, "gpio6_dbclk", "gpio6_dbclk"),
	DT_CLK(NULL, "gpio7_dbclk", "gpio7_dbclk"),
	DT_CLK(NULL, "gpio8_dbclk", "gpio8_dbclk"),
	DT_CLK(NULL, "iss_ctrlclk", "iss_ctrlclk"),
	DT_CLK(NULL, "lli_txphy_clk", "lli_txphy_clk"),
	DT_CLK(NULL, "lli_txphy_ls_clk", "lli_txphy_ls_clk"),
	DT_CLK(NULL, "mmc1_32khz_clk", "mmc1_32khz_clk"),
	DT_CLK(NULL, "sata_ref_clk", "sata_ref_clk"),
	DT_CLK(NULL, "slimbus1_slimbus_clk", "slimbus1_slimbus_clk"),
	DT_CLK(NULL, "usb_host_hs_hsic480m_p1_clk", "usb_host_hs_hsic480m_p1_clk"),
	DT_CLK(NULL, "usb_host_hs_hsic480m_p2_clk", "usb_host_hs_hsic480m_p2_clk"),
	DT_CLK(NULL, "usb_host_hs_hsic480m_p3_clk", "usb_host_hs_hsic480m_p3_clk"),
	DT_CLK(NULL, "usb_host_hs_hsic60m_p1_clk", "usb_host_hs_hsic60m_p1_clk"),
	DT_CLK(NULL, "usb_host_hs_hsic60m_p2_clk", "usb_host_hs_hsic60m_p2_clk"),
	DT_CLK(NULL, "usb_host_hs_hsic60m_p3_clk", "usb_host_hs_hsic60m_p3_clk"),
	DT_CLK(NULL, "usb_host_hs_utmi_p1_clk", "usb_host_hs_utmi_p1_clk"),
	DT_CLK(NULL, "usb_host_hs_utmi_p2_clk", "usb_host_hs_utmi_p2_clk"),
	DT_CLK(NULL, "usb_host_hs_utmi_p3_clk", "usb_host_hs_utmi_p3_clk"),
	DT_CLK(NULL, "usb_otg_ss_refclk960m", "usb_otg_ss_refclk960m"),
	DT_CLK(NULL, "usb_phy_cm_clk32k", "usb_phy_cm_clk32k"),
	DT_CLK(NULL, "usb_tll_hs_usb_ch0_clk", "usb_tll_hs_usb_ch0_clk"),
	DT_CLK(NULL, "usb_tll_hs_usb_ch1_clk", "usb_tll_hs_usb_ch1_clk"),
	DT_CLK(NULL, "usb_tll_hs_usb_ch2_clk", "usb_tll_hs_usb_ch2_clk"),
	DT_CLK(NULL, "aess_fclk", "aess_fclk"),
	DT_CLK(NULL, "dmic_sync_mux_ck", "dmic_sync_mux_ck"),
	DT_CLK(NULL, "dmic_gfclk", "dmic_gfclk"),
	DT_CLK(NULL, "fdif_fclk", "fdif_fclk"),
	DT_CLK(NULL, "gpu_core_gclk_mux", "gpu_core_gclk_mux"),
	DT_CLK(NULL, "gpu_hyd_gclk_mux", "gpu_hyd_gclk_mux"),
	DT_CLK(NULL, "hsi_fclk", "hsi_fclk"),
	DT_CLK(NULL, "mcasp_sync_mux_ck", "mcasp_sync_mux_ck"),
	DT_CLK(NULL, "mcasp_gfclk", "mcasp_gfclk"),
	DT_CLK(NULL, "mcbsp1_sync_mux_ck", "mcbsp1_sync_mux_ck"),
	DT_CLK(NULL, "mcbsp1_gfclk", "mcbsp1_gfclk"),
	DT_CLK(NULL, "mcbsp2_sync_mux_ck", "mcbsp2_sync_mux_ck"),
	DT_CLK(NULL, "mcbsp2_gfclk", "mcbsp2_gfclk"),
	DT_CLK(NULL, "mcbsp3_sync_mux_ck", "mcbsp3_sync_mux_ck"),
	DT_CLK(NULL, "mcbsp3_gfclk", "mcbsp3_gfclk"),
	DT_CLK(NULL, "mmc1_fclk_mux", "mmc1_fclk_mux"),
	DT_CLK(NULL, "mmc1_fclk", "mmc1_fclk"),
	DT_CLK(NULL, "mmc2_fclk_mux", "mmc2_fclk_mux"),
	DT_CLK(NULL, "mmc2_fclk", "mmc2_fclk"),
	DT_CLK(NULL, "timer10_gfclk_mux", "timer10_gfclk_mux"),
	DT_CLK(NULL, "timer11_gfclk_mux", "timer11_gfclk_mux"),
	DT_CLK(NULL, "timer1_gfclk_mux", "timer1_gfclk_mux"),
	DT_CLK(NULL, "timer2_gfclk_mux", "timer2_gfclk_mux"),
	DT_CLK(NULL, "timer3_gfclk_mux", "timer3_gfclk_mux"),
	DT_CLK(NULL, "timer4_gfclk_mux", "timer4_gfclk_mux"),
	DT_CLK(NULL, "timer5_gfclk_mux", "timer5_gfclk_mux"),
	DT_CLK(NULL, "timer6_gfclk_mux", "timer6_gfclk_mux"),
	DT_CLK(NULL, "timer7_gfclk_mux", "timer7_gfclk_mux"),
	DT_CLK(NULL, "timer8_gfclk_mux", "timer8_gfclk_mux"),
	DT_CLK(NULL, "timer9_gfclk_mux", "timer9_gfclk_mux"),
	DT_CLK(NULL, "utmi_p1_gfclk", "utmi_p1_gfclk"),
	DT_CLK(NULL, "utmi_p2_gfclk", "utmi_p2_gfclk"),
	DT_CLK(NULL, "auxclk0_src_ck", "auxclk0_src_ck"),
	DT_CLK(NULL, "auxclk0_ck", "auxclk0_ck"),
	DT_CLK(NULL, "auxclkreq0_ck", "auxclkreq0_ck"),
	DT_CLK(NULL, "auxclk1_src_ck", "auxclk1_src_ck"),
	DT_CLK(NULL, "auxclk1_ck", "auxclk1_ck"),
	DT_CLK(NULL, "auxclkreq1_ck", "auxclkreq1_ck"),
	DT_CLK(NULL, "auxclk2_src_ck", "auxclk2_src_ck"),
	DT_CLK(NULL, "auxclk2_ck", "auxclk2_ck"),
	DT_CLK(NULL, "auxclkreq2_ck", "auxclkreq2_ck"),
	DT_CLK(NULL, "auxclk3_src_ck", "auxclk3_src_ck"),
	DT_CLK(NULL, "auxclk3_ck", "auxclk3_ck"),
	DT_CLK(NULL, "auxclkreq3_ck", "auxclkreq3_ck"),
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
	DT_CLK(NULL, "sys_clkin_ck", "sys_clkin"),
	DT_CLK("4ae18000.timer", "timer_sys_ck", "sys_clkin"),
	DT_CLK("48032000.timer", "timer_sys_ck", "sys_clkin"),
	DT_CLK("48034000.timer", "timer_sys_ck", "sys_clkin"),
	DT_CLK("48036000.timer", "timer_sys_ck", "sys_clkin"),
	DT_CLK("4803e000.timer", "timer_sys_ck", "sys_clkin"),
	DT_CLK("48086000.timer", "timer_sys_ck", "sys_clkin"),
	DT_CLK("48088000.timer", "timer_sys_ck", "sys_clkin"),
	DT_CLK("40138000.timer", "timer_sys_ck", "dss_syc_gfclk_div"),
	DT_CLK("4013a000.timer", "timer_sys_ck", "dss_syc_gfclk_div"),
	DT_CLK("4013c000.timer", "timer_sys_ck", "dss_syc_gfclk_div"),
	DT_CLK("4013e000.timer", "timer_sys_ck", "dss_syc_gfclk_div"),
	{ .node_name = NULL },
};

int __init omap5xxx_dt_clk_init(void)
{
	int rc;
	struct clk *abe_dpll_ref, *abe_dpll, *sys_32k_ck, *usb_dpll;

	ti_dt_clocks_register(omap54xx_clks);

	omap2_clk_disable_autoidle_all();

	abe_dpll_ref = clk_get_sys(NULL, "abe_dpll_clk_mux");
	sys_32k_ck = clk_get_sys(NULL, "sys_32k_ck");
	rc = clk_set_parent(abe_dpll_ref, sys_32k_ck);
	abe_dpll = clk_get_sys(NULL, "dpll_abe_ck");
	if (!rc)
		rc = clk_set_rate(abe_dpll, OMAP5_DPLL_ABE_DEFFREQ);
	if (rc)
		pr_err("%s: failed to configure ABE DPLL!\n", __func__);

	abe_dpll = clk_get_sys(NULL, "dpll_abe_m2x2_ck");
	if (!rc)
		rc = clk_set_rate(abe_dpll, OMAP5_DPLL_ABE_DEFFREQ * 2);
	if (rc)
		pr_err("%s: failed to configure ABE m2x2 DPLL!\n", __func__);

	usb_dpll = clk_get_sys(NULL, "dpll_usb_ck");
	rc = clk_set_rate(usb_dpll, OMAP5_DPLL_USB_DEFFREQ);
	if (rc)
		pr_err("%s: failed to configure USB DPLL!\n", __func__);

	usb_dpll = clk_get_sys(NULL, "dpll_usb_m2_ck");
	rc = clk_set_rate(usb_dpll, OMAP5_DPLL_USB_DEFFREQ/2);
	if (rc)
		pr_err("%s: failed to set USB_DPLL M2 OUT\n", __func__);

	return 0;
}
