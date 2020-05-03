// SPDX-License-Identifier: GPL-2.0+
/*
 * Samsung Exynos7420 clock driver.
 * Copyright (C) 2016 Samsung Electronics
 * Thomas Abraham <thomas.ab@samsung.com>
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <clk-uclass.h>
#include <asm/io.h>
#include <dt-bindings/clock/exynos7420-clk.h>
#include "clk-pll.h"
#include "clk.h"

static struct clk_ops samsung_clk_ops = {
	.set_rate = samsung_clk_set_rate,
	.get_rate = samsung_clk_get_rate,
	.enable = samsung_clk_enable,
	.disable = samsung_clk_disable,
	.set_parent = samsung_clk_set_parent,
};

/* Register Offset definitions for CMU_TOPC (0x10570000) */
#define CC_PLL_LOCK		0x0000
#define BUS0_PLL_LOCK		0x0004
#define BUS1_DPLL_LOCK		0x0008
#define MFC_PLL_LOCK		0x000C
#define AUD_PLL_LOCK		0x0010
#define CC_PLL_CON0		0x0100
#define BUS0_PLL_CON0		0x0110
#define BUS1_DPLL_CON0		0x0120
#define MFC_PLL_CON0		0x0130
#define AUD_PLL_CON0		0x0140
#define MUX_SEL_TOPC0		0x0200
#define MUX_SEL_TOPC1		0x0204
#define MUX_SEL_TOPC2		0x0208
#define MUX_SEL_TOPC3		0x020C
#define DIV_TOPC0		0x0600
#define DIV_TOPC1		0x0604
#define DIV_TOPC3		0x060C
#define ENABLE_ACLK_TOPC0	0x0800
#define ENABLE_ACLK_TOPC1	0x0804
#define ENABLE_SCLK_TOPC1	0x0A04

/* List of parent clocks for Muxes in CMU_TOPC */
PNAME(mout_topc_aud_pll_ctrl_p)	= { "fin_pll", "fout_aud_pll" };
PNAME(mout_topc_bus0_pll_ctrl_p)	= { "fin_pll", "fout_bus0_pll" };
PNAME(mout_topc_bus1_pll_ctrl_p)	= { "fin_pll", "fout_bus1_pll" };
PNAME(mout_topc_cc_pll_ctrl_p)	= { "fin_pll", "fout_cc_pll" };
PNAME(mout_topc_mfc_pll_ctrl_p)	= { "fin_pll", "fout_mfc_pll" };

PNAME(mout_topc_group2) = { "mout_topc_bus0_pll_half",
	"mout_topc_bus1_pll_half", "mout_topc_cc_pll_half",
	"mout_topc_mfc_pll_half" };

PNAME(mout_topc_bus0_pll_half_p) = { "mout_topc_bus0_pll",
	"ffac_topc_bus0_pll_div2", "ffac_topc_bus0_pll_div4"};
PNAME(mout_topc_bus1_pll_half_p) = { "mout_topc_bus1_pll",
	"ffac_topc_bus1_pll_div2"};
PNAME(mout_topc_cc_pll_half_p) = { "mout_topc_cc_pll",
	"ffac_topc_cc_pll_div2"};
PNAME(mout_topc_mfc_pll_half_p) = { "mout_topc_mfc_pll",
	"ffac_topc_mfc_pll_div2"};

PNAME(mout_topc_bus0_pll_out_p) = {"mout_topc_bus0_pll",
	"ffac_topc_bus0_pll_div2"};

static const struct samsung_pll_rate_table pll1460x_24mhz_tbl[] = {
	PLL_36XX_RATE(24 * MHZ, 491519897, 20, 1, 0, 31457),
	{},
};

static int topc_cmu_probe(struct udevice *dev)
{
	void __iomem *base;

	base = dev_read_addr_ptr(dev);
	if (!base) {
		panic("%s: failed to map registers\n", __func__);
		return -EINVAL;
	}

	PLL(pll_1451x, 0, "fout_bus0_pll", "fin_pll", base + BUS0_PLL_LOCK, base + BUS0_PLL_CON0, NULL);
	PLL(pll_1452x, 0, "fout_cc_pll", "fin_pll", base + CC_PLL_LOCK, base + CC_PLL_CON0, NULL);
	PLL(pll_1452x, 0, "fout_bus1_pll", "fin_pll", base + BUS1_DPLL_LOCK, base + BUS1_DPLL_CON0, NULL);
	PLL(pll_1452x, 0, "fout_mfc_pll", "fin_pll", base + MFC_PLL_LOCK, base + MFC_PLL_CON0, NULL);
	PLL(pll_1460x, FOUT_AUD_PLL, "fout_aud_pll", "fin_pll", base + AUD_PLL_LOCK, base + AUD_PLL_CON0, pll1460x_24mhz_tbl);
	MUX(0, "mout_topc_aud_pll", mout_topc_aud_pll_ctrl_p, base + MUX_SEL_TOPC1, 0, 1);
	MUX(0, "mout_topc_bus0_pll", mout_topc_bus0_pll_ctrl_p, base + MUX_SEL_TOPC0, 0, 1);
	MUX(0, "mout_topc_bus1_pll", mout_topc_bus1_pll_ctrl_p, base + MUX_SEL_TOPC0, 4, 1);
	MUX(0, "mout_topc_cc_pll", mout_topc_cc_pll_ctrl_p, base + MUX_SEL_TOPC0, 8, 1);
	MUX(0, "mout_topc_mfc_pll", mout_topc_mfc_pll_ctrl_p, base + MUX_SEL_TOPC0, 12, 1);
	FFACTOR(0, "ffac_topc_bus1_pll_div2", "mout_topc_bus1_pll", 1, 2, 0);
	FFACTOR(0, "ffac_topc_cc_pll_div2", "mout_topc_cc_pll", 1, 2, 0);
	FFACTOR(0, "ffac_topc_mfc_pll_div2", "mout_topc_mfc_pll", 1, 2, 0);
	FFACTOR(0, "ffac_topc_bus0_pll_div2", "mout_topc_bus0_pll", 1, 2, 0);
	FFACTOR(0, "ffac_topc_bus0_pll_div4", "ffac_topc_bus0_pll_div2", 1, 2, 0);
	MUX(0, "mout_topc_bus0_pll_half", mout_topc_bus0_pll_half_p, base + MUX_SEL_TOPC0, 16, 2);
	MUX(0, "mout_topc_bus1_pll_half", mout_topc_bus1_pll_half_p, base + MUX_SEL_TOPC0, 20, 1);
	MUX(0, "mout_topc_cc_pll_half", mout_topc_cc_pll_half_p, base + MUX_SEL_TOPC0, 24, 1);
	MUX(0, "mout_topc_mfc_pll_half", mout_topc_mfc_pll_half_p, base + MUX_SEL_TOPC0, 28, 1);
	MUX(0, "mout_topc_bus0_pll_out", mout_topc_bus0_pll_out_p, base + MUX_SEL_TOPC1, 16, 1);
	MUX(0, "mout_aclk_ccore_133", mout_topc_group2,	base + MUX_SEL_TOPC2, 4, 2);
	MUX(0, "mout_aclk_mscl_532", mout_topc_group2, base + MUX_SEL_TOPC3, 20, 2);
	MUX(0, "mout_aclk_peris_66", mout_topc_group2, base + MUX_SEL_TOPC3, 24, 2);
	DIV(DOUT_ACLK_CCORE_133, "dout_aclk_ccore_133", "mout_aclk_ccore_133", base + DIV_TOPC0, 4, 4);
	DIV(DOUT_ACLK_MSCL_532, "dout_aclk_mscl_532", "mout_aclk_mscl_532", base + DIV_TOPC1, 20, 4);
	DIV(DOUT_ACLK_PERIS, "dout_aclk_peris_66", "mout_aclk_peris_66", base + DIV_TOPC1, 24, 4);
	DIV(DOUT_SCLK_BUS0_PLL, "dout_sclk_bus0_pll", "mout_topc_bus0_pll_out", base + DIV_TOPC3, 0, 4);
	DIV(DOUT_SCLK_BUS1_PLL, "dout_sclk_bus1_pll", "mout_topc_bus1_pll", base + DIV_TOPC3, 8, 4);
	DIV(DOUT_SCLK_CC_PLL, "dout_sclk_cc_pll", "mout_topc_cc_pll", base + DIV_TOPC3, 12, 4);
	DIV(DOUT_SCLK_MFC_PLL, "dout_sclk_mfc_pll", "mout_topc_mfc_pll", base + DIV_TOPC3, 16, 4);
	DIV(DOUT_SCLK_AUD_PLL, "dout_sclk_aud_pll", "mout_topc_aud_pll", base + DIV_TOPC3, 28, 4);
	GATE(ACLK_CCORE_133, "aclk_ccore_133", "dout_aclk_ccore_133", base + ENABLE_ACLK_TOPC0, 4, CLK_IS_CRITICAL, 0);
	GATE(ACLK_MSCL_532, "aclk_mscl_532", "dout_aclk_mscl_532", base + ENABLE_ACLK_TOPC1, 20, 0, 0);
	GATE(ACLK_PERIS_66, "aclk_peris_66", "dout_aclk_peris_66", base + ENABLE_ACLK_TOPC1, 24, 0, 0);
	GATE(SCLK_AUD_PLL, "sclk_aud_pll", "dout_sclk_aud_pll", base + ENABLE_SCLK_TOPC1, 20, 0, 0);
	GATE(SCLK_MFC_PLL_B, "sclk_mfc_pll_b", "dout_sclk_mfc_pll", base + ENABLE_SCLK_TOPC1, 17, 0, 0);
	GATE(SCLK_MFC_PLL_A, "sclk_mfc_pll_a", "dout_sclk_mfc_pll", base + ENABLE_SCLK_TOPC1, 16, 0, 0);
	GATE(SCLK_BUS1_PLL_B, "sclk_bus1_pll_b", "dout_sclk_bus1_pll", base + ENABLE_SCLK_TOPC1, 13, 0, 0);
	GATE(SCLK_BUS1_PLL_A, "sclk_bus1_pll_a", "dout_sclk_bus1_pll", base + ENABLE_SCLK_TOPC1, 12, 0, 0);
	GATE(SCLK_BUS0_PLL_B, "sclk_bus0_pll_b", "dout_sclk_bus0_pll", base + ENABLE_SCLK_TOPC1, 5, 0, 0);
	GATE(SCLK_BUS0_PLL_A, "sclk_bus0_pll_a", "dout_sclk_bus0_pll", base + ENABLE_SCLK_TOPC1, 4, 0, 0);
	GATE(SCLK_CC_PLL_B, "sclk_cc_pll_b", "dout_sclk_cc_pll", base + ENABLE_SCLK_TOPC1, 1, 0, 0);
	GATE(SCLK_CC_PLL_A, "sclk_cc_pll_a", "dout_sclk_cc_pll", base + ENABLE_SCLK_TOPC1, 0, 0, 0);

	return 0;
}

