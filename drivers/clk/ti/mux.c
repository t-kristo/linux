/*
 * TI Multiplexer Clock
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
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/clk/ti.h>

#undef pr_fmt
#define pr_fmt(fmt) "%s: " fmt, __func__

/**
 * of_mux_clk_setup - Setup function for simple mux rate clock
 * @node: DT node for the clock
 *
 * Sets up a basic clock multiplexer.
 */
static int of_mux_clk_setup(struct device_node *node)
{
	struct clk *clk;
	void __iomem *reg;
	int num_parents;
	const char **parent_names;
	int i;
	u8 clk_mux_flags = 0;
	u32 mask = 0;
	u32 shift = 0;
	u32 flags = 0;
	int ret;

	num_parents = of_clk_get_parent_count(node);
	if (num_parents < 2) {
		pr_err("mux-clock %s must have parents\n", node->name);
		return -EINVAL;
	}
	parent_names = kzalloc((sizeof(char *) * num_parents), GFP_KERNEL);
	if (!parent_names)
		return -ENOMEM;

	for (i = 0; i < num_parents; i++)
		parent_names[i] = of_clk_get_parent_name(node, i);

	reg = ti_clk_get_reg_addr(node, 0);

	if (!reg) {
		ret = -EINVAL;
		goto cleanup;
	}

	of_property_read_u32(node, "ti,bit-shift", &shift);

	if (of_property_read_bool(node, "ti,index-starts-at-one"))
		clk_mux_flags |= CLK_MUX_INDEX_ONE;

	if (of_property_read_bool(node, "ti,set-rate-parent"))
		flags |= CLK_SET_RATE_PARENT;

	/* Generate bit-mask based on parent info */
	mask = num_parents;
	if (!(clk_mux_flags & CLK_MUX_INDEX_ONE))
		mask--;

	mask = (1 << fls(mask)) - 1;

	clk = clk_register_mux_table(NULL, node->name, parent_names,
				     num_parents, flags, reg, shift, mask,
				     clk_mux_flags, NULL, NULL);

	if (!IS_ERR(clk)) {
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
		return 0;
	}
	ret = PTR_ERR(clk);

cleanup:
	kfree(parent_names);
	return ret;
}
CLK_OF_DECLARE(mux_clk, "ti,mux-clock", of_mux_clk_setup);

static int __init of_ti_composite_mux_clk_setup(struct device_node *node)
{
	struct clk_mux *mux;
	int num_parents;
	int ret;
	u32 val;

	mux = kzalloc(sizeof(*mux), GFP_KERNEL);
	if (!mux)
		return -ENOMEM;

	mux->reg = ti_clk_get_reg_addr(node, 0);

	if (!mux->reg) {
		ret = -EINVAL;
		goto cleanup;
	}

	if (!of_property_read_u32(node, "ti,bit-shift", &val)) {
		mux->shift = val;
	}

	if (of_property_read_bool(node, "ti,index-starts-at-one"))
		mux->flags |= CLK_MUX_INDEX_ONE;

	num_parents = of_clk_get_parent_count(node);

	if (num_parents < 2) {
		pr_err("%s must have parents\n", node->name);
		ret = -EINVAL;
		goto cleanup;
	}

	mux->mask = num_parents - 1;
	mux->mask = (1 << fls(mux->mask)) - 1;

	ret = ti_clk_add_component(node, &mux->hw, CLK_COMPONENT_TYPE_MUX);
	if (!ret)
		return 0;

cleanup:
	kfree(mux);
	return ret;
}
CLK_OF_DECLARE(ti_composite_mux_clk_setup, "ti,composite-mux-clock",
	       of_ti_composite_mux_clk_setup);
