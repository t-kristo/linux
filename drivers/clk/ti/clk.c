/*
 * TI clock support
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
#include <linux/clkdev.h>
#include <linux/clk/ti.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/list.h>

#undef pr_fmt
#define pr_fmt(fmt) "%s: " fmt, __func__

extern struct of_device_id __clk_of_table[];
static int ti_dt_clk_regmap_index;

/**
 * ti_dt_clocks_register - register DT alias clocks during boot
 * @oclks: list of clocks to register
 *
 * Register alias or non-standard DT clock entries during boot. By
 * default, DT clocks are found based on their node name. If any
 * additional con-id / dev-id -> clock mapping is required, use this
 * function to list these.
 */
void __init ti_dt_clocks_register(struct ti_dt_clk oclks[])
{
	struct ti_dt_clk *c;
	struct device_node *node;
	struct clk *clk;
	struct of_phandle_args clkspec;

	for (c = oclks; c->node_name != NULL; c++) {
		node = of_find_node_by_name(NULL, c->node_name);
		clkspec.np = node;
		clk = of_clk_get_from_provider(&clkspec);

		if (!IS_ERR(clk)) {
			c->lk.clk = clk;
			clkdev_add(&c->lk);
		} else {
			pr_warn("failed to lookup clock node %s\n",
				c->node_name);
		}
	}
}

typedef int (*ti_of_clk_init_cb_t)(struct device_node *);

struct clk_init_item {
	int index;
	struct device_node *np;
	ti_of_clk_init_cb_t init_cb;
	struct list_head node;
};

static LIST_HEAD(retry_list);

/**
 * ti_clk_get_reg_addr - get register address for a clock register
 * @node: device node for the clock
 * @index: register index from the clock node
 *
 * Builds clock register address from device tree information. This
 * is a struct of type clk_omap_reg.
 */
void *ti_clk_get_reg_addr(struct device_node *node, int index)
{
	struct clk_omap_reg *reg;
	u32 val;
	u32 tmp;

	reg = (struct clk_omap_reg *)&tmp;
	reg->index = ti_dt_clk_regmap_index;

	if (of_property_read_u32_index(node, "reg", index, &val)) {
		pr_err("%s must have reg[%d]!\n", node->name, index);
		return NULL;
	}

	reg->offset = val;

	return (void *)tmp;
}

/**
 * ti_dt_clk_init_provider - init master clock provider
 * @parent: master node
 * @ops: struct clk_reg_ops * for accessing the clock registers
 * @index: internal index for clk_reg_ops
 *
 * Initializes a master clock IP block and its child clock nodes.
 * Regmap is provided for accessing the register space for the
 * IP block and all the clocks under it.
 */
void __init ti_dt_clk_init_provider(struct device_node *parent,
				    struct clk_reg_ops *ops,
				    int index)
{
	const struct of_device_id *match;
	struct device_node *np;
	ti_of_clk_init_cb_t clk_init_cb;
	struct clk_init_item *retry;
	struct clk_init_item *tmp;
	int ret;

	ti_dt_clk_regmap_index = index;

	for_each_child_of_node(parent, np) {
		match = of_match_node(__clk_of_table, np);
		if (!match)
			continue;
		clk_init_cb = match->data;
		pr_debug("%s: initializing: %s\n", __func__, np->name);
		ret = clk_init_cb(np);
		if (ret == -EAGAIN) {
			pr_debug("%s: adding to again list...\n", np->name);
			retry = kzalloc(sizeof(*retry), GFP_KERNEL);
			retry->np = np;
			retry->init_cb = clk_init_cb;
			list_add(&retry->node, &retry_list);
		} else if (ret) {
			pr_err("%s: clock init failed for %s (%d)!\n", __func__,
			       np->name, ret);
		}
	}

	list_for_each_entry_safe(retry, tmp, &retry_list, node) {
		pr_debug("%s: retry-init: %s\n", __func__, retry->np->name);
		ti_dt_clk_regmap_index = retry->index;
		ret = retry->init_cb(retry->np);
		if (ret == -EAGAIN) {
			pr_debug("%s failed again?\n", retry->np->name);
		} else {
			if (ret)
				pr_err("%s: clock init failed for %s (%d)!\n",
				       __func__, retry->np->name, ret);
			list_del(&retry->node);
			kfree(retry);
		}
	}
}