static const struct udevice_id topc_cmu_compat[] = {
	{ .compatible = "samsung,exynos7-clock-topc" },
	{ }
};

U_BOOT_DRIVER(exynos7_clock_topc) = {
	.name = "exynos7-clock-topc",
	.id = UCLASS_CLK,
	.of_match = topc_cmu_compat,
	.probe = topc_cmu_probe,
	.ops = &samsung_clk_ops,
	.flags = DM_FLAG_PRE_RELOC,
};

/* Register Offset definitions for CMU_TOP0 (0x105D0000) */
#define MUX_SEL_TOP00			0x0200
#define MUX_SEL_TOP01			0x0204
#define MUX_SEL_TOP03			0x020C
#define MUX_SEL_TOP0_PERIC0		0x0230
#define MUX_SEL_TOP0_PERIC1		0x0234
#define MUX_SEL_TOP0_PERIC2		0x0238
#define MUX_SEL_TOP0_PERIC3		0x023C
#define DIV_TOP03			0x060C
#define DIV_TOP0_PERIC0			0x0630
#define DIV_TOP0_PERIC1			0x0634
#define DIV_TOP0_PERIC2			0x0638
#define DIV_TOP0_PERIC3			0x063C
#define ENABLE_ACLK_TOP03		0x080C
#define ENABLE_SCLK_TOP0_PERIC0		0x0A30
#define ENABLE_SCLK_TOP0_PERIC1		0x0A34
#define ENABLE_SCLK_TOP0_PERIC2		0x0A38
#define ENABLE_SCLK_TOP0_PERIC3		0x0A3C

/* List of parent clocks for Muxes in CMU_TOP0 */
PNAME(mout_top0_bus0_pll_user_p)	= { "fin_pll", "sclk_bus0_pll_a" };
PNAME(mout_top0_bus1_pll_user_p)	= { "fin_pll", "sclk_bus1_pll_a" };
PNAME(mout_top0_cc_pll_user_p)	= { "fin_pll", "sclk_cc_pll_a" };
PNAME(mout_top0_mfc_pll_user_p)	= { "fin_pll", "sclk_mfc_pll_a" };
PNAME(mout_top0_aud_pll_user_p)	= { "fin_pll", "sclk_aud_pll" };

PNAME(mout_top0_bus0_pll_half_p) = {"mout_top0_bus0_pll_user",
	"ffac_top0_bus0_pll_div2"};
PNAME(mout_top0_bus1_pll_half_p) = {"mout_top0_bus1_pll_user",
	"ffac_top0_bus1_pll_div2"};
PNAME(mout_top0_cc_pll_half_p) = {"mout_top0_cc_pll_user",
	"ffac_top0_cc_pll_div2"};
PNAME(mout_top0_mfc_pll_half_p) = {"mout_top0_mfc_pll_user",
	"ffac_top0_mfc_pll_div2"};

PNAME(mout_top0_group1) = {"mout_top0_bus0_pll_half",
	"mout_top0_bus1_pll_half", "mout_top0_cc_pll_half",
	"mout_top0_mfc_pll_half"};
PNAME(mout_top0_group3) = {"ioclk_audiocdclk0",
	"ioclk_audiocdclk1", "ioclk_spdif_extclk",
	"mout_top0_aud_pll_user", "mout_top0_bus0_pll_half",
	"mout_top0_bus1_pll_half"};
PNAME(mout_top0_group4) = {"ioclk_audiocdclk1", "mout_top0_aud_pll_user",
	"mout_top0_bus0_pll_half", "mout_top0_bus1_pll_half"};

