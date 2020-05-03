// SPDX-License-Identifier: GPL-2.0+

#ifndef __EXYNOS_CLK_H
#define __EXYNOS_CLK_H

#include <linux/clk-provider.h>
#include "clk-pll.h"

#define MHZ (1000 * 1000)

#define __PLL(_typ, _id, _name, _pname, _flags, _lock, _con, _rtable)	\
	samsung_clk_register_pll(_id, _name, _pname, _flags, _con, _lock, _typ, _rtable)

#define PLL(_typ, _id, _name, _pname, _lock, _con, _rtable)	\
	__PLL(_typ, _id, _name, _pname, CLK_GET_RATE_NOCACHE, _lock,	\
	      _con, _rtable)

#define FFACTOR(_id, name, pname, m, d, f)		\
	samsung_clk_register_fixed_factor(_id, name, pname, m, d, f)

#define __MUX(_id, cname, pnames, o, s, w, f, mf)		\
	samsung_clk_register_mux(_id, cname, pnames, ARRAY_SIZE(pnames), (f) | CLK_SET_RATE_NO_REPARENT, o, s, w, mf)

#define MUX(_id, cname, pnames, o, s, w)			\
	__MUX(_id, cname, pnames, o, s, w, 0, 0)

#define MUX_F(_id, cname, pnames, o, s, w, f, mf)		\
	__MUX(_id, cname, pnames, o, s, w, f, mf)

#define __DIV(_id, cname, pname, o, s, w, f, df, t)	\
	samsung_clk_register_div(_id, cname, pname, f, o, s, w, df)

#define DIV(_id, cname, pname, o, s, w)				\
	__DIV(_id, cname, pname, o, s, w, 0, 0, NULL)

#define DIV_F(_id, cname, pname, o, s, w, f, df)		\
	__DIV(_id, cname, pname, o, s, w, f, df, NULL)

#define DIV_T(_id, cname, pname, o, s, w, t)			\
	__DIV(_id, cname, pname, o, s, w, 0, 0, t)

#define __GATE(_id, cname, pname, o, b, f, gf)			\
	samsung_clk_register_gate(_id, cname, pname, \
		f, o,b, gf)

#define GATE(_id, cname, pname, o, b, f, gf)			\
	__GATE(_id, cname, pname, o, b, f, gf)

#define PNAME(x) static const char * const x[]

extern void samsung_clk_register_pll(unsigned int id,
			const char *name, const char *parent_name,
			unsigned long flags, void __iomem *con_reg,
			void __iomem *lock_reg, enum samsung_pll_type type,
			const struct samsung_pll_rate_table *rate_table);

extern void samsung_clk_register_fixed_factor(unsigned int id,
			char *name, const char *parent_name, unsigned long mult,
			unsigned long div, unsigned long flags);

extern void samsung_clk_register_mux(unsigned int id,
			const char *name, const char *const *parent_names,
			u8 num_parents, unsigned long flags, void __iomem *reg,
			u8 shift, u8 width, u8 mux_flags);

extern void samsung_clk_register_div(unsigned int id,
			const char *name, const char *parent_name,
			unsigned long flags, void __iomem *reg,
			u8 shift, u8 width, u8 div_flags);

extern void samsung_clk_register_gate(unsigned int id,
			const char *name, const char *parent_name,
			unsigned long flags, void __iomem *reg,
			u8 bit_idx, u8 gate_flags);

extern ulong samsung_clk_get_rate(struct clk *clk);

extern ulong samsung_clk_set_rate(struct clk *clk, unsigned long rate);

extern int samsung_clk_disable(struct clk *clk);

extern int samsung_clk_enable(struct clk *clk);

extern int samsung_clk_set_parent(struct clk *clk, struct clk *parent);

#endif /* __MACH_IMX_CLK_H */