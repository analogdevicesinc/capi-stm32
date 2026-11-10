/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   stm32_capi_spi.h
 * @brief  Header for the STM32 CAPI SPI backend.
 */

#ifndef STM32_CAPI_SPI_H_
#define STM32_CAPI_SPI_H_

#include "stm32_hal_port.h"
#include "capi_spi.h"

/**
 * @struct stm32_spi_extra_config
 * @brief STM32 specific SPI controller configuration
 */
struct stm32_spi_extra_config {
	/** SPI handle of the active HAL. Classic CubeMX makes it global. */
	stm32_hal_spi_t *hspi;
	/**
	 * Getter for the handle, for generators that keep it private.
	 * STM32CubeMX2 keeps the handle static in its mx_<peripheral>.c file.
	 * The getter returns the handle only after the generated init runs.
	 * Set @ref hspi or this member. @ref hspi has priority.
	 */
	stm32_hal_spi_t *(*hspi_get)(void);
};

/**
 * @brief STM32 SPI operations for CAPI
 */
extern const struct capi_spi_ops stm32_capi_spi_ops;

#endif /* STM32_CAPI_SPI_H_ */
