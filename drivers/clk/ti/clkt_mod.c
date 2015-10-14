/*
 * OMAP hardware module clock support
 *
 * Copyright (C) 2015 Texas Instruments, Inc.
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
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/clk/ti.h>
#include <linux/delay.h>
#include "clock.h"

#undef pr_fmt
#define pr_fmt(fmt) "%s: " fmt, __func__

#define OMAP4_MODULEMODE_MASK		0x3
#define OMAP4_IDLEST_MASK		(0x3 << 16)
#define OMAP4_IDLEST_SHIFT		16

#define CLKCTRL_IDLEST_FUNCTIONAL	0x0
#define CLKCTRL_IDLEST_INTERFACE_IDLE	0x2
#define CLKCTRL_IDLEST_DISABLED		0x3

#define OMAP4_MAX_MODULE_READY_TIME	2000
#define OMAP4_MAX_MODULE_DISABLE_TIME	5000

static u32 _omap4_idlest(u32 val)
{
	val &= OMAP4_IDLEST_MASK;
	val >>= OMAP4_IDLEST_SHIFT;

	return val;
}

static bool _omap4_is_idle(u32 val)
{
	val = _omap4_idlest(val);

	return val == CLKCTRL_IDLEST_DISABLED;
}

static bool _omap4_is_ready(u32 val)
{
	val = _omap4_idlest(val);

	return val == CLKCTRL_IDLEST_FUNCTIONAL ||
	       val == CLKCTRL_IDLEST_INTERFACE_IDLE;
}

static int _omap4_hwmod_clk_enable(struct clk_hw *hw)
{
	struct clk_hw_omap *clk = to_clk_hw_omap(hw);
	u32 val;
	int timeout = 0;

	if (!clk->enable_bit)
		return 0;

	val = ti_clk_ll_ops->clk_readl(clk->enable_reg);

	val &= ~OMAP4_MODULEMODE_MASK;
	val |= clk->enable_bit;

	ti_clk_ll_ops->clk_writel(val, clk->enable_reg);

	/* Wait until module is enabled */
	while (!_omap4_is_ready(val)) {
		udelay(1);
		timeout++;
		if (timeout > OMAP4_MAX_MODULE_READY_TIME) {
			pr_err("%s: failed to enable\n", clk_hw_get_name(hw));
			return -EBUSY;
		}
		val = ti_clk_ll_ops->clk_readl(clk->enable_reg);
	}

	//pr_info("%08x: %08x\n", clk->enable_reg, val);

	return 0;
}

static void _omap4_hwmod_clk_disable(struct clk_hw *hw)
{
	struct clk_hw_omap *clk = to_clk_hw_omap(hw);
	u32 val;
	int timeout = 0;

	if (!clk->enable_bit)
		return;

	val = ti_clk_ll_ops->clk_readl(clk->enable_reg);

	val &= ~OMAP4_MODULEMODE_MASK;

	ti_clk_ll_ops->clk_writel(val, clk->enable_reg);

	/* Wait until module is disabled */
	while (!_omap4_is_idle(val)) {
		udelay(1);
		timeout++;
		if (timeout > OMAP4_MAX_MODULE_DISABLE_TIME) {
			pr_err("%s: failed to disable\n", clk_hw_get_name(hw));
			break;
		}
		val = ti_clk_ll_ops->clk_readl(clk->enable_reg);
	}

	//pr_info("%08x: %08x\n", clk->enable_reg, val);
}

static int _omap4_hwmod_clk_is_enabled(struct clk_hw *hw)
{
	struct clk_hw_omap *clk = to_clk_hw_omap(hw);
	u32 val;

	val = ti_clk_ll_ops->clk_readl(clk->enable_reg);

	if (val & clk->enable_bit)
		return 1;

	return 0;
}

