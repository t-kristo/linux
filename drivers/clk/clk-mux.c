/*
 * Copyright (C) 2011 Sascha Hauer, Pengutronix <s.hauer@pengutronix.de>
 * Copyright (C) 2011 Richard Zhao, Linaro <richard.zhao@linaro.org>
 * Copyright (C) 2011-2012 Mike Turquette, Linaro Ltd <mturquette@linaro.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Simple multiplexer clock implementation
 */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/err.h>

/*
 * DOC: basic adjustable multiplexer clock that cannot gate
 *
 * Traits of this clock:
 * prepare - clk_prepare only ensures that parents are prepared
 * enable - clk_enable only ensures that parents are enabled
 * rate - rate is only affected by parent switching.  No clk_set_rate support
 * parent - parent is adjustable through clk_set_parent
 */

#define to_clk_mux(_hw) container_of(_hw, struct clk_mux, hw)

static inline void _mux_writel(u32 val, struct clk_mux *mux)
{
	if (mux->flags & CLK_MUX_REG_OPS) {
		struct clk_reg_def *reg = mux->reg;
		reg->ops->clk_writel(val, reg->offset);
	} else {
		clk_writel(val, mux->reg);
	}
}

static inline u32 _mux_readl(struct clk_mux *mux)
{
	u32 val;

	if (mux->flags & CLK_MUX_REG_OPS) {
		struct clk_reg_def *reg = mux->reg;
		val = reg->ops->clk_readl(reg->offset);
	} else {
		val = clk_readl(mux->reg);
	}

	return val;
}

static u8 clk_mux_get_parent(struct clk_hw *hw)
{
	struct clk_mux *mux = to_clk_mux(hw);
	int num_parents = __clk_get_num_parents(hw->clk);
	u32 val;

	/*
	 * FIXME need a mux-specific flag to determine if val is bitwise or numeric
	 * e.g. sys_clkin_ck's clksel field is 3 bits wide, but ranges from 0x1
	 * to 0x7 (index starts at one)
	 * OTOH, pmd_trace_clk_mux_ck uses a separate bit for each clock, so
	 * val = 0x4 really means "bit 2, index starts at bit 0"
	 */
	val = _mux_readl(mux) >> mux->shift;
	val &= mux->mask;

	if (mux->table) {
		int i;

		for (i = 0; i < num_parents; i++)
			if (mux->table[i] == val)
				return i;
		return -EINVAL;
	}

	if (val && (mux->flags & CLK_MUX_INDEX_BIT))
		val = ffs(val) - 1;

	if (val && (mux->flags & CLK_MUX_INDEX_ONE))
		val--;

	if (val >= num_parents)
		return -EINVAL;

	return val;
}

static int clk_mux_set_parent(struct clk_hw *hw, u8 index)
{
	struct clk_mux *mux = to_clk_mux(hw);
	u32 val;
	unsigned long flags = 0;

	if (mux->table)
		index = mux->table[index];

	else {
		if (mux->flags & CLK_MUX_INDEX_BIT)
			index = (1 << ffs(index));

		if (mux->flags & CLK_MUX_INDEX_ONE)
			index++;
	}

	if (mux->lock)
		spin_lock_irqsave(mux->lock, flags);

	if (mux->flags & CLK_MUX_HIWORD_MASK) {
		val = mux->mask << (mux->shift + 16);
	} else {
		val = _mux_readl(mux);
		val &= ~(mux->mask << mux->shift);
	}
	val |= index << mux->shift;
	_mux_writel(val, mux);

	if (mux->lock)
		spin_unlock_irqrestore(mux->lock, flags);

	return 0;
}

const struct clk_ops clk_mux_ops = {
	.get_parent = clk_mux_get_parent,
	.set_parent = clk_mux_set_parent,
	.determine_rate = __clk_mux_determine_rate,
};
EXPORT_SYMBOL_GPL(clk_mux_ops);

const struct clk_ops clk_mux_ro_ops = {
	.get_parent = clk_mux_get_parent,
};
EXPORT_SYMBOL_GPL(clk_mux_ro_ops);

struct clk *clk_register_mux_table_reg_ops(struct device *dev, const char *name,
		const char **parent_names, u8 num_parents, unsigned long flags,
		void __iomem *reg, u8 shift, u32 mask,
		u8 clk_mux_flags, u32 *table, spinlock_t *lock,
		struct clk_reg_ops *reg_ops)
{
	struct clk_mux *mux;
	struct clk *clk;
	struct clk_init_data init;
	u8 width = 0;

	if (clk_mux_flags & CLK_MUX_HIWORD_MASK) {
		width = fls(mask) - ffs(mask) + 1;
		if (width + shift > 16) {
			pr_err("mux value exceeds LOWORD field\n");
			return ERR_PTR(-EINVAL);
		}
	}

	/* allocate the mux */
	mux = kzalloc(sizeof(struct clk_mux), GFP_KERNEL);
	if (!mux) {
		pr_err("%s: could not allocate mux clk\n", __func__);
		return ERR_PTR(-ENOMEM);
	}

	init.name = name;
	if (clk_mux_flags & CLK_MUX_READ_ONLY)
		init.ops = &clk_mux_ro_ops;
	else
		init.ops = &clk_mux_ops;
	init.flags = flags | CLK_IS_BASIC;
	init.parent_names = parent_names;
	init.num_parents = num_parents;

	/* struct clk_mux assignments */
	if (reg_ops) {
		struct clk_reg_def *reg_def;
		reg_def = kzalloc(sizeof(*reg_def), GFP_KERNEL);
		reg_def->offset = (u32)reg;
		reg_def->ops = reg_ops;
		mux->reg = reg_def;
		clk_mux_flags |= CLK_MUX_REG_OPS;
	} else {
		mux->reg = reg;
	}

	mux->shift = shift;
	mux->mask = mask;
	mux->flags = clk_mux_flags;
	mux->lock = lock;
	mux->table = table;
	mux->hw.init = &init;

	clk = clk_register(dev, &mux->hw);

	if (IS_ERR(clk))
		kfree(mux);

	return clk;
}
EXPORT_SYMBOL_GPL(clk_register_mux_table_reg_ops);

struct clk *clk_register_mux_table(struct device *dev, const char *name,
		const char **parent_names, u8 num_parents, unsigned long flags,
		void __iomem *reg, u8 shift, u32 mask,
		u8 clk_mux_flags, u32 *table, spinlock_t *lock)
{
	return clk_register_mux_table_reg_ops(dev, name, parent_names,
					      num_parents, flags, reg, shift,
					      mask, clk_mux_flags, table, lock,
					      NULL);
}
EXPORT_SYMBOL_GPL(clk_register_mux_table);

struct clk *clk_register_mux(struct device *dev, const char *name,
		const char **parent_names, u8 num_parents, unsigned long flags,
		void __iomem *reg, u8 shift, u8 width,
		u8 clk_mux_flags, spinlock_t *lock)
{
	u32 mask = BIT(width) - 1;

	return clk_register_mux_table(dev, name, parent_names, num_parents,
				      flags, reg, shift, mask, clk_mux_flags,
				      NULL, lock);
}
EXPORT_SYMBOL_GPL(clk_register_mux);
