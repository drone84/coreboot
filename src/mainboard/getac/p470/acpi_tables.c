/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007-2010 coresystems GmbH
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
 * MA 02110-1301 USA
 */

#include <types.h>
#include <string.h>
#include <console/console.h>
#include <arch/acpi.h>
#include <arch/ioapic.h>
#include <arch/acpigen.h>
#include <arch/smp/mpspec.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>

extern unsigned char AmlCode[];

#include "southbridge/intel/i82801gx/nvs.h"

static void acpi_create_gnvs(global_nvs_t *gnvs)
{
	memset((void *)gnvs, 0, sizeof(*gnvs));
	gnvs->apic = 1;
	gnvs->mpen = 1; /* Enable Multi Processing */

	/* Enable COM port(s) */
	gnvs->cmap = 0x01;
	gnvs->cmbp = 0x00;

	/* IGD Displays  */
	gnvs->ndid = 2;
	gnvs->did[0] = 0x80000100;
	gnvs->did[1] = 0x80000410;
	gnvs->did[2] = 0x80000320;
	gnvs->did[3] = 0x80000410;
	gnvs->did[4] = 0x00000005;
}

static void acpi_create_intel_hpet(acpi_hpet_t * hpet)
{
#define HPET_ADDR  0xfed00000ULL
	acpi_header_t *header = &(hpet->header);
	acpi_addr_t *addr = &(hpet->addr);

	memset((void *) hpet, 0, sizeof(acpi_hpet_t));

	/* fill out header fields */
	memcpy(header->signature, "HPET", 4);
	memcpy(header->oem_id, OEM_ID, 6);
	memcpy(header->oem_table_id, ACPI_TABLE_CREATOR, 8);
	memcpy(header->asl_compiler_id, ASLC, 4);

	header->length = sizeof(acpi_hpet_t);
	header->revision = 1;

	/* fill out HPET address */
	addr->space_id = 0;	/* Memory */
	addr->bit_width = 64;
	addr->bit_offset = 0;
	addr->addrl = HPET_ADDR & 0xffffffff;
	addr->addrh = HPET_ADDR >> 32;

	hpet->id = 0x8086a201;	/* Intel */
	hpet->number = 0x00;
	hpet->min_tick = 0x0080;

	header->checksum =
	    acpi_checksum((void *) hpet, sizeof(acpi_hpet_t));
}

static long acpi_create_ecdt(acpi_ecdt_t * ecdt)
{
	/* Attention: Make sure these match the values from
	 * the DSDT's ec.asl
	 */
	static const char ec_id[] = "\\_SB.PCI0.LPCB.EC0";
	int ecdt_len = sizeof(acpi_ecdt_t) + strlen(ec_id) + 1;

	acpi_header_t *header = &(ecdt->header);

	memset((void *) ecdt, 0, ecdt_len);

	/* fill out header fields */
	memcpy(header->signature, "ECDT", 4);
	memcpy(header->oem_id, OEM_ID, 6);
	memcpy(header->oem_table_id, ACPI_TABLE_CREATOR, 8);
	memcpy(header->asl_compiler_id, ASLC, 4);

	header->length = ecdt_len;
	header->revision = 1;

	/* Location of the two EC registers */
	ecdt->ec_control.space_id = ACPI_ADDRESS_SPACE_IO;
	ecdt->ec_control.bit_width = 8;
	ecdt->ec_control.bit_offset = 0;
	ecdt->ec_control.addrl = 0x66;
	ecdt->ec_control.addrh = 0;

	ecdt->ec_data.space_id = ACPI_ADDRESS_SPACE_IO;	/* Memory */
	ecdt->ec_data.bit_width = 8;
	ecdt->ec_data.bit_offset = 0;
	ecdt->ec_data.addrl = 0x62;
	ecdt->ec_data.addrh = 0;

	ecdt->uid = 1; // Must match _UID of the EC0 node.

	ecdt->gpe_bit = 23; // SCI interrupt within GPEx_STS

	strncpy((char *)ecdt->ec_id, ec_id, strlen(ec_id));

	header->checksum =
	    acpi_checksum((void *) ecdt, ecdt_len);

	return header->length;
}

