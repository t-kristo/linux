/*
 * OMAP3xxx Power/Reset Management (PRM) register definitions
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
#ifndef __ARCH_ARM_MACH_OMAP2_PRM3XXX_H
#define __ARCH_ARM_MACH_OMAP2_PRM3XXX_H

#include "prcm-common.h"
#include "prm.h"
#include <linux/power/omap/prm2xxx_3xxx.h>
#include <linux/power/omap/prm3xxx.h>

#define OMAP34XX_PRM_REGADDR(module, reg)				\
		OMAP2_L4_IO_ADDRESS(OMAP3430_PRM_BASE + (module) + (reg))

#endif
