# Copyright (c) 2026 Analog Devices, Inc.
# SPDX-License-Identifier: BSD-3-Clause

# Board defaults for nucleo-h753zi
# The board uses the classic STM32CubeMX backend (.ioc file).
get_filename_component(_BOARD_DIR "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)

if(NOT CONFIG_STM32_IOC_PATH)
    set(CONFIG_STM32_IOC_PATH "${_BOARD_DIR}/nucleo-h753zi.ioc")
endif()

# CubeMX output directory. It must match BOARD_BUILD_DIR/IOC_NAME in sdk/stm32_platform_sdk.cmake.
set(CAPI_STM32_CUBEMX_BUILD_DIR "${CMAKE_BINARY_DIR}/nucleo-h753zi_build/nucleo-h753zi")
set(CAPI_STM32_BOARD_BSP_DIR "${_BOARD_DIR}/bsp")
set(CAPI_STM32_BOARD_BSP_SRC "${_BOARD_DIR}/bsp/parameters.c")

include("${CAPI_STM32_DIR}/cmake/cubemx_paths.cmake")
capi_stm32_resolve_cubemx_includes()
