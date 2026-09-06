/*
 * Copyright (c) 2024 TSI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio/gpio_mmio32.h>
#include <soc.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/init.h>


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

/*
 * IONE UART pad enables (not part of the PL011 register block, so the PL011
 * driver never touches them). Mirrors the bare-metal uart_init() pad setup:
 *   RX/CTS pads -> input enable  (IE, bit4) = 0x610
 *   TX/RTS pads -> output enable (OE, bit3) = 0x608
 * IONE chiplet base (M85 view, skylp) = 0x86000000
 * (0x60000000 + SKYLP_REGS_IONE_REGS_OFFSET 0x26000000). Runs before the PL011
 * driver init so the physical TX/RX pins are live.
 */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart0), okay) || \
    DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay)

#define IONE_BASE            0x86000000UL
#define IONE_UART_PAD_IE     0x610U
#define IONE_UART_PAD_OE     0x608U

/*
 * Pad-control register offsets within IONE. The block moved down by 4 between the G0822 and
 * G0838 RAL releases (the same shift the QSPI pads in IONW took), so the map must follow the
 * bitstream: with the wrong map the TX pads get the input value and RX_1 the output value, and
 * both consoles go dead at boot. Board reading on acboot_838 (G0838, 2026-09-06): RX_0 at 0x100,
 * TX_0 at 0x108, RX_1 at 0x110, TX_1 at 0x114, with TX pads reading 0x8 and transmitting.
 * CONFIG_TSI_IONE_PADS_G0838 selects that map; the default is the G0822 map (ione_regs.h).
 */
#if IS_ENABLED(CONFIG_TSI_IONE_PADS_G0838)
#define IONE_UART_RX_0_CONTROL  (IONE_BASE + 0x100U)
#define IONE_UART_CTS_0_CONTROL (IONE_BASE + 0x104U)
#define IONE_UART_TX_0_CONTROL  (IONE_BASE + 0x108U)
#define IONE_UART_RTS_0_CONTROL (IONE_BASE + 0x10cU)
#define IONE_UART_RX_1_CONTROL  (IONE_BASE + 0x110U)
#define IONE_UART_TX_1_CONTROL  (IONE_BASE + 0x114U)
#define IONE_UART_RTS_1_CONTROL (IONE_BASE + 0x118U)
#define IONE_UART_CTS_1_CONTROL (IONE_BASE + 0x11cU)
#else
#define IONE_UART_RX_0_CONTROL  (IONE_BASE + 0x104U)
#define IONE_UART_CTS_0_CONTROL (IONE_BASE + 0x108U)
#define IONE_UART_TX_0_CONTROL  (IONE_BASE + 0x10cU)
#define IONE_UART_RTS_0_CONTROL (IONE_BASE + 0x110U)
#define IONE_UART_RX_1_CONTROL  (IONE_BASE + 0x114U)
#define IONE_UART_TX_1_CONTROL  (IONE_BASE + 0x118U)
#define IONE_UART_RTS_1_CONTROL (IONE_BASE + 0x11cU)
#define IONE_UART_CTS_1_CONTROL (IONE_BASE + 0x120U)
#endif

#define IONE_REG(addr) (*(volatile uint32_t *)(addr))

static int tsi_ione_uart_pads_init(void)
{
	/* UART0 */
	IONE_REG(IONE_UART_RX_0_CONTROL)  = IONE_UART_PAD_IE;
	IONE_REG(IONE_UART_CTS_0_CONTROL) = IONE_UART_PAD_IE;
	IONE_REG(IONE_UART_TX_0_CONTROL)  = IONE_UART_PAD_OE;
	IONE_REG(IONE_UART_RTS_0_CONTROL) = IONE_UART_PAD_OE;

	/* UART1 */
	IONE_REG(IONE_UART_RX_1_CONTROL)  = IONE_UART_PAD_IE;
	IONE_REG(IONE_UART_CTS_1_CONTROL) = IONE_UART_PAD_IE;
	IONE_REG(IONE_UART_TX_1_CONTROL)  = IONE_UART_PAD_OE;
	IONE_REG(IONE_UART_RTS_1_CONTROL) = IONE_UART_PAD_OE;

	return 0;
}

/* Pads before the PL011 driver init (PRE_KERNEL_1 / SERIAL prio 50). */
SYS_INIT(tsi_ione_uart_pads_init, PRE_KERNEL_1, 0);

#endif /* uart0 or uart1 enabled */
