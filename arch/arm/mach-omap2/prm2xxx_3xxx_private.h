/*
 * OMAP2xxx/3xxx-common Power/Reset Management (PRM) register definitions
 *
 * Copyright (C) 2007-2009, 2011-2012 Texas Instruments, Inc.
 * Copyright (C) 2008-2010 Nokia Corporation
 * Paul Walmsley
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * The PRM hardware modules on the OMAP2/3 are quite similar to each
 * other.  The PRM on OMAP4 has a new register layout, and is handled
 * in a separate file.
 */
#ifndef __ARCH_ARM_MACH_OMAP2_PRM2XXX_3XXX_PRIVATE_H
#define __ARCH_ARM_MACH_OMAP2_PRM2XXX_3XXX_PRIVATE_H

#include "prcm-common.h"
#include "prm.h"
#include "prm2xxx_3xxx.h"

#ifndef __ASSEMBLER__

#include <linux/io.h>
#include "powerdomain.h"

/* Power/reset management domain register get/set */
static inline u32 omap2_prm_read_mod_reg(s16 module, u16 idx)
{
	return __raw_readl(prm_base + module + idx);
}

static inline void omap2_prm_write_mod_reg(u32 val, s16 module, u16 idx)
{
	__raw_writel(val, prm_base + module + idx);
}

/* Read-modify-write a register in a PRM module. Caller must lock */
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

#endif /* __ASSEMBLER */

#endif
