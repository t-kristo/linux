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
#ifndef __LINUX_POWER_OMAP_CM2XXX_3XXX_H
#define __LINUX_POWER_OMAP_CM2XXX_3XXX_H

#include <linux/power/omap/cm.h>

/*
 * Module specific CM register offsets from CM_BASE + domain offset
 * Use cm_{read,write}_mod_reg() with these registers.
 * These register offsets generally appear in more than one PRCM submodule.
 */

/* Common between OMAP2 and OMAP3 */

#define CM_FCLKEN					0x0000
#define CM_FCLKEN1					CM_FCLKEN
#define CM_CLKEN					CM_FCLKEN
#define CM_ICLKEN					0x0010
#define CM_ICLKEN1					CM_ICLKEN
#define CM_ICLKEN2					0x0014
#define CM_ICLKEN3					0x0018
#define CM_IDLEST					0x0020
#define CM_IDLEST1					CM_IDLEST
#define CM_IDLEST2					0x0024
#define OMAP2430_CM_IDLEST3				0x0028
#define CM_AUTOIDLE					0x0030
#define CM_AUTOIDLE1					CM_AUTOIDLE
#define CM_AUTOIDLE2					0x0034
#define CM_AUTOIDLE3					0x0038
#define CM_CLKSEL					0x0040
#define CM_CLKSEL1					CM_CLKSEL
#define CM_CLKSEL2					0x0044
#define OMAP2_CM_CLKSTCTRL				0x0048

#ifndef __ASSEMBLER__

int omap2xxx_cm_apll54_enable(void);
void omap2xxx_cm_apll54_disable(void);
int omap2xxx_cm_apll96_enable(void);
void omap2xxx_cm_apll96_disable(void);

#endif

/* CM register bits shared between 24XX and 3430 */

/* CM_CLKSEL_GFX */
#define OMAP_CLKSEL_GFX_SHIFT				0
#define OMAP_CLKSEL_GFX_MASK				(0x7 << 0)
#define OMAP_CLKSEL_GFX_WIDTH				3

/* CM_ICLKEN_GFX */
#define OMAP_EN_GFX_SHIFT				0
#define OMAP_EN_GFX_MASK				BIT(0)

/* CM_IDLEST_GFX */
#define OMAP_ST_GFX_MASK				BIT(0)

#endif
