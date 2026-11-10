/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   stm32_capi_uart.h
 * @brief  Header for the STM32 CAPI UART backend.
 */

#ifndef STM32_CAPI_UART_H_
#define STM32_CAPI_UART_H_

#include "stm32_hal_port.h"
#include "capi_uart.h"

/**
 * @struct stm32_uart_extra_config
 * @brief STM32 specific UART configuration
 */
struct stm32_uart_extra_config {
	/** UART handle of the active HAL. Classic CubeMX makes it global. */
	stm32_hal_uart_t *huart;
	/**
	 * Getter for the handle, for generators that keep it private.
	 * STM32CubeMX2 keeps the handle static in its mx_<peripheral>.c file.
	 * The getter returns the handle only after the generated init runs.
	 * Set @ref huart or this member. @ref huart has priority.
	 */
	stm32_hal_uart_t *(*huart_get)(void);
};

/**
 * @brief STM32 UART operations for CAPI
 */
extern const struct capi_uart_ops stm32_capi_uart_ops;

/**
 * @brief Send stdout and read stdin through the STM32 UART.
 * @param handle - Pointer to an initialized UART handle.
 * @return 0 in case of success, negative error code otherwise.
 */
int stm32_uart_stdio_enable(struct capi_uart_handle *handle);

#endif /* STM32_CAPI_UART_H_ */
