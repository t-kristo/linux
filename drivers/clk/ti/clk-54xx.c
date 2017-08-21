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
	DT_CLK(NULL, "timer_32k_ck", "sys_32k_ck"),
	DT_CLK(NULL, "sys_clkin_ck", "sys_clkin"),
	{ .node_name = NULL },
};

int __init omap5xxx_dt_clk_init(void)
{
	int rc;
	struct clk *abe_dpll_ref, *abe_dpll, *sys_32k_ck, *usb_dpll;

	ti_dt_clocks_register(omap54xx_clks);

	omap2_clk_disable_autoidle_all();

	ti_clk_add_aliases();

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
