# Copyright (c) 2026 Analog Devices, Inc.
# SPDX-License-Identifier: BSD-3-Clause

# capi-stm32: STM32 platform backend for CAPI
# Provides the capi_stm32 library target (add_subdirectory).
# Needs the CAPI headers (CAPI_DIR) and the CONFIG_* values of Kconfig.

cmake_minimum_required(VERSION 3.27)

if(NOT DEFINED CAPI_DIR)
    message(FATAL_ERROR "CAPI_DIR is not set. Point it at the CAPI interface repo.")
endif()

# Add sources only if the Kconfig symbol is on
function(capi_stm32_sources_ifdef feature_toggle source)
    if(${feature_toggle})
        target_sources(capi_stm32 PRIVATE ${source} ${ARGN})
    endif()
endfunction()

# Core sources. The build always compiles them.
add_library(capi_stm32 OBJECT
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32_capi_platform.c
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32_capi_irq.c
)

# Time and alloc. FreeRTOS provides them, so the build skips them then.
if(NOT CONFIG_FREERTOS)
    target_sources(capi_stm32 PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32_capi_alloc.c
        ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32_capi_time.c
    )
endif()

# HAL generation. STM32C5 is the first family with the new HAL. Its functions
# have the same names as the classic HAL, with other signatures.
# platform/stm32_hal_port.h declares the CAPI names for the differences.
# The build compiles one implementation directory. The drivers use only the
# porting layer, so they build with both.
if(NOT DEFINED CONFIG_STM32_HAL2)
    if("${TARGET}" MATCHES "^stm32c5")
        set(CONFIG_STM32_HAL2 ON)
    else()
        set(CONFIG_STM32_HAL2 OFF)
    endif()
endif()

if(CONFIG_STM32_HAL2)
    set(STM32_HAL_PORT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/platform/hal2)
    # PUBLIC, because the board BSPs include headers whose types depend on the HAL
    target_compile_definitions(capi_stm32 PUBLIC CONFIG_STM32_HAL2)
else()
    set(STM32_HAL_PORT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/platform/hal1)
endif()

# Interface backends, selected by the Kconfig symbols
capi_stm32_sources_ifdef(CONFIG_UART_STM32       ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32_capi_uart.c
                                                 ${STM32_HAL_PORT_DIR}/stm32_hal_port_uart.c)
capi_stm32_sources_ifdef(CONFIG_UART_STDIO_STM32 ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32_capi_uart_stdio.c)
capi_stm32_sources_ifdef(CONFIG_GPIO_STM32       ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32_capi_gpio.c
                                                 ${STM32_HAL_PORT_DIR}/stm32_hal_port_gpio.c)
capi_stm32_sources_ifdef(CONFIG_GPIO_IRQ_STM32   ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32_capi_gpio_irq.c)
capi_stm32_sources_ifdef(CONFIG_SPI_STM32        ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32_capi_spi.c
                                                 ${STM32_HAL_PORT_DIR}/stm32_hal_port_spi.c)
capi_stm32_sources_ifdef(CONFIG_I2C_STM32        ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32_capi_i2c.c
                                                 ${STM32_HAL_PORT_DIR}/stm32_hal_port_i2c.c)
capi_stm32_sources_ifdef(CONFIG_TIMER_STM32      ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32_capi_timer.c)
capi_stm32_sources_ifdef(CONFIG_PWM_STM32        ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32_capi_pwm.c)
capi_stm32_sources_ifdef(CONFIG_DMA_STM32        ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32_capi_dma.c)
capi_stm32_sources_ifdef(CONFIG_GPDMA_STM32      ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32_capi_gpdma.c)
capi_stm32_sources_ifdef(CONFIG_I3C_STM32        ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32_capi_i3c.c)
capi_stm32_sources_ifdef(CONFIG_XSPI_STM32       ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32_capi_xspi.c)
capi_stm32_sources_ifdef(CONFIG_USB_UART_STM32   ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32_capi_usb_uart.c)
capi_stm32_sources_ifdef(CONFIG_TDM_STM32        ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32_capi_tdm.c)

target_include_directories(capi_stm32 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/platform
    ${CAPI_DIR}/inc
)

if(DEFINED CUBEMX_INCLUDE_DIRS)
    target_include_directories(capi_stm32 PUBLIC ${CUBEMX_INCLUDE_DIRS})
endif()

if(TARGET stm32cubemx)
    target_link_libraries(capi_stm32 PUBLIC stm32cubemx)
endif()

# The CubeMX2 backend sets the include paths and device defines on its own
# target. The classic backend uses CUBEMX_INCLUDE_DIRS in board.cmake.
# CubeMX2 leaves main() to the application and generates no init function
# for it. The platform adds stm32_init().
if(TARGET stm32cubemx2)
    target_link_libraries(capi_stm32 PUBLIC stm32cubemx2)
    target_sources(capi_stm32 PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32_capi_init_cubemx2.c)
endif()

add_library(capi::stm32 ALIAS capi_stm32)
