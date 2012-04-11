/*
 * OMAP4 Power Management Routines
 *
 * Copyright (C) 2010-2011 Texas Instruments, Inc.
 * Rajendra Nayak <rnayak@ti.com>
 * Santosh Shilimkar <santosh.shilimkar@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/pm.h>
#include <linux/suspend.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <asm/system_misc.h>
#include <linux/io.h>

#include <mach/ctrl_module_wkup_44xx.h>
#include <mach/hardware.h>

#include "common.h"
#include "clockdomain.h"
#include "powerdomain.h"
#include "pm.h"
#include "voltage.h"
#include "prm44xx.h"
#include "prm-regbits-44xx.h"

#define EMIF_SDRAM_CONFIG2_OFFSET	0xc

struct power_state {
	struct powerdomain *pwrdm;
	u32 next_state;
#ifdef CONFIG_SUSPEND
	u32 saved_state;
#endif
	struct list_head node;
};

static LIST_HEAD(pwrst_list);
u16 pm44xx_errata;

#ifdef CONFIG_SUSPEND
static int omap4_pm_suspend(void)
{
	struct power_state *pwrst;
	int state, ret = 0;
	u32 cpu_id = smp_processor_id();

	/* Save current powerdomain state */
	list_for_each_entry(pwrst, &pwrst_list, node) {
		pwrst->saved_state = pwrdm_read_next_func_pwrst(pwrst->pwrdm);
	}

	/* Set targeted power domain states by suspend */
	list_for_each_entry(pwrst, &pwrst_list, node)
		omap_set_pwrdm_state(pwrst->pwrdm, pwrst->next_state);

	/*
	 * For MPUSS to hit power domain retention(CSWR or OSWR),
	 * CPU0 and CPU1 power domains need to be in OFF or DORMANT state,
	 * since CPU power domain CSWR is not supported by hardware
	 * Only master CPU follows suspend path. All other CPUs follow
	 * CPU hotplug path in system wide suspend. On OMAP4, CPU power
	 * domain CSWR is not supported by hardware.
	 * More details can be found in OMAP4430 TRM section 4.3.4.2.
	 */
	omap4_enter_lowpower(cpu_id, PWRDM_FUNC_PWRST_OFF);

	/* Restore next powerdomain state */
	list_for_each_entry(pwrst, &pwrst_list, node) {
		state = pwrdm_read_prev_func_pwrst(pwrst->pwrdm);
		if (state > pwrst->next_state) {
			pr_info("Powerdomain (%s) didn't enter "
			       "target state %d\n",
			       pwrst->pwrdm->name, pwrst->next_state);
			ret = -1;
		}
		omap_set_pwrdm_state(pwrst->pwrdm, pwrst->saved_state);
	}
	if (ret)
		pr_crit("Could not enter target state in pm_suspend\n");
	else
		pr_info("Successfully put all powerdomains to target state\n");

	return 0;
}
#endif /* CONFIG_SUSPEND */

static int __init pwrdms_setup(struct powerdomain *pwrdm, void *unused)
{
	struct power_state *pwrst;

	if (!pwrdm->pwrsts)
		return 0;

	/*
	 * Skip CPU0 and CPU1 power domains. CPU1 is programmed
	 * through hotplug path and CPU0 explicitly programmed
	 * further down in the code path
	 */
	if (!strncmp(pwrdm->name, "cpu", 3))
		return 0;


	pwrst = kmalloc(sizeof(struct power_state), GFP_ATOMIC);
	if (!pwrst)
		return -ENOMEM;

	pwrst->pwrdm = pwrdm;
	pwrst->next_state = pwrdm_get_achievable_func_pwrst(
						pwrdm,
						PWRDM_FUNC_PWRST_OSWR);
	list_add(&pwrst->node, &pwrst_list);

	return omap_set_pwrdm_state(pwrst->pwrdm, pwrst->next_state);
}

/**
 * omap_default_idle - OMAP4 default ilde routine.'
 *
 * Implements OMAP4 memory, IO ordering requirements which can't be addressed
 * with default cpu_do_idle() hook. Used by all CPUs with !CONFIG_CPUIDLE and
 * by secondary CPU with CONFIG_CPUIDLE.
 */
static void omap_default_idle(void)
{
	local_fiq_disable();

	omap_do_wfi();

	local_fiq_enable();
}

/**
 * omap4_pm_init - Init routine for OMAP4 PM
 *
 * Initializes all powerdomain and clockdomain target states
 * and all PRCM settings.
 */