static const struct clk_ops omap4_module_clk_ops = {
	.enable		= &_omap4_hwmod_clk_enable,
	.disable	= &_omap4_hwmod_clk_disable,
	.is_enabled	= &_omap4_hwmod_clk_is_enabled,
};

static void __init _of_ti_hwmod_clk_setup(struct device_node *node,
					  const struct clk_ops *ops)
{
	const char *parent_name;
	void __iomem *reg;
	u8 enable_bit;
	u32 val;
	struct clk_hw_omap *clk_hw;
	struct clk_init_data init = { NULL };
	struct clk *clk;

	reg = ti_clk_get_reg_addr(node, 0);
	if (IS_ERR(reg))
		return;

	parent_name = of_clk_get_parent_name(node, 0);
	if (!parent_name) {
		pr_err("%s must have 1 parent\n", node->name);
		return;
	}

	if (of_property_read_u32(node, "ti,modulemode", &val)) {
		val = 0;
		//pr_info("%s: no modulemode, no explicit control.\n",
		//	node->name);
	}

	enable_bit = val;

	clk_hw = kzalloc(sizeof(*clk_hw), GFP_KERNEL);

	clk_hw->enable_reg = reg;
	clk_hw->enable_bit = enable_bit;

	init.parent_names = &parent_name;
	init.num_parents = 1;
	init.flags = 0;

	init.ops = ops;
	clk_hw->hw.init = &init;
	init.name = node->name;

	clk = clk_register(NULL, &clk_hw->hw);

	if (!IS_ERR(clk))
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
}

static void __init of_ti_omap4_hwmod_clk_setup(struct device_node *node)
{
	_of_ti_hwmod_clk_setup(node, &omap4_module_clk_ops);
}
CLK_OF_DECLARE(ti_omap4_hwmod_clk, "ti,omap4-mod-clock",
	       of_ti_omap4_hwmod_clk_setup);

static void __init of_ti_omap4_hwmod_mux_clk_setup(struct device_node *node)
{
	struct clk_hw_omap *gate;
	struct clk_mux *mux;
	int num_parents;
	const char **parent_names = NULL;
	u32 val;
	void __iomem *reg;
	struct clk *clk;

	mux = kzalloc(sizeof(*mux), GFP_KERNEL);
	gate = kzalloc(sizeof(*gate), GFP_KERNEL);

	if (!mux || !gate)
		goto err;

	if (!of_property_read_u32(node, "ti,bit-shift", &val))
		mux->shift = val;

	if (of_property_read_bool(node, "ti,index-starts-at-one"))
		mux->flags |= CLK_MUX_INDEX_ONE;

	num_parents = of_clk_get_parent_count(node);

	if (num_parents < 2) {
		pr_err("%s: must have parents\n", node->name);
		goto err;
	}

	parent_names = kzalloc((sizeof(char *) * num_parents), GFP_KERNEL);
	if (!parent_names)
		goto err;

	of_clk_parent_fill(node, parent_names, num_parents);

	reg = ti_clk_get_reg_addr(node, 0);

	if (IS_ERR(reg))
		goto err;

	mux->reg = reg;
	gate->enable_reg = reg;

	if (of_property_read_u32(node, "ti,modulemode", &val)) {
		val = 0;
		//pr_info("%s: no modulemode, no explicit control.\n",
		//	node->name);
	}

	gate->enable_bit = val;
	gate->enable_reg = reg;

	clk = clk_register_composite(NULL, node->name,
				     parent_names, num_parents,
				     &mux->hw, &ti_clk_mux_ops,
				     NULL, NULL,
				     &gate->hw, &omap4_module_clk_ops, 0);

	if (!IS_ERR(clk)) {
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
		goto cleanup;
	}
err:
	kfree(gate);
	kfree(mux);

cleanup:
	kfree(parent_names);
}
CLK_OF_DECLARE(ti_omap4_mux_hwmod_clk, "ti,omap4-mux-mod-clock",
	       of_ti_omap4_hwmod_mux_clk_setup);

