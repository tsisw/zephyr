/*
 * Copyright (c) 2024 TSI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/linker/linker-defs.h>


/* Setup GPIO drivers for accessing FPGAIO registers */
/* We expect there to be 3 arm,mps3-fpgaio-gpio devices:
 * led0, button, and misc
 */
