# ProFPGA Zephyr Enablement (branch `profpga-806`)

**Purpose:** Bring up Zephyr on the Tsavorite **skylp** SoC (Cortex-M85) running on
the **Siemens ProFPGA** prototyping platform, with a working serial console over
the IONE **ARM PL011** UART.

**Scope of this document:** every change on branch `profpga-806` that is not on
`main` (`git diff main...profpga-806`), with the rationale for each.

**Status:** Confirmed working on ProFPGA hardware — Zephyr boots and the real
console (`printk` / logging / shell) prints over IONE PL011 `uart0`
(`0x86001000`): TSI banner, `LOG_INF`, and `HelloWorld!!`.

---

## 1. Background

- **SoC / board:** `tsi_skylp/skylp/m85` (Cortex-M85), `soc/tsi/skylp_v1`,
  `boards/tsi/skylp`.
- **Two platforms share the board:**
  - **Bittware** (Intel FPGA): has an Altera **JTAG UART** used as the console.
  - **ProFPGA** (Siemens, Xilinx-based): has **no** Altera JTAG UART; it uses the
    IONE **PL011** UART (the same UART driven by the bare-metal bring-up tests,
    e.g. `uart_profpga.c`, from reference DV uart enablement sources shared by
    zzhang).
- **Reference for the UART:** the bare-metal `uart_init()` / `uart_write()` from
  the reference DV uart enablement sources shared by zzhang, known to work on
  ProFPGA. The Zephyr work reproduces its proven register configuration.

The branch carries **two independent efforts** that are not on `main`:

| Effort | Commits | Files |
|---|---|---|
| SCU reset enablement (pull M85/A53 out of reset, boot Zephyr) — FIR-686, FIR-1881 (pre-existing) | `8968aa0`, `a7ef1327` | `arch/arm/core/cortex_m/reset.S`, `tsi_core/src/main.c`, `samples/subsys/shell/devmem_load/src/main.c` |
| IONE PL011 UART console enablement (this effort) | `339db02`, `a12d60e`, `ee65b1a`, `0cfc371`, `a40ce44`, `89e6e68`, `41d27fe`, `ebab94d` | `boards/tsi/skylp/*`, `soc/tsi/skylp_v1/*`, `ione-pl011-uart.patch` |

---

## 2. Net file changes (`main` -> `profpga-806`)

| File | +/- | Effort | Summary |
|---|---|---|---|
| `arch/arm/core/cortex_m/reset.S` | +7 | reset | Clear GLOBAL_RESET bits [2:0] at reset entry to pull blocks out of reset |
| `tsi_core/src/main.c` | +10/-11 | reset/app | Banner + log/printk demonstrating console output |
| `samples/subsys/shell/devmem_load/src/main.c` | +44 | reset | Sample tweak (part of FIR-686) |
| `boards/tsi/skylp/tsi_skylp_m85.dts` | +31 | UART | Add `uartclk` + PL011 `uart0`/`uart1` nodes; default console = `jtag_uart` |
| `boards/tsi/skylp/tsi_skylp_m85_defconfig` | +1 | UART | `CONFIG_UART_PL011=y` |
| `boards/tsi/skylp/profpga.overlay` | +32 (new) | UART | ProFPGA overlay: console -> `uart0`, disable `jtag_uart` |
| `soc/tsi/skylp_v1/soc.c` | +46 | UART | IONE UART pad-enable `SYS_INIT` |
| `soc/tsi/skylp_v1/Kconfig.soc` | +1/-1 | UART | `SOC_SKYLP` selects `SOC_SERIES_SKYLP_V1` (was `SKYP_V1`) |
| `ione-pl011-uart.patch` | +160 (new) | UART | Combined patch of the UART changes for reference/re-application |

---

## 3. UART enablement — detailed changes and rationale

### 3.1 Devicetree — `boards/tsi/skylp/tsi_skylp_m85.dts`

Added a 25 MHz fixed-clock and two `arm,pl011` UART nodes at the IONE addresses;
kept the board default console on `jtag_uart` (for Bittware).

