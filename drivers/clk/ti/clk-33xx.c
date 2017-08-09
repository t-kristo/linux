/*
 * AM33XX Clock init
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
#include <dt-bindings/clock/am3.h>

#include "clock.h"

static const char * const am3_gpio1_dbclk_parents[] __initconst = {
	"clkdiv32k_ick",
	NULL,
};

static const struct omap_clkctrl_bit_data am3_gpio2_bit_data[] __initconst = {
	{ 18, TI_CLK_GATE, am3_gpio1_dbclk_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_bit_data am3_gpio3_bit_data[] __initconst = {
	{ 18, TI_CLK_GATE, am3_gpio1_dbclk_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_bit_data am3_gpio4_bit_data[] __initconst = {
	{ 18, TI_CLK_GATE, am3_gpio1_dbclk_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_reg_data am3_l4_per_clkctrl_regs[] __initconst = {
	{ AM3_CPGMAC0_CLKCTRL, NULL, CLKF_SW_SUP, "cpsw_125mhz_gclk" },
	{ AM3_LCDC_CLKCTRL, NULL, CLKF_SW_SUP, "lcd_gclk" },
	{ AM3_USB_OTG_HS_CLKCTRL, NULL, CLKF_SW_SUP, "usbotg_fck" },
	{ AM3_TPTC0_CLKCTRL, NULL, CLKF_SW_SUP, "l3_gclk" },
	{ AM3_EMIF_CLKCTRL, NULL, CLKF_SW_SUP, "dpll_ddr_m2_div2_ck" },
	{ AM3_OCMCRAM_CLKCTRL, NULL, CLKF_SW_SUP, "l3_gclk" },
	{ AM3_GPMC_CLKCTRL, NULL, CLKF_SW_SUP, "l3s_gclk" },
	{ AM3_MCASP0_CLKCTRL, NULL, CLKF_SW_SUP, "mcasp0_fck" },
	{ AM3_UART6_CLKCTRL, NULL, CLKF_SW_SUP, "dpll_per_m2_div4_ck" },
	{ AM3_MMC1_CLKCTRL, NULL, CLKF_SW_SUP, "mmc_clk" },
	{ AM3_ELM_CLKCTRL, NULL, CLKF_SW_SUP, "l4ls_gclk" },
	{ AM3_I2C3_CLKCTRL, NULL, CLKF_SW_SUP, "dpll_per_m2_div4_ck" },
	{ AM3_I2C2_CLKCTRL, NULL, CLKF_SW_SUP, "dpll_per_m2_div4_ck" },
	{ AM3_SPI0_CLKCTRL, NULL, CLKF_SW_SUP, "dpll_per_m2_div4_ck" },
	{ AM3_SPI1_CLKCTRL, NULL, CLKF_SW_SUP, "dpll_per_m2_div4_ck" },
	{ AM3_L4_LS_CLKCTRL, NULL, CLKF_SW_SUP, "l4ls_gclk" },
	{ AM3_MCASP1_CLKCTRL, NULL, CLKF_SW_SUP, "mcasp1_fck" },
	{ AM3_UART2_CLKCTRL, NULL, CLKF_SW_SUP, "dpll_per_m2_div4_ck" },
	{ AM3_UART3_CLKCTRL, NULL, CLKF_SW_SUP, "dpll_per_m2_div4_ck" },
	{ AM3_UART4_CLKCTRL, NULL, CLKF_SW_SUP, "dpll_per_m2_div4_ck" },
	{ AM3_UART5_CLKCTRL, NULL, CLKF_SW_SUP, "dpll_per_m2_div4_ck" },
	{ AM3_TIMER7_CLKCTRL, NULL, CLKF_SW_SUP, "timer7_fck" },
	{ AM3_TIMER2_CLKCTRL, NULL, CLKF_SW_SUP, "timer2_fck" },
	{ AM3_TIMER3_CLKCTRL, NULL, CLKF_SW_SUP, "timer3_fck" },
	{ AM3_TIMER4_CLKCTRL, NULL, CLKF_SW_SUP, "timer4_fck" },
	{ AM3_RNG_CLKCTRL, NULL, CLKF_SW_SUP, "rng_fck" },
	{ AM3_AES_CLKCTRL, NULL, CLKF_SW_SUP, "aes0_fck" },
	{ AM3_SHAM_CLKCTRL, NULL, CLKF_SW_SUP, "l3_gclk" },
	{ AM3_GPIO2_CLKCTRL, am3_gpio2_bit_data, CLKF_SW_SUP, "l4ls_gclk" },
	{ AM3_GPIO3_CLKCTRL, am3_gpio3_bit_data, CLKF_SW_SUP, "l4ls_gclk" },
	{ AM3_GPIO4_CLKCTRL, am3_gpio4_bit_data, CLKF_SW_SUP, "l4ls_gclk" },
	{ AM3_TPCC_CLKCTRL, NULL, CLKF_SW_SUP, "l3_gclk" },
	{ AM3_D_CAN0_CLKCTRL, NULL, CLKF_SW_SUP, "dcan0_fck" },
	{ AM3_D_CAN1_CLKCTRL, NULL, CLKF_SW_SUP, "dcan1_fck" },
	{ AM3_EPWMSS1_CLKCTRL, NULL, CLKF_SW_SUP, "l4ls_gclk" },
	{ AM3_EPWMSS0_CLKCTRL, NULL, CLKF_SW_SUP, "l4ls_gclk" },
	{ AM3_EPWMSS2_CLKCTRL, NULL, CLKF_SW_SUP, "l4ls_gclk" },
	{ AM3_L3_INSTR_CLKCTRL, NULL, CLKF_SW_SUP, "l3_gclk" },
	{ AM3_L3_MAIN_CLKCTRL, NULL, CLKF_SW_SUP, "l3_gclk" },
	{ AM3_PRUSS_CLKCTRL, NULL, CLKF_SW_SUP, "pruss_ocp_gclk" },
	{ AM3_TIMER5_CLKCTRL, NULL, CLKF_SW_SUP, "timer5_fck" },
	{ AM3_TIMER6_CLKCTRL, NULL, CLKF_SW_SUP, "timer6_fck" },
	{ AM3_MMC2_CLKCTRL, NULL, CLKF_SW_SUP, "mmc_clk" },
	{ AM3_MMC3_CLKCTRL, NULL, CLKF_SW_SUP, "mmc_clk" },
	{ AM3_TPTC1_CLKCTRL, NULL, CLKF_SW_SUP, "l3_gclk" },
	{ AM3_TPTC2_CLKCTRL, NULL, CLKF_SW_SUP, "l3_gclk" },
	{ AM3_SPINLOCK_CLKCTRL, NULL, CLKF_SW_SUP, "l4ls_gclk" },
	{ AM3_MAILBOX_CLKCTRL, NULL, CLKF_SW_SUP, "l4ls_gclk" },
	{ AM3_L4_HS_CLKCTRL, NULL, CLKF_SW_SUP, "l4hs_gclk" },
	{ AM3_OCPWP_CLKCTRL, NULL, CLKF_SW_SUP, "l4ls_gclk" },
	{ AM3_CLKDIV32K_CLKCTRL, NULL, CLKF_SW_SUP, "clkdiv32k_ck" },
	{ 0 },
};

static const char * const am3_gpio0_dbclk_parents[] __initconst = {
	"gpio0_dbclk_mux_ck",
	NULL,
};

static const struct omap_clkctrl_bit_data am3_gpio1_bit_data[] __initconst = {
	{ 18, TI_CLK_GATE, am3_gpio0_dbclk_parents, NULL },
	{ 0 },
};

static const char * const am3_dbg_sysclk_ck_parents[] __initconst = {
	"sys_clkin_ck",
	NULL,
};

static const char * const am3_trace_pmd_clk_mux_ck_parents[] __initconst = {
	"dbg_sysclk_ck",
	"dbg_clka_ck",
	NULL,
};

static const char * const am3_trace_clk_div_ck_parents[] __initconst = {
	"trace_pmd_clk_mux_ck",
	NULL,
};

static const struct omap_clkctrl_div_data am3_trace_clk_div_ck_data __initconst = {
	.max_div = 64,
};

static const char * const am3_stm_clk_div_ck_parents[] __initconst = {
	"stm_pmd_clock_mux_ck",
	NULL,
};

static const struct omap_clkctrl_div_data am3_stm_clk_div_ck_data __initconst = {
	.max_div = 64,
};

static const char * const am3_dbg_clka_ck_parents[] __initconst = {
	"dpll_core_m4_ck",
	NULL,
};

static const struct omap_clkctrl_bit_data am3_debugss_bit_data[] __initconst = {
	{ 19, TI_CLK_GATE, am3_dbg_sysclk_ck_parents, NULL },
	{ 20, TI_CLK_MUX, am3_trace_pmd_clk_mux_ck_parents, NULL },
	{ 22, TI_CLK_MUX, am3_trace_pmd_clk_mux_ck_parents, NULL },
	{ 24, TI_CLK_DIVIDER, am3_trace_clk_div_ck_parents, &am3_trace_clk_div_ck_data },
	{ 27, TI_CLK_DIVIDER, am3_stm_clk_div_ck_parents, &am3_stm_clk_div_ck_data },
	{ 30, TI_CLK_GATE, am3_dbg_clka_ck_parents, NULL },
	{ 0 },
};

static const struct omap_clkctrl_reg_data am3_l4_wkup_clkctrl_regs[] __initconst = {
	{ AM3_CONTROL_CLKCTRL, NULL, CLKF_SW_SUP, "dpll_core_m4_div2_ck" },
	{ AM3_GPIO1_CLKCTRL, am3_gpio1_bit_data, CLKF_SW_SUP, "dpll_core_m4_div2_ck" },
	{ AM3_L4_WKUP_CLKCTRL, NULL, CLKF_SW_SUP, "dpll_core_m4_div2_ck" },
	{ AM3_DEBUGSS_CLKCTRL, am3_debugss_bit_data, CLKF_SW_SUP, "trace_clk_div_ck" },
	{ AM3_WKUP_M3_CLKCTRL, NULL, CLKF_NO_IDLEST, "dpll_core_m4_div2_ck" },
	{ AM3_UART1_CLKCTRL, NULL, CLKF_SW_SUP, "dpll_per_m2_div4_wkupdm_ck" },
	{ AM3_I2C1_CLKCTRL, NULL, CLKF_SW_SUP, "dpll_per_m2_div4_wkupdm_ck" },
	{ AM3_ADC_TSC_CLKCTRL, NULL, CLKF_SW_SUP, "adc_tsc_fck" },
	{ AM3_SMARTREFLEX0_CLKCTRL, NULL, CLKF_SW_SUP, "smartreflex0_fck" },
	{ AM3_TIMER1_CLKCTRL, NULL, CLKF_SW_SUP, "timer1_fck" },
	{ AM3_SMARTREFLEX1_CLKCTRL, NULL, CLKF_SW_SUP, "smartreflex1_fck" },
	{ AM3_WD_TIMER2_CLKCTRL, NULL, CLKF_SW_SUP, "wdt1_fck" },
	{ 0 },
};

static const struct omap_clkctrl_reg_data am3_mpu_clkctrl_regs[] __initconst = {
	{ AM3_MPU_CLKCTRL, NULL, CLKF_SW_SUP, "dpll_mpu_m2_ck" },
	{ 0 },
};

static const struct omap_clkctrl_reg_data am3_l4_rtc_clkctrl_regs[] __initconst = {
	{ AM3_RTC_CLKCTRL, NULL, CLKF_SW_SUP, "clk_32768_ck" },
	{ 0 },
};

static const struct omap_clkctrl_reg_data am3_gfx_l3_clkctrl_regs[] __initconst = {
	{ AM3_GFX_CLKCTRL, NULL, CLKF_SW_SUP, "gfx_fck_div_ck" },
	{ 0 },
};

static const struct omap_clkctrl_reg_data am3_l4_cefuse_clkctrl_regs[] __initconst = {
	{ AM3_CEFUSE_CLKCTRL, NULL, CLKF_SW_SUP, "sys_clkin_ck" },
	{ 0 },
};

const struct omap_clkctrl_data am3_clkctrl_data[] __initconst = {
	{ 0x44e00014, am3_l4_per_clkctrl_regs },
	{ 0x44e00404, am3_l4_wkup_clkctrl_regs },
	{ 0x44e00604, am3_mpu_clkctrl_regs },
	{ 0x44e00800, am3_l4_rtc_clkctrl_regs },
	{ 0x44e00904, am3_gfx_l3_clkctrl_regs },
	{ 0x44e00a20, am3_l4_cefuse_clkctrl_regs },
	{ 0 },
};

static struct ti_dt_clk am33xx_clks[] = {
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
	DT_CLK(NULL, "dpll_ddr_m2_div2_ck", "dpll_ddr_m2_div2_ck"),
	DT_CLK(NULL, "dpll_disp_ck", "dpll_disp_ck"),
	DT_CLK(NULL, "dpll_disp_m2_ck", "dpll_disp_m2_ck"),
	DT_CLK(NULL, "dpll_per_ck", "dpll_per_ck"),
	DT_CLK(NULL, "dpll_per_m2_ck", "dpll_per_m2_ck"),
	DT_CLK(NULL, "dpll_per_m2_div4_wkupdm_ck", "dpll_per_m2_div4_wkupdm_ck"),
	DT_CLK(NULL, "dpll_per_m2_div4_ck", "dpll_per_m2_div4_ck"),
	DT_CLK(NULL, "adc_tsc_fck", "adc_tsc_fck"),
	DT_CLK(NULL, "cefuse_fck", "cefuse_fck"),
	DT_CLK(NULL, "clkdiv32k_ck", "clkdiv32k_ck"),
	DT_CLK(NULL, "clkdiv32k_ick", "clkdiv32k_ick"),
	DT_CLK(NULL, "dcan0_fck", "dcan0_fck"),
	DT_CLK("481cc000.d_can", NULL, "dcan0_fck"),
	DT_CLK(NULL, "dcan1_fck", "dcan1_fck"),
	DT_CLK("481d0000.d_can", NULL, "dcan1_fck"),
	DT_CLK(NULL, "pruss_ocp_gclk", "pruss_ocp_gclk"),
	DT_CLK(NULL, "mcasp0_fck", "mcasp0_fck"),
	DT_CLK(NULL, "mcasp1_fck", "mcasp1_fck"),
	DT_CLK(NULL, "mmu_fck", "mmu_fck"),
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
	DT_CLK(NULL, "usbotg_fck", "usbotg_fck"),
	DT_CLK(NULL, "ieee5000_fck", "ieee5000_fck"),
	DT_CLK(NULL, "wdt1_fck", "wdt1_fck"),
	DT_CLK(NULL, "l4_rtc_gclk", "l4_rtc_gclk"),
	DT_CLK(NULL, "l3_gclk", "l3_gclk"),
	DT_CLK(NULL, "dpll_core_m4_div2_ck", "dpll_core_m4_div2_ck"),
	DT_CLK(NULL, "l4hs_gclk", "l4hs_gclk"),
	DT_CLK(NULL, "l3s_gclk", "l3s_gclk"),
	DT_CLK(NULL, "l4fw_gclk", "l4fw_gclk"),
	DT_CLK(NULL, "l4ls_gclk", "l4ls_gclk"),
	DT_CLK(NULL, "clk_24mhz", "clk_24mhz"),
	DT_CLK(NULL, "sysclk_div_ck", "sysclk_div_ck"),
	DT_CLK(NULL, "cpsw_125mhz_gclk", "cpsw_125mhz_gclk"),
	DT_CLK(NULL, "cpsw_cpts_rft_clk", "cpsw_cpts_rft_clk"),
	DT_CLK(NULL, "gpio0_dbclk_mux_ck", "gpio0_dbclk_mux_ck"),
	DT_CLK(NULL, "gpio0_dbclk", "gpio0_dbclk"),
	DT_CLK(NULL, "gpio1_dbclk", "gpio1_dbclk"),
	DT_CLK(NULL, "gpio2_dbclk", "gpio2_dbclk"),
	DT_CLK(NULL, "gpio3_dbclk", "gpio3_dbclk"),
	DT_CLK(NULL, "lcd_gclk", "lcd_gclk"),
	DT_CLK(NULL, "mmc_clk", "mmc_clk"),
	DT_CLK(NULL, "gfx_fclk_clksel_ck", "gfx_fclk_clksel_ck"),
	DT_CLK(NULL, "gfx_fck_div_ck", "gfx_fck_div_ck"),
	DT_CLK(NULL, "sysclkout_pre_ck", "sysclkout_pre_ck"),
	DT_CLK(NULL, "clkout2_div_ck", "clkout2_div_ck"),
	DT_CLK(NULL, "timer_32k_ck", "clkdiv32k_ick"),
	DT_CLK(NULL, "timer_sys_ck", "sys_clkin_ck"),
	DT_CLK(NULL, "dbg_sysclk_ck", "dbg_sysclk_ck"),
	DT_CLK(NULL, "dbg_clka_ck", "dbg_clka_ck"),
	DT_CLK(NULL, "stm_pmd_clock_mux_ck", "stm_pmd_clock_mux_ck"),
	DT_CLK(NULL, "trace_pmd_clk_mux_ck", "trace_pmd_clk_mux_ck"),
	DT_CLK(NULL, "stm_clk_div_ck", "stm_clk_div_ck"),
	DT_CLK(NULL, "trace_clk_div_ck", "trace_clk_div_ck"),
	DT_CLK(NULL, "clkout2_ck", "clkout2_ck"),
	DT_CLK("48300200.ehrpwm", "tbclk", "ehrpwm0_tbclk"),
	DT_CLK("48302200.ehrpwm", "tbclk", "ehrpwm1_tbclk"),
	DT_CLK("48304200.ehrpwm", "tbclk", "ehrpwm2_tbclk"),
	DT_CLK("48300200.pwm", "tbclk", "ehrpwm0_tbclk"),
	DT_CLK("48302200.pwm", "tbclk", "ehrpwm1_tbclk"),
	DT_CLK("48304200.pwm", "tbclk", "ehrpwm2_tbclk"),
	{ .node_name = NULL },
};

static const char *enable_init_clks[] = {
	"dpll_ddr_m2_ck",
	"dpll_mpu_m2_ck",
	"l3_gclk",
	"l4hs_gclk",
	"l4fw_gclk",
	"l4ls_gclk",
	/* Required for external peripherals like, Audio codecs */
	"clkout2_ck",
};

