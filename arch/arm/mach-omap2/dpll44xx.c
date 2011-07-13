/*
 * OMAP4-specific DPLL control functions
 *
 * Copyright (C) 2011 Texas Instruments, Inc.
 * Rajendra Nayak
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/bitops.h>
#include <linux/delay.h>

#include <plat/cpu.h>
#include <plat/clock.h>

#include "clock.h"
#include "clock44xx.h"
#include "cm-regbits-44xx.h"
#include "cm1_44xx.h"
#include "cm2_44xx.h"
#include "prcm44xx.h"
#include "cminst44xx.h"
#include "cm44xx.h"

#define MAX_DPLL_WAIT_TRIES	1000000

struct dpll_reg {
	u16 offset;
	u32 val;
};

struct omap4_dpll_regs {
	char *name;
	u32 mod_partition;
	u32 mod_inst;
	struct dpll_reg clkmode;
	struct dpll_reg autoidle;
	struct dpll_reg idlest;
	struct dpll_reg clksel;
	struct dpll_reg div_m2;
	struct dpll_reg div_m3;
	struct dpll_reg div_m4;
	struct dpll_reg div_m5;
	struct dpll_reg div_m6;
	struct dpll_reg div_m7;
	struct dpll_reg clkdcoldo;
};

static struct omap4_dpll_regs dpll_regs[] = {
	/* MPU DPLL */
	{ .name		= "mpu",
	  .mod_partition = OMAP4430_CM1_PARTITION,
	  .mod_inst	= OMAP4430_CM1_CKGEN_INST,
	  .clkmode	= {.offset = OMAP4_CM_CLKMODE_DPLL_MPU_OFFSET},
	  .autoidle	= {.offset = OMAP4_CM_AUTOIDLE_DPLL_MPU_OFFSET},
	  .idlest	= {.offset = OMAP4_CM_IDLEST_DPLL_MPU_OFFSET},
	  .clksel	= {.offset = OMAP4_CM_CLKSEL_DPLL_MPU_OFFSET},
	  .div_m2	= {.offset = OMAP4_CM_DIV_M2_DPLL_MPU_OFFSET},
	},
	/* IVA DPLL */
	{ .name		= "iva",
	  .mod_partition = OMAP4430_CM1_PARTITION,
	  .mod_inst	= OMAP4430_CM1_CKGEN_INST,
	  .clkmode	= {.offset = OMAP4_CM_CLKMODE_DPLL_IVA_OFFSET},
	  .autoidle	= {.offset = OMAP4_CM_AUTOIDLE_DPLL_IVA_OFFSET},
	  .idlest	= {.offset = OMAP4_CM_IDLEST_DPLL_IVA_OFFSET},
	  .clksel	= {.offset = OMAP4_CM_CLKSEL_DPLL_IVA_OFFSET},
	  .div_m4	= {.offset = OMAP4_CM_DIV_M4_DPLL_IVA_OFFSET},
	  .div_m5	= {.offset = OMAP4_CM_DIV_M5_DPLL_IVA_OFFSET},
	},
	/* ABE DPLL */
	{ .name		= "abe",
	  .mod_partition = OMAP4430_CM1_PARTITION,
	  .mod_inst	= OMAP4430_CM1_CKGEN_INST,
	  .clkmode	= {.offset = OMAP4_CM_CLKMODE_DPLL_ABE_OFFSET},
	  .autoidle	= {.offset = OMAP4_CM_AUTOIDLE_DPLL_ABE_OFFSET},
	  .idlest	= {.offset = OMAP4_CM_IDLEST_DPLL_ABE_OFFSET},
	  .clksel	= {.offset = OMAP4_CM_CLKSEL_DPLL_ABE_OFFSET},
	  .div_m2	= {.offset = OMAP4_CM_DIV_M2_DPLL_ABE_OFFSET},
	  .div_m3	= {.offset = OMAP4_CM_DIV_M3_DPLL_ABE_OFFSET},
	},
	/* USB DPLL */
	{ .name		= "usb",
	  .mod_partition = OMAP4430_CM2_PARTITION,
	  .mod_inst	= OMAP4430_CM2_CKGEN_INST,
	  .clkmode	= {.offset = OMAP4_CM_CLKMODE_DPLL_USB_OFFSET},
	  .autoidle	= {.offset = OMAP4_CM_AUTOIDLE_DPLL_USB_OFFSET},
	  .idlest	= {.offset = OMAP4_CM_IDLEST_DPLL_USB_OFFSET},
	  .clksel	= {.offset = OMAP4_CM_CLKSEL_DPLL_USB_OFFSET},
	  .div_m2	= {.offset = OMAP4_CM_DIV_M2_DPLL_USB_OFFSET},
	  .clkdcoldo	= {.offset = OMAP4_CM_CLKDCOLDO_DPLL_USB_OFFSET},
	 },
	/* PER DPLL */
	{ .name		= "per",
	  .mod_partition = OMAP4430_CM2_PARTITION,
	  .mod_inst	= OMAP4430_CM2_CKGEN_INST,
	  .clkmode	= {.offset = OMAP4_CM_CLKMODE_DPLL_PER_OFFSET},
	  .autoidle	= {.offset = OMAP4_CM_AUTOIDLE_DPLL_PER_OFFSET},
	  .idlest	= {.offset = OMAP4_CM_IDLEST_DPLL_PER_OFFSET},
	  .clksel	= {.offset = OMAP4_CM_CLKSEL_DPLL_PER_OFFSET},
	  .div_m2	= {.offset = OMAP4_CM_DIV_M2_DPLL_PER_OFFSET},
	  .div_m3	= {.offset = OMAP4_CM_DIV_M3_DPLL_PER_OFFSET},
	  .div_m4	= {.offset = OMAP4_CM_DIV_M4_DPLL_PER_OFFSET},
	  .div_m5	= {.offset = OMAP4_CM_DIV_M5_DPLL_PER_OFFSET},
	  .div_m6	= {.offset = OMAP4_CM_DIV_M6_DPLL_PER_OFFSET},
	  .div_m7	= {.offset = OMAP4_CM_DIV_M7_DPLL_PER_OFFSET},
	},
};

