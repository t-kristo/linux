/*
 * OMAP4 Clock Management (CM) function prototypes
 *
 * Copyright (C) 2010 Nokia Corporation
 * Paul Walmsley
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __LINUX_POWER_OMAP_CM44XX_H
#define __LINUX_POWER_OMAP_CM44XX_H

bool omap4_cminst_is_clkdm_in_hwsup(u8 part, u16 inst, u16 cdoffs);
void omap4_cminst_clkdm_enable_hwsup(u8 part, u16 inst, u16 cdoffs);
void omap4_cminst_clkdm_disable_hwsup(u8 part, u16 inst, u16 cdoffs);
void omap4_cminst_clkdm_force_sleep(u8 part, u16 inst, u16 cdoffs);
void omap4_cminst_clkdm_force_wakeup(u8 part, u16 inst, u16 cdoffs);
int omap4_cminst_wait_module_ready(u8 part, u16 inst, s16 cdoffs,
				   u16 clkctrl_offs);
int omap4_cminst_wait_module_idle(u8 part, u16 inst, s16 cdoffs,
				  u16 clkctrl_offs);
void omap4_cminst_module_enable(u8 mode, u8 part, u16 inst, s16 cdoffs,
				u16 clkctrl_offs);
void omap4_cminst_module_disable(u8 part, u16 inst, s16 cdoffs,
				 u16 clkctrl_offs);

void omap_cm_base_init(void);

#endif
