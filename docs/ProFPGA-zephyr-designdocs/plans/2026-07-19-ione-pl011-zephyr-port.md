# IONE PL011 UART Zephyr Port Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the IONE ARM PL011 UART0/UART1 usable in the `tsi_skylp` M85 Zephyr build using the in-tree `arm,pl011` driver, with UART0 as the console/shell, reproducing the bare-metal `uart_init/read/write` behavior (8N1, 115200, polled, no flow control).

**Architecture:** Devicetree adds a 25 MHz `fixed-clock` plus two `arm,pl011` nodes at the IONE MMIO addresses and repoints `zephyr,console`/`zephyr,shell-uart` to UART0. A `SYS_INIT` hook in the skylp SoC writes the 8 IONE pad IE/OE control registers before the UART driver initializes. No new driver C code.

**Tech Stack:** Zephyr 3.7.0-rc1, in-tree `drivers/serial/uart_pl011.c` (`arm,pl011`), Cortex-M85, devicetree, Kconfig.

## Global Constraints

- Target only: board `tsi_skylp/skylp/m85`, SoC dir `soc/tsi/skylp_v1`, board dir `boards/tsi/skylp`. Do NOT touch skyp.
- Never modify anything under `/proj/work/zzhang` (reference only).
- Polled operation: keep `CONFIG_UART_INTERRUPT_DRIVEN` unset. No HW flow control (no `hw-flow-control` property).
- UART clock = 25 MHz via a `fixed-clock` referenced by `clocks = <&uartclk>` (this driver ignores a bare `clock-frequency` on the UART node). `current-speed = <115200>`.
- IONE base address (M85 view) = `0x8500_0000`. UART0 block `0x8500_1000`, UART1 block `0x8500_2000`, pad-control block base `0x8500_0000`. (See spec §4 risk — verified at runtime in Task 3.)
- Build command (from `zephyr/`): `west build -p always -b tsi_skylp/skylp/m85 tsi_core -- -DOVERLAY_CONFIG=prj_poll.conf`
- Reference spec: `docs/superpowers/specs/2026-07-19-ione-pl011-zephyr-port-design.md`

**Note on verification style:** This is a board/DT/SoC change with no host-runnable unit-test surface. "Tests" are (a) build success, (b) generated-devicetree inspection with exact expected content, and (c) a runtime smoke observation in the UVMF sim. Each task ends at an independently checkable deliverable.

---

### Task 1: Devicetree + Kconfig — add clock, PL011 nodes, repoint console

**Files:**
- Modify: `boards/tsi/skylp/tsi_skylp_m85.dts`
- Modify: `boards/tsi/skylp/tsi_skylp_m85_defconfig`

**Interfaces:**
- Produces: DT node labels `uartclk` (fixed-clock), `uart0` and `uart1` (`arm,pl011`); `chosen { zephyr,console = &uart0; zephyr,shell-uart = &uart0; }`. Task 3 consumes these at runtime.

- [ ] **Step 1: Baseline build to confirm current tree builds and console is jtag_uart**

Run (from `/proj/work/mmankali/zephyrproject/zephyr`):
```bash
west build -p always -b tsi_skylp/skylp/m85 tsi_core -- -DOVERLAY_CONFIG=prj_poll.conf
grep -n "zephyr,console\|zephyr,shell-uart" build/zephyr/zephyr.dts
```
Expected: build succeeds; grep shows `zephyr,console = &jtag_uart;` and `zephyr,shell-uart = &jtag_uart;`. This is the "before" state.

- [ ] **Step 2: Add the fixed-clock and two PL011 nodes to the root of the `.dts`**

In `boards/tsi/skylp/tsi_skylp_m85.dts`, inside the top-level `/ { ... }` node (place immediately after the existing `jtag_uart` node, before the `cpus` node), add:

```dts
	uartclk: uartclk {
		compatible = "fixed-clock";
		clock-frequency = <25000000>;
		#clock-cells = <0>;
	};

	uart0: uart@85001000 {
		compatible = "arm,pl011";
		reg = <0x85001000 0x1000>;
		interrupts = <36 3>;    /* unused in polled mode (CONFIG_UART_INTERRUPT_DRIVEN off) */
		interrupt-parent = <&nvic>;
		clocks = <&uartclk>;
		current-speed = <115200>;
		status = "okay";
	};

	uart1: uart@85002000 {
		compatible = "arm,pl011";
		reg = <0x85002000 0x1000>;
		interrupts = <37 3>;    /* unused in polled mode */
		interrupt-parent = <&nvic>;
		clocks = <&uartclk>;
		current-speed = <115200>;
		status = "okay";
	};
```

