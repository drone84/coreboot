/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007-2008 coresystems GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <console/console.h>
#include <arch/io.h>
#include <stdint.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <device/hypertransport.h>
#include <stdlib.h>
#include <string.h>
#include <bitops.h>
#include <cpu/cpu.h>
#include "chip.h"
#include "i945.h"

static void ram_resource(device_t dev, unsigned long index, unsigned long basek,
			 unsigned long sizek)
{
	struct resource *resource;

	resource = new_resource(dev, index);
	resource->base = ((resource_t) basek) << 10;
	resource->size = ((resource_t) sizek) << 10;
	resource->flags = IORESOURCE_MEM | IORESOURCE_CACHEABLE |
	    IORESOURCE_FIXED | IORESOURCE_STORED | IORESOURCE_ASSIGNED;
}

static void pci_domain_read_resources(device_t dev)
{
	struct resource *resource;

	/* Initialize the system wide io space constraints */
	resource = new_resource(dev, IOINDEX_SUBTRACTIVE(0, 0));
	resource->base = 0;
	resource->size = 0;
	resource->align = 0;
	resource->gran = 0;
	resource->limit = 0xffffUL;
	resource->flags =
	    IORESOURCE_IO | IORESOURCE_SUBTRACTIVE | IORESOURCE_ASSIGNED;

	/* Initialize the system wide memory resources constraints */
	resource = new_resource(dev, IOINDEX_SUBTRACTIVE(1, 0));
	resource->base = 0;
	resource->size = 0;
	resource->align = 0;
	resource->gran = 0;
	resource->limit = 0xffffffffUL;
	resource->flags =
	    IORESOURCE_MEM | IORESOURCE_SUBTRACTIVE | IORESOURCE_ASSIGNED;
}

static void tolm_test(void *gp, struct device *dev, struct resource *new)
{
	struct resource **best_p = gp;
	struct resource *best;
	best = *best_p;
	if (!best || (best->base > new->base)) {
		best = new;
	}
	*best_p = best;
}

static uint32_t find_pci_tolm(struct bus *bus)
{
	struct resource *min;
	uint32_t tolm;
	min = 0;
	search_bus_resources(bus, IORESOURCE_MEM, IORESOURCE_MEM, tolm_test,
			     &min);
	tolm = 0xffffffffUL;
	if (min && tolm > min->base) {
		tolm = min->base;
	}
	return tolm;
}

static void pci_domain_set_resources(device_t dev)
{
	uint32_t pci_tolm;
	uint8_t tolud, reg8;
	uint16_t reg16;
	unsigned long long tomk, tolmk;

	pci_tolm = find_pci_tolm(&dev->link[0]);

	printk_spew("Base of stolen memory: 0x%08x\n",
		    pci_read_config32(dev_find_slot(0, PCI_DEVFN(2, 0)), 0x5c));

	tolud = pci_read_config8(dev_find_slot(0, PCI_DEVFN(0, 0)), 0x9c);
	printk_spew("Top of Low Used DRAM: 0x%08x\n", tolud << 24);

	tomk = tolud << 14;

	/* Note: subtract IGD device and TSEG */
	reg8 = pci_read_config8(dev_find_slot(0, PCI_DEVFN(0, 0)), 0x9e);
	if (reg8 & 1) {
		int tseg_size = 0;
		printk_debug("TSEG decoded, subtracting ");
		reg8 >>= 1;
		reg8 &= 3;
		switch (reg8) {
		case 0:
			tseg_size = 1024;
			break;	
		case 1:
			tseg_size = 2048;
			break;	
		case 2:
			tseg_size = 8192;
			break;	
		}

		printk_debug("%dM\n", tseg_size >> 10);
		tomk -= tseg_size;
	}

	reg16 = pci_read_config16(dev_find_slot(0, PCI_DEVFN(0, 0)), GGC);
	if (!(reg16 & 2)) {
		int uma_size = 0;
		printk_debug("IGD decoded, subtracting ");
		reg16 >>= 4;
		reg16 &= 7;
		switch (reg16) {
		case 1:
			uma_size = 1024;
			break;
		case 3:
			uma_size = 8192;
			break;
		}

		printk_debug("%dM UMA\n", uma_size >> 10);
		tomk -= uma_size;
	}

	/* The following needs to be 2 lines, otherwise the second
	 * number is always 0
	 */
	printk_info("Available memory: %dK", tomk);
	printk_info(" (%dM)\n", (tomk >> 10));

	tolmk = tomk;

	/* Report the memory regions */
	ram_resource(dev, 3, 0, 640);
	ram_resource(dev, 4, 768, (tolmk - 768));
	if (tomk > 4 * 1024 * 1024) {
		ram_resource(dev, 5, 4096 * 1024, tomk - 4 * 1024 * 1024);
	}

	assign_resources(&dev->link[0]);
}

