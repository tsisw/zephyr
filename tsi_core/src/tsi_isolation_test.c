/*
 * Copyright (c) 2026 Tsavorite Scalable Intelligence, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file tsi_isolation_test.c
 * @brief GMC (Zephyr) probes for SMC SAU/MPU isolation — see BOOKMARK_SRAM_MPU_SAU.md.
 */

#include "tsi_isolation_test.h"
#include "tsi_mailbox.h"
#include <stdint.h>
#include <zephyr/sys/printk.h>

#define TSI_PROBE_TFM_S_BASE  0x600D0000u
#define TSI_PROBE_BL2_BASE    0x60050000u
#define TSI_PROBE_NS_OWN_BASE 0x60100000u

volatile uint32_t tsi_iso_fault_caught;

static int tsi_iso_probe_read(uint32_t addr, uint32_t *out, bool expect_fault)
{
	volatile uint32_t *p = (volatile uint32_t *)(uintptr_t)addr;
	uint32_t val;

	tsi_iso_fault_caught = 0;
	val = *p;

	if (expect_fault) {
		printk("[GMC-ISO] FAIL probe 0x%08x: read 0x%08x (expected fault on silicon)\n", addr,
		       val);
		return -1;
	}

	if (out != NULL) {
		*out = val;
	}
	printk("[GMC-ISO] PASS probe 0x%08x: read 0x%08x\n", addr, val);
	return 0;
}

int tsi_isolation_run_tests(bool expect_secure_isolation)
{
	int ret = 0;
	uint32_t dummy;

	printk("[GMC-ISO] expect_secure_isolation=%d\n", expect_secure_isolation ? 1 : 0);

	if (tsi_iso_probe_read(TSI_PROBE_NS_OWN_BASE, &dummy, false) != 0) {
		ret = -1;
	}

	if (tsi_iso_probe_read(TSI_MB_SHM_BASE, &dummy, false) != 0) {
		ret = -1;
	}

	if (expect_secure_isolation) {
		if (tsi_iso_probe_read(TSI_PROBE_TFM_S_BASE, &dummy, true) != 0) {
			ret = -1;
		}
		if (tsi_iso_probe_read(TSI_PROBE_BL2_BASE, &dummy, true) != 0) {
			ret = -1;
		}
	} else {
		(void)tsi_iso_probe_read(TSI_PROBE_TFM_S_BASE, &dummy, false);
		(void)tsi_iso_probe_read(TSI_PROBE_BL2_BASE, &dummy, false);
		printk("[GMC-ISO] lab mode: secure probes informational only (MPC may be absent)\n");
	}

	return ret;
}