static int top0_cmu_probe(struct udevice *dev)
{
	void __iomem *base;

	base = dev_read_addr_ptr(dev);
	if (!base) {
		panic("%s: failed to map registers\n", __func__);
		return -EINVAL;
	}

	MUX(0, "mout_top0_bus1_pll_user", mout_top0_bus1_pll_user_p, base + MUX_SEL_TOP00, 12, 1);
	MUX(0, "mout_top0_cc_pll_user", mout_top0_cc_pll_user_p, base + MUX_SEL_TOP00, 8, 1);
	MUX(0, "mout_top0_bus0_pll_user", mout_top0_bus0_pll_user_p, base + MUX_SEL_TOP00, 16, 1);
	MUX(0, "mout_top0_mfc_pll_user", mout_top0_mfc_pll_user_p, base + MUX_SEL_TOP00, 4, 1);
	FFACTOR(0, "ffac_top0_bus0_pll_div2", "mout_top0_bus0_pll_user", 1, 2, 0);
	FFACTOR(0, "ffac_top0_bus1_pll_div2", "mout_top0_bus1_pll_user", 1, 2, 0);
	FFACTOR(0, "ffac_top0_cc_pll_div2", "mout_top0_cc_pll_user", 1, 2, 0);
	FFACTOR(0, "ffac_top0_mfc_pll_div2", "mout_top0_mfc_pll_user", 1, 2, 0);
	MUX(0, "mout_top0_bus1_pll_half", mout_top0_bus1_pll_half_p, base + MUX_SEL_TOP01, 12, 1);
	MUX(0, "mout_top0_cc_pll_half", mout_top0_cc_pll_half_p, base + MUX_SEL_TOP01, 8, 1);
	MUX(0, "mout_top0_aud_pll_user", mout_top0_aud_pll_user_p, base + MUX_SEL_TOP00, 0, 1);
	MUX(0, "mout_top0_bus0_pll_half", mout_top0_bus0_pll_half_p, base + MUX_SEL_TOP01, 16, 1);
	MUX(0, "mout_top0_mfc_pll_half", mout_top0_mfc_pll_half_p, base + MUX_SEL_TOP01, 4, 1);
	MUX(0, "mout_aclk_peric1_66", mout_top0_group1, base + MUX_SEL_TOP03, 12, 2);
	MUX(0, "mout_aclk_peric0_66", mout_top0_group1, base + MUX_SEL_TOP03, 20, 2);
	MUX(0, "mout_sclk_spdif", mout_top0_group3, base + MUX_SEL_TOP0_PERIC0, 4, 3);
	MUX(0, "mout_sclk_pcm1", mout_top0_group4, base + MUX_SEL_TOP0_PERIC0, 8, 2);
	MUX(0, "mout_sclk_i2s1", mout_top0_group4, base + MUX_SEL_TOP0_PERIC0, 20, 2);
	MUX(0, "mout_sclk_spi1", mout_top0_group1, base + MUX_SEL_TOP0_PERIC1, 8, 2);
	MUX(0, "mout_sclk_spi0", mout_top0_group1, base + MUX_SEL_TOP0_PERIC1, 20, 2);
	MUX(0, "mout_sclk_spi3", mout_top0_group1, base + MUX_SEL_TOP0_PERIC2, 8, 2);
	MUX(0, "mout_sclk_spi2", mout_top0_group1, base + MUX_SEL_TOP0_PERIC2, 20, 2);
	MUX(0, "mout_sclk_uart3", mout_top0_group1, base + MUX_SEL_TOP0_PERIC3, 4, 2);
	MUX(0, "mout_sclk_uart2", mout_top0_group1, base + MUX_SEL_TOP0_PERIC3, 8, 2);
	MUX(0, "mout_sclk_uart1", mout_top0_group1, base + MUX_SEL_TOP0_PERIC3, 12, 2);
	MUX(0, "mout_sclk_uart0", mout_top0_group1, base + MUX_SEL_TOP0_PERIC3, 16, 2);
	MUX(0, "mout_sclk_spi4", mout_top0_group1, base + MUX_SEL_TOP0_PERIC3, 20, 2);
	DIV(DOUT_ACLK_PERIC1, "dout_aclk_peric1_66", "mout_aclk_peric1_66", base + DIV_TOP03, 12, 6);
	DIV(DOUT_ACLK_PERIC0, "dout_aclk_peric0_66", "mout_aclk_peric0_66", base + DIV_TOP03, 20, 6);
	DIV(0, "dout_sclk_spdif", "mout_sclk_spdif", base + DIV_TOP0_PERIC0, 4, 4);
	DIV(0, "dout_sclk_pcm1", "mout_sclk_pcm1", base + DIV_TOP0_PERIC0, 8, 12);
	DIV(0, "dout_sclk_i2s1", "mout_sclk_i2s1", base + DIV_TOP0_PERIC0, 20, 10);
	DIV(0, "dout_sclk_spi1", "mout_sclk_spi1", base + DIV_TOP0_PERIC1, 8, 12);
	DIV(0, "dout_sclk_spi0", "mout_sclk_spi0", base + DIV_TOP0_PERIC1, 20, 12);
	DIV(0, "dout_sclk_spi3", "mout_sclk_spi3", base + DIV_TOP0_PERIC2, 8, 12);
	DIV(0, "dout_sclk_spi2", "mout_sclk_spi2", base + DIV_TOP0_PERIC2, 20, 12);
	DIV(0, "dout_sclk_uart3", "mout_sclk_uart3", base + DIV_TOP0_PERIC3, 4, 4);
	DIV(0, "dout_sclk_uart2", "mout_sclk_uart2", base + DIV_TOP0_PERIC3, 8, 4);
	DIV(0, "dout_sclk_uart1", "mout_sclk_uart1", base + DIV_TOP0_PERIC3, 12, 4);
	DIV(0, "dout_sclk_uart0", "mout_sclk_uart0", base + DIV_TOP0_PERIC3, 16, 4);
	DIV(0, "dout_sclk_spi4", "mout_sclk_spi4", base + DIV_TOP0_PERIC3, 20, 12);
	GATE(CLK_ACLK_PERIC0_66, "aclk_peric0_66", "dout_aclk_peric0_66", base + ENABLE_ACLK_TOP03, 20, CLK_SET_RATE_PARENT, 0);
	GATE(CLK_ACLK_PERIC1_66, "aclk_peric1_66", "dout_aclk_peric1_66", base + ENABLE_ACLK_TOP03, 12, CLK_SET_RATE_PARENT, 0);
	GATE(CLK_SCLK_SPDIF, "sclk_spdif", "dout_sclk_spdif", base + ENABLE_SCLK_TOP0_PERIC0, 4, CLK_SET_RATE_PARENT, 0);
	GATE(CLK_SCLK_PCM1, "sclk_pcm1", "dout_sclk_pcm1", base + ENABLE_SCLK_TOP0_PERIC0, 8, CLK_SET_RATE_PARENT, 0);
	GATE(CLK_SCLK_I2S1, "sclk_i2s1", "dout_sclk_i2s1", base + ENABLE_SCLK_TOP0_PERIC0, 20, CLK_SET_RATE_PARENT, 0);
	GATE(CLK_SCLK_SPI1, "sclk_spi1", "dout_sclk_spi1", base + ENABLE_SCLK_TOP0_PERIC1, 8, CLK_SET_RATE_PARENT, 0);
	GATE(CLK_SCLK_SPI0, "sclk_spi0", "dout_sclk_spi0", base + ENABLE_SCLK_TOP0_PERIC1, 20, CLK_SET_RATE_PARENT, 0);
	GATE(CLK_SCLK_SPI3, "sclk_spi3", "dout_sclk_spi3", base + ENABLE_SCLK_TOP0_PERIC2, 8, CLK_SET_RATE_PARENT, 0);
	GATE(CLK_SCLK_SPI2, "sclk_spi2", "dout_sclk_spi2", base + ENABLE_SCLK_TOP0_PERIC2, 20, CLK_SET_RATE_PARENT, 0);
	GATE(CLK_SCLK_UART3, "sclk_uart3", "dout_sclk_uart3", base + ENABLE_SCLK_TOP0_PERIC3, 4, 0, 0);
	GATE(CLK_SCLK_UART2, "sclk_uart2", "dout_sclk_uart2", base + ENABLE_SCLK_TOP0_PERIC3, 8, 0, 0);
	GATE(CLK_SCLK_UART1, "sclk_uart1", "dout_sclk_uart1", base + ENABLE_SCLK_TOP0_PERIC3, 12, 0, 0);
	GATE(CLK_SCLK_UART0, "sclk_uart0", "dout_sclk_uart0", base + ENABLE_SCLK_TOP0_PERIC3, 16, 0, 0);
	GATE(CLK_SCLK_SPI4, "sclk_spi4", "dout_sclk_spi4", base + ENABLE_SCLK_TOP0_PERIC3, 20, CLK_SET_RATE_PARENT, 0);


	return 0;
}

static const struct udevice_id top0_cmu_compat[] = {
	{ .compatible = "samsung,exynos7-clock-top0" },
	{ }
};

U_BOOT_DRIVER(exynos7_clock_top0) = {
	.name = "exynos7-clock-top0",
	.id = UCLASS_CLK,
	.of_match = top0_cmu_compat,
	.probe = top0_cmu_probe,
	.ops = &samsung_clk_ops,
	.flags = DM_FLAG_PRE_RELOC,
};

/* Register Offset definitions for CMU_TOP1 (0x105E0000) */
#define MUX_SEL_TOP10			0x0200
#define MUX_SEL_TOP11			0x0204
#define MUX_SEL_TOP13			0x020C
#define MUX_SEL_TOP1_FSYS0		0x0224
#define MUX_SEL_TOP1_FSYS1		0x0228
#define MUX_SEL_TOP1_FSYS11		0x022C
#define DIV_TOP13			0x060C
#define DIV_TOP1_FSYS0			0x0624
#define DIV_TOP1_FSYS1			0x0628
#define DIV_TOP1_FSYS11			0x062C
#define ENABLE_ACLK_TOP13		0x080C
#define ENABLE_SCLK_TOP1_FSYS0		0x0A24
#define ENABLE_SCLK_TOP1_FSYS1		0x0A28
#define ENABLE_SCLK_TOP1_FSYS11		0x0A2C

/* List of parent clocks for Muxes in CMU_TOP1 */
PNAME(mout_top1_bus0_pll_user_p)	= { "fin_pll", "sclk_bus0_pll_b" };
PNAME(mout_top1_bus1_pll_user_p)	= { "fin_pll", "sclk_bus1_pll_b" };
PNAME(mout_top1_cc_pll_user_p)	= { "fin_pll", "sclk_cc_pll_b" };
PNAME(mout_top1_mfc_pll_user_p)	= { "fin_pll", "sclk_mfc_pll_b" };

PNAME(mout_top1_bus0_pll_half_p) = {"mout_top1_bus0_pll_user",
	"ffac_top1_bus0_pll_div2"};
PNAME(mout_top1_bus1_pll_half_p) = {"mout_top1_bus1_pll_user",
	"ffac_top1_bus1_pll_div2"};
PNAME(mout_top1_cc_pll_half_p) = {"mout_top1_cc_pll_user",
	"ffac_top1_cc_pll_div2"};
PNAME(mout_top1_mfc_pll_half_p) = {"mout_top1_mfc_pll_user",
	"ffac_top1_mfc_pll_div2"};

PNAME(mout_top1_group1) = {"mout_top1_bus0_pll_half",
	"mout_top1_bus1_pll_half", "mout_top1_cc_pll_half",
	"mout_top1_mfc_pll_half"};

