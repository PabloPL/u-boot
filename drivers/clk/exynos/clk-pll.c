// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
 * Copyright (c) 2013 Linaro Ltd.
 *
 * This file contains the utility functions to register the pll clocks.
*/

#include <asm/io.h>
#include <asm/processor.h>
#include <dm/device.h>
#include <div64.h>
#include <linux/bug.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/clk-provider.h>
#include <linux/iopoll.h>
#include "clk.h"
#include "clk-pll.h"

#define UBOOT_DM_CLK_PLL_1451X	"clk_pll_1451x"
#define UBOOT_DM_CLK_PLL_1452X	"clk_pll_1452x"
#define UBOOT_DM_CLK_PLL_1460X	"clk_pll_1460x"

#define LOCK_TIMEOUT_US		10000

struct samsung_clk_pll {
	struct clk			hw;
	void __iomem		*lock_reg;
	void __iomem		*con_reg;
	/* PLL enable control bit offset in @con_reg register */
	unsigned short		enable_offs;
	/* PLL lock status bit offset in @con_reg register */
	unsigned short		lock_offs;
	enum samsung_pll_type	type;
	unsigned int		rate_count;
	const struct samsung_pll_rate_table *rate_table;
};

#define to_clk_pll(_hw) container_of(_hw, struct samsung_clk_pll, hw)

static const struct samsung_pll_rate_table *samsung_get_pll_settings(
				struct samsung_clk_pll *pll, unsigned long rate)
{
	const struct samsung_pll_rate_table  *rate_table = pll->rate_table;
	int i;

	for (i = 0; i < pll->rate_count; i++) {
		if (rate == rate_table[i].rate)
			return &rate_table[i];
	}

	return NULL;
}

/*
 * PLL35xx Clock Type
 */
/* Maximum lock time can be 270 * PDIV cycles */
#define PLL35XX_LOCK_FACTOR	(270)

#define PLL35XX_MDIV_MASK       (0x3FF)
#define PLL35XX_PDIV_MASK       (0x3F)
#define PLL35XX_SDIV_MASK       (0x7)
#define PLL35XX_MDIV_SHIFT      (16)
#define PLL35XX_PDIV_SHIFT      (8)
#define PLL35XX_SDIV_SHIFT      (0)
#define PLL35XX_LOCK_STAT_SHIFT	(29)
#define PLL35XX_ENABLE_SHIFT	(31)

static unsigned long samsung_pll35xx_recalc_rate(struct clk *clk)
{
	struct samsung_clk_pll *pll = to_clk_pll(clk);
	u64 fvco = clk_get_parent_rate(clk);
	u32 mdiv, pdiv, sdiv, pll_con;

	pll_con = readl_relaxed(pll->con_reg);
	mdiv = (pll_con >> PLL35XX_MDIV_SHIFT) & PLL35XX_MDIV_MASK;
	pdiv = (pll_con >> PLL35XX_PDIV_SHIFT) & PLL35XX_PDIV_MASK;
	sdiv = (pll_con >> PLL35XX_SDIV_SHIFT) & PLL35XX_SDIV_MASK;

	fvco *= mdiv;
	do_div(fvco, (pdiv << sdiv));

	return fvco;
}

static inline bool samsung_pll35xx_mp_change(
		const struct samsung_pll_rate_table *rate, u32 pll_con)
{
	u32 old_mdiv, old_pdiv;

	old_mdiv = (pll_con >> PLL35XX_MDIV_SHIFT) & PLL35XX_MDIV_MASK;
	old_pdiv = (pll_con >> PLL35XX_PDIV_SHIFT) & PLL35XX_PDIV_MASK;

	return (rate->mdiv != old_mdiv || rate->pdiv != old_pdiv);
}

