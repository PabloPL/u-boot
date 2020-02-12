/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __CONFIG_ZERO_H
#define __CONFIG_ZERO_H

#include <configs/exynos7420-common.h>

#define CONFIG_BOARD_COMMON

#define CONFIG_ZERO

#define CONFIG_SYS_SDRAM_BASE		0x40000000
#define CONFIG_SYS_INIT_SP_ADDR        (CONFIG_SYS_SDRAM_BASE + 0x7fff0)

/* select serial console configuration */
#define CONFIG_DEFAULT_CONSOLE	"console=ttySAC1,115200n8\0"

/* DRAM Memory Banks */
#define SDRAM_BANK_SIZE		(256UL << 20UL)	/* 256 MB */

#endif	/* __CONFIG_ZERO_H */