unsigned long acpi_fill_madt(unsigned long current)
{
	/* Local APICs */
	current = acpi_create_madt_lapics(current);

	/* IOAPIC */
	current += acpi_create_madt_ioapic((acpi_madt_ioapic_t *) current,
				2, IO_APIC_ADDR, 0);

	/* INT_SRC_OVR */
	current += acpi_create_madt_irqoverride((acpi_madt_irqoverride_t *)
		 current, 0, 0, 2, 0);
	current += acpi_create_madt_irqoverride((acpi_madt_irqoverride_t *)
		 current, 0, 9, 9, MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_HIGH);

	/* LAPIC_NMI */
	current += acpi_create_madt_lapic_nmi((acpi_madt_lapic_nmi_t *)
				current, 0, 0x0005, 0x01);
	current += acpi_create_madt_lapic_nmi((acpi_madt_lapic_nmi_t *)
				current, 1, 0x0005, 0x01);

	return current;
}

unsigned long acpi_fill_ssdt_generator(unsigned long current, const char *oem_table_id)
{
	generate_cpu_entries();
	return (unsigned long) (acpigen_get_current());
}

unsigned long acpi_fill_slit(unsigned long current)
{
	// Not implemented
	return current;
}

unsigned long acpi_fill_srat(unsigned long current)
{
	/* No NUMA, no SRAT */
	return current;
}

void smm_setup_structures(void *gnvs, void *tcg, void *smi1);