static ulong samsung_pll35xx_set_rate(struct clk *clk, ulong drate)
{
	struct samsung_clk_pll *pll = to_clk_pll(clk);
	const struct samsung_pll_rate_table *rate;
	u32 tmp;

	/* Get required rate settings from table */
	rate = samsung_get_pll_settings(pll, drate);
	if (!rate) {
		pr_err("%s: Invalid rate : %lu for pll clk %s\n", __func__,
			drate, clk_hw_get_name(clk));
		return -EINVAL;
	}

	tmp = readl_relaxed(pll->con_reg);

	if (!(samsung_pll35xx_mp_change(rate, tmp))) {
		/* If only s change, change just s value only*/
		tmp &= ~(PLL35XX_SDIV_MASK << PLL35XX_SDIV_SHIFT);
		tmp |= rate->sdiv << PLL35XX_SDIV_SHIFT;
		writel_relaxed(tmp, pll->con_reg);

		return 0;
	}

	/* Set PLL lock time. */
	writel_relaxed(rate->pdiv * PLL35XX_LOCK_FACTOR,
			pll->lock_reg);

	/* Change PLL PMS values */
	tmp &= ~((PLL35XX_MDIV_MASK << PLL35XX_MDIV_SHIFT) |
			(PLL35XX_PDIV_MASK << PLL35XX_PDIV_SHIFT) |
			(PLL35XX_SDIV_MASK << PLL35XX_SDIV_SHIFT));
	tmp |= (rate->mdiv << PLL35XX_MDIV_SHIFT) |
			(rate->pdiv << PLL35XX_PDIV_SHIFT) |
			(rate->sdiv << PLL35XX_SDIV_SHIFT);
	writel_relaxed(tmp, pll->con_reg);

	/* Wait until the PLL is locked if it is enabled. */
	if (tmp & BIT(pll->enable_offs)) {
		return readl_poll_timeout(pll->con_reg, tmp, tmp & BIT(pll->lock_offs),
			LOCK_TIMEOUT_US);
	}
	return 0;
}

static int samsung_pll3xxx_enable(struct clk *clk)
{
	struct samsung_clk_pll *pll = to_clk_pll(clk);
	u32 tmp;

	tmp = readl_relaxed(pll->con_reg);
	tmp |= BIT(pll->enable_offs);
	writel_relaxed(tmp, pll->con_reg);

	/* wait lock time */
	return readl_poll_timeout(pll->con_reg, tmp, tmp & BIT(pll->lock_offs),
			LOCK_TIMEOUT_US);
}

static int samsung_pll3xxx_disable(struct clk *clk)
{
	struct samsung_clk_pll *pll = to_clk_pll(clk);
	u32 tmp;

	tmp = readl_relaxed(pll->con_reg);
	tmp &= ~BIT(pll->enable_offs);
	writel_relaxed(tmp, pll->con_reg);

	return 0;
}

static const struct clk_ops clk_pll3xxx_ops = {
	.get_rate	= samsung_pll35xx_recalc_rate,
	.enable		= samsung_pll3xxx_enable,
	.disable	= samsung_pll3xxx_disable,
	.set_rate	= samsung_pll35xx_set_rate,
};

/*
 * PLL46xx Clock Type
 */
#define PLL46XX_LOCK_FACTOR	3000

#define PLL46XX_VSEL_MASK	(1)
#define PLL46XX_MDIV_MASK	(0x1FF)
#define PLL1460X_MDIV_MASK	(0x3FF)

#define PLL46XX_PDIV_MASK	(0x3F)
#define PLL46XX_SDIV_MASK	(0x7)
#define PLL46XX_VSEL_SHIFT	(27)
#define PLL46XX_MDIV_SHIFT	(16)
#define PLL46XX_PDIV_SHIFT	(8)
#define PLL46XX_SDIV_SHIFT	(0)

#define PLL46XX_KDIV_MASK	(0xFFFF)
#define PLL4650C_KDIV_MASK	(0xFFF)
#define PLL46XX_KDIV_SHIFT	(0)
#define PLL46XX_MFR_MASK	(0x3F)
#define PLL46XX_MRR_MASK	(0x1F)
#define PLL46XX_KDIV_SHIFT	(0)
#define PLL46XX_MFR_SHIFT	(16)
#define PLL46XX_MRR_SHIFT	(24)

