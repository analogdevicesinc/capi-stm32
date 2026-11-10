/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   stm32_capi_i2c.h
 * @brief  Header for the STM32 CAPI I2C backend.
 */

#ifndef STM32_CAPI_I2C_H_
#define STM32_CAPI_I2C_H_

#include "stm32_hal_port.h"
#include "capi_i2c.h"

/**
 * @struct stm32_i2c_extra_config
 * @brief STM32 specific I2C controller configuration
 */
struct stm32_i2c_extra_config {
	/** I2C handle of the active HAL. Classic CubeMX makes it global. */
	stm32_hal_i2c_t *hi2c;
	/**
	 * Getter for the handle, for generators that keep it private.
	 * STM32CubeMX2 keeps the handle static in its mx_<peripheral>.c file.
	 * The getter returns the handle only after the generated init runs.
	 * Set @ref hi2c or this member. @ref hi2c has priority.
	 */
	stm32_hal_i2c_t *(*hi2c_get)(void);
};

/**
 * @brief STM32 I2C operations for CAPI
 */
extern const struct capi_i2c_ops stm32_capi_i2c_ops;

#endif /* STM32_CAPI_I2C_H_ */
