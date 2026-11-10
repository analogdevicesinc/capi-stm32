/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   stm32_capi_timer.h
 * @brief  Header for the STM32 CAPI timer backend.
 */

#ifndef STM32_CAPI_TIMER_H_
#define STM32_CAPI_TIMER_H_

#include "stm32_capi_hal.h"
#include "capi_timer.h"

/**
 * @struct stm32_timer_extra_config
 * @brief STM32 specific timer configuration
 */
struct stm32_timer_extra_config {
	/** HAL timer handle */
	TIM_HandleTypeDef *htim;
};

/**
 * @brief STM32 timer operations for CAPI. They include the PWM channels.
 */
extern const struct capi_timer_ops stm32_capi_timer_ops;

#endif /* STM32_CAPI_TIMER_H_ */
