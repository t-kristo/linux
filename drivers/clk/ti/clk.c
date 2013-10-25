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
#include <linux/list.h>

extern struct of_device_id __clk_of_table[];

/**
 * ti_dt_clocks_register - register DT duplicate clocks during boot
 * @oclks: list of clocks to register
 *
 * Register duplicate or non-standard DT clock entries during boot. By
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
			pr_warn("%s: failed to lookup clock node %s\n",
				__func__, c->node_name);
		}
	}
}

typedef int (*ti_of_clk_init_cb_t)(struct device_node *, struct regmap *);

struct clk_init_item {
	struct regmap *regmap;
	struct device_node *np;
	ti_of_clk_init_cb_t init_cb;
	struct list_head node;
};

static LIST_HEAD(retry_list);

void __init ti_dt_clk_init_provider(struct device_node *parent,
				    struct regmap *regmap)
{
	const struct of_device_id *match;
	struct device_node *np;
	ti_of_clk_init_cb_t clk_init_cb;
	struct clk_init_item *retry;
	struct clk_init_item *tmp;
	int ret;

	for_each_child_of_node(parent, np) {
		match = of_match_node(__clk_of_table, np);
		if (!match)
			continue;
		clk_init_cb = match->data;
		pr_debug("%s: initializing: %s\n", __func__, np->name);
		ret = clk_init_cb(np, regmap);
		if (ret == -EAGAIN) {
			pr_debug("%s: adding to again list...\n", np->name);
			retry = kzalloc(sizeof(*retry), GFP_KERNEL);
			retry->np = np;
			retry->regmap = regmap;
			retry->init_cb = clk_init_cb;
			list_add(&retry->node, &retry_list);
		} else if (ret) {
			pr_err("%s: clock init failed for %s (%d)!\n", __func__,
			       np->name, ret);
		}
	}

	list_for_each_entry_safe(retry, tmp, &retry_list, node) {
		pr_debug("%s: retry-init: %s\n", __func__, retry->np->name);
		ret = retry->init_cb(retry->np, retry->regmap);
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
