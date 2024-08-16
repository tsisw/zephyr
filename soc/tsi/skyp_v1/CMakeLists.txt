# Copyright (c) 2024 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0
if(CONFIG_SOC_SKYP_M85)

zephyr_sources(
  soc.c
  )

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")

endif()

if(CONFIG_SOC_SKYP_TXE)
message(STATUS "CMAKE_CURRENT_SOURCE_DIR: TEST: ${CMAKE_CURRENT_SOURCE_DIR}")
zephyr_sources(
  soc.c
  )
  zephyr_include_directories(${ZEPHYR_BASE}/include/zephyr/arch/xtensa/)
zephyr_include_directories(include/)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/include/xtensa-sample-controller.ld CACHE INTERNAL "")


endif()
