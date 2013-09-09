/*
 * TI composite clock support
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
#include <linux/list.h>

#define to_clk_divider(_hw) container_of(_hw, struct clk_divider, hw)

static unsigned long ti_composite_recalc_rate(struct clk_hw *hw,
					      unsigned long parent_rate)
{
	return clk_divider_ops.recalc_rate(hw, parent_rate);
}

static long ti_composite_round_rate(struct clk_hw *hw, unsigned long rate,
				    unsigned long *prate)
{
	return -EINVAL;
}

static int ti_composite_set_rate(struct clk_hw *hw, unsigned long rate,
				 unsigned long parent_rate)
{
	return -EINVAL;
}

static const struct clk_ops ti_composite_divider_ops = {
	.recalc_rate	= &ti_composite_recalc_rate,
	.round_rate	= &ti_composite_round_rate,
	.set_rate	= &ti_composite_set_rate,
};

static const struct clk_ops ti_composite_gate_ops = {
	.enable		= &omap2_dflt_clk_enable,
	.disable	= &omap2_dflt_clk_disable,
	.is_enabled	= &omap2_dflt_clk_is_enabled,
};

struct component_clk {
	int num_parents;
	const char **parent_names;
	struct device_node *node;
	int type;
	struct clk_hw *hw;
	struct list_head link;
};

static const char * __initdata component_clk_types[] = {
	"mux", "gate", "divider",
};

static LIST_HEAD(component_clks);

static struct component_clk *_lookup_component(struct device_node *node, int i)
{
	int rc;
	struct of_phandle_args clkspec;
	struct component_clk *comp;

	rc = of_parse_phandle_with_args(node, "clocks", "#clock-cells", i,
					&clkspec);
	if (rc)
		return NULL;

	list_for_each_entry(comp, &component_clks, link) {
		if (comp->node == clkspec.np)
			return comp;
	}
	return NULL;
}

static inline struct clk_hw *_get_hw(struct component_clk *clk)
{
	if (clk)
		return clk->hw;

	return NULL;
}

static int __init of_ti_composite_clk_setup(struct device_node *node,
					    struct clk_reg_ops *reg_ops)
{
	int num_clks;
	int i;
	struct clk *clk;
	struct component_clk *comp;
	struct component_clk *clks[3] = { NULL };
	const char *name = node->name;
	int num_parents = 0;
	const char **parent_names = NULL;
	int ret = 0;

	/* Number of component clocks to be put inside this clock */
	num_clks = of_clk_get_parent_count(node);

	if (num_clks < 1) {
		pr_err("%s: ti composite clk %s must have component(s)\n",
		       __func__, name);
		return -EINVAL;
	}

	/* Check for presence of each component clock */
	for (i = 0; i < num_clks; i++) {
		comp = _lookup_component(node, i);
		if (!comp)
			return -EAGAIN;
		if (clks[comp->type] != NULL) {
			pr_err("%s: duplicate component types for %s (%s)!\n",
			       __func__, name, component_clk_types[comp->type]);
			return -EINVAL;
		}
		clks[comp->type] = comp;
	}

	/* All components exists, proceed with registration */
	for (i = 0; i < 3; i++) {
		if (!clks[i])
			continue;
		if (clks[i]->num_parents) {
			num_parents = clks[i]->num_parents;
			parent_names = clks[i]->parent_names;
		}
	}

	/* Enforce mux parents if exists */
	comp = clks[CLK_COMPONENT_TYPE_MUX];
	if (comp) {
		num_parents = comp->num_parents;
		parent_names = comp->parent_names;
	}

	if (!num_parents) {
		pr_err("%s: no parents found for %s!\n", __func__,
		       name);
		return -EINVAL;
	}

	clk = clk_register_composite(NULL, name, parent_names, num_parents,
				     _get_hw(clks[CLK_COMPONENT_TYPE_MUX]),
				     &clk_mux_ops,
				     _get_hw(clks[CLK_COMPONENT_TYPE_DIVIDER]),
				     &ti_composite_divider_ops,
				     _get_hw(clks[CLK_COMPONENT_TYPE_GATE]),
				     &ti_composite_gate_ops, 0);

	if (!IS_ERR(clk)) {
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
		goto cleanup;
	}

	ret = PTR_ERR(clk);
cleanup:
	/* Free component clock list entries */
	for (i = 0; i < 3; i++) {
		if (!clks[i])
			continue;
		list_del(&clks[i]->link);
		kfree(clks[i]);
	}

	return ret;
}
CLK_OF_DECLARE(ti_composite_clock, "ti,composite-clock",
	       of_ti_composite_clk_setup);

int __init ti_clk_add_component(struct device_node *node, struct clk_hw *hw,
				int type)
{
	int num_parents;
	const char **parent_names;
	struct component_clk *clk;
	int i;

	num_parents = of_clk_get_parent_count(node);

	if (num_parents < 1) {
		pr_err("%s: component-clock %s must have parent(s)\n", __func__,
		       node->name);
		return -EINVAL;
        }

	parent_names = kzalloc((sizeof(char *) * num_parents), GFP_KERNEL);
	if (!parent_names)
		return -ENOMEM;

	for (i = 0; i < num_parents; i++)
		parent_names[i] = of_clk_get_parent_name(node, i);

	clk = kzalloc(sizeof(*clk), GFP_KERNEL);
	if (!clk) {
		kfree(parent_names);
		return -ENOMEM;
	}

	clk->num_parents = num_parents;
	clk->parent_names = parent_names;
	clk->hw = hw;
	clk->node = node;
	clk->type = type;
	list_add(&clk->link, &component_clks);

	return 0;
}