```dts
uartclk: uartclk { compatible = "fixed-clock"; clock-frequency = <25000000>; #clock-cells = <0>; };

uart0: uart@86001000 {
    compatible = "arm,pl011";
    reg = <0x86001000 0x1000>;
    interrupts = <36 3>;            /* required by binding; unused in polled mode */
    interrupt-parent = <&nvic>;
    clocks = <&uartclk>;
    current-speed = <115200>;
    status = "okay";
};
uart1: uart@86002000 { ... reg = <0x86002000 0x1000>; interrupts = <37 3>; ... };
```

**Why:**
- **`arm,pl011`** is Zephyr's in-tree PL011 driver; the IONE UART is a PL011, so
  no custom driver is needed.
- **Address `0x86001000`:** the skylp M85 sees the IONE chiplet at `0x86000000`
  (`0x60000000 + SKYLP_REGS_IONE_REGS_OFFSET 0x26000000`, from the generated
  `skylp_regs.h`). UART0 block is `+0x1000`, UART1 `+0x2000`. This was verified
  against the reset register, which resolves the same way:
  `0x60000000 + SCU 0x20000000 + 0x800014 = 0x80800014`.
- **`uartclk` + `clocks` phandle:** this PL011 driver reads its input clock from a
  `clocks` phandle pointing to a `fixed-clock` node (a bare `clock-frequency` on
  the UART node is not consumed). 25 MHz with `current-speed = 115200` makes the
  driver program `IBRD=0xd`, `FBRD=0x24`, exactly matching the bare-metal.
- **`interrupt-parent = <&nvic>`:** required because the node has an `interrupts`
  property and the root node sets no default interrupt-parent. The value itself
  is unused in polled mode.
- **Default console stays `jtag_uart`:** keeps the Bittware flow unchanged; the
  ProFPGA overlay overrides it.

### 3.2 Kconfig — `boards/tsi/skylp/tsi_skylp_m85_defconfig`

```
CONFIG_UART_PL011=y
```
**Why:** enable the PL011 driver. Polled mode is used throughout
(`CONFIG_UART_INTERRUPT_DRIVEN` stays off; the board defconfig also forces
`UART_INTERRUPT_DRIVEN=n` under `SERIAL`), because interrupts are not wired on
the FPGA.

### 3.3 SoC glue — `soc/tsi/skylp_v1/soc.c`

Added a `SYS_INIT` at `PRE_KERNEL_1` that enables the IONE UART **pads**:

```c
#define IONE_BASE 0x86000000UL
/* RX/CTS -> input enable (0x610); TX/RTS -> output enable (0x608) */
IONE_REG(IONE_BASE + 0x104) = 0x610; /* UART_RX_0  */
IONE_REG(IONE_BASE + 0x108) = 0x610; /* UART_CTS_0 */
IONE_REG(IONE_BASE + 0x10c) = 0x608; /* UART_TX_0  */
IONE_REG(IONE_BASE + 0x110) = 0x608; /* UART_RTS_0 */
/* ... UART1 pads at 0x114/0x120/0x118/0x11c ... */
```
**Why:** the pad-control registers are part of the IONE chiplet, not the PL011
register block, so the in-tree PL011 driver never touches them. Without enabling
the pad IE/OE, the physical TX/RX pins are not driven and nothing reaches the
wire. This mirrors the pad setup in the bare-metal `uart_init()`. It runs before
the PL011 driver init (`PRE_KERNEL_1`, prio 0) so the pins are live first.

### 3.4 SoC series fix — `soc/tsi/skylp_v1/Kconfig.soc`

```
-config SOC_SKYLP ... select SOC_SERIES_SKYP_V1
+config SOC_SKYLP ... select SOC_SERIES_SKYLP_V1
```
**Why:** copy-paste bug. `SOC_SKYLP` selected the **skyp_v1** series, so the skylp
build compiled `soc/tsi/skyp_v1/soc.c` and never `skylp_v1/soc.c` — meaning the
SoC pad-init glue would not have been built at all. The two SoC directories are
otherwise byte-identical, so this fix is skylp-only and touches no skyp file.

### 3.5 ProFPGA console overlay — `boards/tsi/skylp/profpga.overlay` (new)

