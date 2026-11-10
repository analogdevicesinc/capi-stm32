/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   stm32_capi_xspi.h
 * @brief  Header for the STM32 CAPI XSPI (OctoSPI) backend.
 */

#ifndef STM32_CAPI_XSPI_H_
#define STM32_CAPI_XSPI_H_

#include "stm32_capi_hal.h"
#include "capi_spi.h"

/**
 * @struct stm32_xspi_extra_config
 * @brief STM32 specific XSPI (OctoSPI/HyperBus) configuration
 *
 * The handle is a void pointer because the HAL type name changes between
 * families: XSPI_HandleTypeDef on H5, OSPI_HandleTypeDef on older HALs.
 */
struct stm32_xspi_extra_config {
	/** HAL XSPI handle (XSPI_HandleTypeDef *). The backend casts it. */
	void *hxspi;
};

/**
 * @brief STM32 XSPI operations for the CAPI SPI interface
 */
extern const struct capi_spi_ops stm32_capi_xspi_ops;

#endif /* STM32_CAPI_XSPI_H_ */
