/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   stm32_capi_pwm.h
 * @brief  Header for PWM on STM32. The timer backend provides PWM.
 */

#ifndef STM32_CAPI_PWM_H_
#define STM32_CAPI_PWM_H_

#include "stm32_capi_timer.h"

/* The timer backend (stm32_capi_timer_ops) provides PWM */
/* Use capi_timer_channel_enable and _disable to start or stop */
/* Use capi_timer_channel_compare_set to set the duty cycle */
/* This header exists for the PWM Kconfig symbol only */

#endif /* STM32_CAPI_PWM_H_ */
