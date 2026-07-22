# Design: Port IONE PL011 UART to Zephyr (skylp / M85)

Date: 2026-07-19
Author: mmankali
Status: Draft, pending review

## 1. Objective

Bring the bare-metal IONE PL011 UART support currently used by the UVMF bench
tests (`uart_profpga.c` + `uart_init/uart_read/uart_write` in `test_lib.c`) into
the Zephyr port for the `tsi_skylp_m85` board, using Zephyr's in-tree
`arm,pl011` driver plus SoC-level glue. The IONE UART0 becomes the Zephyr
console/shell UART; UART1 is available as a second polled UART device.

Reference source (read-only, do NOT modify):
`/proj/work/zzhang/ws4/design/bk/scu/dv/uvmf/project_benches/scu_tb/tb/tests/`
- `c_src/uart_profpga.c` (bench test that calls the helpers)
- `c_lib/test_lib.c` lines 778-908 (`uart_init`, `uart_write`, `uart_read`)
- `c_inc/test_lib.h` (address macros)
- `bk/ione/logic/csr/cheader/ione_regs.h` (register offsets)

Target tree (changes go here):
`/proj/work/mmankali/zephyrproject/zephyr`

## 2. Background: what the source actually is

The bare-metal "driver" bundles three concerns:

| Concern | Registers | Notes |
|---|---|---|
| PL011 core | `UARTIBRD`, `UARTFBRD`, `UARTLCR_H`, `UARTCR`, `UARTDR`, `UARTFR` | Standard ARM PL011, two instances (UART0/UART1) on IONE |
| IO pad mux | `UART_{RX,TX,CTS,RTS}_{0,1}_CONTROL` | Not PL011. ProFPGA/SoC pad IE (`0x610`) / OE (`0x608`) enables |
| Addressing | `PREPEND_CHIPLET(IONE, ..._BYTE_ADDRESS)` | Absolute MMIO base per chiplet |

I/O model is blocking polled: TX polls `FR` bit5 (TXFF), RX polls `FR` bit4 (RXFE).

Zephyr ships `drivers/serial/uart_pl011.c` (`compatible = "arm,pl011"`), which
implements the PL011 core. The port therefore reduces to devicetree + SoC glue;
no new serial driver C code is required.

## 3. Scope decisions (confirmed with user)

- Target: **skylp only** (`boards/tsi/skylp`, `soc/tsi/skylp_v1`).
- Approach: **in-tree `arm,pl011` + SoC glue**; PL011 UART0 becomes console/shell.
- Instances: **both UART0 and UART1**.
- HW flow control: **disabled** (option 2). Use `CR = 0x301`
  (UARTEN|TXE|RXE), no `hw-flow-control` DT property. The bench's `0xf01`
  (RTSEN+CTSEN set) is intentionally not reproduced, to avoid TX stalls when
  CTS is not driven.

## 4. Addressing

The M85 sees chiplets in the `0x8000_0000 + chiplet_offset` window. Evidence:
the working `jtag_uart @ 0x86003000` equals LDU1 base (`SCU_REG_BASE 0x8000_0000
+ 0x0600_0000`) + `0x3000`. IONE base = `0x8000_0000 + 0x0500_0000 =
0x8500_0000` (from `IONE_REG_BASE` in `test_lib.h`).

Register offsets within IONE (from `ione_regs.h`):

| Symbol | Offset |
|---|---|
| UART_0 UARTDR / UARTFR / UARTIBRD / UARTFBRD / UARTLCR_H / UARTCR | `0x1000 / 0x1018 / 0x1024 / 0x1028 / 0x102c / 0x1030` |
| UART_1 (same order) | `0x2000 / 0x2018 / 0x2024 / 0x2028 / 0x202c / 0x2030` |
| UART_RX_0 / CTS_0 / TX_0 / RTS_0 CONTROL | `0x104 / 0x108 / 0x10c / 0x110` |
| UART_RX_1 / TX_1 / RTS_1 / CTS_1 CONTROL | `0x114 / 0x118 / 0x11c / 0x120` |

Resolved absolute addresses (M85 view):

| Node | Address | Size |
|---|---|---|
| PL011 UART0 block | `0x8500_1000` | `0x1000` |
| PL011 UART1 block | `0x8500_2000` | `0x1000` |
| Pad-control block | `0x8500_0000` | `0x1000` |