/* Supported only on OMAP4 */
int omap4_dpllmx_gatectrl_read(struct clk *clk)
{
	u32 v;
	u32 mask;

	if (!clk || !clk->clksel_reg || !cpu_is_omap44xx())
		return -EINVAL;

	mask = clk->flags & CLOCK_CLKOUTX2 ?
			OMAP4430_DPLL_CLKOUTX2_GATE_CTRL_MASK :
			OMAP4430_DPLL_CLKOUT_GATE_CTRL_MASK;

	v = __raw_readl(clk->clksel_reg);
	v &= mask;
	v >>= __ffs(mask);

	return v;
}

void omap4_dpllmx_allow_gatectrl(struct clk *clk)
{
	u32 v;
	u32 mask;

	if (!clk || !clk->clksel_reg || !cpu_is_omap44xx())
		return;

	mask = clk->flags & CLOCK_CLKOUTX2 ?
			OMAP4430_DPLL_CLKOUTX2_GATE_CTRL_MASK :
			OMAP4430_DPLL_CLKOUT_GATE_CTRL_MASK;

	v = __raw_readl(clk->clksel_reg);
	/* Clear the bit to allow gatectrl */
	v &= ~mask;
	__raw_writel(v, clk->clksel_reg);
}

void omap4_dpllmx_deny_gatectrl(struct clk *clk)
{
	u32 v;
	u32 mask;

	if (!clk || !clk->clksel_reg || !cpu_is_omap44xx())
		return;

	mask = clk->flags & CLOCK_CLKOUTX2 ?
			OMAP4430_DPLL_CLKOUTX2_GATE_CTRL_MASK :
			OMAP4430_DPLL_CLKOUT_GATE_CTRL_MASK;

	v = __raw_readl(clk->clksel_reg);
	/* Set the bit to deny gatectrl */
	v |= mask;
	__raw_writel(v, clk->clksel_reg);
}

const struct clkops clkops_omap4_dpllmx_ops = {
	.allow_idle	= omap4_dpllmx_allow_gatectrl,
	.deny_idle	= omap4_dpllmx_deny_gatectrl,
};

/**
 * omap4_dpll_regm4xen_recalc - compute DPLL rate, considering REGM4XEN bit
 * @clk: struct clk * of the DPLL to compute the rate for
 *
 * Compute the output rate for the OMAP4 DPLL represented by @clk.
 * Takes the REGM4XEN bit into consideration, which is needed for the
 * OMAP4 ABE DPLL.  Returns the DPLL's output rate (before M-dividers)
 * upon success, or 0 upon error.
 */
unsigned long omap4_dpll_regm4xen_recalc(struct clk *clk)
{
	u32 v;
	unsigned long rate;
	struct dpll_data *dd;

	if (!clk || !clk->dpll_data)
		return 0;

	dd = clk->dpll_data;

	rate = omap2_get_dpll_rate(clk);

	/* regm4xen adds a multiplier of 4 to DPLL calculations */
	v = __raw_readl(dd->control_reg);
	if (v & OMAP4430_DPLL_REGM4XEN_MASK)
		rate *= OMAP4430_REGM4XEN_MULT;

	return rate;
}

