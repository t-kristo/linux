/*
 * OMAP Power Management debug routines
 *
 * Copyright (C) 2005 Texas Instruments, Inc.
 * Copyright (C) 2006-2008 Nokia Corporation
 *
 * Written by:
 * Richard Woodruff <r-woodruff2@ti.com>
 * Tony Lindgren
 * Juha Yrjola
 * Amit Kucheria <amit.kucheria@nokia.com>
 * Igor Stoppa <igor.stoppa@nokia.com>
 * Jouni Hogander
 *
 * Based on pm.c for omap2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/slab.h>

#include <plat/clock.h>
#include <plat/board.h>
#include "powerdomain.h"
#include "clockdomain.h"
#include <plat/dmtimer.h>
#include <plat/omap-pm.h>

#include "cm2xxx_3xxx.h"
#include "prcm44xx.h"
#include "prcm_mpu44xx.h"
#include "prm2xxx_3xxx.h"
#include "prm44xx.h"
#include "prminst44xx.h"
#include "cminst44xx.h"
#include "cm1_44xx.h"
#include "cm2_44xx.h"
#include "pm.h"

u32 enable_off_mode;

#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#include <linux/seq_file.h>

static int pm_dbg_init_done;

static int pm_dbg_init(void);

#define PM_DBG_MAX_REG_SETS 4

struct dentry *pm_dbg_dir;
static void *pm_dbg_reg_set[PM_DBG_MAX_REG_SETS];

static void pm_dbg_regset_store(u32 *ptr);

enum {
	DEBUG_FILE_COUNTERS = 0,
	DEBUG_FILE_TIMERS,
};

static const char pwrdm_state_names[][PWRDM_MAX_FUNC_PWRSTS] = {
	"OFF",
	"OSWR",
	"CSWR",
	"INA",
	"ON"
};

struct pm_module_def {
	char *name; /* Name of the module */
	short type; /* CM or PRM */
	unsigned short offset;
	int low; /* First register address on this module */
	int high; /* Last register address on this module */
};

enum {
	MOD_CM = 0,
	MOD_PRM,
	MOD_PRM_OMAP4,
	MOD_PRCM_MPU_OMAP4,
	MOD_CM1_OMAP4,
	MOD_CM2_OMAP4
};

