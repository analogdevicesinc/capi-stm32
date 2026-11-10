/*
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   parameters.h
 * @brief  Board parameters of the NUCLEO-C5A3ZG: peripheral names and pins.
 */

#ifndef BOARD_PARAMETERS_H
#define BOARD_PARAMETERS_H

#include "stm32_capi_hal.h"
#include "stm32_capi_uart.h"
#include "stm32_capi_gpio.h"
#include "stm32_capi_spi.h"
#include "stm32_capi_i2c.h"
#include "capi_uart.h"
#include "capi_gpio.h"
#include "capi_spi.h"
#include "capi_i2c.h"

/* Console UART: USART2, the ST-LINK virtual COM port (TX=PA2, RX=PA3) */
#define CONSOLE_UART_INSTANCE  2
#define CONSOLE_UART_BAUDRATE  115200

/* User LED LD2 (red, PG1). LD1 (green, PA5) shares Arduino D13 with SPI1 SCK */
#define LED_GPIO_PORT          STM32_HAL_GPIO_PORT(G)
#define LED_GPIO_PIN           1

/* SPI1 on the Arduino header: SCK=PA5/D13, MISO=PA6/D12, MOSI=PA7/D11 */
#define BSP_SPI_INSTANCE       1

/* I2C1 on the Arduino header: SCL=PB6/D15, SDA=PB7/D14 */
#define BSP_I2C_INSTANCE       1

/* Board peripheral configs (defined in parameters.c) */
extern struct stm32_uart_extra_config bsp_uart_extra;
extern struct capi_uart_line_config bsp_uart_line_cfg;
extern struct capi_uart_config bsp_uart_cfg;

extern struct capi_gpio_port_config bsp_led_gpio_cfg;
extern struct capi_gpio_pin bsp_led;

extern struct stm32_spi_extra_config bsp_spi_extra;
extern struct capi_spi_config bsp_spi_cfg;

extern struct stm32_i2c_extra_config bsp_i2c_extra;
extern struct capi_i2c_config bsp_i2c_cfg;

#endif /* BOARD_PARAMETERS_H */
