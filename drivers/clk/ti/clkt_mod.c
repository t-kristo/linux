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

#define NO_IDLEST			0x1

#define OMAP4_MODULEMODE_MASK		0x3

#define MODULEMODE_HWCTRL		0x1
#define MODULEMODE_SWCTRL		0x2

#define OMAP4_IDLEST_MASK		(0x3 << 16)
#define OMAP4_IDLEST_SHIFT		16

#define CLKCTRL_IDLEST_FUNCTIONAL	0x0
#define CLKCTRL_IDLEST_INTERFACE_IDLE	0x2
#define CLKCTRL_IDLEST_DISABLED		0x3

/* These timeouts are in us */
#define OMAP4_MAX_MODULE_READY_TIME	2000
#define OMAP4_MAX_MODULE_DISABLE_TIME	5000

static bool _early_timeout = true;

union omap4_timeout {
	u32 cycles;
	ktime_t start;
};

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

static bool _omap4_is_timeout(union omap4_timeout *time, u32 timeout)
{
	if (unlikely(_early_timeout)) {
		if (time->cycles++ < timeout) {
			udelay(1);
			return false;
		}
	} else {
		if (!ktime_to_ns(time->start)) {
			time->start = ktime_get();
			return false;
		}

		if (ktime_us_delta(ktime_get(), time->start) < timeout) {
			cpu_relax();
			return false;
		}
	}

	return true;
}

static int __init _omap4_disable_early_timeout(void)
{
	_early_timeout = false;

	return 0;
}
arch_initcall(_omap4_disable_early_timeout);

static int _omap4_hwmod_clk_enable(struct clk_hw *hw)
{
	struct clk_hw_omap *clk = to_clk_hw_omap(hw);
	u32 val;
	int ret;
	union omap4_timeout timeout = { 0 };

	if (!clk->enable_bit)
		return 0;

	if (clk->clkdm) {
		ret = ti_clk_ll_ops->clkdm_clk_enable(clk->clkdm, hw->clk);
		if (ret) {
			WARN(1,
			     "%s: could not enable %s's clockdomain %s: %d\n",
			     __func__, clk_hw_get_name(hw),
			     clk->clkdm_name, ret);
			return ret;
		}
	}

	val = ti_clk_ll_ops->clk_readl(clk->enable_reg);

	val &= ~OMAP4_MODULEMODE_MASK;
	val |= clk->enable_bit;

	ti_clk_ll_ops->clk_writel(val, clk->enable_reg);

	if (clk->flags & NO_IDLEST)
		return 0;

	/* Wait until module is enabled */
	while (!_omap4_is_ready(ti_clk_ll_ops->clk_readl(clk->enable_reg))) {
		if (_omap4_is_timeout(&timeout, OMAP4_MAX_MODULE_READY_TIME)) {
			pr_err("%s: failed to enable\n", clk_hw_get_name(hw));
			return -EBUSY;
		}
	}

	return 0;
}

static void _omap4_hwmod_clk_disable(struct clk_hw *hw)
{
	struct clk_hw_omap *clk = to_clk_hw_omap(hw);
	u32 val;
	union omap4_timeout timeout = { 0 };

	if (!clk->enable_bit)
		return;

	val = ti_clk_ll_ops->clk_readl(clk->enable_reg);

	val &= ~OMAP4_MODULEMODE_MASK;

	ti_clk_ll_ops->clk_writel(val, clk->enable_reg);

	if (clk->flags & NO_IDLEST)
		return;

	/* Wait until module is disabled */
	while (!_omap4_is_idle(ti_clk_ll_ops->clk_readl(clk->enable_reg))) {
		if (_omap4_is_timeout(&timeout,
				      OMAP4_MAX_MODULE_DISABLE_TIME)) {
			pr_err("%s: failed to disable\n", clk_hw_get_name(hw));
			break;
		}
	}

	if (clk->clkdm)
		ti_clk_ll_ops->clkdm_clk_disable(clk->clkdm, hw->clk);
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
	.enable		= _omap4_hwmod_clk_enable,
	.disable	= _omap4_hwmod_clk_disable,
	.is_enabled	= _omap4_hwmod_clk_is_enabled,
};

struct clk *ti_clk_register_hwmod(struct ti_clk *setup)
{
	struct ti_clk_hwmod *data = setup->data;
	struct clk_init_data init = { NULL };
	struct clk_hw_omap *hw;
	struct clk *clk;
	int ret;

	hw = kzalloc(sizeof(*hw), GFP_KERNEL);
	if (!hw) {
		ret = -ENOMEM;
		goto err;
	}

