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
#ifndef __ARCH_ASM_MACH_OMAP2_CM3XXX_H
#define __ARCH_ASM_MACH_OMAP2_CM3XXX_H

#include "prcm-common.h"
#include <linux/power/omap/cm2xxx_3xxx.h>
#include <linux/power/omap/cm3xxx.h>

#define OMAP34XX_CM_REGADDR(module, reg)				\
			OMAP2_L4_IO_ADDRESS(OMAP3430_CM_BASE + (module) + (reg))

#endif
