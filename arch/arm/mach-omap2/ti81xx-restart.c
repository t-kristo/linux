/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/reboot.h>
#include <linux/power/omap/prm3xxx.h>

#include "iomap.h"
#include "common.h"
#include "control.h"

/**
 * ti81xx_restart - trigger a software restart of the SoC
 * @mode: the "reboot mode", see arch/arm/kernel/{setup,process}.c
 * @cmd: passed from the userspace program rebooting the system (if provided)
 *
 * Resets the SoC.  For @cmd, see the 'reboot' syscall in
 * kernel/sys.c.  No return value.
 *
 * NOTE: Warm reset does not seem to work, may require resetting
 * clocks to bypass mode.
 */
void ti81xx_restart(enum reboot_mode mode, const char *cmd)
{
	omap_prm_reset_system();
	while (1);
}