static int top1_cmu_probe(struct udevice *dev)
{
	void __iomem *base;

	base = dev_read_addr_ptr(dev);
	if (!base) {
		panic("%s: failed to map registers\n", __func__);
		return -EINVAL;
	}

	MUX(0, "mout_top1_mfc_pll_user", mout_top1_mfc_pll_user_p, base + MUX_SEL_TOP10, 4, 1);
	MUX(0, "mout_top1_cc_pll_user", mout_top1_cc_pll_user_p, base + MUX_SEL_TOP10, 8, 1);
	MUX(0, "mout_top1_bus1_pll_user", mout_top1_bus1_pll_user_p, base + MUX_SEL_TOP10, 12, 1);
	MUX(0, "mout_top1_bus0_pll_user", mout_top1_bus0_pll_user_p, base + MUX_SEL_TOP10, 16, 1);
	MUX(0, "mout_top1_mfc_pll_half", mout_top1_mfc_pll_half_p, base + MUX_SEL_TOP11, 4, 1);
	MUX(0, "mout_top1_cc_pll_half", mout_top1_cc_pll_half_p, base + MUX_SEL_TOP11, 8, 1);
	MUX(0, "mout_top1_bus1_pll_half", mout_top1_bus1_pll_half_p, base + MUX_SEL_TOP11, 12, 1);
	MUX(0, "mout_top1_bus0_pll_half", mout_top1_bus0_pll_half_p, base + MUX_SEL_TOP11, 16, 1);
	MUX(0, "mout_aclk_fsys1_200", mout_top1_group1, base + MUX_SEL_TOP13, 24, 2);
	MUX(0, "mout_aclk_fsys0_200", mout_top1_group1, base + MUX_SEL_TOP13, 28, 2);
	MUX(0, "mout_sclk_phy_fsys0_26m", mout_top1_group1, base + MUX_SEL_TOP1_FSYS0, 0, 2);
	MUX(0, "mout_sclk_mmc2", mout_top1_group1, base + MUX_SEL_TOP1_FSYS0, 16, 2);
	MUX(0, "mout_sclk_usbdrd300", mout_top1_group1, base + MUX_SEL_TOP1_FSYS0, 28, 2);
	MUX(0, "mout_sclk_phy_fsys1", mout_top1_group1, base + MUX_SEL_TOP1_FSYS1, 0, 2);
	MUX(0, "mout_sclk_ufsunipro20", mout_top1_group1, base + MUX_SEL_TOP1_FSYS1, 16, 2);
	MUX(0, "mout_sclk_mmc1", mout_top1_group1, base + MUX_SEL_TOP1_FSYS11, 0, 2);
	MUX(0, "mout_sclk_mmc0", mout_top1_group1, base + MUX_SEL_TOP1_FSYS11, 12, 2);
	MUX(0, "mout_sclk_phy_fsys1_26m", mout_top1_group1, base + MUX_SEL_TOP1_FSYS11, 24, 2);
	DIV(DOUT_ACLK_FSYS1_200, "dout_aclk_fsys1_200", "mout_aclk_fsys1_200", base + DIV_TOP13, 24, 4);
	DIV(DOUT_ACLK_FSYS0_200, "dout_aclk_fsys0_200", "mout_aclk_fsys0_200", base + DIV_TOP13, 28, 4);
	DIV(DOUT_SCLK_PHY_FSYS1, "dout_sclk_phy_fsys1", "mout_sclk_phy_fsys1", base + DIV_TOP1_FSYS1, 0, 6);
	DIV(DOUT_SCLK_UFSUNIPRO20, "dout_sclk_ufsunipro20", "mout_sclk_ufsunipro20", base + DIV_TOP1_FSYS1, 16, 6);
	DIV(DOUT_SCLK_MMC2, "dout_sclk_mmc2", "mout_sclk_mmc2", base + DIV_TOP1_FSYS0, 16, 10);
	DIV(0, "dout_sclk_usbdrd300", "mout_sclk_usbdrd300", base + DIV_TOP1_FSYS0, 28, 4);
	DIV(DOUT_SCLK_MMC1, "dout_sclk_mmc1", "mout_sclk_mmc1", base + DIV_TOP1_FSYS11, 0, 10);
	DIV(DOUT_SCLK_MMC0, "dout_sclk_mmc0", "mout_sclk_mmc0", base + DIV_TOP1_FSYS11, 12, 10);
	DIV(DOUT_SCLK_PHY_FSYS1_26M, "dout_sclk_phy_fsys1_26m", "mout_sclk_phy_fsys1_26m", base + DIV_TOP1_FSYS11, 24, 6);
	GATE(CLK_SCLK_MMC2, "sclk_mmc2", "dout_sclk_mmc2", base + ENABLE_SCLK_TOP1_FSYS0, 16, CLK_SET_RATE_PARENT, 0);
	GATE(0, "sclk_usbdrd300", "dout_sclk_usbdrd300", base + ENABLE_SCLK_TOP1_FSYS0, 28, 0, 0);
	GATE(CLK_SCLK_PHY_FSYS1, "sclk_phy_fsys1", "dout_sclk_phy_fsys1", base + ENABLE_SCLK_TOP1_FSYS1, 0, CLK_SET_RATE_PARENT, 0);
	GATE(CLK_SCLK_UFSUNIPRO20, "sclk_ufsunipro20", "dout_sclk_ufsunipro20", base + ENABLE_SCLK_TOP1_FSYS1, 16, CLK_SET_RATE_PARENT, 0);
	GATE(CLK_SCLK_MMC1, "sclk_mmc1", "dout_sclk_mmc1", base + ENABLE_SCLK_TOP1_FSYS11, 0, CLK_SET_RATE_PARENT, 0);
	GATE(CLK_SCLK_MMC0, "sclk_mmc0", "dout_sclk_mmc0", base + ENABLE_SCLK_TOP1_FSYS11, 12, CLK_SET_RATE_PARENT, 0);
	GATE(CLK_ACLK_FSYS0_200, "aclk_fsys0_200", "dout_aclk_fsys0_200", base + ENABLE_ACLK_TOP13, 28, CLK_SET_RATE_PARENT | CLK_IS_CRITICAL, 0);
	GATE(CLK_ACLK_FSYS1_200, "aclk_fsys1_200", "dout_aclk_fsys1_200", base + ENABLE_ACLK_TOP13, 24, CLK_SET_RATE_PARENT, 0);
	GATE(CLK_SCLK_PHY_FSYS1_26M, "sclk_phy_fsys1_26m", "dout_sclk_phy_fsys1_26m", base + ENABLE_SCLK_TOP1_FSYS11, 24, CLK_SET_RATE_PARENT, 0);
	FFACTOR(0, "ffac_top1_bus0_pll_div2", "mout_top1_bus0_pll_user", 1, 2, 0);
	FFACTOR(0, "ffac_top1_bus1_pll_div2", "mout_top1_bus1_pll_user", 1, 2, 0);
	FFACTOR(0, "ffac_top1_cc_pll_div2", "mout_top1_cc_pll_user", 1, 2, 0);
	FFACTOR(0, "ffac_top1_mfc_pll_div2", "mout_top1_mfc_pll_user", 1, 2, 0);

	return 0;
}

static const struct udevice_id top1_cmu_compat[] = {
	{ .compatible = "samsung,exynos7-clock-top1" },
	{ }
};

U_BOOT_DRIVER(exynos7_clock_top1) = {
	.name = "exynos7-clock-top1",
	.id = UCLASS_CLK,
	.of_match = top1_cmu_compat,
	.probe = top1_cmu_probe,
	.ops = &samsung_clk_ops,
	.flags = DM_FLAG_PRE_RELOC,
};

/* Register Offset definitions for CMU_CCORE (0x105B0000) */
#define MUX_SEL_CCORE			0x0200
#define DIV_CCORE			0x0600
#define ENABLE_ACLK_CCORE0		0x0800
#define ENABLE_ACLK_CCORE1		0x0804
#define ENABLE_PCLK_CCORE		0x0900

/*
 * List of parent clocks for Muxes in CMU_CCORE
 */
PNAME(mout_aclk_ccore_133_user_p)	= { "fin_pll", "aclk_ccore_133" };

static int ccore_cmu_probe(struct udevice *dev)
{
	void __iomem *base;

	base = dev_read_addr_ptr(dev);
	if (!base) {
		panic("%s: failed to map registers\n", __func__);
		return -EINVAL;
	}

	MUX(0, "mout_aclk_ccore_133_user", mout_aclk_ccore_133_user_p, base + MUX_SEL_CCORE, 1, 1);

	GATE(PCLK_RTC, "pclk_rtc", "mout_aclk_ccore_133_user", base + ENABLE_PCLK_CCORE, 8, 0, 0);

	return 0;
}

static const struct udevice_id ccore_cmu_compat[] = {
	{ .compatible = "samsung,exynos7-clock-ccore" },
	{ }
};

U_BOOT_DRIVER(exynos7_clock_ccore) = {
	.name = "exynos7-clock-ccore",
	.id = UCLASS_CLK,
	.of_match = ccore_cmu_compat,
	.probe = ccore_cmu_probe,
	.ops = &samsung_clk_ops,
	.flags = DM_FLAG_PRE_RELOC,
};

/* Register Offset definitions for CMU_PERIC0 (0x13610000) */
#define MUX_SEL_PERIC0			0x0200
#define ENABLE_PCLK_PERIC0		0x0900
#define ENABLE_SCLK_PERIC0		0x0A00

/* List of parent clocks for Muxes in CMU_PERIC0 */
PNAME(mout_aclk_peric0_66_user_p)	= { "fin_pll", "aclk_peric0_66" };
PNAME(mout_sclk_uart0_user_p)	= { "fin_pll", "sclk_uart0" };

