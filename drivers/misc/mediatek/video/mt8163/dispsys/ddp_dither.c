/*
 * Copyright (C) 2018 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#ifdef CONFIG_MTK_CLKMGR
#include <mach/mt_clkmgr.h>
#endif
#include "cmdq_record.h"
#include "ddp_dither.h"
#include "ddp_path.h"
#include "ddp_reg.h"

#define DITHER_REG(reg_base, index) ((reg_base) + 0x100 + (index)*4)

static void disp_dither_init(enum disp_dither_id_t id, int width, int height,
			     unsigned int dither_bpp, void *cmdq)
{
	unsigned long reg_base = DISPSYS_DITHER_BASE;
	unsigned int enable;

	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 5), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 6), 0x00003004, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 7), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 8), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 9), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 10), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 11), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 12), 0x00000011, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 13), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 14), 0x00000000, ~0);

	enable = 0x1;
	if (dither_bpp == 16) { /* 565 */
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 15), 0x50500001, ~0);
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 16), 0x50504040, ~0);
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 0), 0x00000001, ~0);
	} else if (dither_bpp == 18) { /* 666 */
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 15), 0x40400001, ~0);
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 16), 0x40404040, ~0);
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 0), 0x00000001, ~0);
	} else if (dither_bpp == 24) { /* 888 */
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 15), 0x20200001, ~0);
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 16), 0x20202020, ~0);
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 0), 0x00000001, ~0);
	} else if (dither_bpp > 24) {
		pr_debug("[DITHER] High depth LCM (bpp = %d), no dither\n",
			 dither_bpp);
		enable = 1;
	} else {
		pr_debug("[DITHER] invalid dither bpp = %d\n", dither_bpp);
		/* Bypass dither */
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 0), 0x00000000, ~0);
		enable = 0;
	}

	DISP_REG_MASK(cmdq, DISP_REG_DITHER_EN, enable, 0x1);
	DISP_REG_MASK(cmdq, DISP_REG_DITHER_CFG, enable << 1, 1 << 1);
	DISP_REG_SET(cmdq, DISP_REG_DITHER_SIZE, (width << 16) | height);
}

static int disp_dither_config(enum DISP_MODULE_ENUM module,
			      struct disp_ddp_path_config *pConfig, void *cmdq)
{
	if (pConfig->dst_dirty) {
		disp_dither_init(DISP_DITHER0, pConfig->dst_w, pConfig->dst_h,
				 pConfig->lcm_bpp, cmdq);
	}

	return 0;
}

static int disp_dither_bypass(enum DISP_MODULE_ENUM module, int bypass)
{
	int relay = 0;

	if (bypass)
		relay = 1;

	DISP_REG_MASK(NULL, DISP_REG_DITHER_CFG, relay, 0x1);

	pr_debug("disp_dither_bypass(bypass = %d)", bypass);

	return 0;
}

static int disp_dither_power_on(enum DISP_MODULE_ENUM module, void *handle)
{
	if (module == DISP_MODULE_DITHER)
		ddp_module_clock_enable(MM_CLK_DISP_DITHER, true);

	return 0;
}

static int disp_dither_power_off(enum DISP_MODULE_ENUM module, void *handle)
{
	if (module == DISP_MODULE_DITHER)
		ddp_module_clock_enable(MM_CLK_DISP_DITHER, false);

	return 0;
}

struct DDP_MODULE_DRIVER ddp_driver_dither = {
	.config = disp_dither_config,
	.bypass = disp_dither_bypass,
	.init = disp_dither_power_on,
	.deinit = disp_dither_power_off,
	.power_on = disp_dither_power_on,
	.power_off = disp_dither_power_off,
};
