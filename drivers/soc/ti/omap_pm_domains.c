/*
 * OMAP PM Domains driver
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/bug.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/pm_domain.h>
#include <linux/platform_device.h>

#include <linux/platform_data/omap_pm_domains.h>

struct omap_pm_domain {
	struct device *dev;
	struct omap_pm_domains_pdata *pdata;
	void *data;
	int type;
	struct generic_pm_domain pd;
};

#define to_omap_pm_domain(gpd) container_of(gpd, struct omap_pm_domain, pd)

static int omap_genpd_power_off(struct generic_pm_domain *gpd)
{
	struct omap_pm_domain *pd = to_omap_pm_domain(gpd);
	

	if (pd->type == PD_TYPE_CLKDM)
		pd->pdata->clkdm_allow_idle(pd->data);

	return 0;
}

static int omap_genpd_power_on(struct generic_pm_domain *gpd)
{
	struct omap_pm_domain *pd = to_omap_pm_domain(gpd);

	if (pd->type == PD_TYPE_CLKDM)
		pd->pdata->clkdm_deny_idle(pd->data);

	return 0;
}

static int omap_pm_domains_probe(struct platform_device *pdev)
{
	struct omap_pm_domains_pdata *pdata;
	struct omap_pm_domain *gpd;
	struct device_node *np = pdev->dev.of_node;
	bool enabled = false;

	pdata = dev_get_platdata(pdev->dev.parent);
	if (!pdata)
		return -EINVAL;

	gpd = devm_kzalloc(&pdev->dev, sizeof(*gpd), GFP_KERNEL);
	if (!gpd)
		return -ENOMEM;

	gpd->pd.name = devm_kstrdup(&pdev->dev, np->name, GFP_KERNEL);
	if (!gpd->pd.name)
		return -ENOMEM;

	gpd->data = pdata->clkdm_lookup(np->name);
	if (!gpd->data)
		return -EINVAL;

	gpd->dev = &pdev->dev;
	gpd->pdata = pdata;
	gpd->type = PD_TYPE_CLKDM;
	gpd->pd.power_off = omap_genpd_power_off;
	gpd->pd.power_on = omap_genpd_power_on;

	pm_genpd_init(&gpd->pd, NULL, !enabled);
	of_genpd_add_provider_simple(np, &gpd->pd);

	pr_info("%s: registered %s, clkdm=%p\n", __func__, np->name,
		gpd->data);

	return 0;
}

static int omap_pm_domains_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id omap_pm_domains_of_match[] = {
        { .compatible = "ti,clockdomain" },
        {}
};
MODULE_DEVICE_TABLE(of, omap_pm_domains_of_match);

static struct platform_driver omap_pm_domains_driver = {
        .driver = {
                .name = "omap-pm-domains",
                .of_match_table = of_match_ptr(omap_pm_domains_of_match),
        },
        .probe = omap_pm_domains_probe,
        .remove = omap_pm_domains_remove,
};
module_platform_driver(omap_pm_domains_driver);

MODULE_AUTHOR("Tero Kristo <t-kristo@ti.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("OMAP PM Domains driver");
