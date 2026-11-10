/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   hal2/stm32_hal_port_gpio.c
 * @brief  GPIO porting layer for the new HAL.
 *
 * See platform/stm32_hal_port.h for the purpose of this layer.
 *
 * The new HAL handles single pins. It has no port-wide read or write, and it
 * cannot read the mode of a pin. These three functions use the LL driver.
 * LL needs a GPIO_TypeDef pointer. HAL_GPIO_GetLLInstance converts a hal_gpio_t
 * to this pointer.
 */

#include "stm32_hal_port.h"

#include <errno.h>

int stm32_hal_gpio_clk_enable(stm32_hal_gpio_port_t port)
{
	/* Each port has its own clock function, so a table does not work */
	/* A package without a port has no enumerator for it */
	switch (port) {
	case HAL_GPIOA:
		HAL_RCC_GPIOA_EnableClock();
		break;
	case HAL_GPIOB:
		HAL_RCC_GPIOB_EnableClock();
		break;
	case HAL_GPIOC:
		HAL_RCC_GPIOC_EnableClock();
		break;
	case HAL_GPIOD:
		HAL_RCC_GPIOD_EnableClock();
		break;
	case HAL_GPIOE:
		HAL_RCC_GPIOE_EnableClock();
		break;
	case HAL_GPIOF:
		HAL_RCC_GPIOF_EnableClock();
		break;
	case HAL_GPIOG:
		HAL_RCC_GPIOG_EnableClock();
		break;
	case HAL_GPIOH:
		HAL_RCC_GPIOH_EnableClock();
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int stm32_hal_gpio_set_mode(stm32_hal_gpio_port_t port, uint16_t pins,
			    enum stm32_hal_gpio_mode mode)
{
	hal_gpio_config_t config;
	GPIO_TypeDef *instance;
	uint32_t odr;

	if (!port)
		return -EINVAL;

	config.pull = HAL_GPIO_PULL_NO;
	config.speed = HAL_GPIO_SPEED_FREQ_LOW;
	config.output_type = HAL_GPIO_OUTPUT_PUSHPULL;
	config.alternate = HAL_GPIO_AF_0;
	config.init_state = HAL_GPIO_PIN_RESET;

	switch (mode) {
	case STM32_HAL_GPIO_INPUT:
		config.mode = HAL_GPIO_MODE_INPUT;
		break;
	case STM32_HAL_GPIO_OUTPUT_PP:
		config.mode = HAL_GPIO_MODE_OUTPUT;
		break;
	default:
		return -EINVAL;
	}

	instance = HAL_GPIO_GetLLInstance(port);

	/*
	 * HAL_GPIO_Init drives an output pin to init_state. The classic HAL does
	 * not. It keeps the last level in ODR. Save the levels and restore them.
	 */
	odr = LL_GPIO_ReadOutputPort(instance);

	if (HAL_GPIO_Init(port, pins, &config) != HAL_OK)
		return -EIO;

	if (mode == STM32_HAL_GPIO_OUTPUT_PP)
		LL_GPIO_WriteOutputPort(instance,
					(LL_GPIO_ReadOutputPort(instance) &
					 ~(uint32_t)pins) |
					(odr & (uint32_t)pins));

	return 0;
}

uint16_t stm32_hal_gpio_get_input_mask(stm32_hal_gpio_port_t port)
{
	const GPIO_TypeDef *instance;
	uint16_t result = 0;
	unsigned int i;

	if (!port)
		return 0;

	instance = HAL_GPIO_GetLLInstance(port);

	for (i = 0; i < 16; i++) {
		if (LL_GPIO_GetPinMode(instance, 1U << i) == LL_GPIO_MODE_INPUT)
			result |= (uint16_t)(1U << i);
	}

	return result;
}

void stm32_hal_gpio_write_pins(stm32_hal_gpio_port_t port, uint16_t pins,
			       bool set)
{
	if (!port)
		return;

	HAL_GPIO_WritePin(port, pins,
			  set ? HAL_GPIO_PIN_SET : HAL_GPIO_PIN_RESET);
}

bool stm32_hal_gpio_read_pins(stm32_hal_gpio_port_t port, uint16_t pins)
{
	if (!port)
		return false;

	/* HAL_GPIO_ReadPin takes one pin and asserts on a mask. Read the port. */
	return (LL_GPIO_ReadInputPort(HAL_GPIO_GetLLInstance(port)) &
		(uint32_t)pins) != 0U;
}

void stm32_hal_gpio_toggle_pins(stm32_hal_gpio_port_t port, uint16_t pins)
{
	if (!port)
		return;

	HAL_GPIO_TogglePin(port, pins);
}

void stm32_hal_gpio_write_port(stm32_hal_gpio_port_t port, uint16_t value)
{
	if (!port)
		return;

	LL_GPIO_WriteOutputPort(HAL_GPIO_GetLLInstance(port), value);
}

uint16_t stm32_hal_gpio_read_port(stm32_hal_gpio_port_t port)
{
	if (!port)
		return 0;

	return (uint16_t)LL_GPIO_ReadInputPort(HAL_GPIO_GetLLInstance(port));
}
