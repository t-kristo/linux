/*
 * OMAP4 Power/Reset Management (PRM) function prototypes
 *
 * Copyright (C) 2010 Nokia Corporation
 * Copyright (C) 2011 Texas Instruments, Inc.
 * Paul Walmsley
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __ARCH_ASM_MACH_OMAP2_PRMINST44XX_H
#define __ARCH_ASM_MACH_OMAP2_PRMINST44XX_H

extern void omap4_prminst_global_warm_sw_reset(void);

extern int omap4_prminst_is_hardreset_asserted(u8 shift, u8 part, s16 inst,
					       u16 rstctrl_offs);
extern int omap4_prminst_assert_hardreset(u8 shift, u8 part, s16 inst,
					  u16 rstctrl_offs);
extern int omap4_prminst_deassert_hardreset(u8 shift, u8 part, s16 inst,
					    u16 rstctrl_offs);
void omap4_prminst_mpuss_clear_prev_logic_pwrst(void);

extern void omap_prm_base_init(void);

#endif
