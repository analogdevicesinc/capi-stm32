/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   stm32_capi_gpio_irq.c
 * @brief  Enables and disables the NVIC interrupt of an EXTI line.
 */

#include "stm32_capi_gpio_irq.h"
#include <errno.h>

/**
 * @brief Map a GPIO pin number (0-15) to its EXTI IRQn.
 * @param pin - GPIO pin number.
 * @return The NVIC IRQn for the pin's EXTI line.
 */
static IRQn_Type gpio_pin_to_irqn(uint16_t pin)
{
#if defined(STM32H5) || defined(STM32C5)
	/* H5 and C5 have one EXTI IRQ per pin (EXTI0_IRQn to EXTI15_IRQn) */
	return (IRQn_Type)(EXTI0_IRQn + pin);
#else
	switch (pin) {
	case 0:
		return EXTI0_IRQn;
	case 1:
		return EXTI1_IRQn;
	case 2:
		return EXTI2_IRQn;
	case 3:
		return EXTI3_IRQn;
	case 4:
		return EXTI4_IRQn;
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
		return EXTI9_5_IRQn;
	default:
		return EXTI15_10_IRQn;
	}
#endif
}

int stm32_capi_gpio_irq_enable(uint16_t pin, uint32_t trigger)
{
	IRQn_Type irqn;

	if (pin > 15)
		return -EINVAL;

	(void)trigger;

	irqn = gpio_pin_to_irqn(pin);
	/* Priority 0 has the same raw value in every priority grouping */
	NVIC_SetPriority(irqn, 0);
	NVIC_EnableIRQ(irqn);

	return 0;
}

int stm32_capi_gpio_irq_disable(uint16_t pin)
{
	IRQn_Type irqn;

	if (pin > 15)
		return -EINVAL;

	irqn = gpio_pin_to_irqn(pin);
	NVIC_DisableIRQ(irqn);

	return 0;
}
