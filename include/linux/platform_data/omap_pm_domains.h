/*
 * OMAP PM Domains platform data
 *
 * Copyright (C) 2017 Texas Instruments, Inc.
 *
 * Tero Kristo <t-kristo@ti.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __OMAP_PM_DOMAINS_PDATA_H
#define __OMAP_PM_DOMAINS_PDATA_H

struct clockdomain;
struct powerdomain;

enum {
	PD_TYPE_PWRDM,
	PD_TYPE_CLKDM,
};

struct omap_pm_domains_pdata {
	struct clockdomain * (*clkdm_lookup)(const char *name);
	struct powerdomain * (*pwrdm_lookup)(const char *name);
	void (*clkdm_deny_idle)(struct clockdomain *clkdm);
	void (*clkdm_allow_idle)(struct clockdomain *clkdm);
};

#endif /* __OMAP_PM_DOMAINS_PDATA_H */