> **Execution note:** `interrupt-parent = <&nvic>` is required. Without it,
> `gen_defines.py` fails: "has an 'interrupts' property, but neither the node nor
> any of its parents has an 'interrupt-parent' property" (the root node sets no
> default interrupt-parent). Added during Task 1 execution.

- [ ] **Step 3: Repoint the chosen console/shell to uart0**

In the same file, in the existing `chosen { ... }` block, change:
```dts
		zephyr,console = &jtag_uart;
		zephyr,shell-uart = &jtag_uart;
```
to:
```dts
		zephyr,console = &uart0;
		zephyr,shell-uart = &uart0;
```
Leave `zephyr,sram` and `zephyr,flash` unchanged. Leave the `jtag_uart` node defined (just no longer chosen). Do NOT uncomment the `/*soc { ... }*/` block (that would pull in the conflicting `cmsdk-uart` `uart0` label).

- [ ] **Step 4: Enable the PL011 driver in the board defconfig**

In `boards/tsi/skylp/tsi_skylp_m85_defconfig`, under the `# Serial` section, add:
```
CONFIG_UART_PL011=y
```
Do not add `CONFIG_UART_INTERRUPT_DRIVEN`.

- [ ] **Step 5: Build and inspect generated devicetree**

Run:
```bash
west build -p always -b tsi_skylp/skylp/m85 tsi_core -- -DOVERLAY_CONFIG=prj_poll.conf
grep -n "zephyr,console\|zephyr,shell-uart" build/zephyr/zephyr.dts
grep -n "uart@85001000\|uart@85002000\|arm,pl011\|fixed-clock" build/zephyr/zephyr.dts
grep -n "CONFIG_UART_PL011" build/zephyr/.config
```
Expected:
- Build succeeds with no devicetree errors.
- `zephyr,console = &uart0;` and `zephyr,shell-uart = &uart0;`.
- Both `uart@85001000` and `uart@85002000` present with `compatible = "arm,pl011"`; a `fixed-clock` node present.
- `.config` contains `CONFIG_UART_PL011=y`.

- [ ] **Step 6: Commit**

```bash
git add boards/tsi/skylp/tsi_skylp_m85.dts boards/tsi/skylp/tsi_skylp_m85_defconfig
git commit -m "boards: tsi_skylp: add IONE PL011 uart0/uart1, console on uart0"
```

---

### Task 2: SoC glue — enable IONE UART pads before driver init

**Files:**
- Modify: `soc/tsi/skylp_v1/soc.c`
- Modify: `soc/tsi/skylp_v1/Kconfig.soc` (execution deviation, see notes below)

> **Execution notes (Task 2):**
> 1. **`skylp_v1/soc.c` was not being compiled.** `SOC_SKYLP` in
>    `soc/tsi/skylp_v1/Kconfig.soc` selected `SOC_SERIES_SKYP_V1` (copy-paste
>    bug), so the skylp build compiled `soc/tsi/skyp_v1/soc.c`. Fixed to
>    `select SOC_SERIES_SKYLP_V1`. The two SoC dirs are byte-identical apart from
>    the series symbol, so behavior is unchanged except that the glue now links.
>    No skyp file touched. Verified: `CONFIG_SOC_SERIES="skylp_v1"` and
>    `soc/tsi/skylp_v1/soc.c` in `build.ninja`.
> 2. **`sys_write32` is not inline in this config** (undefined reference at link).
>    Replaced with `volatile uint32_t *` writes via an `IONE_REG(addr)` macro,
>    matching the bare-metal source style. Dropped the `<zephyr/sys/sys_io.h>`
>    include; kept `<zephyr/init.h>` for `SYS_INIT`.

**Interfaces:**
- Consumes: nothing from Task 1 at build time (independent file).
- Produces: a `SYS_INIT` that runs at `PRE_KERNEL_1` priority `0`, before the PL011 device init (`PRE_KERNEL_1` / `CONFIG_SERIAL_INIT_PRIORITY`), so the RX/TX/CTS/RTS pads for UART0 and UART1 are enabled before first UART access.

- [ ] **Step 1: Add includes, the pad-init function, and the SYS_INIT hook**

