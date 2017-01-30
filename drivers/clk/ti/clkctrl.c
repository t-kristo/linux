/*
 * OMAP clkctrl clock support
 *
 * Copyright (C) 2017 Texas Instruments, Inc.
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

enum {
	CLKCTRL_CLK_TYPE_MODULE,
};

static bool _early_timeout = true;

struct omap_clkctrl_provider {
	void __iomem *base;
	struct list_head clocks;
};

struct omap_clkctrl_clk {
	struct clk_hw_omap *clk;
	u16 reg_offset;
	int bit_offset;
	int type;
	struct list_head node;
};

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

	val = readl_relaxed(clk->enable_reg);

	val &= ~OMAP4_MODULEMODE_MASK;
	val |= clk->enable_bit;

	writel_relaxed(val, clk->enable_reg);

	pr_info("%s: wrote %x to %p\n", __func__, val, clk->enable_reg);

	if (clk->flags & NO_IDLEST)
		return 0;

	/* Wait until module is enabled */
	while (!_omap4_is_ready(readl_relaxed(clk->enable_reg))) {
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

	val = readl_relaxed(clk->enable_reg);

	val &= ~OMAP4_MODULEMODE_MASK;

	writel_relaxed(val, clk->enable_reg);

	pr_info("%s: wrote %x to %p\n", __func__, val, clk->enable_reg);

	if (clk->flags & NO_IDLEST)
		goto exit;

	/* Wait until module is disabled */
	while (!_omap4_is_idle(readl_relaxed(clk->enable_reg))) {
		if (_omap4_is_timeout(&timeout,
				      OMAP4_MAX_MODULE_DISABLE_TIME)) {
			pr_err("%s: failed to disable\n", clk_hw_get_name(hw));
			break;
		}
	}

exit:
	if (clk->clkdm)
		ti_clk_ll_ops->clkdm_clk_disable(clk->clkdm, hw->clk);
}

static int _omap4_hwmod_clk_is_enabled(struct clk_hw *hw)
{
	struct clk_hw_omap *clk = to_clk_hw_omap(hw);
	u32 val;

	val = readl_relaxed(clk->enable_reg);

	pr_info("%s: val=%x, ptr=%p\n", __func__, val, clk->enable_reg);

	if (val & clk->enable_bit)
		return 1;

	return 0;
}

static const struct clk_ops omap4_module_clk_ops = {
	.enable		= _omap4_hwmod_clk_enable,
	.disable	= _omap4_hwmod_clk_disable,
	.is_enabled	= _omap4_hwmod_clk_is_enabled,
};

static struct clk_hw *_ti_omap4_clkctrl_xlate(struct of_phandle_args *clkspec,
					      void *data)
{
	struct omap_clkctrl_provider *provider = data;
	struct omap_clkctrl_clk *entry;

	if (clkspec->args_count != 2)
		return ERR_PTR(-EINVAL);

	pr_info("%s: looking for %x:%x\n", __func__,
		clkspec->args[0], clkspec->args[1]);

	list_for_each_entry(entry, &provider->clocks, node) {
		if (entry->reg_offset == clkspec->args[0] &&
		    entry->bit_offset == clkspec->args[1])
			break;
	}

	if (!entry)
		return ERR_PTR(-EINVAL);

	return &entry->clk->hw;
}

static void __init _ti_clkctrl_setup_subclks(struct omap_clkctrl_reg_data *data,
					     void __iomem *reg)
{
	struct omap_clkctrl_bit_data *bits = data->bit_data;

	if (!bits)
		return;

	while (bits->bit) {
		switch (bits->type) {
		case TI_CLK_GATE:

		case TI_CLK_DIVIDER:

		case TI_CLK_MUX:

		default:
			pr_err("%s: bad subclk type: %d\n", __func__,
			       bits->type);
			return;
		}
		bits++;
	}
}

static void __init _ti_omap4_clkctrl_setup(struct device_node *node)
{
	struct omap_clkctrl_provider *provider;
	struct omap_clkctrl_data *data;
	struct omap_clkctrl_reg_data *reg_data;
	struct clk_init_data init = { NULL };
	struct clk_hw_omap *hw;
	struct clk *clk;
	struct omap_clkctrl_clk *clkctrl_clk;
	char *buf;
	const __be32 *addrp;
	u32 addr;

	addrp = of_get_address(node, 0, NULL, NULL);
	addr = (u32)of_translate_address(node, addrp);

	pr_info("physical addr for %s = %x\n", node->name, addr);

	if (of_machine_is_compatible("ti,omap4"))
		data = omap4_clkctrl_data;
	else
		return;

	while (data->addr) {
		pr_info("checking %x against %x\n", addr, data->addr);
		if (addr == data->addr) {
			break;
		}
		data++;	
	}

	if (!data->addr) {
		pr_err("%s not found from clkctrl data.\n", node->name);
		return;
	}

	provider = kzalloc(sizeof(*provider), GFP_KERNEL);
	if (!provider)
		return;

	provider->base = of_iomap(node, 0);

	INIT_LIST_HEAD(&provider->clocks);

	/* Generate clocks */
	reg_data = data->regs;

	while (reg_data->parent) {
		hw = kzalloc(sizeof(*hw), GFP_KERNEL);
		if (!hw)
			return;

		hw->enable_reg = provider->base + reg_data->offset;

		_ti_clkctrl_setup_subclks(reg_data, hw->enable_reg);

		if (reg_data->flags & CLKF_SW_SUP)
			hw->enable_bit = MODULEMODE_SWCTRL;
		if (reg_data->flags & CLKF_HW_SUP)
			hw->enable_bit = MODULEMODE_HWCTRL;
		if (reg_data->flags & CLKF_NO_IDLEST)
			hw->flags |= NO_IDLEST;

		init.parent_names = &reg_data->parent;
		init.num_parents = 1;
		init.flags = 0;
	        buf = kzalloc(strlen(node->name) + strlen(":0000:00"),
			      GFP_KERNEL);
		sprintf(buf, "%s:%04x:%d", node->name, reg_data->offset, 0);
		init.name = buf;
		init.ops = &omap4_module_clk_ops;
		hw->hw.init = &init;

		pr_info("registering %s\n", init.name);
		clk = ti_clk_register(NULL, &hw->hw, init.name);
		if (IS_ERR_OR_NULL(clk))
			return;

		clkctrl_clk = kzalloc(sizeof(*clkctrl_clk), GFP_KERNEL);
		if (!clkctrl_clk)
			return;

		clkctrl_clk->reg_offset = reg_data->offset;
		clkctrl_clk->type = CLKCTRL_CLK_TYPE_MODULE;
		clkctrl_clk->clk = hw;

		list_add(&clkctrl_clk->node, &provider->clocks);

		reg_data++;
	}

	of_clk_add_hw_provider(node, _ti_omap4_clkctrl_xlate, provider);
}
CLK_OF_DECLARE(ti_omap4_clkctrl_clock, "ti,omap4-clkctrl",
	       _ti_omap4_clkctrl_setup);