static const struct pm_module_def pm_dbg_reg_modules[] = {
	/* CM1 */
	{ "OCP", MOD_CM1_OMAP4, OMAP4430_CM1_OCP_SOCKET_INST, 0, 0xf0 },
	{ "CKGEN", MOD_CM1_OMAP4, OMAP4430_CM1_CKGEN_INST, 0, 0x280 },
	{ "MPU", MOD_CM1_OMAP4, OMAP4430_CM1_MPU_INST, 0, 0x20 },
	{ "DSP", MOD_CM1_OMAP4, OMAP4430_CM1_TESLA_INST, 0, 0x20 },
	{ "ABE", MOD_CM1_OMAP4, OMAP4430_CM1_ABE_INST, 0, 0x88 },
	{ "REST", MOD_CM1_OMAP4, OMAP4430_CM1_RESTORE_INST, 0, 0x58 },
	/*{ "INSTR", MOD_CM1_OMAP4, OMAP4430_CM1_INSTR_INST, 0, 0x30 },*/

	/* CM2 */
	{ "OCP", MOD_CM2_OMAP4, OMAP4430_CM2_OCP_SOCKET_INST, 0, 0xf0 },
	{ "CKGEN", MOD_CM2_OMAP4, OMAP4430_CM2_CKGEN_INST, 0, 0xec },
	{ "ALWON", MOD_CM2_OMAP4, OMAP4430_CM2_ALWAYS_ON_INST, 0, 0x40 },
	{ "CORE", MOD_CM2_OMAP4, OMAP4430_CM2_CORE_INST, 0, 0x740 },
	{ "IVAHD", MOD_CM2_OMAP4, OMAP4430_CM2_IVAHD_INST, 0, 0x20 },
	{ "CAM", MOD_CM2_OMAP4, OMAP4430_CM2_CAM_INST, 0, 0x28 },
	{ "DSS", MOD_CM2_OMAP4, OMAP4430_CM2_DSS_INST, 0, 0x28 },
	{ "SGX", MOD_CM2_OMAP4, OMAP4430_CM2_GFX_INST, 0, 0x20 },
	{ "L3INIT", MOD_CM2_OMAP4, OMAP4430_CM2_L3INIT_INST, 0, 0xe0 },
	{ "L4PER", MOD_CM2_OMAP4, OMAP4430_CM2_L4PER_INST, 0, 0x1d8 },
	{ "REST", MOD_CM2_OMAP4, OMAP4430_CM2_RESTORE_INST, 0, 0x5c },
	/*{ "INSTR", MOD_CM2_OMAP4, OMAP4430_CM2_INSTR_INST, 0, 0x30 },*/

	/* PRM */
	{ "OCP", MOD_PRM_OMAP4, OMAP4430_PRM_OCP_SOCKET_INST, 0, 0xf0 },
	{ "CKGEN", MOD_PRM_OMAP4, OMAP4430_PRM_CKGEN_INST, 0, 0x10 },
	{ "MPU", MOD_PRM_OMAP4, OMAP4430_PRM_MPU_INST, 0, 0x24 },
	{ "DSP", MOD_PRM_OMAP4, OMAP4430_PRM_TESLA_INST, 0, 0x24 },
	{ "ABE", MOD_PRM_OMAP4, OMAP4430_PRM_ABE_INST, 0, 0x8c },
	{ "ALWON", MOD_PRM_OMAP4, OMAP4430_PRM_ALWAYS_ON_INST, 0x28, 0x3c },
	{ "CORE", MOD_PRM_OMAP4, OMAP4430_PRM_CORE_INST, 0, 0x744 },
	{ "IVAHD", MOD_PRM_OMAP4, OMAP4430_PRM_IVAHD_INST, 0, 0x2c },
	{ "CAM", MOD_PRM_OMAP4, OMAP4430_PRM_CAM_INST, 0, 0x2c },
	{ "DSS", MOD_PRM_OMAP4, OMAP4430_PRM_DSS_INST, 0, 0x24 },
	{ "SGX", MOD_PRM_OMAP4, OMAP4430_PRM_GFX_INST, 0, 0x24 },
	{ "L3INIT", MOD_PRM_OMAP4, OMAP4430_PRM_L3INIT_INST, 0, 0xe4 },
	{ "L4PER", MOD_PRM_OMAP4, OMAP4430_PRM_L4PER_INST, 0, 0x1dc },
	{ "WKUP", MOD_PRM_OMAP4, OMAP4430_PRM_WKUP_INST, 0x24, 0x84 },
	{ "WKUP_CM", MOD_PRM_OMAP4, OMAP4430_PRM_WKUP_CM_INST, 0, 0x88 },
	{ "EMU", MOD_PRM_OMAP4, OMAP4430_PRM_EMU_INST, 0, 0x24 },
	{ "EMU_CM", MOD_PRM_OMAP4, OMAP4430_PRM_EMU_CM_INST, 0, 0x20 },
	{ "DEVICE", MOD_PRM_OMAP4, OMAP4430_PRM_DEVICE_INST, 0, 0xf8 },
	/*{ "INSTR", MOD_PRM_OMAP4, OMAP4430_PRM_INSTR_INST, 0, 0x30 },*/

	/* OMAP4 MPU PRCM partition */
	{ "PRM", MOD_PRCM_MPU_OMAP4, OMAP4430_PRCM_MPU_DEVICE_PRM_INST, 0, 0x4c },
	{ "CPU0", MOD_PRCM_MPU_OMAP4, OMAP4430_PRCM_MPU_CPU0_INST, 0, 0x4c },
	{ "CPU1", MOD_PRCM_MPU_OMAP4, OMAP4430_PRCM_MPU_CPU1_INST, 0, 0x4c },
	{ NULL, 0, 0, 0, 0 },
};

static int pm_dbg_get_regset_size(void)
{
	static int regset_size;
	const struct pm_module_def *mod;

	if (regset_size == 0) {
		mod = pm_dbg_reg_modules;
		while (mod->name != NULL) {
			regset_size += mod->high + 4 - mod->low;
			mod++;
		}
	}
	return regset_size;
}

