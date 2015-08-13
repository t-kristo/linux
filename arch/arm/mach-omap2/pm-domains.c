/*
 * DT based powerdomain support for OMAP2+ SoCs
 *
 * Copyright (C) 2015 Texas Instruments, Inc.
 *   Tero Kristo <t-kristo@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/bug.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/pm_domain.h>

#include "powerdomain.h"
#include "clockdomain.h"
#include "prcm-common.h"

#define OMAP_PD_TYPE_PWRDM	0x1
#define OMAP_PD_TYPE_CLKDM	0x2

struct omap_pm_domain {
	void *domain;
	int type;
	struct generic_pm_domain pd;
};

#define to_omap_pm_domain(genpd) container_of(genpd, struct omap_pm_domain, pd)

static int _get_offset(struct device_node *node, u16 *val)
{
	u32 tmp;

	if (of_property_read_u32(node, "reg", &tmp)) {
		pr_err("%s: %s does not provide reg addr!\n", __func__,
		       node->name);
		return -EINVAL;
	}

	*val = (u16)tmp;

	return 0;
}

static int omap_genpd_power_off(struct generic_pm_domain *gen_pd)
{
	return 0;
}

static int omap_genpd_power_on(struct generic_pm_domain *gen_pd)
{
	return 0;
}

int __init of_omap_powerdomain_init(const struct of_device_id *match_table)
{
	struct device_node *np;
	const struct of_device_id *match;
	struct powerdomain *pd_tmpl, *pd;
	struct omap_pm_domain *domain;
	void *arr[2];
	bool enabled;

	arr[1] = NULL;

	for_each_matching_node_and_match(np, match_table, &match) {
		pd_tmpl = (struct powerdomain *)match->data;

		pd = kmemdup(pd_tmpl, sizeof(*pd), GFP_KERNEL);
		if (!pd)
			return -ENOMEM;

		domain = kzalloc(sizeof(*domain), GFP_KERNEL);
		if (!domain)
			return -ENOMEM;

		if (_get_offset(np, &pd->prcm_offs))
			return -EINVAL;

		pd->name = kstrdup(np->name, GFP_KERNEL);

		domain->domain = pd;
		domain->type = OMAP_PD_TYPE_PWRDM;
		domain->pd.name = pd->name;
		domain->pd.power_off = omap_genpd_power_off;
		domain->pd.power_on = omap_genpd_power_on;

		pd->prcm_partition =
			omap_prcm_get_partition(np->parent->parent);

		arr[0] = pd;

		pwrdm_register_pwrdms((struct powerdomain **)arr);

		enabled = pwrdm_read_pwrst(pd) == PWRDM_POWER_ON ? true : false;

		pm_genpd_init(&domain->pd, NULL, !enabled);
		of_genpd_add_provider_simple(np, &domain->pd);
		pr_info("%s: registered %s, %08x, part=%d\n", __func__,
			np->name, (u32)&domain->pd, pd->prcm_partition);
	}

	return 0;
}

int __init of_omap_clockdomain_init(const struct of_device_id *match_table)
{
	struct device_node *np;
	const struct of_device_id *match;
	struct clockdomain *cd_tmpl, *cd;
	struct of_phandle_args args;
	void *arr[2];
	struct omap_pm_domain *domain, *parent_domain;
	struct generic_pm_domain *gen_pd;
	struct powerdomain *pd;
	int num, i;
	struct clkdm_dep *dep_arr;

	arr[1] = NULL;

	for_each_matching_node_and_match(np, match_table, &match) {
		cd_tmpl = (struct clockdomain *)match->data;

		cd = kmemdup(cd_tmpl, sizeof(*cd), GFP_KERNEL);
		if (!cd)
			return -ENOMEM;

		domain = kzalloc(sizeof(*domain), GFP_KERNEL);
		if (!domain)
			return -ENOMEM;

		if (_get_offset(np, &cd->clkdm_offs))
			return -EINVAL;

		cd->name = kstrdup(np->name, GFP_KERNEL);

		domain->pd.name = cd->name;
		domain->pd.power_off = omap_genpd_power_off;
		domain->pd.power_on = omap_genpd_power_on;

		num = of_count_phandle_with_args(np, "power-domains",
						 "#power-domain-cells");

		pr_info("%s: %s has %d pds defined.\n", __func__, np->name,
			num);

		if (of_parse_phandle_with_args(np, "power-domains",
					       "#power-domain-cells", 0,
					       &args)) {
			pr_err("%s: %s: missing parent pwrdm.\n", __func__,
			       np->name);
			return -EINVAL;
		}

		gen_pd = of_genpd_get_from_provider(&args);
		pr_info("%s: %s->%s: parent_gen_pd=%08x\n", __func__, np->name,
			np->parent->name, (u32)gen_pd);
		if (!gen_pd)
			return -EINVAL;

		parent_domain = to_omap_pm_domain(gen_pd);

		pd = parent_domain->domain;

		cd->pwrdm.name = pd->name;

		cd->prcm_partition =
			omap_prcm_get_partition(np->parent->parent);

		if (cd->prcm_partition != pd->prcm_partition) {
			/* Different partitions, same inst offset */
			cd->cm_inst = pd->prcm_offs;
		} else if (pd->prcm_offs + 0x100 <= cd->clkdm_offs) {
			/* Same partition, clkdm inst offset + 0x100 */
			cd->cm_inst = pd->prcm_offs + 0x100;
		} else {
			/*
			 * Same partition, but offset so close we must
			 * use same instance.
			 */
			cd->cm_inst = pd->prcm_offs;
		}

		/* fix clkdm offset based on instance offset */
		cd->clkdm_offs -= cd->cm_inst;

		domain->domain = cd;
		domain->type = OMAP_PD_TYPE_CLKDM;

		if (num > 1) {
			dep_arr = kcalloc(num, sizeof(*dep_arr), GFP_KERNEL);
			cd->sleepdep_srcs = dep_arr;
			cd->wkdep_srcs = dep_arr;

			for (i = 1; i < num; i++) {
				if (of_parse_phandle_with_args(np, "power-domains",
							       "#power-domain-cells",
							       i, &args)) {
					pr_err("parsing of clkdm dep %d failed for %s\n", i, np->name);
					return -EINVAL;
				}

				dep_arr[i - 1].clkdm_name = args.np->name;

				pr_info("%s: %s: dep[%d] = %s\n", __func__,
					np->name, i - 1, args.np->name);
			}
		}

		arr[0] = cd;

		clkdm_register_clkdms((struct clockdomain **)arr);

		/* During boot, all clockdomains are basically idle */
		pm_genpd_init(&domain->pd, NULL, true);
		of_genpd_add_provider_simple(np, &domain->pd);
		pm_genpd_add_subdomain(gen_pd, &domain->pd);
		pr_info("%s: registered %s, %08x, part=%d, inst=%04x, offs=%04x\n", __func__,
			np->name, (u32)&domain->pd, cd->prcm_partition,
			cd->cm_inst, cd->clkdm_offs);
	}

	return 0;
}
