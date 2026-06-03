# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# NS SRAM slot (TSI_NONSECURE_BASE); avoids collision with BL1_2 @ 0x60000000 in guest_mem.
set(DTC_OVERLAY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/boards/gmc_ns_mem.overlay)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(m85)

option(TSI_EXPECT_SECURE_ISOLATION "Expect secure SRAM probes to fault (silicon)" OFF)
if(TSI_EXPECT_SECURE_ISOLATION)
  target_compile_definitions(app PRIVATE TSI_EXPECT_SECURE_ISOLATION=1)
endif()

target_sources(app PRIVATE
  src/main.c
  src/tsi_mailbox.c
  src/tsi_isolation_test.c
)
