# Copyright (c) 2026 Analog Devices, Inc.
# SPDX-License-Identifier: BSD-3-Clause

# Set CUBEMX_INCLUDE_DIRS from TARGET and CAPI_STM32_CUBEMX_BUILD_DIR.
# Call it after board.cmake sets CAPI_STM32_CUBEMX_BUILD_DIR.
# The family comes from TARGET: stm32h753 gives H7, stm32f756 gives F7.
# This layout is for classic CubeMX. CubeMX2 boards do not call it.

macro(capi_stm32_resolve_cubemx_includes)
    string(REGEX MATCH "^stm32([a-z])([0-9])" _target_match "${TARGET}")
    if(_target_match)
        string(TOUPPER "${CMAKE_MATCH_1}" _family_letter)
        set(_family_number "${CMAKE_MATCH_2}")
        set(_stm32_family "${_family_letter}${_family_number}")
    else()
        message(FATAL_ERROR "Cannot derive STM32 family from TARGET='${TARGET}'")
    endif()

    set(CUBEMX_INCLUDE_DIRS
        ${CAPI_STM32_CUBEMX_BUILD_DIR}/Core/Inc
        ${CAPI_STM32_CUBEMX_BUILD_DIR}/Drivers/STM32${_stm32_family}xx_HAL_Driver/Inc
        ${CAPI_STM32_CUBEMX_BUILD_DIR}/Drivers/CMSIS/Device/ST/STM32${_stm32_family}xx/Include
        ${CAPI_STM32_CUBEMX_BUILD_DIR}/Drivers/CMSIS/Include
    )
endmacro()
