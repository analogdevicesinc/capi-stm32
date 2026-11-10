/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   stm32_capi_i3c.h
 * @brief  Header for the STM32 CAPI I3C backend.
 */

#ifndef STM32_CAPI_I3C_H_
#define STM32_CAPI_I3C_H_

#include "stm32_capi_hal.h"
#include "capi_i3c.h"

/**
 * @struct stm32_i3c_extra_config
 * @brief STM32 specific I3C controller configuration
 */
struct stm32_i3c_extra_config {
	/** HAL I3C handle */
	I3C_HandleTypeDef *hi3c;
};

/**
 * @brief STM32 I3C operations for CAPI
 */
extern const struct capi_i3c_ops stm32_capi_i3c_ops;

#endif /* STM32_CAPI_I3C_H_ */
