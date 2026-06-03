/*
 * Copyright (c) 2024-2025 TSI
 *
 */

#include <stddef.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/version.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_output.h>
#include "tsi_mailbox.h"
#include "tsi_isolation_test.h"

#define LOG_MODULE_NAME plat_boot
LOG_MODULE_REGISTER(LOG_MODULE_NAME);
#define PRINT_TSI_BOOT_BANNER() printk("        Tsavorite Scalable Intelligence \n")

int main(void)
{
	int ret;

	/* TSI banner */
	PRINT_TSI_BOOT_BANNER();
	printf("\n");
	printf("    |||||||||||||||||||||||||||||||||||||\n");
	printf("    |||||||||||||||||||||||||||||||||||||\n");
	printf("    ||||||                          |||||\n");
	printf("    ||||||                          |||||\n");
	printf("    |||||||||||||||||   |||||       |||||\n");
	printf("               ||||||   |||||\n");
	printf("    ||||||     ||||||   |||||      ||||||\n");
	printf("     ||||||||  ||||||   |||||   ||||||||\n");
	printf("       ||||||||||||||   ||||||||||||||\n");
	printf("          |||||||||||   ||||||||||||\n");
	printf("            |||||||||   ||||||||||\n");
	printf("              |||||||   ||||||||\n");
	printf("                |||||   |||||\n");
	printf("                  |||   |||\n");
	printf("                        |\n");
	LOG_WRN("TSI Platform Software ");
	LOG_INF("Zephyr OS");
	printk("Zephyr Kernel version: %s\n", KERNEL_VERSION_STRING);
	printk("TSI Logging enabled & printk is functional\n");
	printk("Vtiwari custom GMC zephyr, with mailbox support: C1 \n");
	printk("[GMC-MB] main: starting mailbox handshake\n");
	tsi_mailbox_log_transport();

#ifdef TSI_EXPECT_SECURE_ISOLATION
	ret = tsi_isolation_run_tests(true);
#else
	ret = tsi_isolation_run_tests(false);
#endif
	if (ret != 0) {
		printk("[GMC-ISO] isolation probes failed ret=%d\n", ret);
		return ret;
	}

	ret = tsi_mailbox_boot_handshake(0U);
	if (ret != 0) {
		printk("[GMC-MB] main: handshake failed ret=%d\n", ret);
		LOG_ERR("Mailbox handshake failed: %d", ret);
		return ret;
	}
	printk("[GMC-MB] main: handshake done spe_ready=%d doorbell=0x%08x\n",
	       tsi_mailbox_spe_ready_seen() ? 1 : 0,
	       tsi_mailbox_read_doorbell());

	LOG_INF("Mailbox ready: spe_ready=%d doorbell=0x%08x",
		tsi_mailbox_spe_ready_seen() ? 1 : 0,
		tsi_mailbox_read_doorbell());

	ret = tsi_mailbox_run_psa_lab_sequence();
	if (ret != 0) {
		printk("[GMC-MB] main: PSA lab sequence failed ret=%d\n", ret);
		LOG_ERR("PSA lab sequence failed: %d", ret);
		return ret;
	}

	LOG_INF("PSA lab sequence OK (probe + VERSION + CONNECT + CALL over SHM)");
	return 0;
}