#define PLL46XX_ENABLE		BIT(31)
#define PLL46XX_LOCKED		BIT(29)
#define PLL46XX_VSEL		BIT(27)

static unsigned long clk_pll46xx_recalc_rate(struct clk *clk)
{
	struct samsung_clk_pll *pll = to_clk_pll(clk);
	u64 fvco = clk_get_parent_rate(clk);
	u32 mdiv, pdiv, sdiv, kdiv, pll_con0, pll_con1, shift;

	pll_con0 = readl_relaxed(pll->con_reg);
	pll_con1 = readl_relaxed(pll->con_reg + 4);
	mdiv = (pll_con0 >> PLL46XX_MDIV_SHIFT) & PLL1460X_MDIV_MASK;
	pdiv = (pll_con0 >> PLL46XX_PDIV_SHIFT) & PLL46XX_PDIV_MASK;
	sdiv = (pll_con0 >> PLL46XX_SDIV_SHIFT) & PLL46XX_SDIV_MASK;
	kdiv = (pll_con1 & PLL46XX_KDIV_MASK);

	shift = 16;

	fvco *= (mdiv << shift) + kdiv;
	do_div(fvco, (pdiv << sdiv));
	fvco >>= shift;

	return (unsigned long)fvco;
}

static bool clk_pll46xx_mpk_change(u32 pll_con0, u32 pll_con1,
				const struct samsung_pll_rate_table *rate)
{
	u32 old_mdiv, old_pdiv, old_kdiv;

	old_mdiv = (pll_con0 >> PLL46XX_MDIV_SHIFT) & PLL46XX_MDIV_MASK;
	old_pdiv = (pll_con0 >> PLL46XX_PDIV_SHIFT) & PLL46XX_PDIV_MASK;
	old_kdiv = (pll_con1 >> PLL46XX_KDIV_SHIFT) & PLL46XX_KDIV_MASK;

	return (old_mdiv != rate->mdiv || old_pdiv != rate->pdiv
		|| old_kdiv != rate->kdiv);
}

static ulong clk_pll46xx_set_rate(struct clk *clk, ulong drate)
{
	struct samsung_clk_pll *pll = to_clk_pll(clk);
	const struct samsung_pll_rate_table *rate;
	u32 con0, con1, lock, tmp;

	/* Get required rate settings from table */
	rate = samsung_get_pll_settings(pll, drate);
	if (!rate) {
		pr_err("%s: Invalid rate : %lu for pll clk %s\n", __func__,
			drate, clk_hw_get_name(clk));
		return -EINVAL;
	}

	con0 = readl_relaxed(pll->con_reg);
	con1 = readl_relaxed(pll->con_reg + 0x4);

	if (!(clk_pll46xx_mpk_change(con0, con1, rate))) {
		/* If only s change, change just s value only*/
		con0 &= ~(PLL46XX_SDIV_MASK << PLL46XX_SDIV_SHIFT);
		con0 |= rate->sdiv << PLL46XX_SDIV_SHIFT;
		writel_relaxed(con0, pll->con_reg);

		return 0;
	}

	/* Set PLL lock time. */
	lock = rate->pdiv * PLL46XX_LOCK_FACTOR;
	if (lock > 0xffff)
		/* Maximum lock time bitfield is 16-bit. */
		lock = 0xffff;

	/* Set PLL PMS and VSEL values. */
	con0 &= ~((PLL1460X_MDIV_MASK << PLL46XX_MDIV_SHIFT) |
		(PLL46XX_PDIV_MASK << PLL46XX_PDIV_SHIFT) |
		(PLL46XX_SDIV_MASK << PLL46XX_SDIV_SHIFT));

	con0 |= (rate->mdiv << PLL46XX_MDIV_SHIFT) |
			(rate->pdiv << PLL46XX_PDIV_SHIFT) |
			(rate->sdiv << PLL46XX_SDIV_SHIFT);

	/* Set PLL K, MFR and MRR values. */
	con1 = readl_relaxed(pll->con_reg + 0x4);
	con1 &= ~((PLL46XX_KDIV_MASK << PLL46XX_KDIV_SHIFT) |
			(PLL46XX_MFR_MASK << PLL46XX_MFR_SHIFT) |
			(PLL46XX_MRR_MASK << PLL46XX_MRR_SHIFT));
	con1 |= (rate->kdiv << PLL46XX_KDIV_SHIFT) |
			(rate->mfr << PLL46XX_MFR_SHIFT) |
			(rate->mrr << PLL46XX_MRR_SHIFT);

	/* Write configuration to PLL */
	writel_relaxed(lock, pll->lock_reg);
	writel_relaxed(con0, pll->con_reg);
	writel_relaxed(con1, pll->con_reg + 0x4);

	/* Wait for locking. */
	return readl_poll_timeout(pll->con_reg, tmp, tmp & PLL46XX_LOCKED,
			LOCK_TIMEOUT_US);
}

