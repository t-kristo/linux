/*
 * AM33XX PRM instance offset macros
 *
 * Copyright (C) 2011-2012 Texas Instruments Incorporated - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __LINUX_POWER_OMAP_PRM33XX_H
#define __LINUX_POWER_OMAP_PRM33XX_H

/* PRM instances */
#define AM33XX_PRM_OCP_SOCKET_MOD	0x0B00
#define AM33XX_PRM_PER_MOD		0x0C00
#define AM33XX_PRM_WKUP_MOD		0x0D00
#define AM33XX_PRM_MPU_MOD		0x0E00
#define AM33XX_PRM_DEVICE_MOD		0x0F00
#define AM33XX_PRM_RTC_MOD		0x1000
#define AM33XX_PRM_GFX_MOD		0x1100
#define AM33XX_PRM_CEFUSE_MOD		0x1200

#ifndef __ASSEMBLER__
void am33xx_prm_global_warm_sw_reset(void);
int am33xx_prm_is_hardreset_asserted(u8 shift, s16 inst, u16 rstctrl_offs);
int am33xx_prm_assert_hardreset(u8 shift, s16 inst, u16 rstctrl_offs);
int am33xx_prm_deassert_hardreset(u8 shift, u8 st_shift, s16 inst,
				  u16 rstctrl_offs, u16 rstst_offs);
#endif /* ASSEMBLER */
#endif