static unsigned int pci_domain_scan_bus(device_t dev, unsigned int max)
{
	max = pci_scan_bus(&dev->link[0], 0, 0xff, max);
	/* TODO We could determine how many PCIe busses we need in
	 * the bar. For now that number is hardcoded to a max of 64.
	 */
	return max;
}

static struct device_operations pci_domain_ops = {
	.read_resources   = pci_domain_read_resources,
	.set_resources    = pci_domain_set_resources,
	.enable_resources = enable_childrens_resources,
	.init             = 0,
	.scan_bus         = pci_domain_scan_bus,
	.ops_pci_bus      = &pci_cf8_conf1,	/* Do we want to use the memory mapped space here? */
};

static void mc_read_resources(device_t dev)
{
	struct resource *resource;

	pci_dev_read_resources(dev);

	/* So, this is one of the big mysteries in the coreboot resource
	 * allocator. This resource should make sure that the address space
	 * of the PCIe memory mapped config space bar. But it does not.
	 */

	/* We use 0xcf as an unused index for our PCIe bar so that we find it again */
	resource = new_resource(dev, 0xcf);
	resource->base = DEFAULT_PCIEXBAR;
	resource->size = 64 * 1024 * 1024;	/* 64MB hard coded PCIe config space */
	resource->flags =
	    IORESOURCE_MEM | IORESOURCE_FIXED | IORESOURCE_STORED |
	    IORESOURCE_ASSIGNED;
	printk_debug("Adding PCIe enhanced config space BAR 0x%08x-0x%08x.\n",
		     resource->base, (resource->base + resource->size));
}

static void mc_set_resources(device_t dev)
{
	struct resource *resource, *last;

	/* Report the PCIe BAR */
	last = &dev->resource[dev->resources];
	resource = find_resource(dev, 0xcf);
	if (resource) {
		report_resource_stored(dev, resource, "<mmconfig>");
	}

	/* And call the normal set_resources */
	pci_dev_set_resources(dev);
}

static void intel_set_subsystem(device_t dev, unsigned vendor, unsigned device)
{
	pci_write_config32(dev, PCI_SUBSYSTEM_VENDOR_ID,
			   ((device & 0xffff) << 16) | (vendor & 0xffff));
}

static struct pci_operations intel_pci_ops = {
	.set_subsystem    = intel_set_subsystem,
};

static struct device_operations mc_ops = {
	.read_resources   = mc_read_resources,
	.set_resources    = mc_set_resources,
	.enable_resources = pci_dev_enable_resources,
	.init             = 0,
	.scan_bus         = 0,
	.ops_pci          = &intel_pci_ops,
};

static const struct pci_driver mc_driver __pci_driver = {
	.ops    = &mc_ops,
	.vendor = PCI_VENDOR_ID_INTEL,
	.device = 0x27a0,
};

static void cpu_bus_init(device_t dev)
{
	initialize_cpus(&dev->link[0]);
}

static void cpu_bus_noop(device_t dev)
{
}

static struct device_operations cpu_bus_ops = {
	.read_resources   = cpu_bus_noop,
	.set_resources    = cpu_bus_noop,
	.enable_resources = cpu_bus_noop,
	.init             = cpu_bus_init,
	.scan_bus         = 0,
};

static void enable_dev(device_t dev)
{
	/* Set the operations if it is a special bus type */
	if (dev->path.type == DEVICE_PATH_PCI_DOMAIN) {
		dev->ops = &pci_domain_ops;
	} else if (dev->path.type == DEVICE_PATH_APIC_CLUSTER) {
		dev->ops = &cpu_bus_ops;
	}
}

struct chip_operations northbridge_intel_i945_ops = {
	CHIP_NAME("Intel i945 Northbridge")
	.enable_dev = enable_dev,
};