/**
 * omap4_dpll_regm4xen_round_rate - round DPLL rate, considering REGM4XEN bit
 * @clk: struct clk * of the DPLL to round a rate for
 * @target_rate: the desired rate of the DPLL
 *
 * Compute the rate that would be programmed into the DPLL hardware
 * for @clk if set_rate() were to be provided with the rate
 * @target_rate.  Takes the REGM4XEN bit into consideration, which is
 * needed for the OMAP4 ABE DPLL.  Returns the rounded rate (before
 * M-dividers) upon success, -EINVAL if @clk is null or not a DPLL, or
 * ~0 if an error occurred in omap2_dpll_round_rate().
 */
long omap4_dpll_regm4xen_round_rate(struct clk *clk, unsigned long target_rate)
{
	u32 v;
	struct dpll_data *dd;
	long r;

	if (!clk || !clk->dpll_data)
		return -EINVAL;

	dd = clk->dpll_data;

	/* regm4xen adds a multiplier of 4 to DPLL calculations */
	v = __raw_readl(dd->control_reg) & OMAP4430_DPLL_REGM4XEN_MASK;

	if (v)
		target_rate = target_rate / OMAP4430_REGM4XEN_MULT;

	r = omap2_dpll_round_rate(clk, target_rate);
	if (r == ~0)
		return r;

	if (v)
		clk->dpll_data->last_rounded_rate *= OMAP4430_REGM4XEN_MULT;

	return clk->dpll_data->last_rounded_rate;
}

/**
 * omap4_dpll_read_reg - reads DPLL register value
 * @dpll_reg: DPLL register to read
 *
 * Reads the value of a single DPLL register.
 */
static inline u32 omap4_dpll_read_reg(struct omap4_dpll_regs *dpll_reg,
				      struct dpll_reg *tuple)
{
	if (tuple->offset)
		return omap4_cminst_read_inst_reg(dpll_reg->mod_partition,
						  dpll_reg->mod_inst,
						  tuple->offset);
	return 0;
}

/**
 * omap4_dpll_store_reg - stores DPLL register value to memory location
 * @dpll_reg: DPLL register to save
 * @tuple: save address
 *
 * Saves a single DPLL register content to memory location defined by
 * @tuple before entering device off mode.
 */
static inline void omap4_dpll_store_reg(struct omap4_dpll_regs *dpll_reg,
					struct dpll_reg *tuple)
{
	tuple->val = omap4_dpll_read_reg(dpll_reg, tuple);
}

/**
 * omap4_dpll_prepare_off - stores DPLL settings before off mode
 *
 * Saves all DPLL register settings. This must be called before
 * entering device off.
 */
void omap4_dpll_prepare_off(void)
{
	u32 i;
	struct omap4_dpll_regs *dpll_reg = dpll_regs;

	for (i = 0; i < ARRAY_SIZE(dpll_regs); i++, dpll_reg++) {
		omap4_dpll_store_reg(dpll_reg, &dpll_reg->clkmode);
		omap4_dpll_store_reg(dpll_reg, &dpll_reg->autoidle);
		omap4_dpll_store_reg(dpll_reg, &dpll_reg->clksel);
		omap4_dpll_store_reg(dpll_reg, &dpll_reg->div_m2);
		omap4_dpll_store_reg(dpll_reg, &dpll_reg->div_m3);
		omap4_dpll_store_reg(dpll_reg, &dpll_reg->div_m4);
		omap4_dpll_store_reg(dpll_reg, &dpll_reg->div_m5);
		omap4_dpll_store_reg(dpll_reg, &dpll_reg->div_m6);
		omap4_dpll_store_reg(dpll_reg, &dpll_reg->div_m7);
		omap4_dpll_store_reg(dpll_reg, &dpll_reg->clkdcoldo);
		omap4_dpll_store_reg(dpll_reg, &dpll_reg->idlest);
	}
}

/**
 * omap4_dpll_print_reg - dump out a single DPLL register value
 * @dpll_reg: register to dump
 * @name: name of the register
 * @tuple: content of the register
 *
 * Helper dump function to print out a DPLL register value in case
 * of restore failures.
 */
static void omap4_dpll_print_reg(struct omap4_dpll_regs *dpll_reg, char *name,
				 struct dpll_reg *tuple)
{
	if (tuple->offset)
		pr_warn("%s - offset = 0x%04x, value = 0x%08x\n", name,
			tuple->offset, tuple->val);
}

/*
 * omap4_dpll_dump_regs - dump out DPLL registers
 * @dpll_reg: DPLL to dump
 *
 * Dump out the contents of the registers for a DPLL. Called if a
 * restore for DPLL fails to lock.
 */
