/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   stm32_capi_pwm.c
 * @brief  PWM on STM32. The timer backend provides PWM.
 *
 * This file exists for the PWM Kconfig symbol only. The timer backend
 * stm32_capi_timer.c starts PWM with channel_enable (HAL_TIM_PWM_Start).
 * It sets the duty cycle with channel_compare_set.
 */
