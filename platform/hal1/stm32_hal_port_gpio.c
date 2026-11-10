/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   hal1/stm32_hal_port_gpio.c
 * @brief  GPIO porting layer for the classic HAL.
 *
 * See platform/stm32_hal_port.h for the purpose of this layer.
 */

#include "stm32_hal_port.h"

#include <errno.h>

int stm32_hal_gpio_clk_enable(stm32_hal_gpio_port_t port)
{
	/* Each port has its own clock macro, so a table does not work */
	/* The DFP defines GPIOx only for the ports that the package has */
	if (port == GPIOA)
		__HAL_RCC_GPIOA_CLK_ENABLE();
	else if (port == GPIOB)
		__HAL_RCC_GPIOB_CLK_ENABLE();
	else if (port == GPIOC)
		__HAL_RCC_GPIOC_CLK_ENABLE();
#ifdef GPIOD
	else if (port == GPIOD)
		__HAL_RCC_GPIOD_CLK_ENABLE();
#endif
#ifdef GPIOE
	else if (port == GPIOE)
		__HAL_RCC_GPIOE_CLK_ENABLE();
#endif
#ifdef GPIOF
	else if (port == GPIOF)
		__HAL_RCC_GPIOF_CLK_ENABLE();
#endif
#ifdef GPIOG
	else if (port == GPIOG)
		__HAL_RCC_GPIOG_CLK_ENABLE();
#endif
#ifdef GPIOH
	else if (port == GPIOH)
		__HAL_RCC_GPIOH_CLK_ENABLE();
#endif
#ifdef GPIOI
	else if (port == GPIOI)
		__HAL_RCC_GPIOI_CLK_ENABLE();
#endif
	else
		return -EINVAL;

	return 0;
}

int stm32_hal_gpio_set_mode(stm32_hal_gpio_port_t port, uint16_t pins,
			    enum stm32_hal_gpio_mode mode)
{
	GPIO_InitTypeDef gpio_init;

	if (!port)
		return -EINVAL;

	gpio_init.Pin = pins;
	gpio_init.Pull = GPIO_NOPULL;
	gpio_init.Speed = GPIO_SPEED_FREQ_LOW;

	switch (mode) {
	case STM32_HAL_GPIO_INPUT:
		gpio_init.Mode = GPIO_MODE_INPUT;
		break;
	case STM32_HAL_GPIO_OUTPUT_PP:
		gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
		break;
	default:
		return -EINVAL;
	}

	HAL_GPIO_Init(port, &gpio_init);

	return 0;
}

uint16_t stm32_hal_gpio_get_input_mask(stm32_hal_gpio_port_t port)
{
	uint32_t moder;
	uint16_t result = 0;
	unsigned int i;

	if (!port)
		return 0;

	moder = port->MODER;

	for (i = 0; i < 16; i++) {
		/* MODER: 00 input, 01 output, 10 alternate, 11 analog */
		if (((moder >> (i * 2)) & 0x3) == 0x00)
			result |= (uint16_t)(1U << i);
	}

	return result;
}

void stm32_hal_gpio_write_pins(stm32_hal_gpio_port_t port, uint16_t pins,
			       bool set)
{
	if (!port)
		return;

	HAL_GPIO_WritePin(port, pins, set ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

bool stm32_hal_gpio_read_pins(stm32_hal_gpio_port_t port, uint16_t pins)
{
	if (!port)
		return false;

	return HAL_GPIO_ReadPin(port, pins) == GPIO_PIN_SET;
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

	port->ODR = value;
}

uint16_t stm32_hal_gpio_read_port(stm32_hal_gpio_port_t port)
{
	if (!port)
		return 0;

	return (uint16_t)port->IDR;
}
