/*
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   parameters.h
 * @brief  Board parameters of the EVAL-ADIN1110EBZ: peripheral names and pins.
 *
 * The board has an STM32L4S5QII and an ADIN1110 10BASE-T1L MAC-PHY on SPI2.
 * The pins of the ADIN1110 are the ones of no-OS projects/adin1110.
 */

#ifndef BOARD_PARAMETERS_H
#define BOARD_PARAMETERS_H

#include "stm32_capi_hal.h"
#include "stm32_capi_uart.h"
#include "stm32_capi_gpio.h"
#include "stm32_capi_spi.h"
#include "capi_uart.h"
#include "capi_gpio.h"
#include "capi_spi.h"

/* Console UART: USART1 (PA9/PA10), the FT232R USB UART of the board */
#define CONSOLE_UART_INSTANCE  1
#define CONSOLE_UART_IRQN      USART1_IRQn
#define CONSOLE_UART_BAUDRATE  115200

/* User LED uC1 (PC13) */
#define LED_GPIO_PORT          GPIOC
#define LED_GPIO_PIN           13

/* SPI2 to the ADIN1110: SCLK=PB13, MISO=PB14, MOSI=PB15 */
#define BSP_SPI_INSTANCE       2
/*
 * SCLK of SPI2. The backend keeps the rate of the .ioc: PCLK1 96 MHz with
 * prescaler 8. Change both together.
 */
#ifndef ADIN1110_SPI_HZ
#define ADIN1110_SPI_HZ        12000000U
#endif

/* ADIN1110 control pins */
#define ADIN1110_CS_PORT       GPIOB   /* T1L_CS_N, software chip select */
#define ADIN1110_CS_PIN        12
#define ADIN1110_RESET_PORT    GPIOC   /* T1L_RESET_N, active low */
#define ADIN1110_RESET_PIN     7
#define ADIN1110_INT_PORT      GPIOB   /* T1L_INT_N, active low, EXTI11 */
#define ADIN1110_INT_PIN       11
#define ADIN1110_CFG_PORT      GPIOC   /* T1L_CFG0/1: the SPI_CFG0/1 straps, inputs */
#define ADIN1110_CFG0_PIN      8
#define ADIN1110_CFG1_PIN      9

/* Board peripheral configs (defined in parameters.c) */
extern struct stm32_uart_extra_config bsp_uart_extra;
extern struct capi_uart_line_config bsp_uart_line_cfg;
extern struct capi_uart_config bsp_uart_cfg;

extern struct capi_gpio_port_config bsp_led_gpio_cfg;
extern struct capi_gpio_pin bsp_led;

extern struct stm32_spi_extra_config bsp_spi_extra;
extern struct capi_spi_config bsp_spi_cfg;

#endif /* BOARD_PARAMETERS_H */