int __init am33xx_dt_clk_init(void)
{
	struct clk *clk1, *clk2;

	ti_dt_clocks_register(am33xx_clks);

	omap2_clk_disable_autoidle_all();

	omap2_clk_enable_init_clocks(enable_init_clks,
				     ARRAY_SIZE(enable_init_clks));

	/* TRM ERRATA: Timer 3 & 6 default parent (TCLKIN) may not be always
	 *    physically present, in such a case HWMOD enabling of
	 *    clock would be failure with default parent. And timer
	 *    probe thinks clock is already enabled, this leads to
	 *    crash upon accessing timer 3 & 6 registers in probe.
	 *    Fix by setting parent of both these timers to master
	 *    oscillator clock.
	 */

	clk1 = clk_get_sys(NULL, "sys_clkin_ck");
	clk2 = clk_get_sys(NULL, "timer3_fck");
	clk_set_parent(clk2, clk1);

	clk2 = clk_get_sys(NULL, "timer6_fck");
	clk_set_parent(clk2, clk1);
	/*
	 * The On-Chip 32K RC Osc clock is not an accurate clock-source as per
	 * the design/spec, so as a result, for example, timer which supposed
	 * to get expired @60Sec, but will expire somewhere ~@40Sec, which is
	 * not expected by any use-case, so change WDT1 clock source to PRCM
	 * 32KHz clock.
	 */
	clk1 = clk_get_sys(NULL, "wdt1_fck");
	clk2 = clk_get_sys(NULL, "clkdiv32k_ick");
	clk_set_parent(clk1, clk2);

	return 0;
}
