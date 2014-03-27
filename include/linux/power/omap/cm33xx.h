/*
 * AM33XX CM offset macros
 *
 * Copyright (C) 2011-2012 Texas Instruments Incorporated - http://www.ti.com/
 * Vaibhav Hiremath <hvaibhav@ti.com>
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

#ifndef __LINUX_POWER_OMAP_CM33XX_H
#define __LINUX_POWER_OMAP_CM33XX_H

/* CM instances */
#define AM33XX_CM_PER_MOD		0x0000
#define AM33XX_CM_WKUP_MOD		0x0400
#define AM33XX_CM_DPLL_MOD		0x0500
#define AM33XX_CM_MPU_MOD		0x0600
#define AM33XX_CM_DEVICE_MOD		0x0700
#define AM33XX_CM_RTC_MOD		0x0800
#define AM33XX_CM_GFX_MOD		0x0900
#define AM33XX_CM_CEFUSE_MOD		0x0A00

#ifndef __ASSEMBLER__
bool am33xx_cm_is_clkdm_in_hwsup(u16 inst, u16 cdoffs);
void am33xx_cm_clkdm_enable_hwsup(u16 inst, u16 cdoffs);
void am33xx_cm_clkdm_disable_hwsup(u16 inst, u16 cdoffs);
void am33xx_cm_clkdm_force_sleep(u16 inst, u16 cdoffs);
void am33xx_cm_clkdm_force_wakeup(u16 inst, u16 cdoffs);

#ifdef CONFIG_SOC_AM33XX
int am33xx_cm_wait_module_idle(u16 inst, s16 cdoffs,
			       u16 clkctrl_offs);
void am33xx_cm_module_enable(u8 mode, u16 inst, s16 cdoffs,
			     u16 clkctrl_offs);
void am33xx_cm_module_disable(u16 inst, s16 cdoffs,
			      u16 clkctrl_offs);
int am33xx_cm_wait_module_ready(u16 inst, s16 cdoffs,
				u16 clkctrl_offs);
#else
static inline int am33xx_cm_wait_module_idle(u16 inst, s16 cdoffs,
					u16 clkctrl_offs)
{
	return 0;
}
static inline void am33xx_cm_module_enable(u8 mode, u16 inst, s16 cdoffs,
					u16 clkctrl_offs)
{
}
static inline void am33xx_cm_module_disable(u16 inst, s16 cdoffs,
					u16 clkctrl_offs)
{
}
static inline int am33xx_cm_wait_module_ready(u16 inst, s16 cdoffs,
					u16 clkctrl_offs)
{
	return 0;
}
#endif

#endif /* ASSEMBLER */
#endif
