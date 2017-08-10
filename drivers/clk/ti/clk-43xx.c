/*
 * AM43XX Clock init
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
#include <dt-bindings/clock/am4.h>

#include "clock.h"

static const char * const am4_synctimer_32kclk_parents[] __initconst = {
	"mux_synctimer32k_ck",
	NULL,
};

static const struct omap_clkctrl_bit_data am4_counter_32k_bit_data[] __initconst = {
	{ 8, TI_CLK_GATE, am4_synctimer_32kclk_parents, NULL },
	{ 0 },
};

static const char * const am4_gpio0_dbclk_parents[] __initconst = {
	"gpio0_dbclk_mux_ck",
	NULL,
};

static const struct omap_clkctrl_bit_data am4_gpio1_bit_data[] __initconst = {
	{ 8, TI_CLK_GATE, am4_gpio0_dbclk_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_reg_data am4_l4_wkup_clkctrl_regs[] __initconst = {
	{ AM4_ADC_TSC_CLKCTRL, NULL, CLKF_SW_SUP, "adc_tsc_fck" },
	{ AM4_L4_WKUP_CLKCTRL, NULL, CLKF_SW_SUP, "sys_clkin_ck" },
	{ AM4_WKUP_M3_CLKCTRL, NULL, CLKF_NO_IDLEST, "sys_clkin_ck" },
	{ AM4_COUNTER_32K_CLKCTRL, am4_counter_32k_bit_data, CLKF_SW_SUP, "synctimer_32kclk" },
	{ AM4_TIMER1_CLKCTRL, NULL, CLKF_SW_SUP, "timer1_fck" },
	{ AM4_WD_TIMER2_CLKCTRL, NULL, CLKF_SW_SUP, "wdt1_fck" },
	{ AM4_I2C1_CLKCTRL, NULL, CLKF_SW_SUP, "dpll_per_m2_div4_wkupdm_ck" },
	{ AM4_UART1_CLKCTRL, NULL, CLKF_SW_SUP, "dpll_per_m2_div4_wkupdm_ck" },
	{ AM4_SMARTREFLEX0_CLKCTRL, NULL, CLKF_SW_SUP, "smartreflex0_fck" },
	{ AM4_SMARTREFLEX1_CLKCTRL, NULL, CLKF_SW_SUP, "smartreflex1_fck" },
	{ AM4_CONTROL_CLKCTRL, NULL, CLKF_SW_SUP, "sys_clkin_ck" },
	{ AM4_GPIO1_CLKCTRL, am4_gpio1_bit_data, CLKF_SW_SUP, "sys_clkin_ck" },
	{ 0 },
};

static const struct omap_clkctrl_reg_data am4_mpu_clkctrl_regs[] __initconst = {
	{ AM4_MPU_CLKCTRL, NULL, CLKF_SW_SUP, "dpll_mpu_m2_ck" },
	{ 0 },
};

static const struct omap_clkctrl_reg_data am4_gfx_l3_clkctrl_regs[] __initconst = {
	{ AM4_GFX_CLKCTRL, NULL, CLKF_SW_SUP, "gfx_fck_div_ck" },
	{ 0 },
};

static const struct omap_clkctrl_reg_data am4_l4_rtc_clkctrl_regs[] __initconst = {
	{ AM4_RTC_CLKCTRL, NULL, CLKF_SW_SUP, "clk_32768_ck" },
	{ 0 },
};

static const char * const am4_usb_otg_ss0_refclk960m_parents[] __initconst = {
	"dpll_per_clkdcoldo",
	NULL,
};

static const struct omap_clkctrl_bit_data am4_usb_otg_ss0_bit_data[] __initconst = {
	{ 8, TI_CLK_GATE, am4_usb_otg_ss0_refclk960m_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_bit_data am4_usb_otg_ss1_bit_data[] __initconst = {
	{ 8, TI_CLK_GATE, am4_usb_otg_ss0_refclk960m_parents, NULL },
	{ 0 },
};

static const char * const am4_gpio1_dbclk_parents[] __initconst = {
	"clkdiv32k_ick",
	NULL,
};

static const struct omap_clkctrl_bit_data am4_gpio2_bit_data[] __initconst = {
	{ 8, TI_CLK_GATE, am4_gpio1_dbclk_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_bit_data am4_gpio3_bit_data[] __initconst = {
	{ 8, TI_CLK_GATE, am4_gpio1_dbclk_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_bit_data am4_gpio4_bit_data[] __initconst = {
	{ 8, TI_CLK_GATE, am4_gpio1_dbclk_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_bit_data am4_gpio5_bit_data[] __initconst = {
	{ 8, TI_CLK_GATE, am4_gpio1_dbclk_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_bit_data am4_gpio6_bit_data[] __initconst = {
	{ 8, TI_CLK_GATE, am4_gpio1_dbclk_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_reg_data am4_l4_per_clkctrl_regs[] __initconst = {
	{ AM4_L3_MAIN_CLKCTRL, NULL, CLKF_SW_SUP, "l3_gclk" },
	{ AM4_AES_CLKCTRL, NULL, CLKF_SW_SUP, "aes0_fck" },
	{ AM4_DES_CLKCTRL, NULL, CLKF_SW_SUP, "l3_gclk" },
	{ AM4_L3_INSTR_CLKCTRL, NULL, CLKF_SW_SUP, "l3_gclk" },
	{ AM4_OCMCRAM_CLKCTRL, NULL, CLKF_SW_SUP, "l3_gclk" },
	{ AM4_SHAM_CLKCTRL, NULL, CLKF_SW_SUP, "l3_gclk" },
	{ AM4_VPFE0_CLKCTRL, NULL, CLKF_SW_SUP, "l3_gclk" },
	{ AM4_VPFE1_CLKCTRL, NULL, CLKF_SW_SUP, "l3_gclk" },
	{ AM4_TPCC_CLKCTRL, NULL, CLKF_SW_SUP, "l3_gclk" },
	{ AM4_TPTC0_CLKCTRL, NULL, CLKF_SW_SUP, "l3_gclk" },
	{ AM4_TPTC1_CLKCTRL, NULL, CLKF_SW_SUP, "l3_gclk" },
	{ AM4_TPTC2_CLKCTRL, NULL, CLKF_SW_SUP, "l3_gclk" },
	{ AM4_L4_HS_CLKCTRL, NULL, CLKF_SW_SUP, "l4hs_gclk" },
	{ AM4_GPMC_CLKCTRL, NULL, CLKF_SW_SUP, "l3s_gclk" },
	{ AM4_MCASP0_CLKCTRL, NULL, CLKF_SW_SUP, "mcasp0_fck" },
	{ AM4_MCASP1_CLKCTRL, NULL, CLKF_SW_SUP, "mcasp1_fck" },
	{ AM4_MMC3_CLKCTRL, NULL, CLKF_SW_SUP, "mmc_clk" },
	{ AM4_QSPI_CLKCTRL, NULL, CLKF_SW_SUP, "l3s_gclk" },
	{ AM4_USB_OTG_SS0_CLKCTRL, am4_usb_otg_ss0_bit_data, CLKF_SW_SUP, "l3s_gclk" },
	{ AM4_USB_OTG_SS1_CLKCTRL, am4_usb_otg_ss1_bit_data, CLKF_SW_SUP, "l3s_gclk" },
	{ AM4_PRUSS_CLKCTRL, NULL, CLKF_SW_SUP, "pruss_ocp_gclk" },
	{ AM4_L4_LS_CLKCTRL, NULL, CLKF_SW_SUP, "l4ls_gclk" },
	{ AM4_D_CAN0_CLKCTRL, NULL, CLKF_SW_SUP, "dcan0_fck" },
	{ AM4_D_CAN1_CLKCTRL, NULL, CLKF_SW_SUP, "dcan1_fck" },
	{ AM4_EPWMSS0_CLKCTRL, NULL, CLKF_SW_SUP, "l4ls_gclk" },
	{ AM4_EPWMSS1_CLKCTRL, NULL, CLKF_SW_SUP, "l4ls_gclk" },
	{ AM4_EPWMSS2_CLKCTRL, NULL, CLKF_SW_SUP, "l4ls_gclk" },
	{ AM4_EPWMSS3_CLKCTRL, NULL, CLKF_SW_SUP, "l4ls_gclk" },
	{ AM4_EPWMSS4_CLKCTRL, NULL, CLKF_SW_SUP, "l4ls_gclk" },
	{ AM4_EPWMSS5_CLKCTRL, NULL, CLKF_SW_SUP, "l4ls_gclk" },
	{ AM4_ELM_CLKCTRL, NULL, CLKF_SW_SUP, "l4ls_gclk" },
	{ AM4_GPIO2_CLKCTRL, am4_gpio2_bit_data, CLKF_SW_SUP, "l4ls_gclk" },
	{ AM4_GPIO3_CLKCTRL, am4_gpio3_bit_data, CLKF_SW_SUP, "l4ls_gclk" },
	{ AM4_GPIO4_CLKCTRL, am4_gpio4_bit_data, CLKF_SW_SUP, "l4ls_gclk" },
	{ AM4_GPIO5_CLKCTRL, am4_gpio5_bit_data, CLKF_SW_SUP, "l4ls_gclk" },
	{ AM4_GPIO6_CLKCTRL, am4_gpio6_bit_data, CLKF_SW_SUP, "l4ls_gclk" },
	{ AM4_HDQ1W_CLKCTRL, NULL, CLKF_SW_SUP, "func_12m_clk" },
	{ AM4_I2C2_CLKCTRL, NULL, CLKF_SW_SUP, "dpll_per_m2_div4_ck" },
	{ AM4_I2C3_CLKCTRL, NULL, CLKF_SW_SUP, "dpll_per_m2_div4_ck" },
	{ AM4_MAILBOX_CLKCTRL, NULL, CLKF_SW_SUP, "l4ls_gclk" },
	{ AM4_MMC1_CLKCTRL, NULL, CLKF_SW_SUP, "mmc_clk" },
	{ AM4_MMC2_CLKCTRL, NULL, CLKF_SW_SUP, "mmc_clk" },
	{ AM4_RNG_CLKCTRL, NULL, CLKF_SW_SUP, "rng_fck" },
	{ AM4_SPI0_CLKCTRL, NULL, CLKF_SW_SUP, "dpll_per_m2_div4_ck" },
	{ AM4_SPI1_CLKCTRL, NULL, CLKF_SW_SUP, "dpll_per_m2_div4_ck" },
	{ AM4_SPI2_CLKCTRL, NULL, CLKF_SW_SUP, "dpll_per_m2_div4_ck" },
	{ AM4_SPI3_CLKCTRL, NULL, CLKF_SW_SUP, "dpll_per_m2_div4_ck" },
	{ AM4_SPI4_CLKCTRL, NULL, CLKF_SW_SUP, "dpll_per_m2_div4_ck" },
	{ AM4_SPINLOCK_CLKCTRL, NULL, CLKF_SW_SUP, "l4ls_gclk" },
	{ AM4_TIMER2_CLKCTRL, NULL, CLKF_SW_SUP, "timer2_fck" },
	{ AM4_TIMER3_CLKCTRL, NULL, CLKF_SW_SUP, "timer3_fck" },
	{ AM4_TIMER4_CLKCTRL, NULL, CLKF_SW_SUP, "timer4_fck" },
	{ AM4_TIMER5_CLKCTRL, NULL, CLKF_SW_SUP, "timer5_fck" },
	{ AM4_TIMER6_CLKCTRL, NULL, CLKF_SW_SUP, "timer6_fck" },
	{ AM4_TIMER7_CLKCTRL, NULL, CLKF_SW_SUP, "timer7_fck" },
	{ AM4_TIMER8_CLKCTRL, NULL, CLKF_SW_SUP, "timer8_fck" },
	{ AM4_TIMER9_CLKCTRL, NULL, CLKF_SW_SUP, "timer9_fck" },
	{ AM4_TIMER10_CLKCTRL, NULL, CLKF_SW_SUP, "timer10_fck" },
	{ AM4_TIMER11_CLKCTRL, NULL, CLKF_SW_SUP, "timer11_fck" },
	{ AM4_UART2_CLKCTRL, NULL, CLKF_SW_SUP, "dpll_per_m2_div4_ck" },
	{ AM4_UART3_CLKCTRL, NULL, CLKF_SW_SUP, "dpll_per_m2_div4_ck" },
	{ AM4_UART4_CLKCTRL, NULL, CLKF_SW_SUP, "dpll_per_m2_div4_ck" },
	{ AM4_UART5_CLKCTRL, NULL, CLKF_SW_SUP, "dpll_per_m2_div4_ck" },
	{ AM4_UART6_CLKCTRL, NULL, CLKF_SW_SUP, "dpll_per_m2_div4_ck" },
	{ AM4_OCP2SCP0_CLKCTRL, NULL, CLKF_SW_SUP, "l4ls_gclk" },
	{ AM4_OCP2SCP1_CLKCTRL, NULL, CLKF_SW_SUP, "l4ls_gclk" },
	{ AM4_EMIF_CLKCTRL, NULL, CLKF_SW_SUP, "dpll_ddr_m2_ck" },
	{ AM4_DSS_CORE_CLKCTRL, NULL, CLKF_SW_SUP, "disp_clk" },
	{ AM4_CPGMAC0_CLKCTRL, NULL, CLKF_SW_SUP, "cpsw_125mhz_gclk" },
	{ 0 },
};

const struct omap_clkctrl_data am4_clkctrl_data[] __initconst = {
	{ 0x44df2820, am4_l4_wkup_clkctrl_regs },
	{ 0x44df8320, am4_mpu_clkctrl_regs },
	{ 0x44df8420, am4_gfx_l3_clkctrl_regs },
	{ 0x44df8520, am4_l4_rtc_clkctrl_regs },
	{ 0x44df8820, am4_l4_per_clkctrl_regs },
	{ 0 },
};

static struct ti_dt_clk am43xx_clks[] = {
	DT_CLK(NULL, "clk_32768_ck", "clk_32768_ck"),
	DT_CLK(NULL, "clk_rc32k_ck", "clk_rc32k_ck"),
	DT_CLK(NULL, "virt_19200000_ck", "virt_19200000_ck"),
	DT_CLK(NULL, "virt_24000000_ck", "virt_24000000_ck"),
	DT_CLK(NULL, "virt_25000000_ck", "virt_25000000_ck"),
	DT_CLK(NULL, "virt_26000000_ck", "virt_26000000_ck"),
	DT_CLK(NULL, "sys_clkin_ck", "sys_clkin_ck"),
	DT_CLK(NULL, "tclkin_ck", "tclkin_ck"),
	DT_CLK(NULL, "dpll_core_ck", "dpll_core_ck"),
	DT_CLK(NULL, "dpll_core_x2_ck", "dpll_core_x2_ck"),
	DT_CLK(NULL, "dpll_core_m4_ck", "dpll_core_m4_ck"),
	DT_CLK(NULL, "dpll_core_m5_ck", "dpll_core_m5_ck"),
	DT_CLK(NULL, "dpll_core_m6_ck", "dpll_core_m6_ck"),
	DT_CLK(NULL, "dpll_mpu_ck", "dpll_mpu_ck"),
	DT_CLK(NULL, "dpll_mpu_m2_ck", "dpll_mpu_m2_ck"),
	DT_CLK(NULL, "dpll_ddr_ck", "dpll_ddr_ck"),
	DT_CLK(NULL, "dpll_ddr_m2_ck", "dpll_ddr_m2_ck"),
	DT_CLK(NULL, "dpll_disp_ck", "dpll_disp_ck"),
	DT_CLK(NULL, "dpll_disp_m2_ck", "dpll_disp_m2_ck"),
	DT_CLK(NULL, "dpll_per_ck", "dpll_per_ck"),
	DT_CLK(NULL, "dpll_per_m2_ck", "dpll_per_m2_ck"),
	DT_CLK(NULL, "dpll_per_m2_div4_wkupdm_ck", "dpll_per_m2_div4_wkupdm_ck"),
	DT_CLK(NULL, "dpll_per_m2_div4_ck", "dpll_per_m2_div4_ck"),
	DT_CLK(NULL, "adc_tsc_fck", "adc_tsc_fck"),
	DT_CLK(NULL, "clkdiv32k_ck", "clkdiv32k_ck"),
	DT_CLK(NULL, "clkdiv32k_ick", "clkdiv32k_ick"),
	DT_CLK(NULL, "dcan0_fck", "dcan0_fck"),
	DT_CLK(NULL, "dcan1_fck", "dcan1_fck"),
	DT_CLK(NULL, "pruss_ocp_gclk", "pruss_ocp_gclk"),
	DT_CLK(NULL, "mcasp0_fck", "mcasp0_fck"),
	DT_CLK(NULL, "mcasp1_fck", "mcasp1_fck"),
	DT_CLK(NULL, "smartreflex0_fck", "smartreflex0_fck"),
	DT_CLK(NULL, "smartreflex1_fck", "smartreflex1_fck"),
	DT_CLK(NULL, "sha0_fck", "sha0_fck"),
	DT_CLK(NULL, "aes0_fck", "aes0_fck"),
	DT_CLK(NULL, "rng_fck", "rng_fck"),
	DT_CLK(NULL, "timer1_fck", "timer1_fck"),
	DT_CLK(NULL, "timer2_fck", "timer2_fck"),
	DT_CLK(NULL, "timer3_fck", "timer3_fck"),
	DT_CLK(NULL, "timer4_fck", "timer4_fck"),
	DT_CLK(NULL, "timer5_fck", "timer5_fck"),
	DT_CLK(NULL, "timer6_fck", "timer6_fck"),
	DT_CLK(NULL, "timer7_fck", "timer7_fck"),
	DT_CLK(NULL, "wdt1_fck", "wdt1_fck"),
	DT_CLK(NULL, "l3_gclk", "l3_gclk"),
	DT_CLK(NULL, "dpll_core_m4_div2_ck", "dpll_core_m4_div2_ck"),
	DT_CLK(NULL, "l4hs_gclk", "l4hs_gclk"),
	DT_CLK(NULL, "l3s_gclk", "l3s_gclk"),
	DT_CLK(NULL, "l4ls_gclk", "l4ls_gclk"),
	DT_CLK(NULL, "clk_24mhz", "clk_24mhz"),
	DT_CLK(NULL, "cpsw_125mhz_gclk", "cpsw_125mhz_gclk"),
	DT_CLK(NULL, "cpsw_cpts_rft_clk", "cpsw_cpts_rft_clk"),
	DT_CLK(NULL, "dpll_clksel_mac_clk", "dpll_clksel_mac_clk"),
	DT_CLK(NULL, "gpio0_dbclk_mux_ck", "gpio0_dbclk_mux_ck"),
	DT_CLK(NULL, "gpio0_dbclk", "gpio0_dbclk"),
	DT_CLK(NULL, "gpio1_dbclk", "gpio1_dbclk"),
	DT_CLK(NULL, "gpio2_dbclk", "gpio2_dbclk"),
	DT_CLK(NULL, "gpio3_dbclk", "gpio3_dbclk"),
	DT_CLK(NULL, "gpio4_dbclk", "gpio4_dbclk"),
	DT_CLK(NULL, "gpio5_dbclk", "gpio5_dbclk"),
	DT_CLK(NULL, "mmc_clk", "mmc_clk"),
	DT_CLK(NULL, "gfx_fclk_clksel_ck", "gfx_fclk_clksel_ck"),
	DT_CLK(NULL, "gfx_fck_div_ck", "gfx_fck_div_ck"),
	DT_CLK(NULL, "timer_32k_ck", "clkdiv32k_ick"),
	DT_CLK(NULL, "timer_sys_ck", "sys_clkin_ck"),
	DT_CLK(NULL, "sysclk_div", "sysclk_div"),
	DT_CLK(NULL, "disp_clk", "disp_clk"),
	DT_CLK(NULL, "clk_32k_mosc_ck", "clk_32k_mosc_ck"),
	DT_CLK(NULL, "clk_32k_tpm_ck", "clk_32k_tpm_ck"),
	DT_CLK(NULL, "dpll_extdev_ck", "dpll_extdev_ck"),
	DT_CLK(NULL, "dpll_extdev_m2_ck", "dpll_extdev_m2_ck"),
	DT_CLK(NULL, "mux_synctimer32k_ck", "mux_synctimer32k_ck"),
	DT_CLK(NULL, "synctimer_32kclk", "synctimer_32kclk"),
	DT_CLK(NULL, "timer8_fck", "timer8_fck"),
	DT_CLK(NULL, "timer9_fck", "timer9_fck"),
	DT_CLK(NULL, "timer10_fck", "timer10_fck"),
	DT_CLK(NULL, "timer11_fck", "timer11_fck"),
	DT_CLK(NULL, "cpsw_50m_clkdiv", "cpsw_50m_clkdiv"),
	DT_CLK(NULL, "cpsw_5m_clkdiv", "cpsw_5m_clkdiv"),
	DT_CLK(NULL, "dpll_ddr_x2_ck", "dpll_ddr_x2_ck"),
	DT_CLK(NULL, "dpll_ddr_m4_ck", "dpll_ddr_m4_ck"),
	DT_CLK(NULL, "dpll_per_clkdcoldo", "dpll_per_clkdcoldo"),
	DT_CLK(NULL, "dll_aging_clk_div", "dll_aging_clk_div"),
	DT_CLK(NULL, "div_core_25m_ck", "div_core_25m_ck"),
	DT_CLK(NULL, "func_12m_clk", "func_12m_clk"),
	DT_CLK(NULL, "vtp_clk_div", "vtp_clk_div"),
	DT_CLK(NULL, "usbphy_32khz_clkmux", "usbphy_32khz_clkmux"),
	DT_CLK("48300200.ehrpwm", "tbclk", "ehrpwm0_tbclk"),
	DT_CLK("48302200.ehrpwm", "tbclk", "ehrpwm1_tbclk"),
	DT_CLK("48304200.ehrpwm", "tbclk", "ehrpwm2_tbclk"),
	DT_CLK("48306200.ehrpwm", "tbclk", "ehrpwm3_tbclk"),
	DT_CLK("48308200.ehrpwm", "tbclk", "ehrpwm4_tbclk"),
	DT_CLK("4830a200.ehrpwm", "tbclk", "ehrpwm5_tbclk"),
	DT_CLK("48300200.pwm", "tbclk", "ehrpwm0_tbclk"),
	DT_CLK("48302200.pwm", "tbclk", "ehrpwm1_tbclk"),
	DT_CLK("48304200.pwm", "tbclk", "ehrpwm2_tbclk"),
	DT_CLK("48306200.pwm", "tbclk", "ehrpwm3_tbclk"),
	DT_CLK("48308200.pwm", "tbclk", "ehrpwm4_tbclk"),
	DT_CLK("4830a200.pwm", "tbclk", "ehrpwm5_tbclk"),
	{ .node_name = NULL },
};

int __init am43xx_dt_clk_init(void)
{
	struct clk *clk1, *clk2;

	ti_dt_clocks_register(am43xx_clks);

	omap2_clk_disable_autoidle_all();

	/*
	 * cpsw_cpts_rft_clk  has got the choice of 3 clocksources
	 * dpll_core_m4_ck, dpll_core_m5_ck and dpll_disp_m2_ck.
	 * By default dpll_core_m4_ck is selected, witn this as clock
	 * source the CPTS doesnot work properly. It gives clockcheck errors
	 * while running PTP.
	 * clockcheck: clock jumped backward or running slower than expected!
	 * By selecting dpll_core_m5_ck as the clocksource fixes this issue.
	 * In AM335x dpll_core_m5_ck is the default clocksource.
	 */
	clk1 = clk_get_sys(NULL, "cpsw_cpts_rft_clk");
	clk2 = clk_get_sys(NULL, "dpll_core_m5_ck");
	clk_set_parent(clk1, clk2);

	return 0;
}