#define DBG_PRINTF(s, fmt, args...) {		\
	if (s) seq_printf(s, fmt, ## args);	\
	else pr_info(fmt, ## args);		\
	}

void pm_dbg_print_regs(struct seq_file *s, int reg_set)
{
	int i;
	unsigned long val;
	u32 *ptr;
	void *store = NULL;
	int regs;
	int linefeed;
	const struct pm_module_def *mod;
	u32 offset;
	char *type_name;

	if (reg_set == 0) {
		store = kmalloc(pm_dbg_get_regset_size(), GFP_KERNEL);
		ptr = store;
		pm_dbg_regset_store(ptr);
	} else {
		ptr = pm_dbg_reg_set[reg_set - 1];
	}
	mod = pm_dbg_reg_modules;
	offset = 0;
	type_name = NULL;

	while (mod->name != NULL) {
		regs = 0;
		linefeed = 0;
		switch (mod->type) {
		case MOD_CM:
			type_name = "INV";
			break;
		case MOD_PRM:
			type_name = "INV";
			break;
		case MOD_CM1_OMAP4:
			offset = OMAP4430_CM1_BASE;
			type_name = "CM1";
			break;
		case MOD_CM2_OMAP4:
			offset = OMAP4430_CM2_BASE;
			type_name = "CM2";
			break;
		case MOD_PRCM_MPU_OMAP4:
			offset = OMAP4430_PRCM_MPU_BASE;
			type_name = "PRCM_MPU";
			break;
		case MOD_PRM_OMAP4:
			offset = OMAP4430_PRM_BASE;
			type_name = "PRM";
			break;
		}

		DBG_PRINTF(s, "MOD: %s_%s (%08x)\n",
			type_name, mod->name, offset + mod->offset);

		for (i = mod->low; i <= mod->high; i += 4) {
			val = *(ptr++);
			if (val != 0) {
				regs++;

				if (linefeed) {
					DBG_PRINTF(s, "\n");
					linefeed = 0;
				}
				DBG_PRINTF(s, "  %02x => %08lx", i, val);
				if (regs % 4 == 0)
					linefeed = 1;
			}
		}
		DBG_PRINTF(s, "\n");
		mod++;
	}

	if (store != NULL)
		kfree(store);
}

static int pm_dbg_show_regs(struct seq_file *s, void *unused)
{
        int reg_set = (int)s->private;

	pm_dbg_print_regs(s, reg_set);

	return 0;
}

static void pm_dbg_regset_store(u32 *ptr)
{
	int i;
	u32 val;
	const struct pm_module_def *mod;

	mod = pm_dbg_reg_modules;

	val = 0;
	while (mod->name != NULL) {
		for (i = mod->low; i <= mod->high; i += 4) {
			switch (mod->type) {
			case MOD_CM:
				val = omap2_cm_read_mod_reg(
					mod->offset, i);
				break;
			case MOD_PRM:
				val = omap2_prm_read_mod_reg(
					mod->offset, i);
				break;
			case MOD_CM1_OMAP4:
				val = omap4_cminst_read_inst_reg(
					OMAP4430_CM1_PARTITION,
					mod->offset, i);
				break;
			case MOD_CM2_OMAP4:
				val = omap4_cminst_read_inst_reg(
					OMAP4430_CM2_PARTITION,
					mod->offset, i);
				break;
			case MOD_PRM_OMAP4:
				val = omap4_prminst_read_inst_reg(
					OMAP4430_PRM_PARTITION,
					mod->offset, i);
				break;
			case MOD_PRCM_MPU_OMAP4:
				val = omap4_prminst_read_inst_reg(
					OMAP4430_PRCM_MPU_PARTITION,
					mod->offset, i);
				break;
			}
			*(ptr++) = val;
		}
		mod++;
	}
}

int pm_dbg_regset_save(int reg_set)
{
	if (pm_dbg_reg_set[reg_set-1] == NULL)
		return -EINVAL;

	pm_dbg_regset_store(pm_dbg_reg_set[reg_set-1]);

	return 0;
}

static int pm_dbg_reg_open(struct inode *inode, struct file *file)
{
	return single_open(file, pm_dbg_show_regs, inode->i_private);
}

static const struct file_operations debug_reg_fops = {
	.open		= pm_dbg_reg_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

int pm_dbg_regset_init(int reg_set)
{
	char name[2];

	if (reg_set < 1 || reg_set > PM_DBG_MAX_REG_SETS ||
		pm_dbg_reg_set[reg_set-1] != NULL)
		return -EINVAL;

	pm_dbg_reg_set[reg_set-1] =
		kmalloc(pm_dbg_get_regset_size(), GFP_KERNEL);

	if (pm_dbg_reg_set[reg_set-1] == NULL)
		return -ENOMEM;

	if (pm_dbg_dir != NULL) {
		sprintf(name, "%d", reg_set);

		(void) debugfs_create_file(name, S_IRUGO,
			pm_dbg_dir, (void *)reg_set, &debug_reg_fops);
	}

	return 0;
}


void pm_dbg_update_time(struct powerdomain *pwrdm, int prev)
{
	s64 t;

	if (!pm_dbg_init_done)
		return ;

	/* Update timer for previous state */
	t = sched_clock();

	pwrdm->state_timer[prev] += t - pwrdm->timer;

	pwrdm->timer = t;
}

static int clkdm_dbg_show_counter(struct clockdomain *clkdm, void *user)
{
	struct seq_file *s = (struct seq_file *)user;

	if (strcmp(clkdm->name, "emu_clkdm") == 0 ||
		strcmp(clkdm->name, "wkup_clkdm") == 0 ||
		strncmp(clkdm->name, "dpll", 4) == 0)
		return 0;

	seq_printf(s, "%s->%s (%d)", clkdm->name,
			clkdm->pwrdm.ptr->name,
			atomic_read(&clkdm->usecount));
	seq_printf(s, "\n");

	return 0;
}

static int pwrdm_dbg_show_counter(struct powerdomain *pwrdm, void *user)
{
	struct seq_file *s = (struct seq_file *)user;
	int i;

	if (strcmp(pwrdm->name, "emu_pwrdm") == 0 ||
		strcmp(pwrdm->name, "wkup_pwrdm") == 0 ||
		strncmp(pwrdm->name, "dpll", 4) == 0)
		return 0;

	if (pwrdm->state != pwrdm_read_func_pwrst(pwrdm))
		printk(KERN_ERR "pwrdm state mismatch(%s) %d != %d\n",
		       pwrdm->name, pwrdm->state, pwrdm_read_func_pwrst(pwrdm));

	seq_printf(s, "%s (%s)", pwrdm->name,
			pwrdm_state_names[pwrdm->state]);
	for (i = 0; i < PWRDM_MAX_FUNC_PWRSTS; i++)
		seq_printf(s, ",%s:%d", pwrdm_state_names[i],
			pwrdm->state_counter[i]);

	seq_printf(s, ",RET-LOGIC-OFF:%d", pwrdm->ret_logic_off_counter);
	for (i = 0; i < pwrdm->banks; i++)
		seq_printf(s, ",RET-MEMBANK%d-OFF:%d", i + 1,
				pwrdm->ret_mem_off_counter[i]);

	seq_printf(s, "\n");

	return 0;
}

static int pwrdm_dbg_show_timer(struct powerdomain *pwrdm, void *user)
{
	struct seq_file *s = (struct seq_file *)user;
	int i;

	if (strcmp(pwrdm->name, "emu_pwrdm") == 0 ||
		strcmp(pwrdm->name, "wkup_pwrdm") == 0 ||
		strncmp(pwrdm->name, "dpll", 4) == 0)
		return 0;

	pwrdm_state_switch(pwrdm);

	seq_printf(s, "%s (%s)", pwrdm->name,
		pwrdm_state_names[pwrdm->state]);

	for (i = 0; i < PWRDM_MAX_FUNC_PWRSTS; i++)
		seq_printf(s, ",%s:%lld", pwrdm_state_names[i],
			pwrdm->state_timer[i]);

	seq_printf(s, "\n");
	return 0;
}

static int pm_dbg_show_counters(struct seq_file *s, void *unused)
{
	pwrdm_for_each(pwrdm_dbg_show_counter, s);
	clkdm_for_each(clkdm_dbg_show_counter, s);

	return 0;
}

static int pm_dbg_show_timers(struct seq_file *s, void *unused)
{
	pwrdm_for_each(pwrdm_dbg_show_timer, s);
	return 0;
}

static int pm_dbg_open(struct inode *inode, struct file *file)
{
	switch ((int)inode->i_private) {
	case DEBUG_FILE_COUNTERS:
		return single_open(file, pm_dbg_show_counters,
			&inode->i_private);
	case DEBUG_FILE_TIMERS:
	default:
		return single_open(file, pm_dbg_show_timers,
			&inode->i_private);
	};
}

static const struct file_operations debug_fops = {
	.open           = pm_dbg_open,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.release        = single_release,
};

static int pwrdm_suspend_get(void *data, u64 *val)
{
	int ret = -EINVAL;

	if (cpu_is_omap34xx())
		ret = omap3_pm_get_suspend_state((struct powerdomain *)data);
	*val = ret;

	if (ret >= 0)
		return 0;
	return *val;
}

static int pwrdm_suspend_set(void *data, u64 val)
{
	if (cpu_is_omap34xx())
		return omap3_pm_set_suspend_state(
			(struct powerdomain *)data, (int)val);
	return -EINVAL;
}

DEFINE_SIMPLE_ATTRIBUTE(pwrdm_suspend_fops, pwrdm_suspend_get,
			pwrdm_suspend_set, "%llu\n");

static int __init pwrdms_setup(struct powerdomain *pwrdm, void *dir)
{
	int i;
	s64 t;
	struct dentry *d;

	t = sched_clock();

	for (i = 0; i < PWRDM_MAX_FUNC_PWRSTS; i++)
		pwrdm->state_timer[i] = 0;

	pwrdm->timer = t;

	if (strncmp(pwrdm->name, "dpll", 4) == 0)
		return 0;

	d = debugfs_create_dir(pwrdm->name, (struct dentry *)dir);
	if (!(IS_ERR_OR_NULL(d)))
		(void) debugfs_create_file("suspend", S_IRUGO|S_IWUSR, d,
			(void *)pwrdm, &pwrdm_suspend_fops);

	return 0;
}

static int option_get(void *data, u64 *val)
{
	u32 *option = data;

	*val = *option;

	return 0;
}

static int option_set(void *data, u64 val)
{
	u32 *option = data;

	*option = val;

	if (option == &enable_off_mode) {
		if (val)
			omap_pm_enable_off_mode();
		else
			omap_pm_disable_off_mode();
		if (cpu_is_omap34xx())
			omap3_pm_off_mode_enable(val);
	}

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(pm_dbg_option_fops, option_get, option_set, "%llu\n");

static int __init pm_dbg_init(void)
{
	struct dentry *d;
	int i;
	char buf[8];

	if (pm_dbg_init_done)
		return 0;

	d = debugfs_create_dir("pm_debug", NULL);
	if (IS_ERR_OR_NULL(d))
		return PTR_ERR(d);

	(void) debugfs_create_file("count", S_IRUGO,
		d, (void *)DEBUG_FILE_COUNTERS, &debug_fops);
	(void) debugfs_create_file("time", S_IRUGO,
		d, (void *)DEBUG_FILE_TIMERS, &debug_fops);

	pwrdm_for_each(pwrdms_setup, (void *)d);

	(void) debugfs_create_file("enable_off_mode", S_IRUGO | S_IWUSR, d,
				   &enable_off_mode, &pm_dbg_option_fops);

	pm_dbg_dir = debugfs_create_dir("registers", d);
	if (IS_ERR(pm_dbg_dir))
		return PTR_ERR(pm_dbg_dir);

	(void) debugfs_create_file("current", S_IRUGO,
		pm_dbg_dir, (void *)0, &debug_reg_fops);

	for (i = 0; i < PM_DBG_MAX_REG_SETS; i++)
		if (pm_dbg_reg_set[i] != NULL) {
			sprintf(buf, "%d", i+1);
			(void) debugfs_create_file(buf, S_IRUGO,
				pm_dbg_dir, (void *)(i+1), &debug_reg_fops);
	}

	pm_dbg_init_done = 1;

	return 0;
}
arch_initcall(pm_dbg_init);

#endif
