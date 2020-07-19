// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <dm/uclass.h>
#include <dm/device.h>

int exynos_init(void)
{
	struct udevice *dev;

	puts("Probing all clock devices\n");
	for (uclass_first_device(UCLASS_CLK, &dev);
	     dev;
	     uclass_next_device(&dev)) {
		pr_debug("probe pincontrol = %s\n", dev->name);
	}

	return 0;
}
