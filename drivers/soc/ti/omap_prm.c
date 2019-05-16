// SPDX-License-Identifier: GPL-2.0
/*
 * OMAP2+ PRM driver
 *
 * Copyright (C) 2019 Texas Instruments Incorporated - http://www.ti.com/
 *	Tero Kristo <t-kristo@ti.com>
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/reset-controller.h>
#include <linux/delay.h>

struct omap_rst_map {
	s8 rst;
	s8 st;
};

struct omap_prm_data {
	u32 base;
	const char *name;
	u16 pwstctrl;
	u16 pwstst;
	u16 rstctl;
	u16 rstst;
	struct omap_rst_map *rstmap;
	u8 flags;
};

struct omap_prm {
	const struct omap_prm_data *data;
	void __iomem *base;
};

struct omap_reset_data {
	struct reset_controller_dev rcdev;
	struct omap_prm *prm;
};

#define to_omap_reset_data(p) container_of((p), struct omap_reset_data, rcdev)

#define OMAP_MAX_RESETS		8
#define OMAP_RESET_MAX_WAIT	10000

#define OMAP_PRM_NO_RSTST	BIT(0)

static const struct of_device_id omap_prm_id_table[] = {
	{ },
};

static int omap_reset_status(struct reset_controller_dev *rcdev,
			     unsigned long id)
{
	struct omap_reset_data *reset = to_omap_reset_data(rcdev);
	u32 v;

	v = readl_relaxed(reset->prm->base + reset->prm->data->rstst);
	v &= 1 << id;
	v >>= id;

	return v;
}

static int omap_reset_assert(struct reset_controller_dev *rcdev,
			     unsigned long id)
{
	struct omap_reset_data *reset = to_omap_reset_data(rcdev);
	u32 v;

	/* assert the reset control line */
	v = readl_relaxed(reset->prm->base + reset->prm->data->rstctl);
	v |= 1 << id;
	writel_relaxed(v, reset->prm->base + reset->prm->data->rstctl);

	return 0;
}

static int omap_reset_get_st_bit(struct omap_reset_data *reset,
				 unsigned long id)
{
	struct omap_rst_map *map = reset->prm->data->rstmap;

	while (map && map->rst >= 0) {
		if (map->rst == id)
			return map->st;

		map++;
	}

	return id;
}

/*
 * Note that status will not change until clocks are on, and clocks cannot be
 * enabled until reset is deasserted. Consumer drivers must check status
 * separately after enabling clocks.
 */
static int omap_reset_deassert(struct reset_controller_dev *rcdev,
			       unsigned long id)
{
	struct omap_reset_data *reset = to_omap_reset_data(rcdev);
	u32 v;
	int st_bit = id;
	bool has_rstst;

	/* check the current status to avoid de-asserting the line twice */
	v = readl_relaxed(reset->prm->base + reset->prm->data->rstctl);
	if (!(v & BIT(id)))
		return -EEXIST;

	has_rstst = !(reset->prm->data->flags & OMAP_PRM_NO_RSTST);

	if (has_rstst) {
		st_bit = omap_reset_get_st_bit(reset, id);

		/* Clear the reset status by writing 1 to the status bit */
		v = readl_relaxed(reset->prm->base + reset->prm->data->rstst);
		v |= 1 << st_bit;
		writel_relaxed(v, reset->prm->base + reset->prm->data->rstst);
	}

	/* de-assert the reset control line */
	v = readl_relaxed(reset->prm->base + reset->prm->data->rstctl);
	v &= ~(1 << id);
	writel_relaxed(v, reset->prm->base + reset->prm->data->rstctl);

	return 0;
}

static const struct reset_control_ops omap_reset_ops = {
	.assert		= omap_reset_assert,
	.deassert	= omap_reset_deassert,
	.status		= omap_reset_status,
};

static int omap_prm_reset_probe(struct platform_device *pdev,
				struct omap_prm *prm)
{
	struct omap_reset_data *reset;

	/*
	 * Check if we have resets. If either rstctl or rstst is
	 * non-zero, we have reset registers in place. Additionally
	 * the flag OMAP_PRM_NO_RSTST implies that we have resets.
	 */
	if (!prm->data->rstctl && !prm->data->rstst &&
	    !(prm->data->flags & OMAP_PRM_NO_RSTST))
		return 0;

	reset = devm_kzalloc(&pdev->dev, sizeof(*reset), GFP_KERNEL);
	if (!reset)
		return -ENOMEM;

	reset->rcdev.owner = THIS_MODULE;
	reset->rcdev.ops = &omap_reset_ops;
	reset->rcdev.of_node = pdev->dev.of_node;
	reset->rcdev.nr_resets = OMAP_MAX_RESETS;

	reset->prm = prm;

	return devm_reset_controller_register(&pdev->dev, &reset->rcdev);
}

static int omap_prm_probe(struct platform_device *pdev)
{
	struct resource *res;
	const struct omap_prm_data *data;
	struct omap_prm *prm;
	const struct of_device_id *match;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;

	match = of_match_device(omap_prm_id_table, &pdev->dev);
	if (!match)
		return -ENOTSUPP;

	prm = devm_kzalloc(&pdev->dev, sizeof(*prm), GFP_KERNEL);
	if (!prm)
		return -ENOMEM;

	data = match->data;

	while (data->base != res->start) {
		if (!data->base)
			return -EINVAL;
		data++;
	}

	prm->data = data;

	prm->base = devm_ioremap_resource(&pdev->dev, res);
	if (!prm->base)
		return -ENOMEM;

	return omap_prm_reset_probe(pdev, prm);
}

static struct platform_driver omap_prm_driver = {
	.probe = omap_prm_probe,
	.driver = {
		.name		= KBUILD_MODNAME,
		.of_match_table	= omap_prm_id_table,
	},
};
builtin_platform_driver(omap_prm_driver);

MODULE_ALIAS("platform:prm");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("omap2+ prm driver");