int __init omap4_pm_init(void)
{
	int ret;
	struct clockdomain *emif_clkdm, *mpuss_clkdm, *l3_1_clkdm, *l4wkup;
	struct clockdomain *ducati_clkdm, *l3_2_clkdm, *l4_per_clkdm;
	struct voltagedomain *mpu_voltdm;

	if (omap_rev() == OMAP4430_REV_ES1_0) {
		WARN(1, "Power Management not supported on OMAP4430 ES1.0\n");
		return -ENODEV;
	}

	pr_err("Power Management for TI OMAP4.\n");

	/*
	 * Work around for OMAP443x Errata i632: "LPDDR2 Corruption After OFF
	 * Mode Transition When CS1 Is Used On EMIF":
	 * Overwrite EMIF1/EMIF2
	 * SECURE_EMIF1_SDRAM_CONFIG2_REG
	 * SECURE_EMIF2_SDRAM_CONFIG2_REG
	 */
	if (cpu_is_omap443x()) {
		void __iomem *secure_ctrl_mod;
		void __iomem *emif1;
		void __iomem *emif2;
		u32 val;

		secure_ctrl_mod = ioremap(OMAP4_CTRL_MODULE_WKUP, SZ_4K);
		emif1 = ioremap(OMAP44XX_EMIF1_BASE, SZ_1M);
		emif2 = ioremap(OMAP44XX_EMIF2_BASE, SZ_1M);

		BUG_ON(!secure_ctrl_mod || !emif1 || !emif2);

		val = __raw_readl(emif1 + EMIF_SDRAM_CONFIG2_OFFSET);
		__raw_writel(val, secure_ctrl_mod +
			     OMAP4_CTRL_SECURE_EMIF1_SDRAM_CONFIG2_REG);
		val = __raw_readl(emif2 + EMIF_SDRAM_CONFIG2_OFFSET);
		__raw_writel(val, secure_ctrl_mod +
			     OMAP4_CTRL_SECURE_EMIF2_SDRAM_CONFIG2_REG);
		wmb();
		iounmap(secure_ctrl_mod);
		iounmap(emif1);
		iounmap(emif2);
	}

	ret = pwrdm_for_each(pwrdms_setup, NULL);
	if (ret) {
		pr_err("Failed to setup powerdomains\n");
		goto err2;
	}

	/*
	 * The dynamic dependency between MPUSS -> MEMIF and
	 * MPUSS -> L4_PER/L3_* and DUCATI -> L3_* doesn't work as
	 * expected. The hardware recommendation is to enable static
	 * dependencies for these to avoid system lock ups or random crashes.
	 * The L4 wakeup depedency is added to workaround the OCP sync hardware
	 * BUG with 32K synctimer which lead to incorrect timer value read
	 * from the 32K counter. The BUG applies for GPTIMER1 and WDT2 which
	 * are part of L4 wakeup clockdomain.
	 */
	mpuss_clkdm = clkdm_lookup("mpuss_clkdm");
	emif_clkdm = clkdm_lookup("l3_emif_clkdm");
	l3_1_clkdm = clkdm_lookup("l3_1_clkdm");
	l3_2_clkdm = clkdm_lookup("l3_2_clkdm");
	l4_per_clkdm = clkdm_lookup("l4_per_clkdm");
	l4wkup = clkdm_lookup("l4_wkup_clkdm");
	ducati_clkdm = clkdm_lookup("ducati_clkdm");
	if ((!mpuss_clkdm) || (!emif_clkdm) || (!l3_1_clkdm) || (!l4wkup) ||
		(!l3_2_clkdm) || (!ducati_clkdm) || (!l4_per_clkdm))
		goto err2;

	ret = clkdm_add_wkdep(mpuss_clkdm, emif_clkdm);
	ret |= clkdm_add_wkdep(mpuss_clkdm, l3_1_clkdm);
	ret |= clkdm_add_wkdep(mpuss_clkdm, l3_2_clkdm);
	ret |= clkdm_add_wkdep(mpuss_clkdm, l4_per_clkdm);
	ret |= clkdm_add_wkdep(mpuss_clkdm, l4wkup);
	ret |= clkdm_add_wkdep(ducati_clkdm, l3_1_clkdm);
	ret |= clkdm_add_wkdep(ducati_clkdm, l3_2_clkdm);
	if (ret) {
		pr_err("Failed to add MPUSS -> L3/EMIF/L4PER, DUCATI -> L3 "
				"wakeup dependency\n");
		goto err2;
	}

	/*
	 * XXX: voltage config is not still completely valid for
	 * OMAP4, and this causes crashes on some platform during
	 * device off because voltage transitions for device off
	 * are enabled on reset. Thus, we have to disable the I2C
	 * channel completely in the VOLTCTRL register to avoid
	 * trouble. Remove this once voltconfigs are valid.
	 */
	mpu_voltdm = voltdm_lookup("mpu");
	if (!mpu_voltdm) {
		pr_err("Failed to get MPU voltdm\n");
		goto err2;
	}
	mpu_voltdm->write(OMAP4430_VDD_MPU_I2C_DISABLE_MASK |
			  OMAP4430_VDD_CORE_I2C_DISABLE_MASK |
			  OMAP4430_VDD_IVA_I2C_DISABLE_MASK,
			  OMAP4_PRM_VOLTCTRL_OFFSET);

	ret = omap4_mpuss_init();
	if (ret) {
		pr_err("Failed to initialise OMAP4 MPUSS\n");
		goto err2;
	}

	(void) clkdm_for_each(omap_pm_clkdms_setup, NULL);

	/*
	 * ROM code initializes IVAHD and TESLA clock registers during
	 * secure RAM restore phase on omap4430 EMU/HS devices, thus
	 * IVAHD / TESLA clock registers must be saved / restored
	 * during MPU OSWR / device off.
	 */
	if (omap_rev() >= OMAP4430_REV_ES1_0 &&
	    omap_rev() < OMAP4430_REV_ES2_3 &&
	    omap_type() != OMAP2_DEVICE_TYPE_GP)
		pm44xx_errata |= PM_OMAP4_ROM_IVAHD_TESLA_ERRATUM_xxx;

	/*
	 * Similar to above errata, ROM code modifies L3INSTR clock
	 * registers also and these must be saved / restored during
	 * MPU OSWR / device off.
	 */
	if (omap_type() != OMAP2_DEVICE_TYPE_GP)
		pm44xx_errata |= PM_OMAP4_ROM_L3INSTR_ERRATUM_xxx;

#ifdef CONFIG_SUSPEND
	omap_pm_suspend = omap4_pm_suspend;
#endif

	/* Overwrite the default cpu_do_idle() */
	arm_pm_idle = omap_default_idle;

	omap4_idle_init();

err2:
	return ret;
}