In `soc/tsi/skylp_v1/soc.c`, add near the existing includes (after `#include <soc.h>`):
```c
#include <zephyr/init.h>
#include <zephyr/sys/sys_io.h>
```

Then add, after the existing `FPGAIO_INIT(2);` line:
```c
/*
 * IONE UART pad enables (not part of the PL011 register block).
 * Mirrors the bare-metal uart_init() pad setup:
 *   RX/CTS pads -> input enable (IE, bit4) = 0x610
 *   TX/RTS pads -> output enable (OE, bit3) = 0x608
 * IONE chiplet base (M85 view) = 0x85000000.
 */
#define IONE_BASE            0x85000000UL
#define IONE_UART_PAD_IE     0x610U
#define IONE_UART_PAD_OE     0x608U

/* Pad-control register offsets within IONE (from ione_regs.h) */
#define IONE_UART_RX_0_CONTROL  (IONE_BASE + 0x104U)
#define IONE_UART_CTS_0_CONTROL (IONE_BASE + 0x108U)
#define IONE_UART_TX_0_CONTROL  (IONE_BASE + 0x10cU)
#define IONE_UART_RTS_0_CONTROL (IONE_BASE + 0x110U)
#define IONE_UART_RX_1_CONTROL  (IONE_BASE + 0x114U)
#define IONE_UART_TX_1_CONTROL  (IONE_BASE + 0x118U)
#define IONE_UART_RTS_1_CONTROL (IONE_BASE + 0x11cU)
#define IONE_UART_CTS_1_CONTROL (IONE_BASE + 0x120U)

static int tsi_ione_uart_pads_init(void)
{
	/* UART0 */
	sys_write32(IONE_UART_PAD_IE, IONE_UART_RX_0_CONTROL);
	sys_write32(IONE_UART_PAD_IE, IONE_UART_CTS_0_CONTROL);
	sys_write32(IONE_UART_PAD_OE, IONE_UART_TX_0_CONTROL);
	sys_write32(IONE_UART_PAD_OE, IONE_UART_RTS_0_CONTROL);

	/* UART1 */
	sys_write32(IONE_UART_PAD_IE, IONE_UART_RX_1_CONTROL);
	sys_write32(IONE_UART_PAD_IE, IONE_UART_CTS_1_CONTROL);
	sys_write32(IONE_UART_PAD_OE, IONE_UART_TX_1_CONTROL);
	sys_write32(IONE_UART_PAD_OE, IONE_UART_RTS_1_CONTROL);

	return 0;
}

SYS_INIT(tsi_ione_uart_pads_init, PRE_KERNEL_1, 0);
```

- [ ] **Step 2: Build and confirm the init symbol is linked**

Run:
```bash
west build -p always -b tsi_skylp/skylp/m85 tsi_core -- -DOVERLAY_CONFIG=prj_poll.conf
grep -c "tsi_ione_uart_pads_init" build/zephyr/zephyr.map
```
Expected: build succeeds; grep count >= 1 (symbol present in the map, i.e., the SYS_INIT entry was pulled in).

- [ ] **Step 3: Commit**

```bash
git add soc/tsi/skylp_v1/soc.c
git commit -m "soc: tsi/skylp_v1: enable IONE UART0/1 pads before UART init"
```

---

### Task 3: Runtime smoke test — console/shell on IONE UART0 (verifies IONE base)

**Files:** none (verification task).

**Interfaces:**
- Consumes: the `uart0` console from Task 1 and the pad init from Task 2.

- [ ] **Step 1: Build the image**

```bash
west build -p always -b tsi_skylp/skylp/m85 tsi_core -- -DOVERLAY_CONFIG=prj_poll.conf
```
Expected: `build/zephyr/zephyr.hex` and the TSV stripped hex (`IM*.*`) are produced (board sets `CONFIG_BUILD_OUTPUT_TSV_STRIPPED_HEX=y`).

- [ ] **Step 2: Deploy to the UVMF sim and run**

Use the existing deploy flow (`run_west`/`meera_west` scp the images to the sim benches), then run the `scu_tb` UVMF sim for the skylp M85 as usual. Watch the IONE UART0 output in the sim's UART capture/log (the same monitor that the bare-metal `uart_profpga` test used for UART0).

Expected: the `tsi_core` shell/console output (shell prompt, and any app banner) appears on IONE UART0 at 115200 8N1. `CONFIG_BOOT_BANNER=n` is set in `prj_poll.conf`, so expect the shell prompt / app prints rather than the Zephyr banner.

