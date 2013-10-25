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

static struct clk_div_table
__init *ti_clk_get_div_table(struct device_node *node)
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

	num_div /= 4;

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
		of_property_read_u32_index(node, "ti,dividers", i, &val);
		if (val) {
			table[valid_div].div = val;
			table[valid_div].val = i;
			valid_div++;
		}
	}

	return table;
}

static int _get_divider_width(struct device_node *node,
			      const struct clk_div_table *table,
			      u8 flags)
{
	u32 min_div;
	u32 max_div;
	u32 val = 0;
	u32 div;

	if (!table) {
		/* Clk divider table not provided, determine min/max divs */
		if (of_property_read_u32(node, "ti,min-div", &min_div)) {
			pr_debug("%s: no min-div for %s, default=1\n",
				 __func__, node->name);
			min_div = 1;
		}

		if (of_property_read_u32(node, "ti,max-div", &max_div)) {
			pr_err("%s: no max-div for %s!\n", __func__,
			       node->name);
			return -EINVAL;
		}

		/* Determine bit width for the field */
		if (flags & CLK_DIVIDER_ONE_BASED)
			val = 1;

		div = min_div;

		while (div < max_div) {
			if (flags & CLK_DIVIDER_POWER_OF_TWO)
				div <<= 1;
			else
				div++;
			val++;
		}
	} else {
		div = 0;

		while (table[div].div) {
			val = table[div].val;
			div++;
		}
	}

	return fls(val);
}

/**
 * of_ti_divider_clk_setup() - Setup function for simple div rate clock
 */
static int __init of_ti_divider_clk_setup(struct device_node *node,
					  struct regmap *regmap)
{
	struct clk *clk;
	const char *clk_name = node->name;
	void __iomem *reg;
	const char *parent_name;
	u8 clk_divider_flags = 0;
	u8 width = 0;
	u8 shift = 0;
	struct clk_div_table *table = NULL;
	u32 val = 0;
	u32 flags = 0;
	int ret = 0;

	parent_name = of_clk_get_parent_name(node, 0);

	if (of_property_read_u32(node, "reg", &val)) {
		pr_err("%s: %s must have reg\n", __func__, clk_name);
		return -EINVAL;
	}

	reg = (void *)val;

	if (!of_property_read_u32(node, "ti,bit-shift", &val))
		shift = val;

	if (of_property_read_bool(node, "ti,index-starts-at-one"))
		clk_divider_flags |= CLK_DIVIDER_ONE_BASED;

	if (of_property_read_bool(node, "ti,index-power-of-two"))
		clk_divider_flags |= CLK_DIVIDER_POWER_OF_TWO;

	if (of_property_read_bool(node, "ti,set-rate-parent"))
		flags |= CLK_SET_RATE_PARENT;

	table = ti_clk_get_div_table(node);

	if (IS_ERR(table))
		return PTR_ERR(table);

	ret = _get_divider_width(node, table, clk_divider_flags);
	if (ret < 0)
		goto cleanup;

	width = ret;

	clk = clk_register_divider_table_regmap(NULL, clk_name, parent_name,
						flags, reg, regmap, shift,
						width, clk_divider_flags, table,
						NULL);

	if (!IS_ERR(clk)) {
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
		ret = of_ti_autoidle_setup(node, regmap);
	} else {
		ret = PTR_ERR(clk);
	}

	if (!ret)
		return 0;
cleanup:
	kfree(table);
	return ret;
}
CLK_OF_DECLARE(divider_clk, "ti,divider-clock", of_ti_divider_clk_setup);

static int __init of_ti_composite_divider_clk_setup(struct device_node *node,
						     struct regmap *regmap)
{
	struct clk_divider *div;
	u32 val;
	int ret;

	div = kzalloc(sizeof(*div), GFP_KERNEL);
	if (!div)
		return -ENOMEM;

	of_property_read_u32(node, "reg", &val);
	div->reg = (void *)val;
	div->regmap = regmap;

	div->table = ti_clk_get_div_table(node);

	if (of_property_read_bool(node, "ti,index-starts-at-one"))
		div->flags |= CLK_DIVIDER_ONE_BASED;

	ret = _get_divider_width(node, div->table, div->flags);
	if (ret < 0)
		goto cleanup;

	div->width = ret;

	if (of_property_read_u32(node, "ti,bit-shift", &val)) {
		pr_debug("%s: missing bit-shift property for %s, default=0\n",
			 __func__, node->name);
		val = 0;
	}
	div->shift = val;

	ret = ti_clk_add_component(node, &div->hw, CLK_COMPONENT_TYPE_DIVIDER);
	if (!ret)
		return 0;

cleanup:
	kfree(div);
	return ret;
}
CLK_OF_DECLARE(ti_composite_divider_clk, "ti,composite-divider-clock",
	       of_ti_composite_divider_clk_setup);
