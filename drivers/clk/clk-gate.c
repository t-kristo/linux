/*
 * Copyright (C) 2010-2011 Canonical Ltd <jeremy.kerr@canonical.com>
 * Copyright (C) 2011-2012 Mike Turquette, Linaro Ltd <mturquette@linaro.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Gated clock implementation
 */

#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/of.h>
#include <linux/of_address.h>

/**
 * DOC: basic gatable clock which can gate and ungate it's ouput
 *
 * Traits of this clock:
 * prepare - clk_(un)prepare only ensures parent is (un)prepared
 * enable - clk_enable and clk_disable are functional & control gating
 * rate - inherits rate from parent.  No clk_set_rate support
 * parent - fixed parent.  No clk_set_parent support
 */

#define to_clk_gate(_hw) container_of(_hw, struct clk_gate, hw)

/*
 * It works on following logic:
 *
 * For enabling clock, enable = 1
 *	set2dis = 1	-> clear bit	-> set = 0
 *	set2dis = 0	-> set bit	-> set = 1
 *
 * For disabling clock, enable = 0
 *	set2dis = 1	-> set bit	-> set = 1
 *	set2dis = 0	-> clear bit	-> set = 0
 *
 * So, result is always: enable xor set2dis.
 */
static void clk_gate_endisable(struct clk_hw *hw, int enable)
{
	struct clk_gate *gate = to_clk_gate(hw);
	int set = gate->flags & CLK_GATE_SET_TO_DISABLE ? 1 : 0;
	unsigned long flags = 0;
	u32 reg;

	set ^= enable;

	if (gate->lock)
		spin_lock_irqsave(gate->lock, flags);

	if (gate->flags & CLK_GATE_HIWORD_MASK) {
		reg = BIT(gate->bit_idx + 16);
		if (set)
			reg |= BIT(gate->bit_idx);
	} else {
		reg = readl(gate->reg);

		if (set)
			reg |= BIT(gate->bit_idx);
		else
			reg &= ~BIT(gate->bit_idx);
	}

	writel(reg, gate->reg);

	if (gate->lock)
		spin_unlock_irqrestore(gate->lock, flags);
}

static int clk_gate_enable(struct clk_hw *hw)
{
	clk_gate_endisable(hw, 1);

	return 0;
}

static void clk_gate_disable(struct clk_hw *hw)
{
	clk_gate_endisable(hw, 0);
}

static int clk_gate_is_enabled(struct clk_hw *hw)
{
	u32 reg;
	struct clk_gate *gate = to_clk_gate(hw);

	reg = readl(gate->reg);

	/* if a set bit disables this clk, flip it before masking */
	if (gate->flags & CLK_GATE_SET_TO_DISABLE)
		reg ^= BIT(gate->bit_idx);

	reg &= BIT(gate->bit_idx);

	return reg ? 1 : 0;
}

const struct clk_ops clk_gate_ops = {
	.enable = clk_gate_enable,
	.disable = clk_gate_disable,
	.is_enabled = clk_gate_is_enabled,
};
EXPORT_SYMBOL_GPL(clk_gate_ops);

/**
 * clk_register_gate - register a gate clock with the clock framework
 * @dev: device that is registering this clock
 * @name: name of this clock
 * @parent_name: name of this clock's parent
 * @flags: framework-specific flags for this clock
 * @reg: register address to control gating of this clock
 * @bit_idx: which bit in the register controls gating of this clock
 * @clk_gate_flags: gate-specific flags for this clock
 * @lock: shared register lock for this clock
 */
struct clk *clk_register_gate(struct device *dev, const char *name,
		const char *parent_name, unsigned long flags,
		void __iomem *reg, u8 bit_idx,
		u8 clk_gate_flags, spinlock_t *lock)
{
	struct clk_gate *gate;
	struct clk *clk;
	struct clk_init_data init;

	if (clk_gate_flags & CLK_GATE_HIWORD_MASK) {
		if (bit_idx > 16) {
			pr_err("gate bit exceeds LOWORD field\n");
			return ERR_PTR(-EINVAL);
		}
	}

	/* allocate the gate */
	gate = kzalloc(sizeof(struct clk_gate), GFP_KERNEL);
	if (!gate) {
		pr_err("%s: could not allocate gated clk\n", __func__);
		return ERR_PTR(-ENOMEM);
	}

	init.name = name;
	init.ops = &clk_gate_ops;
	init.flags = flags | CLK_IS_BASIC;
	init.parent_names = (parent_name ? &parent_name: NULL);
	init.num_parents = (parent_name ? 1 : 0);

	/* struct clk_gate assignments */
	gate->reg = reg;
	gate->bit_idx = bit_idx;
	gate->flags = clk_gate_flags;
	gate->lock = lock;
	gate->hw.init = &init;

	clk = clk_register(dev, &gate->hw);

	if (IS_ERR(clk))
		kfree(gate);

	return clk;
}

#ifdef CONFIG_OF
/**
 * of_gate_clk_setup() - Setup function for simple gate rate clock
 */
void of_gate_clk_setup(struct device_node *node)
{
	struct clk *clk;
	const char *clk_name = node->name;
	void __iomem *reg;
	const char *parent_name;
	u8 clk_gate_flags = 0;
	u32 bit_idx = 0;
	u32 flags = 0;

	of_property_read_string(node, "clock-output-names", &clk_name);

	parent_name = of_clk_get_parent_name(node, 0);

	reg = of_iomap(node, 0);
	if (!reg) {
		pr_err("%s: no memory mapped for property reg\n", __func__);
		return;
	}

	if (of_property_read_u32(node, "bit-shift", &bit_idx)) {
		pr_err("%s: missing bit-shift property for %s\n",
				__func__, node->name);
		return;
	}

	if (of_property_read_bool(node, "set-bit-to-disable"))
		clk_gate_flags |= CLK_GATE_SET_TO_DISABLE;

	if (of_property_read_bool(node, "hiword-mask"))
		clk_gate_flags |= CLK_GATE_HIWORD_MASK;

	if (of_property_read_bool(node, "set-rate-parent"))
		flags |= CLK_SET_RATE_PARENT;

	clk = clk_register_gate(NULL, clk_name, parent_name, flags, reg,
			bit_idx, clk_gate_flags, NULL);

	if (!IS_ERR(clk))
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
}
EXPORT_SYMBOL_GPL(of_gate_clk_setup);
CLK_OF_DECLARE(gate_clk, "gate-clock", of_gate_clk_setup);
#endif