**OPEN RISK (verify in implementation step 1):** IONE base = `0x8500_0000` is
derived, not confirmed. The bench macro for skylp uses a different aperture
(`SKYLP_BASE_OFFSET 0x6000_0000 + SKYLP_REGS_IONE_REGS_OFFSET`). The `0x8x`
scheme is strongly favored by the working jtag_uart precedent. Confirm the base
before finalizing `reg` (inspect a working boot/address map, or smoke-test one
byte to UART0 DR). If the base is wrong, the symptom is no output or a bus fault.

## 5. Clock and baud

Bench `uart_init` sets `IBRD = 0xd` (13), `FBRD = 0x24` (36),
`LCR_H = 0x70` (8-bit word + FIFO enable), `CR` enables the UART.

PL011 divisor = `IBRD + FBRD/64 = 13 + 36/64 = 13.5625`.
`baud = UARTCLK / (16 * 13.5625)`. With `UARTCLK = 25 MHz` (the board `sysclk`),
`baud = 25e6 / 217 = 115207 ~= 115200`.

Therefore the UART clock must be 25 MHz and `current-speed = <115200>`. The
in-tree driver then computes `IBRD = 13`, `FBRD = round(0.5625 * 64) = 36`,
exactly matching the bench. Word length 8N1 + FIFO is the driver default.

**Driver clock-source detail (verified in `uart_pl011.c`):** this driver version
takes the UART clock from a `clocks` phandle that points to a `fixed-clock` node:

```c
.clk_freq = COND_CODE_1(
    DT_NODE_HAS_COMPAT(DT_INST_CLOCKS_CTLR(n), fixed_clock),
    (DT_INST_PROP_BY_PHANDLE(n, clocks, clock_frequency)), (0)),
```

A bare `clock-frequency` property on the UART node is **not** consumed. The
existing `sysclk` fixed-clock is defined only in `tsi_skylp_m85-common.dtsi`,
which is commented out of the `.dts`. Therefore the design adds a `fixed-clock`
node (`uartclk`, 25 MHz) in the `.dts` and references it from both PL011 nodes
via `clocks = <&uartclk>`. Without this, `clk_freq` resolves to 0 and the baud
divisor is invalid.

## 6. Changes

### 6.1 Devicetree: `boards/tsi/skylp/tsi_skylp_m85.dts`

Add two PL011 nodes and repoint the chosen console/shell from `jtag_uart` to
`uart0`. Keep the `jtag_uart` node defined but no longer chosen.

```dts
chosen {
    zephyr,console = &uart0;
    zephyr,shell-uart = &uart0;
    /* ... existing sram/flash entries unchanged ... */
};

uartclk: uartclk {
    compatible = "fixed-clock";
    clock-frequency = <25000000>;
    #clock-cells = <0>;
};

uart0: uart@85001000 {
    compatible = "arm,pl011";
    reg = <0x85001000 0x1000>;
    interrupts = <36 3>;    /* unused in polled mode; see note */
    clocks = <&uartclk>;
    current-speed = <115200>;
    status = "okay";
};

uart1: uart@85002000 {
    compatible = "arm,pl011";
    reg = <0x85002000 0x1000>;
    interrupts = <37 3>;    /* unused in polled mode; see note */
    clocks = <&uartclk>;
    current-speed = <115200>;
    status = "okay";
};
```

Notes:
- The `arm,pl011` binding marks `interrupts` as required, but all IRQ wiring in
  `uart_pl011.c` is under `#ifdef CONFIG_UART_INTERRUPT_DRIVEN`. With that config
  off (our polled console), the `interrupts` value is never used at runtime, so
  `<36 3>` / `<37 3>` are documented unused placeholders. They must be replaced
  with the real IONE-to-M85 NVIC lines only if interrupt-driven mode is enabled
  later. (No NVIC line map for IONE UART exists in `ione_regs.h`.)
- `clocks = <&uartclk>` is required (see section 5). `current-speed` comes from
  `uart-controller.yaml`.
- The stale `uart0: uart@14001000` (`arm,cmsdk-uart`) node in
  `tsi_skylp_m85-common.dtsi` is **confirmed commented out** (the whole `soc { }`
  block including its `#include` is inside `/* ... */`), so there is no `uart0`
  label collision. No action needed beyond not re-enabling that block.

### 6.2 SoC glue: `soc/tsi/skylp_v1/soc.c`

