/*
 * omap4-sar-layout.h: OMAP4 SAR RAM layout header file
 *
 * Copyright (C) 2011 Texas Instruments, Inc.
 *	Santosh Shilimkar <santosh.shilimkar@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef OMAP_ARCH_OMAP4_SAR_LAYOUT_H
#define OMAP_ARCH_OMAP4_SAR_LAYOUT_H

#include <mach/hardware.h>
#include <mach/ctrl_module_pad_core_44xx.h>

#include "cm1_44xx.h"
#include "cm2_44xx.h"
#include "prcm-common.h"

/*
 * The SAR RAM is maintained during Device OFF mode.
 * It is split into 4 banks with different privilege accesses
 *
 * ---------------------------------------------------------------------
 * Access mode			Bank	Address Range
 * ---------------------------------------------------------------------
 * HS/GP : Public		1	0x4A32_6000 - 0x4A32_6FFF (4kB)
 * HS/GP : Public, Secured
 * if padconfaccdisable=1	2	0x4A32_7000 - 0x4A32_73FF (1kB)
 * HS/EMU : Secured
 * GP : Public			3	0x4A32_8000 - 0x4A32_87FF (2kB)
 * HS/GP :
 * Secure Priviledge,
 * write once.			4	0x4A32_9000 - 0x4A32_93FF (1kB)
 * ---------------------------------------------------------------------
 * The SAR RAM save regiter layout is fixed since restore is done by hardware.
 */

/*
 * SAR BANK offsets from base address OMAP44XX_SAR_RAM_BASE
 */
#define SAR_BANK1_OFFSET		0x0000
#define SAR_BANK2_OFFSET		0x1000
#define SAR_BANK3_OFFSET		0x2000
#define SAR_BANK4_OFFSET		0x3000

/* Scratch pad memory offsets from SAR_BANK1 */
#define SCU_OFFSET0				0xd00
#define SCU_OFFSET1				0xd04
#define OMAP_TYPE_OFFSET			0xd10
#define L2X0_SAVE_OFFSET0			0xd14
#define L2X0_SAVE_OFFSET1			0xd18
#define L2X0_AUXCTRL_OFFSET			0xd1c
#define L2X0_PREFETCH_CTRL_OFFSET		0xd20

/* CPUx Wakeup Non-Secure Physical Address offsets in SAR_BANK3 */
#define CPU0_WAKEUP_NS_PA_ADDR_OFFSET		0xa04
#define CPU1_WAKEUP_NS_PA_ADDR_OFFSET		0xa08

#define SAR_BACKUP_STATUS_OFFSET		(SAR_BANK3_OFFSET + 0x500)
#define SAR_SECURE_RAM_SIZE_OFFSET		(SAR_BANK3_OFFSET + 0x504)
#define SAR_SECRAM_SAVED_AT_OFFSET		(SAR_BANK3_OFFSET + 0x508)

/* WakeUpGen save restore offset from OMAP44XX_SAR_RAM_BASE */
#define WAKEUPGENENB_OFFSET_CPU0		(SAR_BANK3_OFFSET + 0x684)
#define WAKEUPGENENB_SECURE_OFFSET_CPU0		(SAR_BANK3_OFFSET + 0x694)
#define WAKEUPGENENB_OFFSET_CPU1		(SAR_BANK3_OFFSET + 0x6a4)
#define WAKEUPGENENB_SECURE_OFFSET_CPU1		(SAR_BANK3_OFFSET + 0x6b4)
#define AUXCOREBOOT0_OFFSET			(SAR_BANK3_OFFSET + 0x6c4)
#define AUXCOREBOOT1_OFFSET			(SAR_BANK3_OFFSET + 0x6c8)
#define PTMSYNCREQ_MASK_OFFSET			(SAR_BANK3_OFFSET + 0x6cc)
#define PTMSYNCREQ_EN_OFFSET			(SAR_BANK3_OFFSET + 0x6d0)
#define SAR_BACKUP_STATUS_WAKEUPGEN		0x10

#endif