```dts
/ { chosen { zephyr,console = &uart0; zephyr,shell-uart = &uart0; }; };
&jtag_uart { status = "disabled"; };
```
Applied only for ProFPGA builds:
```
-DEXTRA_DTC_OVERLAY_FILE=<zephyr>/boards/tsi/skylp/profpga.overlay
```
**Why (two parts):**
1. **Console -> `uart0` (PL011):** ProFPGA has no JTAG UART, so the console must
   use the PL011.
2. **Disable `jtag_uart`:** this was the final unlock. `CONFIG_UART_ALTERA_JTAG`
   is enabled and the `altr,jtag-uart` node was `okay`, so its driver
   `uart_altera_jtag_init()` runs at `PRE_KERNEL_1` and **reads/writes
   `0x86003000`** (the Altera JTAG UART). That IP does not exist on ProFPGA, so
   the access faulted/hung and **killed boot before the application ran** — which
   is why the console appeared silent. Disabling the node drops that driver
   instance entirely. (On Bittware the node stays enabled via the board default,
   since the IP is present there.)

---

## 4. Root-cause chain (why the console was silent, in order of discovery)

Each of these had to be fixed before the console worked on ProFPGA:

1. **Wrong IONE base.** First attempt used `0x85000000` (a generic
   `IONE_REG_BASE` define). The skylp-correct base is `0x86000000`. Until fixed,
   all UART access hit the wrong address.
2. **Pads not enabled.** The PL011 driver does not configure the IONE pad
   IE/OE; without the SoC `SYS_INIT`, TX/RX pins are not driven.
3. **Hardware flow control.** The bare-metal `CR=0xf01` enables RTS/CTS; CTS is
   not asserted on ProFPGA, so TX stalled after a few characters. Correct value
   is `CR=0x301` (UARTEN|TXE|RXE, no flow control) — which is exactly what the
   in-tree driver programs (`flow_ctrl = NONE`).
4. **Altera `jtag_uart` init fault.** With 1-3 fixed, boot still hung before the
   app because the enabled `altr,jtag-uart` driver touched non-existent
   `0x86003000`. Disabling the node on ProFPGA was the final fix.

Ruled out along the way (did **not** matter): a full SCU global-reset write from
Zephyr (the reset handled in `reset.S` is sufficient; the UART worked without an
extra global-reset write), and a non-cacheable MPU region (the D-cache is off in
this build — `CONFIG_ARCH_CACHE` not set — so MMIO is never cached).

### Diagnostic method (how #4 was found without JTAG on ProFPGA)
ProFPGA exposes no JTAG port, so the UART was the only observable. A raw PL011
transmit (mirroring the bare-metal register sequence) was added at each boot
`SYS_INIT` level as progress markers (`M1..M5`). Only `M1` (before the PL011
driver) printed, even with console/log/shell disabled, which localized the hang
to a device init at `PRE_KERNEL_1` — the Altera JTAG UART driver. The markers
were removed after the fix (`soc.c` final state keeps only the pad-init).

---

## 5. Build and run

### ProFPGA (console = PL011)
```bash
cd /proj/work/mmankali/zephyrproject/zephyr
west build -d build_profpga -p always -b tsi_skylp/skylp/m85 tsi_core -- \
    -DOVERLAY_CONFIG=prj_poll.conf \
    -DEXTRA_DTC_OVERLAY_FILE=/proj/work/mmankali/zephyrproject/zephyr/boards/tsi/skylp/profpga.overlay
# then convert build_profpga/zephyr/IMAGE0_testfile.hex -> rom.hex / sram0.hex
# and load ProFPGA.
```

### Bittware (console = JTAG UART, board default)
```bash
west build -d build_bittware -p always -b tsi_skylp/skylp/m85 tsi_core -- \
    -DOVERLAY_CONFIG=prj_poll.conf
```

Note: `run_west` builds without the overlay and copies from `build/`; add the
`-DEXTRA_DTC_OVERLAY_FILE=...profpga.overlay` flag for ProFPGA, or build into the
default `build/` dir with the overlay so `run_west`'s scp picks up the right
image. Builds run on `wssim0`; the `scp` targets are same-host copies into the
sim bench directories.

---

## 6. Verified on ProFPGA hardware

Console output over IONE PL011 `uart0` at 115200 8N1, polled:
```
        Tsavorite Scalable Intelligence
    (ASCII art banner)
[00:00:00.0xx,000] <inf> m85: Logging Info: Test Platform: tsi_skylp/skylp/m85
 SCU Global Reset exercised successfully.
 HelloWorld!!
uart:~$
```

