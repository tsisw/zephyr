/*
 * Copyright (c) 2024 TSI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CONFIG_XTENSA_TENSILICA_NX
#include <zephyr/drivers/gpio/gpio_mmio32.h>
#include <soc.h>
#include <zephyr/linker/linker-defs.h>

/* Setup GPIO drivers for accessing FPGAIO registers */
#define FPGAIO_NODE(n) DT_INST(n, arm_mps3_fpgaio_gpio)
#define FPGAIO_INIT(n)						\
	GPIO_MMIO32_INIT(FPGAIO_NODE(n),			\
			DT_REG_ADDR(FPGAIO_NODE(n)),		\
			BIT_MASK(DT_PROP(FPGAIO_NODE(n), ngpios)))

/* We expect there to be 3 arm,mps3-fpgaio-gpio devices:
 * led0, button, and misc
 */

FPGAIO_INIT(2);
#else
#include <mpu.h>
#ifdef XTENSA_MPU
const int xtensa_soc_mpu_ranges_num = 0;

const struct xtensa_mpu_range xtensa_soc_mpu_ranges[] = {
	{
		/* This includes .data, .bss and various kobject sections. */
		.start = (uintptr_t)0x60600000,
		.end   = (uintptr_t)0x60700000,
		.access_rights = XTENSA_MPU_ACCESS_P_RO_U_RO,
		.memory_type = CONFIG_XTENSA_MPU_DEFAULT_MEM_TYPE,	
	},
        {
                /* This includes .data, .bss and various kobject sections. */
                .start = (uintptr_t)0x60000000,
                .end   = (uintptr_t)0x60100000,
                .access_rights =  XTENSA_MPU_ACCESS_P_RO_U_RO,
                .memory_type = CONFIG_XTENSA_MPU_DEFAULT_MEM_TYPE,
        }
};
#endif
#endif
