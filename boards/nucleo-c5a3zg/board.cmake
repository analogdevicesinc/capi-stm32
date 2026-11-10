# Copyright (c) 2026 Analog Devices, Inc.
# SPDX-License-Identifier: BSD-3-Clause

# Board defaults for nucleo-c5a3zg
# The board uses the STM32CubeMX2 backend. The classic tool has no STM32C5 device.
# The .ioc2 extension selects this backend. See capi_stm32_resolve_ioc_file().
get_filename_component(_BOARD_DIR "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)

if(NOT CONFIG_STM32_IOC_PATH)
    set(CONFIG_STM32_IOC_PATH "${_BOARD_DIR}/nucleo-c5a3zg.ioc2")
endif()

# The OpenOCD script of STM32C5 has no built-in flash driver. It needs a .stldr
# loader for the device ID, and adds no flash bank without it.
# 0x45A is the device ID of STM32C5A3xx. See capi_stm32_add_flash_target().
if(NOT DEFINED STM32_OPENOCD_STLDR_ID)
    set(STM32_OPENOCD_STLDR_ID "0x45A")
endif()

set(CAPI_STM32_BOARD_BSP_DIR "${_BOARD_DIR}/bsp")
set(CAPI_STM32_BOARD_BSP_SRC "${_BOARD_DIR}/bsp/parameters.c")

# Do not call capi_stm32_resolve_cubemx_includes() here. It assumes the classic
# Core/Inc and Drivers/... layout. CubeMX2 generates another layout and sets the
# include paths in its cmake/flags.cmake. They reach capi_stm32 and the
# application through the stm32cubemx2 target.