`<zephyr/platform/hooks.h>` / `soc_early_init_hook()` does **not** exist in this
tree (Zephyr 3.7.0-rc1), so use a `SYS_INIT()` at `PRE_KERNEL_1` priority `0`
(runs before the PL011 device init at `PRE_KERNEL_1` / `CONFIG_SERIAL_INIT_PRIORITY`).
It performs the 8 pad IE/OE writes:

| Register | Address | Value |
|---|---|---|
| UART_RX_0 / RX_1 / CTS_0 / CTS_1 CONTROL | `0x85000104 / 114 / 108 / 120` | `0x610` (IE, bit4) |
| UART_TX_0 / TX_1 / RTS_0 / RTS_1 CONTROL | `0x8500010c / 118 / 110 / 11c` | `0x608` (OE, bit3) |

Implementation uses `sys_write32(value, addr)` from `<zephyr/sys/sys_io.h>`.
On Cortex-M there is no MMU, so the physical addresses are written directly.

### 6.3 Kconfig: `boards/tsi/skylp/tsi_skylp_m85_defconfig`

Already present: `CONFIG_SERIAL=y`, `CONFIG_CONSOLE=y`, `CONFIG_UART_CONSOLE=y`.
Add:

```
CONFIG_UART_PL011=y
```

Leave `CONFIG_UART_INTERRUPT_DRIVEN` unset (polled console, matches bare-metal).

## 7. Memory attributes

The IONE region is in the `0x8x` window that the bench marks non-cacheable
(`set_non_cacheable(0x80000000, ...)`). On M85 under Zephyr the MPU governs
this. Because the existing `jtag_uart @ 0x86003000` in the same window already
works as the console, follow that precedent: no additional MPU region is
expected. Verify during bring-up that PL011 MMIO reads/writes are not cached
(symptom of a cache problem: stale `FR` reads, TX/RX hangs). Add an MPU device
region for `0x8500_0000` only if observed necessary.

## 8. Test plan and failure states

Step-by-step verification:

1. **DT resolves:** build `tsi_skylp_m85`; inspect `build/zephyr/zephyr.dts` for
   both `arm,pl011` nodes at `0x85001000` / `0x85002000` and
   `zephyr,console = &uart0`.
2. **Compiles/links:** `west build -b tsi_skylp_m85 samples/hello_world` with the
   defconfig change; no link errors, `uart_pl011.c` pulled in.
3. **Console output:** run `hello_world`; the banner must appear on IONE UART0.
4. **Second instance:** minimal app doing `uart_poll_out`/`uart_poll_in` on
   `DEVICE_DT_GET(DT_NODELABEL(uart1))` to confirm UART1 works.
5. **Loopback / echo (optional):** mirror the bench read-then-write to confirm RX.

Failure states to check explicitly:

| Failure | Likely cause | Symptom |
|---|---|---|
| No output at all | Wrong IONE base (section 4 risk), or `status` not okay | Silent, or bus/hard fault on first access |
| Garbled output | Wrong `clock-frequency`/`current-speed` (baud mismatch) | Framing errors on terminal |
| First chars lost / no TX | Pad IE/OE not enabled before UART init (ordering) | TX never leaves the pad |
| TX hangs | HW flow control accidentally enabled (must stay off) | `FR` TXFF never clears |
| Stale `FR` reads / hang | IONE MMIO cached (MPU) | Poll loops never exit |
| DT build error | `uart0` label collision with cmsdk node | devicetree compile failure |

## 9. Out of scope

- skyp target (only skylp requested).
- Interrupt-driven or async UART API.
- A pinctrl driver for the pad mux (glue in `soc.c` is sufficient for 8 fixed
  writes).
- Any change under `/proj/work/zzhang` (reference only).

## 10. Open items to resolve during implementation

1. Confirm IONE base address `0x8500_0000` (section 4 risk) — verified by build +
   `zephyr.dts` inspection and a runtime smoke test (console banner appears).

Resolved during design:
- Real IONE UART NVIC lines are **not needed** for the polled console; the
  `interrupts` values are documented unused placeholders (section 6.1).
- The `cmsdk-uart` `uart0` node is confirmed commented out; no label collision.
- `soc_early_init_hook` is unavailable in 3.7.0-rc1; the pad glue uses
  `SYS_INIT(PRE_KERNEL_1, 0)`.