static void omap4_dpll_dump_regs(struct omap4_dpll_regs *dpll_reg)
{
	pr_warn("%s: Unable to lock dpll %s[part=%x inst=%x]:\n",
		__func__, dpll_reg->name, dpll_reg->mod_partition,
		dpll_reg->mod_inst);
	omap4_dpll_print_reg(dpll_reg, "clksel", &dpll_reg->clksel);
	omap4_dpll_print_reg(dpll_reg, "div_m2", &dpll_reg->div_m2);
	omap4_dpll_print_reg(dpll_reg, "div_m3", &dpll_reg->div_m3);
	omap4_dpll_print_reg(dpll_reg, "div_m4", &dpll_reg->div_m4);
	omap4_dpll_print_reg(dpll_reg, "div_m5", &dpll_reg->div_m5);
	omap4_dpll_print_reg(dpll_reg, "div_m6", &dpll_reg->div_m6);
	omap4_dpll_print_reg(dpll_reg, "div_m7", &dpll_reg->div_m7);
	omap4_dpll_print_reg(dpll_reg, "clkdcoldo", &dpll_reg->clkdcoldo);
	omap4_dpll_print_reg(dpll_reg, "clkmode", &dpll_reg->clkmode);
	omap4_dpll_print_reg(dpll_reg, "autoidle", &dpll_reg->autoidle);
	if (dpll_reg->idlest.offset)
		pr_warn("idlest[0x%04x] - before val = 0x%08x"
			" after val = 0x%08x\n", dpll_reg->idlest.offset,
			dpll_reg->idlest.val,
			omap4_dpll_read_reg(dpll_reg, &dpll_reg->idlest));
}

/**
 * omap4_wait_dpll_lock - wait for a DPLL lock
 * @dpll_reg: DPLL to wait for
 *
 * Waits for a DPLL lock after restore.
 */
static void omap4_wait_dpll_lock(struct omap4_dpll_regs *dpll_reg)
{
	int j = 0;
	u32 status;

	/* Return if we dont need to lock. */
	if ((dpll_reg->clkmode.val & OMAP4430_DPLL_EN_MASK) !=
	     DPLL_LOCKED << OMAP4430_DPLL_EN_SHIFT)
		return;

	while (1) {
		status = (omap4_dpll_read_reg(dpll_reg, &dpll_reg->idlest)
			  & OMAP4430_ST_DPLL_CLK_MASK)
			 >> OMAP4430_ST_DPLL_CLK_SHIFT;
		if (status == 0x1)
			break;
		if (j == MAX_DPLL_WAIT_TRIES) {
			/* If we are unable to lock, warn and move on.. */
			omap4_dpll_dump_regs(dpll_reg);
			break;
		}
		j++;
		udelay(1);
	}
}

/**
 * omap4_dpll_restore_reg - restores a single register for a DPLL
 * @dpll_reg: DPLL to restore
 * @tuple: register value to restore
 *
 * Restores a single register for a DPLL.
 */
static inline void omap4_dpll_restore_reg(struct omap4_dpll_regs *dpll_reg,
					  struct dpll_reg *tuple)
{
	if (tuple->offset)
		omap4_cminst_write_inst_reg(tuple->val, dpll_reg->mod_partition,
					    dpll_reg->mod_inst, tuple->offset);
}

/**
 * omap4_dpll_resume_off - restore DPLL settings after device off
 *
 * Restores all DPLL settings. Must be called after wakeup from device
 * off.
 */
void omap4_dpll_resume_off(void)
{
	u32 i;
	struct omap4_dpll_regs *dpll_reg = dpll_regs;

	for (i = 0; i < ARRAY_SIZE(dpll_regs); i++, dpll_reg++) {
		omap4_dpll_restore_reg(dpll_reg, &dpll_reg->clksel);
		omap4_dpll_restore_reg(dpll_reg, &dpll_reg->div_m2);
		omap4_dpll_restore_reg(dpll_reg, &dpll_reg->div_m3);
		omap4_dpll_restore_reg(dpll_reg, &dpll_reg->div_m4);
		omap4_dpll_restore_reg(dpll_reg, &dpll_reg->div_m5);
		omap4_dpll_restore_reg(dpll_reg, &dpll_reg->div_m6);
		omap4_dpll_restore_reg(dpll_reg, &dpll_reg->div_m7);
		omap4_dpll_restore_reg(dpll_reg, &dpll_reg->clkdcoldo);

		/* Restore clkmode after the above registers are restored */
		omap4_dpll_restore_reg(dpll_reg, &dpll_reg->clkmode);

		omap4_wait_dpll_lock(dpll_reg);

		/* Restore autoidle settings after the dpll is locked */
		omap4_dpll_restore_reg(dpll_reg, &dpll_reg->autoidle);
	}
}