- [ ] **Step 3: Interpret result against failure table**

If output is correct: IONE base `0x85000000`, baud, and pad init are all confirmed. Done.

If no output at all: the IONE base assumption or `reg` is wrong (spec §4 risk). Cross-check against the working `jtag_uart` and the bare-metal resolved address. If the M85 actually reaches IONE via the `0x60000000` skylp aperture instead, change both `reg` values (UART0 `0x60000000 + SKYLP_REGS_IONE_REGS_OFFSET + 0x1000`, UART1 `+ 0x2000`) and the `IONE_BASE` macro in `soc.c` to match, then rebuild and rerun. Obtain `SKYLP_REGS_IONE_REGS_OFFSET` from the generated `skylp_regs.h` in the sim build.

If output is garbled: baud mismatch — confirm the sim's UART clock into IONE really is 25 MHz; adjust the `uartclk` `clock-frequency` so `clk/(16*115200)` gives IBRD=13/FBRD=36.

If the poll loop hangs / stale reads: IONE MMIO is being cached — add an MPU device region for `0x85000000` in the skylp SoC MPU config (follow the region attributes used for the working jtag_uart window).

- [ ] **Step 4: Commit any fixups from Step 3 (only if changes were made)**

```bash
git add -A
git commit -m "boards/soc: tsi_skylp: fix IONE UART base/baud/mpu per bring-up"
```

---

### Task 4: Verify UART1 as a second polled device

**Files:**
- Modify (temporary verification): `tsi_core/src/main.c` (revert after, or gate behind a build flag)

**Interfaces:**
- Consumes: `uart1` node label from Task 1.

- [ ] **Step 1: Add a minimal UART1 echo to `main` (temporary)**

In `tsi_core/src/main.c`, near the top of `main()` (adjust to the file's existing structure), add a self-contained block:
```c
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

/* --- temporary UART1 bring-up check --- */
{
	const struct device *u1 = DEVICE_DT_GET(DT_NODELABEL(uart1));

	if (device_is_ready(u1)) {
		const char msg[] = "UART1 OK\r\n";

		for (int i = 0; i < (int)sizeof(msg) - 1; i++) {
			uart_poll_out(u1, msg[i]);
		}
	}
}
```

- [ ] **Step 2: Build**

```bash
west build -p always -b tsi_skylp/skylp/m85 tsi_core -- -DOVERLAY_CONFIG=prj_poll.conf
```
Expected: build succeeds; `DEVICE_DT_GET(DT_NODELABEL(uart1))` resolves (no build error means the `uart1` node exists and is `status = "okay"`).

- [ ] **Step 3: Deploy and observe UART1**

Deploy to sim as in Task 3. Expected: `UART1 OK` appears on the IONE UART1 capture (distinct from the UART0 console).

- [ ] **Step 4: Revert the temporary check and commit**

Remove the temporary block from `main.c` (UART1 is now proven available for real app use).
```bash
git add tsi_core/src/main.c
git commit -m "tsi_core: revert temporary UART1 bring-up check"
```

---

## Self-Review

**Spec coverage:**
- §4 addressing → Task 1 `reg` values + Task 2 `IONE_BASE`; runtime confirmation Task 3.
- §5 clock/baud → Task 1 `uartclk` fixed-clock + `current-speed`.
- §6.1 DT nodes + chosen → Task 1.
- §6.2 pad glue via SYS_INIT → Task 2.
- §6.3 `CONFIG_UART_PL011=y` → Task 1 Step 4.
- §7 memory attributes → Task 3 Step 3 (MPU fallback).
- §8 test plan / failure states → Tasks 1 Step 5, 3, 4.
- §9 out of scope respected (skylp only, polled, no pinctrl driver).
- §10 open item 1 (IONE base) → Task 3 Step 3.

**Placeholder scan:** No "TBD/TODO". The `interrupts = <36 3>`/`<37 3>` values are concrete and documented as unused in polled mode (spec §6.1). The `0x60000000`-aperture path in Task 3 Step 3 is a conditional fallback, not a placeholder.

**Type/name consistency:** node labels `uartclk`/`uart0`/`uart1`, macro `IONE_BASE`, function `tsi_ione_uart_pads_init` used consistently across tasks. Pad offsets match spec §4 / `ione_regs.h`.
