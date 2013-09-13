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
static void of_mux_clk_setup(struct device_node *node)
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

	num_parents = of_clk_get_parent_count(node);
	if (num_parents < 1) {
		pr_err("%s: mux-clock %s must have parent(s)\n",
		       __func__, node->name);
		return;
	}
	parent_names = kzalloc((sizeof(char *) * num_parents), GFP_KERNEL);

	for (i = 0; i < num_parents; i++)
		parent_names[i] = of_clk_get_parent_name(node, i);

	reg = of_iomap(node, 0);
	if (!reg) {
		pr_err("%s: no memory mapped for property reg\n", __func__);
		return;
	}

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

	clk = clk_register_mux_table(NULL, clk_name, parent_names, num_parents,
				     flags, reg, shift, mask, clk_mux_flags,
				     NULL, NULL);

	if (!IS_ERR(clk))
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
}
CLK_OF_DECLARE(mux_clk, "ti,mux-clock", of_mux_clk_setup);