static const struct clk_ops clk_pll1460x_ops = {
	.get_rate	= clk_pll46xx_recalc_rate,
	.set_rate	= clk_pll46xx_set_rate,
};

void samsung_clk_register_pll(unsigned int id,
			const char *name, const char *parent_name,
			unsigned long flags, void __iomem *con_reg,
			void __iomem *lock_reg, enum samsung_pll_type type,
			const struct samsung_pll_rate_table *rate_table)
{
	struct samsung_clk_pll *pll;
	int ret, len;
	char *drv_name;

	pll = kzalloc(sizeof(*pll), GFP_KERNEL);
	if (!pll) {
		pr_err("%s: could not allocate pll clk %s\n",
			__func__, name);
		return;
	}

	if (rate_table) {
		/* find count of rates in rate_table */
		for (len = 0; rate_table[len].rate != 0; )
			len++;

		pll->rate_count = len;
		pll->rate_table = kmemdup(rate_table,
					pll->rate_count *
					sizeof(struct samsung_pll_rate_table),
					GFP_KERNEL);
		WARN(!pll->rate_table,
			"%s: could not allocate rate table for %s\n",
			__func__, name);
	}

	switch (type) {
	case pll_1451x:
		pll->enable_offs = PLL35XX_ENABLE_SHIFT;
		pll->lock_offs = PLL35XX_LOCK_STAT_SHIFT;
		drv_name = UBOOT_DM_CLK_PLL_1451X;
		break;
	case pll_1452x:
		pll->enable_offs = PLL35XX_ENABLE_SHIFT;
		pll->lock_offs = PLL35XX_LOCK_STAT_SHIFT;
		drv_name = UBOOT_DM_CLK_PLL_1452X;
		break;
	case pll_1460x:
		drv_name = UBOOT_DM_CLK_PLL_1460X;
		break;
	default:
		pr_warn("%s: Unsupported pll type for pll clk %s\n",
			__func__, name);

		kfree(pll);
		return;
	}

	pll->type = type;
	pll->lock_reg = lock_reg;
	pll->con_reg = con_reg;

	ret = clk_register(&pll->hw, drv_name, name, parent_name);
	if (ret) {
		pr_err("%s: failed to register pll clock %s : %d\n",
			__func__, name, ret);
		kfree(pll);
		return;
	}

	clk_dm(id, &pll->hw);
}

U_BOOT_DRIVER(clk_pll_1451x) = {
	.name	= UBOOT_DM_CLK_PLL_1451X,
	.id	= UCLASS_CLK,
	.ops	= &clk_pll3xxx_ops,
	.flags = DM_FLAG_PRE_RELOC,
};

U_BOOT_DRIVER(clk_pll_1452x) = {
	.name	= UBOOT_DM_CLK_PLL_1452X,
	.id	= UCLASS_CLK,
	.ops	= &clk_pll3xxx_ops,
	.flags = DM_FLAG_PRE_RELOC,
};

U_BOOT_DRIVER(clk_pll_1460x) = {
	.name	= UBOOT_DM_CLK_PLL_1460X,
	.id	= UCLASS_CLK,
	.ops	= &clk_pll1460x_ops,
	.flags = DM_FLAG_PRE_RELOC,
};