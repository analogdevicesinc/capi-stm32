/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   stm32_capi_tdm.h
 * @brief  Header for the STM32 CAPI TDM backend (CAPI I2S on SAI).
 */

#ifndef STM32_CAPI_TDM_H_
#define STM32_CAPI_TDM_H_

#include "stm32_capi_hal.h"
#include "capi_i2s.h"

/**
 * @struct stm32_tdm_extra_config
 * @brief STM32 specific TDM (SAI) configuration
 */
struct stm32_tdm_extra_config {
	/** HAL SAI handle */
	SAI_HandleTypeDef *hsai;
};

/**
 * @brief STM32 TDM (SAI) operations for the CAPI I2S interface
 */
extern const struct capi_i2s_ops stm32_capi_tdm_ops;

#endif /* STM32_CAPI_TDM_H_ */