---

## 7. Key address reference (skylp M85 view)

Formula: `0x60000000 + SKYLP_REGS_<blk>_REGS_OFFSET + reg_offset`.

| Item | Address |
|---|---|
| IONE chiplet base | `0x86000000` |
| PL011 UART0 block | `0x86001000` |
| PL011 UART1 block | `0x86002000` |
| IONE UART pad controls | `0x86000104` .. `0x86000120` |
| Altera JTAG UART (Bittware only) | `0x86003000` |
| SCU GLOBAL_RESET | `0x80800014` |

UART0 config values (match bare-metal `uart_init`): `IBRD=0xd`, `FBRD=0x24`,
`LCR_H=0x70` (8N1 + FIFO), `CR=0x301` (UARTEN|TXE|RXE, no flow control).

---

## 8. Notes and open items

- **Branch scope:** `profpga-806` is intentionally kept off `main`; it also
  carries the FIR-686/1881 reset commits. Nothing has been pushed.
- **`ione-pl011-uart.patch`** is a reference copy of the UART DT/SoC changes;
  it applies to a tree that does not already have them (it will not apply on
  `profpga-806`, which already contains them).
- **Interrupt mode is not used** (no interrupts wired on the FPGA). If enabled
  later, the DT `interrupts` values are placeholders and must be set to the real
  IONE-to-M85 NVIC lines.
- **UART1** is instantiated and available as a second polled device
  (`0x86002000`).

---

## 9. Reproduction from the tag (procedure for another user)

The tag **`profpga-zephyr-ver1.0`** captures the full working state (Zephyr
source, board/SoC changes, app, deploy script, and docs). The build is
deterministic: the same source + Zephyr SDK + `west` modules produce the same
`IMAGE0_testfile.hex`.

### Prerequisites
- A `west` workspace with this zephyr as the manifest repo.
- **Zephyr SDK 0.16.8** installed (toolchain variant `zephyr`, `arm-zephyr-eabi`).
- `west` >= 1.2.0, and the design/sim tree if loading via the sim bench dirs.

### Steps
```bash
# 1. Check out the tagged version and fetch its pinned modules
cd <zephyrproject>/zephyr
git fetch --tags
git checkout profpga-zephyr-ver1.0
west update                     # fetch the module versions this manifest pins

# 2. Build the ProFPGA image (console = IONE PL011 uart0)
west build -d build_profpga -p always -b tsi_skylp/skylp/m85 tsi_core -- \
    -DOVERLAY_CONFIG=prj_poll.conf \
    -DEXTRA_DTC_OVERLAY_FILE=$PWD/boards/tsi/skylp/profpga.overlay
#    (equivalently: run ./run_west from the zephyr dir, which builds with the
#     overlay and scp's the image to the sim bench dirs)

# 3. Output image:
#    build_profpga/zephyr/IMAGE0_testfile.hex

# 4. Convert IMAGE0_testfile.hex -> rom.hex / sram0.hex
#    TODO(mmankali): document the conversion tool/command here.

# 5. Load rom.hex / sram0.hex onto the ProFPGA (Siemens M85) board.
#    TODO(mmankali): document the ProFPGA load procedure here.

# 6. Open a serial terminal on the IONE UART0 port: 115200 8N1, no flow control.
```

### Expected result (console on IONE PL011 uart0)
```
        Tsavorite Scalable Intelligence
    (ASCII art banner)
[00:00:00.0xx,000] <inf> m85: Logging Info: Test Platform: tsi_skylp/skylp/m85
 SCU Global Reset exercised successfully.
 HelloWorld!!
uart:~$
```

### Notes for reproduction
- Use the **`profpga.overlay`** for ProFPGA; without it the console reverts to
  the Altera `jtag_uart` (Bittware) and there will be no output on ProFPGA.
- Polled mode only (`CONFIG_UART_INTERRUPT_DRIVEN` off) — interrupts are not
  wired on the FPGA.
- If the build image differs from the reference, check the Zephyr SDK version
  and `west update` module state first (those are environment-level, not in the
  tag).
