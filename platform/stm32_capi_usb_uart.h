/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   stm32_capi_usb_uart.h
 * @brief  Header for the STM32 CAPI USB CDC UART backend (stub).
 */

#ifndef STM32_CAPI_USB_UART_H_
#define STM32_CAPI_USB_UART_H_

#include "stm32_capi_hal.h"
#include "capi_uart.h"

/**
 * @struct stm32_usb_uart_extra_config
 * @brief STM32 specific USB CDC UART configuration
 *
 * A USB CDC port is usually unique, so it needs no HAL handle.
 */
struct stm32_usb_uart_extra_config {
	/** Reserved for the CDC interface index */
	uint8_t cdc_index;
};

/**
 * @brief STM32 USB CDC UART operations for CAPI
 */
extern const struct capi_uart_ops stm32_capi_usb_uart_ops;

#endif /* STM32_CAPI_USB_UART_H_ */
