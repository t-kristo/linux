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
#include <linux/reset.h>
#include <linux/of.h>
#include <linux/uaccess.h>

#include "clock.h"
#include "powerdomain.h"
#include "clockdomain.h"
#include "omap-pm.h"
#include "omap_hwmod.h"

#include "soc.h"
#include "cm2xxx_3xxx.h"
#include "prm2xxx_3xxx.h"
#include "pm.h"

u32 enable_off_mode;

#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#include <linux/seq_file.h>

static int pm_dbg_init_done;

static int pm_dbg_init(void);

enum {
	DEBUG_FILE_COUNTERS = 0,
	DEBUG_FILE_TIMERS,
};

static const char pwrdm_state_names[][PWRDM_MAX_PWRSTS] = {
	"OFF",
	"RET",
	"INA",
	"ON"
};

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

	seq_printf(s, "%s->%s (%d)\n", clkdm->name, clkdm->pwrdm.ptr->name,
		   clkdm->usecount);

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

	if (pwrdm->state != pwrdm_read_pwrst(pwrdm))
		printk(KERN_ERR "pwrdm state mismatch(%s) %d != %d\n",
			pwrdm->name, pwrdm->state, pwrdm_read_pwrst(pwrdm));

	seq_printf(s, "%s (%s)", pwrdm->name,
			pwrdm_state_names[pwrdm->state]);
	for (i = 0; i < PWRDM_MAX_PWRSTS; i++)
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

	for (i = 0; i < 4; i++)
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
	}
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

	for (i = 0; i < 4; i++)
		pwrdm->state_timer[i] = 0;

	pwrdm->timer = t;

	if (strncmp(pwrdm->name, "dpll", 4) == 0)
		return 0;

	d = debugfs_create_dir(pwrdm->name, (struct dentry *)dir);
	if (d)
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

static int hwmod_get(void *data, u64 *val)
{
	*val = 0;

	return 0;
}

static int hwmod_set(void *data, u64 val)
{
	struct omap_hwmod *oh = data;

	if (val)
		omap_hwmod_enable(oh);
	else
		omap_hwmod_idle(oh);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(hwmod_control_fops, hwmod_get, hwmod_set, "%llu\n");

static int hwmod_reset_get(void *data, u64 *val)
{
	*val = 0;
	return 0;
}

static int hwmod_reset_set(void *data, u64 val)
{
	struct omap_hwmod *oh = data;
	int i;

	if (val)
		for (i = 0; i < oh->rst_lines_cnt; i++)
			omap_hwmod_assert_hardreset(oh, oh->rst_lines[i].name);
	else
		for (i = 0; i < oh->rst_lines_cnt; i++) {
			omap_hwmod_deassert_hardreset(oh,
						      oh->rst_lines[i].name);
		}

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(hwmod_reset_fops, hwmod_reset_get, hwmod_reset_set,
			"%llu\n");

static int reset_get(void *data, u64 *val)
{
	struct reset_control *rc = data;

	*val = reset_control_status(rc);

	return 0;
}

static int reset_set(void *data, u64 val)
{
	struct reset_control *rc = data;

	if (val)
		reset_control_assert(rc);
	else
		reset_control_deassert(rc);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(reset_control_fops, reset_get, reset_set, "%llu\n");

static int reset_signal_show(struct seq_file *s, void *unused)
{
	return 0;
}

static int reset_create_open(struct inode *inode, struct file *file)
{
	return single_open(file, reset_signal_show, inode->i_private);
}

static ssize_t reset_create_write(struct file *file,
				  const char __user *user_buf,
				  size_t count, loff_t *ppos)
{
	char *buf;
	struct omap_hwmod *oh;
	struct reset_control *reset;
	const char *reset_name;
	const char *oh_name;
	int i;
	int ret;
	struct dentry *d = ((struct seq_file *)file->private_data)->private;
	struct device_node *np;
	int idx;
	char *c;

	buf = kmalloc(count, GFP_KERNEL);

	ret = copy_from_user(buf, user_buf, count);
	if (ret)
		goto cleanup;

	buf[count - 1] = 0;

	c = buf;
	idx = 0;

	while (*c) {
		if (*c == ':') {
			*c = 0;
			c++;
			ret = kstrtoint(c, 10, &idx);
			if (ret) {
				pr_err("%s: bad index: %s\n", __func__, c);
				goto cleanup;
			}
			break;
		}
		c++;
	}

	pr_info("%s: looking up %s:%d...\n", __func__, buf, idx);

	np = NULL;

	while (idx >= 0) {
		np = of_find_node_by_name(np, buf);
		idx--;
		if (!np)
			break;
	}

	if (!np) {
		pr_err("%s: node %s not found\n", __func__, buf);
		ret = -EINVAL;
		goto cleanup;
	}

	of_property_read_string_index(np, "ti,hwmods", 0, &oh_name);

	oh = omap_hwmod_lookup(oh_name);
	if (!oh) {
		pr_err("%s: hwmod %s for %s not found\n", __func__,
		       oh_name, buf);
		ret = -EINVAL;
		goto cleanup;
	}

	d = debugfs_create_dir(oh_name, d);

	debugfs_create_file("hwmod_enable", S_IRUGO | S_IWUSR, d, (void *)oh,
			    &hwmod_control_fops);

	debugfs_create_file("hwmod_hardreset", S_IRUGO | S_IWUSR, d,
			    (void *)oh, &hwmod_reset_fops);

	i = 0;

	while (1) {
		ret = of_property_read_string_index(np, "reset-names", i,
						    &reset_name);
		if (ret) {
			ret = 0;
			pr_info("%s: created %s\n", __func__, oh_name);
			goto cleanup;
		}

		reset = of_reset_control_get(np, reset_name);
		if (IS_ERR(reset)) {
			ret = PTR_ERR(reset);
			goto cleanup;
		}

		debugfs_create_file(reset_name, S_IRUGO | S_IWUSR, d,
				    (void *)reset, &reset_control_fops);
		i++;
	}

cleanup:
	kfree(buf);

	if (ret)
		return ret;

	return count;
}

static const struct file_operations reset_create_fops = {
	.open		= reset_create_open,
	.read		= seq_read,
	.write		= reset_create_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init pm_dbg_init(void)
{
	struct dentry *d;

	if (pm_dbg_init_done)
		return 0;

	d = debugfs_create_dir("pm_debug", NULL);
	if (!d)
		return -EINVAL;

	(void) debugfs_create_file("count", S_IRUGO,
		d, (void *)DEBUG_FILE_COUNTERS, &debug_fops);
	(void) debugfs_create_file("time", S_IRUGO,
		d, (void *)DEBUG_FILE_TIMERS, &debug_fops);

	pwrdm_for_each(pwrdms_setup, (void *)d);

	(void) debugfs_create_file("enable_off_mode", S_IRUGO | S_IWUSR, d,
				   &enable_off_mode, &pm_dbg_option_fops);

	d = debugfs_create_dir("resets", d);

	pr_info("%s: resets-dir=%08x\n", __func__, (u32)d);

	(void)debugfs_create_file("create", S_IRUGO | S_IWUSR, d,
				  d, &reset_create_fops);

	pm_dbg_init_done = 1;

	return 0;
}
omap_arch_initcall(pm_dbg_init);

#endif
