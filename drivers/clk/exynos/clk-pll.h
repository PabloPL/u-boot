/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
 * Copyright (c) 2013 Linaro Ltd.
 *
 * Common Clock Framework support for all PLL's in Samsung platforms
*/

#ifndef __SAMSUNG_CLK_PLL_H
#define __SAMSUNG_CLK_PLL_H

#include <linux/build_bug.h>

enum samsung_pll_type {
	pll_1451x,
	pll_1452x,
	pll_1460x,
};

#define PLL_RATE(_fin, _m, _p, _s, _k, _ks) \
	((u64)(_fin) * (BIT(_ks) * (_m) + (_k)) / BIT(_ks) / ((_p) << (_s)))
#define PLL_VALID_RATE(_fin, _fout, _m, _p, _s, _k, _ks) ((_fout) + \
	BUILD_BUG_ON_ZERO(PLL_RATE(_fin, _m, _p, _s, _k, _ks) != (_fout)))

#define PLL_36XX_RATE(_fin, _rate, _m, _p, _s, _k)		\
	{							\
		.rate	=	PLL_VALID_RATE(_fin, _rate,	\
				_m, _p, _s, _k, 16),		\
		.mdiv	=	(_m),				\
		.pdiv	=	(_p),				\
		.sdiv	=	(_s),				\
		.kdiv	=	(_k),				\
	}

struct samsung_pll_rate_table {
	unsigned int rate;
	unsigned int pdiv;
	unsigned int mdiv;
	unsigned int sdiv;
	unsigned int kdiv;
	unsigned int afc;
	unsigned int mfr;
	unsigned int mrr;
	unsigned int vsel;
};

#endif /* __SAMSUNG_CLK_PLL_H */
