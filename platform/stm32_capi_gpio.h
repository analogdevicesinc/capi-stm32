/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   stm32_capi_gpio.h
 * @brief  Header for the STM32 CAPI GPIO backend.
 */

#ifndef STM32_CAPI_GPIO_H_
#define STM32_CAPI_GPIO_H_

#include "stm32_hal_port.h"
#include "capi_gpio.h"

/**
 * @struct stm32_gpio_extra_config
 * @brief STM32 specific GPIO port configuration
 */
struct stm32_gpio_extra_config {
	/** GPIO port, in the form that the active HAL uses */
	stm32_hal_gpio_port_t port;
};

/**
 * @brief STM32 platform specific GPIO operations for CAPI
 */
extern const struct capi_gpio_ops stm32_capi_gpio_ops;

#endif /* STM32_CAPI_GPIO_H_ */
