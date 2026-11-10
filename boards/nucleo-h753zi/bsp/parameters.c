/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   parameters.c
 * @brief  Board configurations of the NUCLEO-H753ZI for the CAPI interfaces.
 */

#include "parameters.h"

extern UART_HandleTypeDef huart3;

struct stm32_uart_extra_config bsp_uart_extra = {
	.huart = &huart3,
};

struct capi_uart_line_config bsp_uart_line_cfg = {
	.baudrate = CONSOLE_UART_BAUDRATE,
	.size = CAPI_UART_DATA_BITS_8,
	.parity = CAPI_UART_PARITY_NONE,
	.stop_bits = CAPI_UART_STOP_1_BIT,
	.flow_control = CAPI_UART_FLOW_CONTROL_NONE,
};

/* Weak, so an application can define its own bsp_uart_cfg */
__attribute__((weak)) struct capi_uart_config bsp_uart_cfg = {
	.identifier = CONSOLE_UART_INSTANCE,
	.line_config = &bsp_uart_line_cfg,
	.extra = &bsp_uart_extra,
	.ops = &stm32_capi_uart_ops,
};

static struct stm32_gpio_extra_config bsp_led_gpio_extra = {
	.port = LED_GPIO_PORT,
};

/*
 * Weak, so an application can define its own bsp_led_gpio_cfg and bsp_led.
 * It must also define its own extra config, because it replaces the struct.
 */
__attribute__((weak)) struct capi_gpio_port_config bsp_led_gpio_cfg = {
	.ops = &stm32_capi_gpio_ops,
	.identifier = (uint64_t)(uintptr_t)LED_GPIO_PORT,
	.num_pins = 16,
	.extra = &bsp_led_gpio_extra,
};

__attribute__((weak)) struct capi_gpio_pin bsp_led = {
	.port_handle = NULL,
	.number = LED_GPIO_PIN,
};

extern SPI_HandleTypeDef hspi1;

struct stm32_spi_extra_config bsp_spi_extra = {
	.hspi = &hspi1,
};

/* Weak, so an application can define its own bsp_spi_cfg */
__attribute__((weak)) struct capi_spi_config bsp_spi_cfg = {
	.ops = &stm32_capi_spi_ops,
	.identifier = BSP_SPI_INSTANCE,
	.extra = &bsp_spi_extra,
};

extern I2C_HandleTypeDef hi2c1;

struct stm32_i2c_extra_config bsp_i2c_extra = {
	.hi2c = &hi2c1,
};

/* Weak, so an application can define its own bsp_i2c_cfg */
__attribute__((weak)) struct capi_i2c_config bsp_i2c_cfg = {
	.ops = &stm32_capi_i2c_ops,
	.identifier = BSP_I2C_INSTANCE,
	.initiator = true,
	.extra = &bsp_i2c_extra,
};
