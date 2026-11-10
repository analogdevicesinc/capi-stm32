/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   stm32_capi_gpio_irq.h
 * @brief  Header for the STM32 GPIO interrupt (EXTI) helpers.
 */

#ifndef STM32_CAPI_GPIO_IRQ_H_
#define STM32_CAPI_GPIO_IRQ_H_

#include "stm32_capi_hal.h"
#include "stm32_capi_irq.h"

/**
 * @brief Enable the EXTI interrupt of a GPIO pin.
 * @param pin - GPIO pin number (0-15).
 * @param trigger - Trigger mode. The function does not use it. Set the edge in
 *                  the EXTI registers, or with capi_irq_set_level_edge_trigger.
 * @return 0 in case of success, negative error code otherwise.
 */
int stm32_capi_gpio_irq_enable(uint16_t pin, uint32_t trigger);

/**
 * @brief Disable the EXTI interrupt of a GPIO pin.
 * @param pin - GPIO pin number (0-15).
 * @return 0 in case of success, negative error code otherwise.
 */
int stm32_capi_gpio_irq_disable(uint16_t pin);

#endif /* STM32_CAPI_GPIO_IRQ_H_ */
