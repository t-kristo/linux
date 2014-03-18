/*
 * OMAP2xxx Power/Reset Management (PRM) register definitions
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
#ifndef __LINUX_POWER_OMAP_PRM2XXX_H
#define __LINUX_POWER_OMAP_PRM2XXX_H

#define OMAP2_PRCM_REVISION_OFFSET		0x0000
#define OMAP2_PRCM_SYSCONFIG_OFFSET		0x0010
#define OMAP2_PRCM_VOLTCTRL_OFFSET		0x0050
#define OMAP2_PRCM_CLKSRC_CTRL_OFFSET		0x0060
#define OMAP2_PRCM_VOLTSETUP_OFFSET		0x0090
#define OMAP2_PRCM_CLKSSETUP_OFFSET		0x0094

#ifndef __ASSEMBLER__

struct clockdomain;

/* Function prototypes */
int omap2xxx_clkdm_sleep(struct clockdomain *clkdm);
int omap2xxx_clkdm_wakeup(struct clockdomain *clkdm);
void omap2xxx_prm_dpll_reset(void);
void omap2xxx_prm_clear_mod_irqs(s16 module, u8 regs, u32 wkst_mask);
void omap2xxx_prm_init_pm(void);
int __init omap2xxx_prm_init(void);
#endif

#endif
