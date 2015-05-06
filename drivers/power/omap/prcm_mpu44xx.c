/*
 * OMAP4 PRCM_MPU module functions
 *
 * Copyright (C) 2009 Nokia Corporation
 * Paul Walmsley
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/power/omap/prcm_mpu44xx.h>
#include <linux/power/omap/prcm_mpu54xx.h>

/*
 * prcm_mpu_base: the virtual address of the start of the PRCM_MPU IP
 *   block registers
 */
void __iomem *prcm_mpu_base;
static u16 cpu_context_offset;

/* PRCM_MPU low-level functions */

static u32 omap4_prcm_mpu_read_inst_reg(s16 inst, u16 reg)
{
	return readl_relaxed(prcm_mpu_base + inst + reg);
}

static void omap4_prcm_mpu_write_inst_reg(u32 val, s16 inst, u16 reg)
{
	writel_relaxed(val, prcm_mpu_base + inst + reg);
}

/**
 * omap4_prcm_mpu_set_cpu_context_offset - set CPU context offset
 * @offset: offset from the instance base
 *
 * Sets CPU context offset within PRCM MPU instance.
 */
void omap4_prcm_mpu_set_cpu_context_offset(u16 offset)
{
	cpu_context_offset = offset;
}

/**
 * omap4_prcm_mpu_clear_prev_cpu_logic_pwrst - clear previous CPU logic
 *					       powerstate after idle
 * @cpu_id: CPU to clear status for (0 or 1)
 *
 * Clears previous logic powerstate from hardware. Used for tracking
 * purposes to see if the CPU has entered idle properly or not.
 */
void omap4_prcm_mpu_clear_prev_cpu_logic_pwrst(unsigned int cpu_id)
{
	u32 reg;
	u16 inst;

	if (cpu_id)
		inst = OMAP4430_PRCM_MPU_CPU1_INST;
	else
		inst = OMAP4430_PRCM_MPU_CPU0_INST;

	reg = omap4_prcm_mpu_read_inst_reg(inst, cpu_context_offset);
	omap4_prcm_mpu_write_inst_reg(reg, inst, cpu_context_offset);
}

/**
 * omap5_prcm_mpu_enable_mercury_retention_mode - enable mercury retention
 *						  on MPU domain
 *
 * Enables fast mercury retention mode on MPU subsystem.
 */
void omap5_prcm_mpu_enable_mercury_retention_mode(void)
{
	u32 reg;

	reg = omap4_prcm_mpu_read_inst_reg(OMAP54XX_PRCM_MPU_DEVICE_INST,
					   OMAP54XX_PRCM_MPU_PRM_PSCON_COUNT_OFFSET);
	/* Enable HG_EN, HG_RAMPUP = fast mode */
	reg |= BIT(24) | BIT(25);
	omap4_prcm_mpu_write_inst_reg(reg, OMAP54XX_PRCM_MPU_DEVICE_INST,
				      OMAP54XX_PRCM_MPU_PRM_PSCON_COUNT_OFFSET);
}

/**
 * omap2_set_globals_prcm_mpu - set the MPU PRCM base address (for early use)
 * @prcm_mpu: PRCM_MPU base virtual address
 *
 * XXX Will be replaced when the PRM/CM drivers are completed.
 */
void __init omap2_set_globals_prcm_mpu(void __iomem *prcm_mpu)
{
	prcm_mpu_base = prcm_mpu;
}
