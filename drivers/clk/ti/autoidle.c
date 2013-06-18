/*
 * TI clock autoidle support
 *
 * Copyright (C) 2013 Texas Instruments, Inc.
 *
 * Tero Kristo <t-kristo@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/clk-provider.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/clk/ti.h>

struct clk_ti_autoidle {
	u32			reg;
	struct clk_reg_ops	*reg_ops;
	u8			shift;
	u8			flags;
	const char		*name;
	struct list_head	node;
};

#define AUTOIDLE_LOW		0x1

static LIST_HEAD(autoidle_clks);

static void ti_allow_autoidle(struct clk_ti_autoidle *clk)
{
	u32 val;

	val = clk->reg_ops->clk_readl(clk->reg);

	if (clk->flags & AUTOIDLE_LOW)
		val &= ~(1 << clk->shift);
	else
		val |= (1 << clk->shift);

	clk->reg_ops->clk_writel(val, clk->reg);
}

static void ti_deny_autoidle(struct clk_ti_autoidle *clk)
{
	u32 val;

	val = clk->reg_ops->clk_readl(clk->reg);

	if (clk->flags & AUTOIDLE_LOW)
		val |= (1 << clk->shift);
	else
		val &= ~(1 << clk->shift);

	clk->reg_ops->clk_writel(val, clk->reg);
}

void of_ti_clk_allow_autoidle_all(void)
{
	struct clk_ti_autoidle *c;

	list_for_each_entry(c, &autoidle_clks, node)
		ti_allow_autoidle(c);
}

void of_ti_clk_deny_autoidle_all(void)
{
	struct clk_ti_autoidle *c;

	list_for_each_entry(c, &autoidle_clks, node)
		ti_deny_autoidle(c);
}

int __init of_ti_autoidle_setup(struct device_node *node,
				 struct clk_reg_ops *ops)
{
	u32 shift;
	struct clk_ti_autoidle *clk;

	if (of_property_read_u32(node, "ti,autoidle-shift", &shift))
		return 0;

	clk = kzalloc(sizeof(*clk), GFP_KERNEL);

	if (!clk) {
		pr_err("%s: kzalloc failed\n", __func__);
		return -ENOMEM;
	}

	clk->shift = shift;
	clk->name = node->name;
	of_property_read_u32(node, "reg", &clk->reg);
	clk->reg_ops = ops;

	if (of_property_read_bool(node, "ti,autoidle-low"))
		clk->flags |= AUTOIDLE_LOW;

	list_add(&clk->node, &autoidle_clks);

	return 0;
}
