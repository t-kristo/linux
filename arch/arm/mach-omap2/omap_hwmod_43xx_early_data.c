/*
 * Copyright (C) 2013 Texas Instruments Incorporated
 *
 * Hwmod present only in AM43x and those that differ other than register
 * offsets as compared to AM335x.
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

#include <linux/platform_data/gpio-omap.h>
#include <linux/platform_data/spi-omap2-mcspi.h>
#include "omap_hwmod.h"
#include "omap_hwmod_33xx_43xx_common_data.h"
#include "prcm43xx.h"
#include "omap_hwmod_common_data.h"


static struct omap_hwmod_ocp_if am43xx_l4_wkup__timer1 = {
	.master		= &am33xx_l4_wkup_hwmod,
	.slave		= &am33xx_timer1_hwmod,
	.clk		= "sys_clkin_ck",
	.user		= OCP_USER_MPU,
};

static struct omap_hwmod_ocp_if am43xx_l4_wkup__uart1 = {
	.master		= &am33xx_l4_wkup_hwmod,
	.slave		= &am33xx_uart1_hwmod,
	.clk		= "sys_clkin_ck",
	.user		= OCP_USER_MPU,
};

static struct omap_hwmod_ocp_if *am43xx_hwmod_ocp_ifs[] __initdata = {
	&am33xx_mpu__l3_main,
	&am43xx_l4_wkup__uart1,
	&am43xx_l4_wkup__timer1,
	&am33xx_l4_ls__timer2,
	&am33xx_l4_ls__uart2,
	&am33xx_l4_ls__uart3,
	&am33xx_l4_ls__uart4,
	&am33xx_l4_ls__uart5,
	&am33xx_l4_ls__uart6,
	NULL,
};

int __init am43xx_hwmod_early_init(void)
{
	omap_hwmod_am43xx_reg();
	omap_hwmod_init();
	return omap_hwmod_register_links(am43xx_hwmod_ocp_ifs);
}
