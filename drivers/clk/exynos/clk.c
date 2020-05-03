#include "clk.h"
#include "clk-pll.h"

ulong samsung_clk_get_rate(struct clk *clk)
{
	struct clk *c;
	int ret;

	pr_debug("%s(#%lu)\n", __func__, clk->id);

	ret = clk_get_by_id(clk->id, &c);
	if (ret)
		return ret;

	return clk_get_rate(c);
}

ulong samsung_clk_set_rate(struct clk *clk, unsigned long rate)
{
	struct clk *c;
	int ret;

	pr_debug("%s(#%lu), rate: %lu\n", __func__, clk->id, rate);

	ret = clk_get_by_id(clk->id, &c);
	if (ret)
		return ret;

	return clk_set_rate(c, rate);
}

int __samsung_clk_enable(struct clk *clk, bool enable)
{
	struct clk *c;
	int ret;

	pr_debug("%s(#%lu) en: %d\n", __func__, clk->id, enable);

	ret = clk_get_by_id(clk->id, &c);
	if (ret)
		return ret;

	if (enable)
		ret = clk_enable(c);
	else
		ret = clk_disable(c);

	return ret;
}

int samsung_clk_disable(struct clk *clk)
{
	return __samsung_clk_enable(clk, 0);
}

int samsung_clk_enable(struct clk *clk)
{
	return __samsung_clk_enable(clk, 1);
}

int samsung_clk_set_parent(struct clk *clk, struct clk *parent)
{
	struct clk *c, *cp;
	int ret;

	pr_debug("%s(#%lu), parent: %lu\n", __func__, clk->id, parent->id);

	ret = clk_get_by_id(clk->id, &c);
	if (ret)
		return ret;

	ret = clk_get_by_id(parent->id, &cp);
	if (ret)
		return ret;

	return clk_set_parent(c, cp);
}

/* register fixed factor clock */
void samsung_clk_register_fixed_factor(unsigned int id,
	char *name, const char *parent_name, unsigned long mult,
	unsigned long div, unsigned long flags)
{
	struct clk *clk;

	clk = clk_register_fixed_factor(NULL, name,
			parent_name, flags, mult, div);
	if (IS_ERR(clk)) {
		pr_err("%s: failed to register clock %s\n", __func__,
			name);
		return;
	}

	clk_dm(id, clk);
}

/* register mux clock */
void samsung_clk_register_mux(unsigned int id,
			const char *name, const char *const *parent_names,
			u8 num_parents, unsigned long flags, void __iomem *reg,
			u8 shift, u8 width, u8 mux_flags)
{
	struct clk *clk;

	clk = clk_register_mux(NULL, name,
		parent_names, num_parents, flags,
		reg, shift, width, mux_flags);
	if (IS_ERR(clk)) {
		pr_err("%s: failed to register clock %s\n", __func__,
			name);
		return;
	}

	clk_dm(id, clk);
}

/* register div clock */
void samsung_clk_register_div(unsigned int id,
			const char *name, const char *parent_name,
			unsigned long flags, void __iomem *reg,
			u8 shift, u8 width, u8 div_flags)
{
	struct clk *clk;

	clk = clk_register_divider(NULL, name,
		parent_name, flags, reg,
		shift, width, div_flags);
	if (IS_ERR(clk)) {
		pr_err("%s: failed to register clock %s\n", __func__,
			name);
		return;
	}

	clk_dm(id, clk);
}

/* register gate clock */
void samsung_clk_register_gate(unsigned int id,
			const char *name, const char *parent_name,
			unsigned long flags, void __iomem *reg,
			u8 bit_idx, u8 gate_flags)
{
	struct clk *clk;

	clk = clk_register_gate(NULL, name, parent_name,
			flags, reg, bit_idx, gate_flags, NULL);
	if (IS_ERR(clk)) {
		pr_err("%s: failed to register clock %s\n", __func__,
			name);
		return;
	}

	clk_dm(id, clk);
}