static int peric0_cmu_probe(struct udevice *dev)
{
	void __iomem *base;

	base = dev_read_addr_ptr(dev);
	if (!base) {
		panic("%s: failed to map registers\n", __func__);
		return -EINVAL;
	}

	MUX(0, "mout_aclk_peric0_66_user", mout_aclk_peric0_66_user_p, base + MUX_SEL_PERIC0, 0, 1);
	MUX(0, "mout_sclk_uart0_user", mout_sclk_uart0_user_p, base + MUX_SEL_PERIC0, 16, 1);
	GATE(PCLK_HSI2C0, "pclk_hsi2c0", "mout_aclk_peric0_66_user", base + ENABLE_PCLK_PERIC0, 8, 0, 0);
	GATE(PCLK_HSI2C1, "pclk_hsi2c1", "mout_aclk_peric0_66_user", base + ENABLE_PCLK_PERIC0, 9, 0, 0);
	GATE(PCLK_HSI2C4, "pclk_hsi2c4", "mout_aclk_peric0_66_user", base + ENABLE_PCLK_PERIC0, 10, 0, 0);
	GATE(PCLK_HSI2C5, "pclk_hsi2c5", "mout_aclk_peric0_66_user", base + ENABLE_PCLK_PERIC0, 11, 0, 0);
	GATE(PCLK_HSI2C9, "pclk_hsi2c9", "mout_aclk_peric0_66_user", base + ENABLE_PCLK_PERIC0, 12, 0, 0);
	GATE(PCLK_HSI2C10, "pclk_hsi2c10", "mout_aclk_peric0_66_user", base + ENABLE_PCLK_PERIC0, 13, 0, 0);
	GATE(PCLK_HSI2C11, "pclk_hsi2c11", "mout_aclk_peric0_66_user", base + ENABLE_PCLK_PERIC0, 14, 0, 0);
	GATE(PCLK_UART0, "pclk_uart0", "mout_aclk_peric0_66_user", base + ENABLE_PCLK_PERIC0, 16, 0, 0);
	GATE(PCLK_ADCIF, "pclk_adcif", "mout_aclk_peric0_66_user", base + ENABLE_PCLK_PERIC0, 20, 0, 0);
	GATE(PCLK_PWM, "pclk_pwm", "mout_aclk_peric0_66_user", base + ENABLE_PCLK_PERIC0, 21, 0, 0);
	GATE(SCLK_UART0, "sclk_uart0_user", "mout_sclk_uart0_user", base + ENABLE_SCLK_PERIC0, 16, 0, 0);
	GATE(SCLK_PWM, "sclk_pwm", "fin_pll", base + ENABLE_SCLK_PERIC0, 21, 0, 0);

	return 0;
}

static const struct udevice_id peric0_cmu_compat[] = {
	{ .compatible = "samsung,exynos7-clock-peric0" },
	{ }
};

U_BOOT_DRIVER(exynos7_clock_peric0) = {
	.name = "exynos7-clock-peric0",
	.id = UCLASS_CLK,
	.of_match = peric0_cmu_compat,
	.probe = peric0_cmu_probe,
	.ops = &samsung_clk_ops,
	.flags = DM_FLAG_PRE_RELOC,
};

/* Register Offset definitions for CMU_PERIC1 (0x14C80000) */
#define MUX_SEL_PERIC10			0x0200
#define MUX_SEL_PERIC11			0x0204
#define MUX_SEL_PERIC12			0x0208
#define ENABLE_PCLK_PERIC1		0x0900
#define ENABLE_SCLK_PERIC10		0x0A00

/* List of parent clocks for Muxes in CMU_PERIC1 */
PNAME(mout_aclk_peric1_66_user_p)	= { "fin_pll", "aclk_peric1_66" };
PNAME(mout_sclk_uart1_user_p)	= { "fin_pll", "sclk_uart1" };
PNAME(mout_sclk_uart2_user_p)	= { "fin_pll", "sclk_uart2" };
PNAME(mout_sclk_uart3_user_p)	= { "fin_pll", "sclk_uart3" };
PNAME(mout_sclk_spi0_user_p)		= { "fin_pll", "sclk_spi0" };
PNAME(mout_sclk_spi1_user_p)		= { "fin_pll", "sclk_spi1" };
PNAME(mout_sclk_spi2_user_p)		= { "fin_pll", "sclk_spi2" };
PNAME(mout_sclk_spi3_user_p)		= { "fin_pll", "sclk_spi3" };
PNAME(mout_sclk_spi4_user_p)		= { "fin_pll", "sclk_spi4" };

static int peric1_cmu_probe(struct udevice *dev)
{
	void __iomem *base;

	base = dev_read_addr_ptr(dev);
	if (!base) {
		panic("%s: failed to map registers\n", __func__);
		return -EINVAL;
	}

	MUX(0, "mout_aclk_peric1_66_user", mout_aclk_peric1_66_user_p, base + MUX_SEL_PERIC10, 0, 1);
	MUX(0, "mout_sclk_uart1_user", mout_sclk_uart1_user_p, base + MUX_SEL_PERIC11, 20, 1),
	MUX(0, "mout_sclk_uart2_user", mout_sclk_uart2_user_p, base + MUX_SEL_PERIC11, 24, 1);
	MUX(0, "mout_sclk_uart3_user", mout_sclk_uart3_user_p, base + MUX_SEL_PERIC11, 28, 1);
	MUX_F(0, "mout_sclk_spi0_user", mout_sclk_spi0_user_p, base + MUX_SEL_PERIC11, 0, 1, CLK_SET_RATE_PARENT, 0);
	MUX_F(0, "mout_sclk_spi1_user", mout_sclk_spi1_user_p, base + MUX_SEL_PERIC11, 4, 1, CLK_SET_RATE_PARENT, 0);
	MUX_F(0, "mout_sclk_spi2_user", mout_sclk_spi2_user_p, base + MUX_SEL_PERIC11, 8, 1, CLK_SET_RATE_PARENT, 0);
	MUX_F(0, "mout_sclk_spi3_user", mout_sclk_spi3_user_p, base + MUX_SEL_PERIC11, 12, 1, CLK_SET_RATE_PARENT, 0);
	MUX_F(0, "mout_sclk_spi4_user", mout_sclk_spi4_user_p, base + MUX_SEL_PERIC11, 16, 1, CLK_SET_RATE_PARENT, 0);
	GATE(PCLK_HSI2C2, "pclk_hsi2c2", "mout_aclk_peric1_66_user", base + ENABLE_PCLK_PERIC1, 4, 0, 0);
	GATE(PCLK_HSI2C3, "pclk_hsi2c3", "mout_aclk_peric1_66_user", base + ENABLE_PCLK_PERIC1, 5, 0, 0);
	GATE(PCLK_HSI2C6, "pclk_hsi2c6", "mout_aclk_peric1_66_user", base + ENABLE_PCLK_PERIC1, 6, 0, 0);
	GATE(PCLK_HSI2C7, "pclk_hsi2c7", "mout_aclk_peric1_66_user", base + ENABLE_PCLK_PERIC1, 7, 0, 0);
	GATE(PCLK_HSI2C8, "pclk_hsi2c8", "mout_aclk_peric1_66_user", base + ENABLE_PCLK_PERIC1, 8, 0, 0);
	GATE(PCLK_UART1, "pclk_uart1", "mout_aclk_peric1_66_user", base + ENABLE_PCLK_PERIC1, 9, 0, 0);
	GATE(PCLK_UART2, "pclk_uart2", "mout_aclk_peric1_66_user", base + ENABLE_PCLK_PERIC1, 10, 0, 0);
	GATE(PCLK_UART3, "pclk_uart3", "mout_aclk_peric1_66_user", base + ENABLE_PCLK_PERIC1, 11, 0, 0);
	GATE(PCLK_SPI0, "pclk_spi0", "mout_aclk_peric1_66_user", base + ENABLE_PCLK_PERIC1, 12, 0, 0);
	GATE(PCLK_SPI1, "pclk_spi1", "mout_aclk_peric1_66_user", base + ENABLE_PCLK_PERIC1, 13, 0, 0);
	GATE(PCLK_SPI2, "pclk_spi2", "mout_aclk_peric1_66_user", base + ENABLE_PCLK_PERIC1, 14, 0, 0);
	GATE(PCLK_SPI3, "pclk_spi3", "mout_aclk_peric1_66_user", base + ENABLE_PCLK_PERIC1, 15, 0, 0);
	GATE(PCLK_SPI4, "pclk_spi4", "mout_aclk_peric1_66_user", base + ENABLE_PCLK_PERIC1, 16, 0, 0);
	GATE(PCLK_I2S1, "pclk_i2s1", "mout_aclk_peric1_66_user", base + ENABLE_PCLK_PERIC1, 17, CLK_SET_RATE_PARENT, 0);
	GATE(PCLK_PCM1, "pclk_pcm1", "mout_aclk_peric1_66_user", base + ENABLE_PCLK_PERIC1, 18, 0, 0);
	GATE(PCLK_SPDIF, "pclk_spdif", "mout_aclk_peric1_66_user", base + ENABLE_PCLK_PERIC1, 19, 0, 0);
	GATE(SCLK_UART1, "sclk_uart1_user", "mout_sclk_uart1_user", base + ENABLE_SCLK_PERIC10, 9, 0, 0);
	GATE(SCLK_UART2, "sclk_uart2_user", "mout_sclk_uart2_user", base + ENABLE_SCLK_PERIC10, 10, 0, 0);
	GATE(SCLK_UART3, "sclk_uart3_user", "mout_sclk_uart3_user", base + ENABLE_SCLK_PERIC10, 11, 0, 0);
	GATE(SCLK_SPI0, "sclk_spi0_user", "mout_sclk_spi0_user", base + ENABLE_SCLK_PERIC10, 12, CLK_SET_RATE_PARENT, 0);
	GATE(SCLK_SPI1, "sclk_spi1_user", "mout_sclk_spi1_user", base + ENABLE_SCLK_PERIC10, 13, CLK_SET_RATE_PARENT, 0);
	GATE(SCLK_SPI2, "sclk_spi2_user", "mout_sclk_spi2_user", base + ENABLE_SCLK_PERIC10, 14, CLK_SET_RATE_PARENT, 0);
	GATE(SCLK_SPI3, "sclk_spi3_user", "mout_sclk_spi3_user", base + ENABLE_SCLK_PERIC10, 15, CLK_SET_RATE_PARENT, 0);
	GATE(SCLK_SPI4, "sclk_spi4_user", "mout_sclk_spi4_user", base + ENABLE_SCLK_PERIC10, 16, CLK_SET_RATE_PARENT, 0);
	GATE(SCLK_I2S1, "sclk_i2s1_user", "sclk_i2s1", base + ENABLE_SCLK_PERIC10, 17, CLK_SET_RATE_PARENT, 0);
	GATE(SCLK_PCM1, "sclk_pcm1_user", "sclk_pcm1", base + ENABLE_SCLK_PERIC10, 18, CLK_SET_RATE_PARENT, 0);
	GATE(SCLK_SPDIF, "sclk_spdif_user", "sclk_spdif", base + ENABLE_SCLK_PERIC10, 19, CLK_SET_RATE_PARENT, 0);

	return 0;
}

