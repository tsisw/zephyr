# Copyright (c) 2024 TSI
# SPDX-License-Identifier: Apache-2.0

if(DEFINED SOC_SERIES)
  add_subdirectory(${SOC_SERIES})
else()
  add_subdirectory(${SOC_NAME})
endif()

message(STATUS "SOC_SERIES Name: ${SOC_SERIES}")