	hw->enable_reg = ti_clk_get_reg_addr_clkdm(setup->clkdm_name,
						   data->reg);
	if (!hw->enable_reg) {
		ret = -EINVAL;
		goto err;
	}

	if (data->flags & CLKF_SW_SUP)
		hw->enable_bit = MODULEMODE_SWCTRL;
	if (data->flags & CLKF_HW_SUP)
		hw->enable_bit = MODULEMODE_HWCTRL;
	if (data->flags & CLKF_NO_IDLEST)
		hw->flags |= NO_IDLEST;

	init.parent_names = &data->parent;
	init.num_parents = 1;
	init.flags = 0;
	init.name = setup->name;
	init.ops = &omap4_module_clk_ops;
	hw->hw.init = &init;

	clk = ti_clk_register(NULL, &hw->hw, setup->name);
	if (!IS_ERR(clk))
		return clk;
err:
	kfree(hw);
	return ERR_PTR(ret);
}

static u8 _omap4_mux_mod_get_parent(struct clk_hw *hw)
{
	struct clk_hw_omap *clk = to_clk_hw_omap(hw);
	struct clk_hw *mux_hw = clk->mux;

	__clk_hw_set_clk(mux_hw, hw);

	return ti_clk_mux_get_parent(mux_hw);
}

static int _omap4_mux_mod_set_parent(struct clk_hw *hw, u8 index)
{
	struct clk_hw_omap *clk = to_clk_hw_omap(hw);
	struct clk_hw *mux_hw = clk->mux;

	__clk_hw_set_clk(mux_hw, hw);

	return ti_clk_mux_set_parent(mux_hw, index);
}

static int _omap4_mux_mod_determine_rate(struct clk_hw *hw,
					 struct clk_rate_request *req)
{
	struct clk_hw_omap *clk = to_clk_hw_omap(hw);
	struct clk_hw *mux_hw = clk->mux;

	__clk_hw_set_clk(mux_hw, hw);

	return __clk_mux_determine_rate(mux_hw, req);
}

static const struct clk_ops omap4_mux_module_clk_ops = {
	.enable		= _omap4_hwmod_clk_enable,
	.disable	= _omap4_hwmod_clk_disable,
	.is_enabled	= _omap4_hwmod_clk_is_enabled,
	.get_parent	= _omap4_mux_mod_get_parent,
	.set_parent	= _omap4_mux_mod_set_parent,
	.determine_rate	= _omap4_mux_mod_determine_rate,
};

struct clk *ti_clk_register_hwmod_mux(struct ti_clk *setup)
{
	struct ti_clk_hwmod_mux *data = setup->data;
	struct clk_init_data init = { NULL };
	struct clk_mux *mux;
	struct clk_hw_omap *gate;
	struct clk *clk;
	int ret;
	u8 modulemode;

	if (data->num_parents < 2) {
		pr_err("%s: must have parents\n", setup->name);
		return ERR_PTR(-EINVAL);
	}

	mux = kzalloc(sizeof(*mux), GFP_KERNEL);
	gate = kzalloc(sizeof(*gate), GFP_KERNEL);

	if (!mux || !gate) {
		ret = -ENOMEM;
		goto err;
	}

	gate->mux = &mux->hw;
	mux->shift = data->shift;

	if (data->flags & CLKF_INDEX_STARTS_AT_ONE)
		mux->flags |= CLK_MUX_INDEX_ONE;

	if (data->flags & CLKF_SW_SUP)
		modulemode = MODULEMODE_SWCTRL;
	if (data->flags & CLKF_HW_SUP)
		modulemode = MODULEMODE_HWCTRL;

	gate->enable_bit = modulemode;
	gate->enable_reg = ti_clk_get_reg_addr_clkdm(setup->clkdm_name,
						     data->mod_reg);
	mux->reg = ti_clk_get_reg_addr_clkdm(setup->clkdm_name, data->mux_reg);

	if (!gate->enable_reg || !mux->reg) {
		ret = -EINVAL;
		goto err;
	}

	init.num_parents = data->num_parents;
	init.parent_names = data->parents;
	init.flags = 0;

	init.name = setup->name;
	init.ops = &omap4_mux_module_clk_ops;
	gate->hw.init = &init;

	clk = ti_clk_register(NULL, &gate->hw, setup->name);

	if (!IS_ERR(clk))
		return clk;

err:
	kfree(gate);
	kfree(mux);

	return ERR_PTR(ret);
}