static const struct udevice_id peric1_cmu_compat[] = {
	{ .compatible = "samsung,exynos7-clock-peric1" },
	{ }
};

U_BOOT_DRIVER(exynos7_clock_peric1) = {
	.name = "exynos7-clock-peric1",
	.id = UCLASS_CLK,
	.of_match = peric1_cmu_compat,
	.probe = peric1_cmu_probe,
	.ops = &samsung_clk_ops,
	.flags = DM_FLAG_PRE_RELOC,
};

/* Register Offset definitions for CMU_PERIS (0x10040000) */
#define MUX_SEL_PERIS			0x0200
#define ENABLE_PCLK_PERIS		0x0900
#define ENABLE_PCLK_PERIS_SECURE_CHIPID	0x0910
#define ENABLE_SCLK_PERIS		0x0A00
#define ENABLE_SCLK_PERIS_SECURE_CHIPID	0x0A10

/* List of parent clocks for Muxes in CMU_PERIS */
PNAME(mout_aclk_peris_66_user_p) = { "fin_pll", "aclk_peris_66" };

static int peris_cmu_probe(struct udevice *dev)
{
	void __iomem *base;

	base = dev_read_addr_ptr(dev);
	if (!base) {
		panic("%s: failed to map registers\n", __func__);
		return -EINVAL;
	}

	MUX(0, "mout_aclk_peris_66_user", mout_aclk_peris_66_user_p, base + MUX_SEL_PERIS, 0, 1);
	GATE(PCLK_WDT, "pclk_wdt", "mout_aclk_peris_66_user", base + ENABLE_PCLK_PERIS, 6, 0, 0);
	GATE(PCLK_MCT, "pclk_mct", "mout_aclk_peris_66_user", base + ENABLE_PCLK_PERIS, 5, 0, 0);
	GATE(PCLK_TMU, "pclk_tmu_apbif", "mout_aclk_peris_66_user", base + ENABLE_PCLK_PERIS, 10, 0, 0);
	GATE(PCLK_CHIPID, "pclk_chipid", "mout_aclk_peris_66_user", base + ENABLE_PCLK_PERIS_SECURE_CHIPID, 0, 0, 0);
	GATE(SCLK_CHIPID, "sclk_chipid", "fin_pll", base + ENABLE_SCLK_PERIS_SECURE_CHIPID, 0, 0, 0);
	GATE(SCLK_TMU, "sclk_tmu", "fin_pll", base + ENABLE_SCLK_PERIS, 10, 0, 0);

	return 0;
}

static const struct udevice_id peris_cmu_compat[] = {
	{ .compatible = "samsung,exynos7-clock-peris" },
	{ }
};

U_BOOT_DRIVER(exynos7_clock_peris) = {
	.name = "exynos7-clock-peris",
	.id = UCLASS_CLK,
	.of_match = peris_cmu_compat,
	.probe = peris_cmu_probe,
	.ops = &samsung_clk_ops,
	.flags = DM_FLAG_PRE_RELOC,
};

/* Register Offset definitions for CMU_FSYS0 (0x10E90000) */
#define MUX_SEL_FSYS00			0x0200
#define MUX_SEL_FSYS01			0x0204
#define MUX_SEL_FSYS02			0x0208
#define ENABLE_ACLK_FSYS00		0x0800
#define ENABLE_ACLK_FSYS01		0x0804
#define ENABLE_SCLK_FSYS01		0x0A04
#define ENABLE_SCLK_FSYS02		0x0A08
#define ENABLE_SCLK_FSYS04		0x0A10

/*
 * List of parent clocks for Muxes in CMU_FSYS0
 */
PNAME(mout_aclk_fsys0_200_user_p)	= { "fin_pll", "aclk_fsys0_200" };
PNAME(mout_sclk_mmc2_user_p)		= { "fin_pll", "sclk_mmc2" };

PNAME(mout_sclk_usbdrd300_user_p)	= { "fin_pll", "sclk_usbdrd300" };
PNAME(mout_phyclk_usbdrd300_udrd30_phyclk_user_p)	= { "fin_pll",
				"phyclk_usbdrd300_udrd30_phyclock" };
PNAME(mout_phyclk_usbdrd300_udrd30_pipe_pclk_user_p)	= { "fin_pll",
				"phyclk_usbdrd300_udrd30_pipe_pclk" };

static int fsys0_cmu_probe(struct udevice *dev)
{
	void __iomem *base;

	base = dev_read_addr_ptr(dev);
	if (!base) {
		panic("%s: failed to map registers\n", __func__);
		return -EINVAL;
	}

	MUX(0, "mout_phyclk_usbdrd300_udrd30_phyclk_user", mout_phyclk_usbdrd300_udrd30_phyclk_user_p, base + MUX_SEL_FSYS02, 28, 1);
	MUX(0, "mout_aclk_fsys0_200_user", mout_aclk_fsys0_200_user_p, base + MUX_SEL_FSYS00, 24, 1);
	MUX(0, "mout_phyclk_usbdrd300_udrd30_pipe_pclk_user", mout_phyclk_usbdrd300_udrd30_pipe_pclk_user_p, base + MUX_SEL_FSYS02, 24, 1);
	MUX(0, "mout_sclk_usbdrd300_user", mout_sclk_usbdrd300_user_p, base + MUX_SEL_FSYS01, 28, 1);
	MUX(0, "mout_sclk_mmc2_user", mout_sclk_mmc2_user_p, base + MUX_SEL_FSYS01, 24, 1);
	GATE(ACLK_AXIUS_USBDRD30X_FSYS0X, "aclk_axius_usbdrd30x_fsys0x", "mout_aclk_fsys0_200_user", base + ENABLE_ACLK_FSYS00, 19, 0, 0);
	GATE(SCLK_USBDRD300_REFCLK, "sclk_usbdrd300_refclk", "fin_pll", base + ENABLE_SCLK_FSYS01, 8, 0, 0);
	GATE(ACLK_MMC2, "aclk_mmc2", "mout_aclk_fsys0_200_user", base + ENABLE_ACLK_FSYS01, 31, 0, 0);
	GATE(ACLK_USBDRD300, "aclk_usbdrd300", "mout_aclk_fsys0_200_user", base + ENABLE_ACLK_FSYS01, 29, 0, 0);
	GATE(PHYCLK_USBDRD300_UDRD30_PIPE_PCLK_USER, "phyclk_usbdrd300_udrd30_pipe_pclk_user", "mout_phyclk_usbdrd300_udrd30_pipe_pclk_user", base + ENABLE_SCLK_FSYS02, 24, 0, 0);
	GATE(ACLK_PDMA0, "aclk_pdma0", "mout_aclk_fsys0_200_user", base + ENABLE_ACLK_FSYS00, 4, 0, 0);
	GATE(ACLK_PDMA1, "aclk_pdma1", "mout_aclk_fsys0_200_user", base + ENABLE_ACLK_FSYS00, 3, 0, 0);
	GATE(OSCCLK_PHY_CLKOUT_USB30_PHY, "oscclk_phy_clkout_usb30_phy", "fin_pll", base + ENABLE_SCLK_FSYS04, 28, 0, 0);
	GATE(PHYCLK_USBDRD300_UDRD30_PHYCLK_USER, "phyclk_usbdrd300_udrd30_phyclk_user", "mout_phyclk_usbdrd300_udrd30_phyclk_user", base + ENABLE_SCLK_FSYS02, 28, 0, 0);
	GATE(SCLK_USBDRD300_SUSPENDCLK, "sclk_usbdrd300_suspendclk", "mout_sclk_usbdrd300_user", base + ENABLE_SCLK_FSYS01, 4, 0, 0);

	return 0;
}

static const struct udevice_id fsys0_cmu_compat[] = {
	{ .compatible = "samsung,exynos7-clock-fsys0" },
	{ }
};

U_BOOT_DRIVER(exynos7_clock_fsys0) = {
	.name = "exynos7-clock-fsys0",
	.id = UCLASS_CLK,
	.of_match = fsys0_cmu_compat,
	.probe = fsys0_cmu_probe,
	.ops = &samsung_clk_ops,
	.flags = DM_FLAG_PRE_RELOC,
};

