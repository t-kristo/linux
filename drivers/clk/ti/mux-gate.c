/*
 * OMAP mux-gate clock support
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

#define to_clk_divider(_hw) container_of(_hw, struct clk_divider, hw)

static unsigned long omap_mux_gate_recalc_rate(struct clk_hw *hw,
					unsigned long parent_rate)
{
	return clk_divider_ops.recalc_rate(hw, parent_rate);
}

static long omap_mux_gate_round_rate(struct clk_hw *hw, unsigned long rate,
				unsigned long *prate)
{
	return -EINVAL;
}

static int omap_mux_gate_set_rate(struct clk_hw *hw, unsigned long rate,
				unsigned long parent_rate)
{
	return -EINVAL;
}

static const struct clk_ops omap_mux_gate_divider_ops = {
	.recalc_rate	= &omap_mux_gate_recalc_rate,
	.round_rate	= &omap_mux_gate_round_rate,
	.set_rate	= &omap_mux_gate_set_rate,
};

static const struct clk_ops omap_mux_gate_gate_ops = {
	.enable		= &omap2_dflt_clk_enable,
	.disable	= &omap2_dflt_clk_disable,
	.is_enabled	= &omap2_dflt_clk_is_enabled,
};

static void __init
_of_ti_mux_gate_clk_setup(struct device_node *node,
			  const struct clk_hw_omap_ops *hw_ops)
{
	struct clk *clk;
	const char *clk_name = node->name;
	const char **parent_names = NULL;
	int num_parents;
	struct clk_hw_omap *gate = NULL;
	struct clk_divider *div = NULL;
	struct clk_mux *mux = NULL;
	const struct clk_ops *mux_ops, *div_ops, *gate_ops;
	u32 val;
	int i;
	u32 min_div, max_div, divider;

	of_property_read_string(node, "clock-output-names", &clk_name);

	num_parents = of_clk_get_parent_count(node);

	if (num_parents < 1) {
		pr_err("%s: omap-mux-clock %s must have parent(s)\n", __func__,
		       node->name);
		return;
	}

	parent_names = kzalloc((sizeof(char *) * num_parents), GFP_KERNEL);

	for (i = 0; i < num_parents; i++)
		parent_names[i] = of_clk_get_parent_name(node, i);

	i = of_property_match_string(node, "reg-names", "gate-reg");
	if (i >= 0) {
		gate = kzalloc(sizeof(*gate), GFP_KERNEL);
		gate->enable_reg = of_iomap(node, i);
		if (of_property_read_u32(node, "ti,gate-bit-shift", &val)) {
			pr_err("%s: missing gate-bit-shift property for %s\n",
			       __func__, node->name);
			goto cleanup;
		}
		gate->enable_bit = val;
		gate->ops = hw_ops;

		gate_ops = &omap_mux_gate_gate_ops;
	}

	i = of_property_match_string(node, "reg-names", "mux-reg");
	if (i >= 0) {
		mux = kzalloc(sizeof(*mux), GFP_KERNEL);
		mux->reg = of_iomap(node, i);
		mux_ops = &clk_mux_ops;
		if (of_property_read_u32(node, "ti,mux-bit-shift", &val)) {
			pr_debug("%s: missing mux-bit-shift property for %s, defaulting to 0\n",
				 __func__, node->name);
			val = 0;
		}
		mux->shift = val;

		mux->mask = num_parents - 1;
		mux->mask = (1 << fls(mux->mask)) - 1;

		mux_ops = &clk_mux_ops;
	}

	i = of_property_match_string(node, "reg-names", "div-reg");
	if (i >= 0) {
		div = kzalloc(sizeof(*div), GFP_KERNEL);
		div->reg = of_iomap(node, i);

		div->table = ti_clk_get_div_table(node);

		if (of_property_read_bool(node, "ti,div-index-starts-at-one"))
			div->flags |= CLK_DIVIDER_ONE_BASED;

		if (!div->table) {
			if (of_property_read_u32(node, "ti,min-div",
						 &min_div)) {
				pr_debug("%s: ti,min-div not declared for %s, defaulting to 1\n",
					 __func__, node->name);
				min_div = 1;
			}

			if (of_property_read_u32(node, "ti,max-div",
						 &max_div)) {
				pr_err("%s: ti,max-div not declared for %s\n",
				       __func__, node->name);
				goto cleanup;
			}

			val = 0;

			if (div->flags & CLK_DIVIDER_ONE_BASED)
				val = 1;

			divider = min_div;

			while (divider < max_div) {
				divider++;
				val++;
			}
		} else {
			divider = 0;
			while (div->table[divider].val) {
				val = div->table[divider].val;
				divider++;
			}
		}

		div->width = fls(val);

		if (of_property_read_u32(node, "ti,div-bit-shift", &val)) {
			pr_debug("%s: missing div-bit-shift property for %s, defaulting to 0\n",
				 __func__, node->name);
			val = 0;
		}
		div->shift = val;

		div->table = ti_clk_get_div_table(node);

		div_ops = &omap_mux_gate_divider_ops;
	}

	clk = clk_register_composite(NULL, clk_name,
			parent_names, num_parents,
			mux ? &mux->hw : NULL, mux_ops,
			div ? &div->hw : NULL, div_ops,
			gate ? &gate->hw : NULL, gate_ops, 0);

	if (!IS_ERR(clk))
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
	return;
cleanup:
	kfree(mux);
	kfree(div);
	kfree(gate);
	kfree(parent_names);
}

static void __init of_ti_no_wait_mux_gate_clk_setup(struct device_node *node)
{
	_of_ti_mux_gate_clk_setup(node, NULL);
}
CLK_OF_DECLARE(ti_no_wait_mux_gate_clk, "ti,no-wait-mux-gate-clock",
	       of_ti_no_wait_mux_gate_clk_setup);

static void __init of_ti_interface_mux_gate_clk_setup(struct device_node *node)
{
	_of_ti_mux_gate_clk_setup(node, &clkhwops_iclk_wait);
}
CLK_OF_DECLARE(ti_interface_mux_gate_clk, "ti,interface-mux-gate-clock",
	       of_ti_interface_mux_gate_clk_setup);

static void __init of_ti_mux_gate_clk_setup(struct device_node *node)
{
	_of_ti_mux_gate_clk_setup(node, &clkhwops_wait);
}
CLK_OF_DECLARE(ti_mux_gate_clk, "ti,mux-gate-clock",
	       of_ti_mux_gate_clk_setup);