#define ALIGN_CURRENT current = ((current + 0x0f) & -0x10)
unsigned long write_acpi_tables(unsigned long start)
{
	unsigned long current;
	int i;
	acpi_rsdp_t *rsdp;
	acpi_rsdt_t *rsdt;
	acpi_xsdt_t *xsdt;
	acpi_hpet_t *hpet;
	acpi_madt_t *madt;
	acpi_mcfg_t *mcfg;
	acpi_fadt_t *fadt;
	acpi_facs_t *facs;
#if CONFIG_HAVE_ACPI_SLIC
	acpi_header_t *slic;
#endif
	acpi_header_t *ssdt;
	acpi_header_t *dsdt;
	acpi_header_t *ecdt;

	void *gnvs, *smi1;

	current = start;

	/* Align ACPI tables to 16byte */
	ALIGN_CURRENT;

	printk(BIOS_INFO, "ACPI: Writing ACPI tables at %lx.\n", start);

	/* We need at least an RSDP and an RSDT Table */
	rsdp = (acpi_rsdp_t *) current;
	current += sizeof(acpi_rsdp_t);
	ALIGN_CURRENT;
	rsdt = (acpi_rsdt_t *) current;
	current += sizeof(acpi_rsdt_t);
	ALIGN_CURRENT;
	xsdt = (acpi_xsdt_t *) current;
	current += sizeof(acpi_xsdt_t);
	ALIGN_CURRENT;

	/* clear all table memory */
	memset((void *) start, 0, current - start);

	acpi_write_rsdp(rsdp, rsdt, xsdt);
	acpi_write_rsdt(rsdt);
	acpi_write_xsdt(xsdt);

	/*
	 * We explicitly add these tables later on:
	 */
	printk(BIOS_DEBUG, "ACPI:    * HPET\n");

	hpet = (acpi_hpet_t *) current;
	current += sizeof(acpi_hpet_t);
	ALIGN_CURRENT;
	acpi_create_intel_hpet(hpet);
	acpi_add_table(rsdp, hpet);

	/* If we want to use HPET Timers Linux wants an MADT */
	printk(BIOS_DEBUG, "ACPI:    * MADT\n");

	madt = (acpi_madt_t *) current;
	acpi_create_madt(madt);
	current += madt->header.length;
	ALIGN_CURRENT;
	acpi_add_table(rsdp, madt);

	printk(BIOS_DEBUG, "ACPI:    * MCFG\n");
	mcfg = (acpi_mcfg_t *) current;
	acpi_create_mcfg(mcfg);
	current += mcfg->header.length;
	ALIGN_CURRENT;
	acpi_add_table(rsdp, mcfg);

	printk(BIOS_DEBUG, "ACPI:     * FACS\n");
	facs = (acpi_facs_t *) current;
	current += sizeof(acpi_facs_t);
	ALIGN_CURRENT;
	acpi_create_facs(facs);

	dsdt = (acpi_header_t *) current;
	memcpy(dsdt, &AmlCode, sizeof(acpi_header_t));
	current += dsdt->length;
	memcpy(dsdt, &AmlCode, dsdt->length);

	/* Fix up global NVS region for SMI handler. The GNVS region lives
	 * in the (high) table area. The low memory map looks like this:
	 *
	 * 0x00000000 - 0x000003ff	Real Mode IVT
	 * 0x00000400 - 0x000004ff	BDA (somewhat unused)
	 * 0x00000500 - 0x00000518	coreboot table forwarder
	 * 0x00000600 - 0x00000???	realmode trampoline
	 * 0x0007c000 - 0x0007dfff	OS boot sector (unused?)
	 * 0x0007e000 - 0x0007ffff	free to use (so no good for acpi+smi)
	 * 0x00080000 - 0x0009fbff	usable ram
	 * 0x0009fc00 - 0x0009ffff	EBDA (unused?)
	 * 0x000a0000 - 0x000bffff	VGA memory
	 * 0x000c0000 - 0x000cffff	VGA option rom
	 * 0x000d0000 - 0x000dffff	free for other option roms?
	 * 0x000e0000 - 0x000fffff	SeaBIOS? (conflict with low tables:)
	 * 0x000f0000 - 0x000f03ff	PIRQ table
	 * 0x000f0400 - 0x000f66??	ACPI tables
	 * 0x000f66?? - 0x000f????	DMI tables
	 */

	ALIGN_CURRENT;

	/* Pack GNVS into the ACPI table area */
	for (i=0; i < dsdt->length; i++) {
		if (*(u32*)(((u32)dsdt) + i) == 0xC0DEBABE) {
			printk(BIOS_DEBUG, "ACPI: Patching up global NVS in DSDT at offset 0x%04x -> 0x%08x\n", i, (u32)current);
			*(u32*)(((u32)dsdt) + i) = current; // 0x92 bytes
			break;
		}
	}

	/* And fill it */
	acpi_create_gnvs((global_nvs_t *)current);

	/* Keep pointer around */
	gnvs = (void *)current;

	current += 0x100;
	ALIGN_CURRENT;

	for (i=0; i < dsdt->length; i++) {
		if (*(u32*)(((u32)dsdt) + i) == 0xC0DEDEAD) {
			printk(BIOS_DEBUG, "ACPI: Patching up SMI1 area in DSDT at offset 0x%04x -> 0x%08x\n", i, (u32)current);
			*(u32*)(((u32)dsdt) + i) = current; // 0x100 bytes
			break;
		}
	}

	/* Keep pointer around */
	smi1 = (void *)current;

	current += 0x100;
	ALIGN_CURRENT;

	/* And tell SMI about it */
	smm_setup_structures(gnvs, NULL, smi1);

	/* We patched up the DSDT, so we need to recalculate the checksum */
	dsdt->checksum = 0;
	dsdt->checksum = acpi_checksum((void *)dsdt, dsdt->length);

	printk(BIOS_DEBUG, "ACPI:     * DSDT @ %p Length %x\n", dsdt,
		     dsdt->length);

	printk(BIOS_DEBUG, "ACPI:     * ECDT\n");
	ecdt = (acpi_header_t *)current;
	current += acpi_create_ecdt((acpi_ecdt_t *)current);
	ALIGN_CURRENT;
	acpi_add_table(rsdp, ecdt);

#if CONFIG_HAVE_ACPI_SLIC
	printk(BIOS_DEBUG, "ACPI:     * SLIC\n");
	slic = (acpi_header_t *)current;
	current += acpi_create_slic(current);
	ALIGN_CURRENT;
	acpi_add_table(rsdp, slic);
#endif

	printk(BIOS_DEBUG, "ACPI:     * FADT\n");
	fadt = (acpi_fadt_t *) current;
	current += sizeof(acpi_fadt_t);
	ALIGN_CURRENT;

	acpi_create_fadt(fadt, facs, dsdt);
	acpi_add_table(rsdp, fadt);

	printk(BIOS_DEBUG, "ACPI:     * SSDT\n");
	ssdt = (acpi_header_t *)current;
	acpi_create_ssdt_generator(ssdt, ACPI_TABLE_CREATOR);
	current += ssdt->length;
	acpi_add_table(rsdp, ssdt);
	ALIGN_CURRENT;

	printk(BIOS_DEBUG, "current = %lx\n", current);
	printk(BIOS_INFO, "ACPI: done.\n");
	return current;
}