/* Register Offset definitions for CMU_FSYS1 (0x156E0000) */
#define MUX_SEL_FSYS10			0x0200
#define MUX_SEL_FSYS11			0x0204
#define MUX_SEL_FSYS12			0x0208
#define DIV_FSYS1			0x0600
#define ENABLE_ACLK_FSYS1		0x0800
#define ENABLE_PCLK_FSYS1               0x0900
#define ENABLE_SCLK_FSYS11              0x0A04
#define ENABLE_SCLK_FSYS12              0x0A08
#define ENABLE_SCLK_FSYS13              0x0A0C

/*
 * List of parent clocks for Muxes in CMU_FSYS1
 */
PNAME(mout_aclk_fsys1_200_user_p)	= { "fin_pll", "aclk_fsys1_200" };
PNAME(mout_fsys1_group_p)	= { "fin_pll", "fin_pll_26m",
				"sclk_phy_fsys1_26m" };
PNAME(mout_sclk_mmc0_user_p)		= { "fin_pll", "sclk_mmc0" };
PNAME(mout_sclk_mmc1_user_p)		= { "fin_pll", "sclk_mmc1" };
PNAME(mout_sclk_ufsunipro20_user_p)  = { "fin_pll", "sclk_ufsunipro20" };
PNAME(mout_phyclk_ufs20_tx0_user_p) = { "fin_pll", "phyclk_ufs20_tx0_symbol" };
PNAME(mout_phyclk_ufs20_rx0_user_p) = { "fin_pll", "phyclk_ufs20_rx0_symbol" };
PNAME(mout_phyclk_ufs20_rx1_user_p) = { "fin_pll", "phyclk_ufs20_rx1_symbol" };

static int fsys1_cmu_probe(struct udevice *dev)
{
	void __iomem *base;

	base = dev_read_addr_ptr(dev);
	if (!base) {
		panic("%s: failed to map registers\n", __func__);
		return -EINVAL;
	}

	MUX(0, "mout_aclk_fsys1_200_user", mout_aclk_fsys1_200_user_p, base + MUX_SEL_FSYS10, 28, 1);
	DIV(DOUT_PCLK_FSYS1, "dout_pclk_fsys1", "mout_aclk_fsys1_200_user", base + DIV_FSYS1, 0, 2);
	MUX(MOUT_FSYS1_PHYCLK_SEL1, "mout_fsys1_phyclk_sel1", mout_fsys1_group_p, base + MUX_SEL_FSYS10, 16, 2);
	MUX(0, "mout_phyclk_ufs20_rx1_symbol_user", mout_phyclk_ufs20_rx1_user_p, base + MUX_SEL_FSYS12, 16, 1);
	MUX(0, "mout_sclk_ufsunipro20_user", mout_sclk_ufsunipro20_user_p, base + MUX_SEL_FSYS11, 20, 1);
	MUX(0, "mout_phyclk_ufs20_tx0_symbol_user", mout_phyclk_ufs20_tx0_user_p, base + MUX_SEL_FSYS12, 28, 1);
	MUX(0, "mout_phyclk_ufs20_rx0_symbol_user", mout_phyclk_ufs20_rx0_user_p, base + MUX_SEL_FSYS12, 24, 1);
	MUX(0, "mout_sclk_mmc1_user", mout_sclk_mmc1_user_p, base + MUX_SEL_FSYS11, 24, 1);
	MUX(0, "mout_fsys1_phyclk_sel0", mout_fsys1_group_p, base + MUX_SEL_FSYS10, 20, 2);
	MUX(0, "mout_sclk_mmc0_user", mout_sclk_mmc0_user_p, base + MUX_SEL_FSYS11, 28, 1);
	GATE(SCLK_UFSUNIPRO20_USER, "sclk_ufsunipro20_user", "mout_sclk_ufsunipro20_user", base + ENABLE_SCLK_FSYS11, 20, 0, 0);
	GATE(ACLK_MMC1, "aclk_mmc1", "mout_aclk_fsys1_200_user", base + ENABLE_ACLK_FSYS1, 29, 0, 0);
	GATE(ACLK_MMC0, "aclk_mmc0", "mout_aclk_fsys1_200_user", base + ENABLE_ACLK_FSYS1, 30, 0, 0);
	GATE(ACLK_UFS20_LINK, "aclk_ufs20_link", "dout_pclk_fsys1", base + ENABLE_ACLK_FSYS1, 31, 0, 0);
	GATE(PCLK_GPIO_FSYS1, "pclk_gpio_fsys1", "mout_aclk_fsys1_200_user", base + ENABLE_PCLK_FSYS1, 30, 0, 0);
	GATE(OSCCLK_PHY_CLKOUT_EMBEDDED_COMBO_PHY, "oscclk_phy_clkout_embedded_combo_phy", "fin_pll", base + ENABLE_SCLK_FSYS12, 4, CLK_IGNORE_UNUSED, 0);
	GATE(PHYCLK_UFS20_TX0_SYMBOL_USER, "phyclk_ufs20_tx0_symbol_user", "mout_phyclk_ufs20_tx0_symbol_user", base + ENABLE_SCLK_FSYS12, 28, 0, 0);
	GATE(SCLK_COMBO_PHY_EMBEDDED_26M, "sclk_combo_phy_embedded_26m", "mout_fsys1_phyclk_sel1", base + ENABLE_SCLK_FSYS13, 24, CLK_IGNORE_UNUSED, 0);
	GATE(PHYCLK_UFS20_RX0_SYMBOL_USER, "phyclk_ufs20_rx0_symbol_user", "mout_phyclk_ufs20_rx0_symbol_user", base + ENABLE_SCLK_FSYS12, 24, 0, 0);
	GATE(PHYCLK_UFS20_RX1_SYMBOL_USER, "phyclk_ufs20_rx1_symbol_user", "mout_phyclk_ufs20_rx1_symbol_user", base + ENABLE_SCLK_FSYS12, 16, 0, 0);

	return 0;
}

static const struct udevice_id fsys1_cmu_compat[] = {
	{ .compatible = "samsung,exynos7-clock-fsys1" },
	{ }
};

U_BOOT_DRIVER(exynos7_clock_fsys1) = {
	.name = "exynos7-clock-fsys1",
	.id = UCLASS_CLK,
	.of_match = fsys1_cmu_compat,
	.probe = fsys1_cmu_probe,
	.ops = &samsung_clk_ops,
	.flags = DM_FLAG_PRE_RELOC,
};

#define MUX_SEL_MSCL			0x0200
#define DIV_MSCL			0x0600
#define ENABLE_ACLK_MSCL		0x0800
#define ENABLE_PCLK_MSCL		0x0900

/* List of parent clocks for Muxes in CMU_MSCL */
PNAME(mout_aclk_mscl_532_user_p)	= { "fin_pll", "aclk_mscl_532" };

