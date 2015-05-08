/*
 * OMAP2+ Power Management private declarations
 *
 * Copyright (C) 2015 Texas Instruments, Inc.
 *     Tero Kristo (t-kristo@ti.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#ifndef __DRIVERS_POWER_OMAP_PRM_H
#define __DRIVERS_POWER_OMAP_PRM_H

#include <linux/io.h>

static inline u32 omap2_prm_read_mod_reg(s16 module, u16 idx)
{
	return readl_relaxed(prm_base + module + idx);
}

static inline void omap2_prm_write_mod_reg(u32 val, s16 module, u16 idx)
{
	writel_relaxed(val, prm_base + module + idx);
}

/* Read-modify-write a register in a CM module. Caller must lock */
static inline u32 omap2_prm_rmw_mod_reg_bits(u32 mask, u32 bits, s16 module,
					     s16 idx)
{
	u32 v;

	v = omap2_prm_read_mod_reg(module, idx);
	v &= ~mask;
	v |= bits;
	omap2_prm_write_mod_reg(v, module, idx);

	return v;
}

/* Read a PRM register, AND it, and shift the result down to bit 0 */
static inline u32 omap2_prm_read_mod_bits_shift(s16 domain, s16 idx, u32 mask)
{
	u32 v;

	v = omap2_prm_read_mod_reg(domain, idx);
	v &= mask;
	v >>= __ffs(mask);

	return v;
}

static inline u32 omap2_prm_set_mod_reg_bits(u32 bits, s16 module, s16 idx)
{
	return omap2_prm_rmw_mod_reg_bits(bits, bits, module, idx);
}

static inline u32 omap2_prm_clear_mod_reg_bits(u32 bits, s16 module, s16 idx)
{
	return omap2_prm_rmw_mod_reg_bits(bits, 0x0, module, idx);
}
#endif
