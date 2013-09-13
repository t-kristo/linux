/*
 * TI Divider Clock
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

struct clk_div_table __init *ti_clk_get_div_table(struct device_node *node)
{
	struct clk_div_table *table;
	const __be32 *divspec;
	u32 val;
	u32 num_div;
	u32 valid_div;
	int i;

	divspec = of_get_property(node, "ti,dividers", &num_div);

	if (!divspec)
		return NULL;

	valid_div = 0;

	/* Determine required size for divider table */
	for (i = 0; i < num_div; i++) {
		of_property_read_u32_index(node, "ti,dividers", i, &val);
		if (val)
			valid_div++;
	}

	if (!valid_div) {
		pr_err("%s: no valid dividers for %s table\n", __func__,
		       node->name);
		return ERR_PTR(-EINVAL);
	}

	table = kzalloc(sizeof(*table) * (valid_div + 1), GFP_KERNEL);

	if (!table) {
		pr_err("%s: unable to allocate memory for %s table\n", __func__,
		       node->name);
		return ERR_PTR(-ENOMEM);
	}

	valid_div = 0;

	for (i = 0; i < num_div; i++) {
		of_property_read_u32_index(node, "dividers", i, &val);
		if (val) {
			table[valid_div].div = val;
			table[valid_div].val = i;
			valid_div++;
		}
	}

	return table;
}

/**
 * of_ti_divider_clk_setup() - Setup function for simple div rate clock
 */
static void __init of_ti_divider_clk_setup(struct device_node *node)
{
	struct clk *clk;
	const char *clk_name = node->name;
	void __iomem *reg;
	const char *parent_name;
	u8 clk_divider_flags = 0;
	u32 width = 0;
	u32 shift = 0;
	struct clk_div_table *table;
	int min_div, max_div, div;
	u32 val = 0;
	u32 flags = 0;

	parent_name = of_clk_get_parent_name(node, 0);

	reg = of_iomap(node, 0);
	if (!reg) {
		pr_err("%s: no memory mapped for property reg\n", __func__);
		return;
	}

	of_property_read_u32(node, "ti,bit-shift", &shift);

	if (of_property_read_bool(node, "ti,index-starts-at-one"))
		clk_divider_flags |= CLK_DIVIDER_ONE_BASED;

	if (of_property_read_bool(node, "ti,index-power-of-two"))
		clk_divider_flags |= CLK_DIVIDER_POWER_OF_TWO;

	if (of_property_read_bool(node, "ti,set-rate-parent"))
		flags |= CLK_SET_RATE_PARENT;

	table = ti_clk_get_div_table(node);

	if (IS_ERR(table))
		return;

	if (!table) {
		/* Clk divider table not provided, determine min/max divs */
		if (of_property_read_u32(node, "ti,min-div", &min_div)) {
			pr_debug("%s: ti,min-div not declared for %s, defaulting to 1\n", __func__,
				 node->name);
			min_div = 1;
		}

		if (of_property_read_u32(node, "ti,max-div", &max_div)) {
			pr_err("%s: ti,max-div not declared for %s\n", __func__,
			       node->name);
			return;
		}

		/* Determine bit width for the field */
		if (clk_divider_flags & CLK_DIVIDER_ONE_BASED)
			val = 1;

		div = min_div;

		while (div < max_div) {
			if (clk_divider_flags & CLK_DIVIDER_POWER_OF_TWO)
				div <<= 1;
			else
				div++;
			val++;
		}
	} else {
		div = 0;

		while (table[div].val) {
			val = table[div].val;
			div++;
		}
	}

	width = fls(val);

	clk = clk_register_divider_table(NULL, clk_name, parent_name, flags,
					 reg, shift, width, clk_divider_flags,
					 table, NULL);

	if (!IS_ERR(clk)) {
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
		of_ti_autoidle_setup(node);
	}
}
CLK_OF_DECLARE(divider_clk, "ti,divider-clock", of_ti_divider_clk_setup);