static int mscl_cmu_probe(struct udevice *dev)
{
	void __iomem *base;

	base = dev_read_addr_ptr(dev);
	if (!base) {
		panic("%s: failed to map registers\n", __func__);
		return -EINVAL;
	}

	MUX(USERMUX_ACLK_MSCL_532, "usermux_aclk_mscl_532", mout_aclk_mscl_532_user_p, base + MUX_SEL_MSCL, 0, 1);
	DIV(DOUT_PCLK_MSCL, "dout_pclk_mscl", "usermux_aclk_mscl_532", base + DIV_MSCL, 0, 3);
	GATE(PCLK_QE_MSCL_0, "pclk_qe_mscl_0", "dout_pclk_mscl", base + ENABLE_PCLK_MSCL, 27, 0, 0);
	GATE(PCLK_QE_MSCL_1, "pclk_qe_mscl_1", "dout_pclk_mscl", base + ENABLE_PCLK_MSCL, 26, 0, 0);
	GATE(PCLK_AXI2ACEL_BRIDGE, "pclk_axi2acel_bridge", "dout_pclk_mscl", base + ENABLE_PCLK_MSCL, 21, 0, 0);
	GATE(ACLK_QE_G2D, "aclk_qe_g2d", "usermux_aclk_mscl_532", base + ENABLE_ACLK_MSCL, 19, 0, 0);
	GATE(ACLK_PPMU_MSCL_0, "aclk_ppmu_mscl_0", "usermux_aclk_mscl_532", base + ENABLE_ACLK_MSCL, 18, 0, 0);
	GATE(PCLK_MSCL_0, "pclk_mscl_0", "dout_pclk_mscl", base + ENABLE_PCLK_MSCL, 31, 0, 0);
	GATE(ACLK_PPMU_MSCL_1, "aclk_ppmu_mscl_1", "usermux_aclk_mscl_532", base + ENABLE_ACLK_MSCL, 17, 0, 0);
	GATE(PCLK_MSCL_1, "pclk_mscl_1", "dout_pclk_mscl", base + ENABLE_PCLK_MSCL, 30, 0, 0);
	GATE(ACLK_G2D, "aclk_g2d", "usermux_aclk_mscl_532", base + ENABLE_ACLK_MSCL, 28, 0, 0);
	GATE(ACLK_AHB2APB_MSCL1P, "aclk_ahb2apb_mscl1p", "usermux_aclk_mscl_532", base + ENABLE_ACLK_MSCL, 14, 0, 0);
	GATE(ACLK_QE_MSCL_0, "aclk_qe_mscl_0", "usermux_aclk_mscl_532", base + ENABLE_ACLK_MSCL, 22, 0, 0);
	GATE(ACLK_MSCLNP_133, "aclk_msclnp_133", "usermux_aclk_mscl_532", base + ENABLE_ACLK_MSCL, 16, 0, 0);
	GATE(ACLK_QE_MSCL_1, "aclk_qe_mscl_1", "usermux_aclk_mscl_532", base + ENABLE_ACLK_MSCL, 21, 0, 0);
	GATE(PCLK_QE_JPEG, "pclk_qe_jpeg", "dout_pclk_mscl", base + ENABLE_PCLK_MSCL, 25, 0, 0);
	GATE(ACLK_QE_JPEG, "aclk_qe_jpeg", "usermux_aclk_mscl_532", base + ENABLE_ACLK_MSCL, 20, 0, 0);
	GATE(ACLK_AXI2ACEL_BRIDGE, "aclk_axi2acel_bridge", "usermux_aclk_mscl_532", base + ENABLE_ACLK_MSCL, 23, 0, 0);
	GATE(ACLK_MSCL_0, "aclk_mscl_0", "usermux_aclk_mscl_532", base + ENABLE_ACLK_MSCL, 31, 0, 0);
	GATE(ACLK_JPEG, "aclk_jpeg", "usermux_aclk_mscl_532", base + ENABLE_ACLK_MSCL, 29, 0, 0);
	GATE(ACLK_MSCL_1, "aclk_mscl_1", "usermux_aclk_mscl_532", base + ENABLE_ACLK_MSCL, 30, 0, 0);
	GATE(PCLK_G2D, "pclk_g2d", "dout_pclk_mscl", base + ENABLE_PCLK_MSCL, 28, 0, 0);
	GATE(PCLK_QE_G2D, "pclk_qe_g2d", "dout_pclk_mscl", base + ENABLE_PCLK_MSCL, 24, 0, 0);
	GATE(ACLK_AHB2APB_MSCL0P, "aclk_ahb2apb_mscl0p", "usermux_aclk_mscl_532", base + ENABLE_ACLK_MSCL, 15, 0, 0);
	GATE(ACLK_XIU_MSCLX_0, "aclk_xiu_msclx_0", "usermux_aclk_mscl_532", base + ENABLE_ACLK_MSCL, 25, 0, 0);
	GATE(ACLK_XIU_MSCLX_1, "aclk_xiu_msclx_1", "usermux_aclk_mscl_532", base + ENABLE_ACLK_MSCL, 24, 0, 0);
	GATE(PCLK_PPMU_MSCL_0, "pclk_ppmu_mscl_0", "dout_pclk_mscl", base + ENABLE_PCLK_MSCL, 23, 0, 0);
	GATE(PCLK_PPMU_MSCL_1, "pclk_ppmu_mscl_1", "dout_pclk_mscl", base + ENABLE_PCLK_MSCL, 22, 0, 0);
	GATE(PCLK_PMU_MSCL, "pclk_pmu_mscl", "dout_pclk_mscl", base + ENABLE_PCLK_MSCL, 20, 0, 0);
	GATE(ACLK_LH_ASYNC_SI_MSCL_1, "aclk_lh_async_si_mscl_1", "usermux_aclk_mscl_532", base + ENABLE_ACLK_MSCL, 26, 0, 0);
	GATE(PCLK_JPEG, "pclk_jpeg", "dout_pclk_mscl", base + ENABLE_PCLK_MSCL, 29, 0, 0);
	GATE(ACLK_LH_ASYNC_SI_MSCL_0, "aclk_lh_async_si_mscl_0", "usermux_aclk_mscl_532", base + ENABLE_ACLK_MSCL, 27, 0, 0);

	return 0;
}

static const struct udevice_id mscl_cmu_compat[] = {
	{ .compatible = "samsung,exynos7-clock-mscl" },
	{ }
};

U_BOOT_DRIVER(exynos7_clock_mscl) = {
	.name = "exynos7-clock-mscl",
	.id = UCLASS_CLK,
	.of_match = mscl_cmu_compat,
	.probe = mscl_cmu_probe,
	.ops = &samsung_clk_ops,
	.flags = DM_FLAG_PRE_RELOC,
};

/* Register Offset definitions for CMU_AUD (0x114C0000) */
#define	MUX_SEL_AUD			0x0200
#define	DIV_AUD0			0x0600
#define	DIV_AUD1			0x0604
#define	ENABLE_ACLK_AUD			0x0800
#define	ENABLE_PCLK_AUD			0x0900
#define	ENABLE_SCLK_AUD			0x0A00

/*
 * List of parent clocks for Muxes in CMU_AUD
 */
PNAME(mout_aud_pll_user_p) = { "fin_pll", "fout_aud_pll" };
PNAME(mout_aud_group_p) = { "dout_aud_cdclk", "ioclk_audiocdclk0" };

static int aud_cmu_probe(struct udevice *dev)
{
	void __iomem *base;

	base = dev_read_addr_ptr(dev);
	if (!base) {
		panic("%s: failed to map registers\n", __func__);
		return -EINVAL;
	}

	MUX(0, "mout_aud_pll_user", mout_aud_pll_user_p, base + MUX_SEL_AUD, 20, 1);
	MUX(0, "mout_sclk_pcm", mout_aud_group_p, base + MUX_SEL_AUD, 16, 1);
	MUX(0, "mout_sclk_i2s", mout_aud_group_p, base + MUX_SEL_AUD, 12, 1);
	DIV(0, "dout_aud_cdclk", "mout_aud_pll_user", base + DIV_AUD1, 24, 4);
	DIV(0, "dout_aud_ca5", "mout_aud_pll_user", base + DIV_AUD0, 0, 4);
	DIV(0, "dout_aud_pclk_dbg", "dout_aud_ca5", base + DIV_AUD0, 8, 4);
	DIV(0, "dout_aclk_aud", "dout_aud_ca5", base + DIV_AUD0, 4, 4);
	DIV(0, "dout_sclk_i2s", "mout_sclk_i2s", base + DIV_AUD1, 0, 4);
	DIV(0, "dout_sclk_uart", "dout_aud_cdclk", base + DIV_AUD1, 12, 4);
	DIV(0, "dout_sclk_pcm", "mout_sclk_pcm", base + DIV_AUD1, 4, 8);
	DIV(0, "dout_sclk_slimbus", "dout_aud_cdclk", base + DIV_AUD1, 16, 5);
	GATE(0, "pclk_smmu_aud", "dout_aclk_aud", base + ENABLE_PCLK_AUD, 31, 0, 0);
	GATE(0, "sclk_uart", "dout_sclk_uart", base + ENABLE_SCLK_AUD, 29, 0, 0);
	GATE(0, "pclk_gpio_aud", "dout_aclk_aud", base + ENABLE_PCLK_AUD, 20, 0, 0);
	GATE(0, "pclk_uart", "dout_aclk_aud", base + ENABLE_PCLK_AUD, 25, 0, 0);
	GATE(0, "pclk_slimbus", "dout_aclk_aud", base + ENABLE_PCLK_AUD, 24, 0, 0);
	GATE(0, "pclk_wdt0", "dout_aclk_aud", base + ENABLE_PCLK_AUD, 23, 0, 0);
	GATE(0, "pclk_wdt1", "dout_aclk_aud", base + ENABLE_PCLK_AUD, 22, 0, 0);
	GATE(0, "pclk_dbg_aud", "dout_aud_pclk_dbg", base + ENABLE_PCLK_AUD, 19, 0, 0);
	GATE(0, "sclk_slimbus", "dout_sclk_slimbus", base + ENABLE_SCLK_AUD, 30, 0, 0);
	GATE(PCLK_PCM, "pclk_pcm", "dout_aclk_aud", base + ENABLE_PCLK_AUD, 26, CLK_SET_RATE_PARENT, 0);
	GATE(SCLK_PCM, "sclk_pcm", "dout_sclk_pcm", base + ENABLE_SCLK_AUD, 27, CLK_SET_RATE_PARENT, 0);
	GATE(0, "aclk_smmu_aud", "dout_aclk_aud", base + ENABLE_ACLK_AUD, 27, 0, 0);
	GATE(ACLK_ADMA, "aclk_dmac", "dout_aclk_aud", base + ENABLE_ACLK_AUD, 31, 0, 0);
	GATE(0, "aclk_acel_lh_async_si_top", "dout_aclk_aud", base + ENABLE_ACLK_AUD, 28, 0, 0);
	GATE(PCLK_I2S, "pclk_i2s", "dout_aclk_aud", base + ENABLE_PCLK_AUD, 27, CLK_SET_RATE_PARENT, 0);
	GATE(SCLK_I2S, "sclk_i2s", "dout_sclk_i2s", base + ENABLE_SCLK_AUD, 28, CLK_SET_RATE_PARENT, 0);
	GATE(0, "pclk_timer", "dout_aclk_aud", base + ENABLE_PCLK_AUD, 28, 0, 0);

	return 0;
}

static const struct udevice_id aud_cmu_compat[] = {
	{ .compatible = "samsung,exynos7-clock-aud" },
	{ }
};

U_BOOT_DRIVER(exynos7_clock_aud) = {
	.name = "exynos7-clock-aud",
	.id = UCLASS_CLK,
	.of_match = aud_cmu_compat,
	.probe = aud_cmu_probe,
	.ops = &samsung_clk_ops,
	.flags = DM_FLAG_PRE_RELOC,
};