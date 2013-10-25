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

/**
 * of_mux_clk_setup() - Setup function for simple mux rate clock
 */
static int of_mux_clk_setup(struct device_node *node, struct regmap *regmap)
{
	struct clk *clk;
	const char *clk_name = node->name;
	void __iomem *reg;
	int num_parents;
	const char **parent_names;
	int i;
	u8 clk_mux_flags = 0;
	u32 mask = 0;
	u32 shift = 0;
	u32 flags = 0;
	u32 val;

	num_parents = of_clk_get_parent_count(node);
	if (num_parents < 1) {
		pr_err("%s: mux-clock %s must have parent(s)\n",
		       __func__, node->name);
		return -EINVAL;
	}
	parent_names = kzalloc((sizeof(char *) * num_parents), GFP_KERNEL);
	if (!parent_names) {
		pr_err("%s: memory alloc failed\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < num_parents; i++)
		parent_names[i] = of_clk_get_parent_name(node, i);

	of_property_read_u32(node, "reg", &val);
	reg = (void *)val;

	if (of_property_read_u32(node, "ti,bit-shift", &shift)) {
		pr_debug("%s: bit-shift property defaults to 0x%x for %s\n",
			 __func__, shift, node->name);
	}

	if (of_property_read_bool(node, "ti,index-starts-at-one"))
		clk_mux_flags |= CLK_MUX_INDEX_ONE;

	if (of_property_read_bool(node, "ti,set-rate-parent"))
		flags |= CLK_SET_RATE_PARENT;

	/* Generate bit-mask based on parent info */
	mask = num_parents;
	if (!(clk_mux_flags & CLK_MUX_INDEX_ONE))
		mask--;

	mask = (1 << fls(mask)) - 1;

	clk = clk_register_mux_table_regmap(NULL, clk_name, parent_names,
					    num_parents, flags, reg, regmap,
					    shift, mask, clk_mux_flags, NULL,
					    NULL);

	if (!IS_ERR(clk)) {
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
		return 0;
	}

	return PTR_ERR(clk);
}
CLK_OF_DECLARE(mux_clk, "ti,mux-clock", of_mux_clk_setup);

static int __init of_ti_composite_mux_clk_setup(struct device_node *node,
						struct regmap *regmap)
{
	struct clk_mux *mux;
	int num_parents;
	int ret;
	u32 val;

	mux = kzalloc(sizeof(*mux), GFP_KERNEL);
	if (!mux)
		return -ENOMEM;

	of_property_read_u32(node, "reg", &val);

	mux->reg = (void *)val;
	mux->regmap = regmap;

	if (of_property_read_u32(node, "ti,bit-shift", &val)) {
		pr_debug("%s: no bit-shift for %s, default=0\n",
			 __func__, node->name);
		val = 0;
	}
	mux->shift = val;

	num_parents = of_clk_get_parent_count(node);

	mux->mask = num_parents - 1;
	mux->mask = (1 << fls(mux->mask)) - 1;

	ret = ti_clk_add_component(node, &mux->hw, CLK_COMPONENT_TYPE_MUX);
	if (!ret)
		return 0;

	kfree(mux);
	return -ret;
}
CLK_OF_DECLARE(ti_composite_mux_clk_setup, "ti,composite-mux-clock",
	       of_ti_composite_mux_clk_setup);
