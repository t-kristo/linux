/*
 * OMAP2/3 Clock Management (CM) register definitions
 *
 * Copyright (C) 2007-2009 Texas Instruments, Inc.
 * Copyright (C) 2007-2010 Nokia Corporation
 * Paul Walmsley
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * The CM hardware modules on the OMAP2/3 are quite similar to each
 * other.  The CM modules/instances on OMAP4 are quite different, so
 * they are handled in a separate file.
 */
#ifndef __ARCH_ARM_MACH_OMAP2_CM2XXX_3XXX_PRIVATE_H
#define __ARCH_ARM_MACH_OMAP2_CM2XXX_3XXX_PRIVATE_H

#include "cm.h"
#include "cm2xxx_3xxx.h"

#ifndef __ASSEMBLER__

#include <linux/io.h>

static inline u32 omap2_cm_read_mod_reg(s16 module, u16 idx)
{
	return __raw_readl(cm_base + module + idx);
}

static inline void omap2_cm_write_mod_reg(u32 val, s16 module, u16 idx)
{
	__raw_writel(val, cm_base + module + idx);
}

/* Read-modify-write a register in a CM module. Caller must lock */
static inline u32 omap2_cm_rmw_mod_reg_bits(u32 mask, u32 bits, s16 module,
					    s16 idx)
{
	u32 v;

	v = omap2_cm_read_mod_reg(module, idx);
	v &= ~mask;
	v |= bits;
	omap2_cm_write_mod_reg(v, module, idx);

	return v;
}

/* Read a CM register, AND it, and shift the result down to bit 0 */
static inline u32 omap2_cm_read_mod_bits_shift(s16 domain, s16 idx, u32 mask)
{
	u32 v;

	v = omap2_cm_read_mod_reg(domain, idx);
	v &= mask;
	v >>= __ffs(mask);

	return v;
}

static inline u32 omap2_cm_set_mod_reg_bits(u32 bits, s16 module, s16 idx)
{
	return omap2_cm_rmw_mod_reg_bits(bits, bits, module, idx);
}

static inline u32 omap2_cm_clear_mod_reg_bits(u32 bits, s16 module, s16 idx)
{
	return omap2_cm_rmw_mod_reg_bits(bits, 0x0, module, idx);
}

#endif

#endif
