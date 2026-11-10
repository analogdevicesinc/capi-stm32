/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   stm32_capi_gpio.c
 * @brief  CAPI GPIO backend for STM32.
 *
 * The backend uses the porting layer in stm32_hal_port.h.
 */

#include "stm32_capi_gpio.h"
#include "capi_alloc.h"
#include <errno.h>

/* Private handle stored in capi_gpio_port_handle->priv */
struct stm32_gpio_priv {
	stm32_hal_gpio_port_t port;
};

static int stm32_capi_gpio_port_init(struct capi_gpio_port_handle **handle,
				     const struct capi_gpio_port_config *config)
{
	struct capi_gpio_port_handle *gpio_handle;
	struct stm32_gpio_priv *priv;
	struct stm32_gpio_extra_config *extra;
	int ret;

	if (!handle || !config || !config->extra)
		return -EINVAL;

	extra = (struct stm32_gpio_extra_config *)config->extra;
	if (!extra->port)
		return -EINVAL;

	ret = stm32_hal_gpio_clk_enable(extra->port);
	if (ret)
		return ret;

	if (*handle == NULL) {
		gpio_handle = (struct capi_gpio_port_handle *)capi_calloc(1,
				sizeof(*gpio_handle));
		if (!gpio_handle)
			return -ENOMEM;

		priv = (struct stm32_gpio_priv *)capi_calloc(1, sizeof(*priv));
		if (!priv) {
			capi_free(gpio_handle);
			return -ENOMEM;
		}

		gpio_handle->priv = priv;
		gpio_handle->init_allocated = true;
	} else {
		gpio_handle = *handle;
		gpio_handle->init_allocated = false;

		if (!gpio_handle->priv)
			return -EINVAL;

		priv = gpio_handle->priv;
	}

	gpio_handle->ops = config->ops;
	priv->port = extra->port;

	*handle = gpio_handle;

	return 0;
}

static int stm32_capi_gpio_port_deinit(struct capi_gpio_port_handle **handle)
{
	struct capi_gpio_port_handle *gpio_handle;

	if (!handle || !*handle)
		return -EINVAL;

	gpio_handle = *handle;

	if (gpio_handle->init_allocated) {
		capi_free(gpio_handle->priv);
		capi_free(gpio_handle);
		*handle = NULL;
	}

	return 0;
}

static int stm32_capi_gpio_port_set_direction(struct capi_gpio_port_handle *handle,
					      uint64_t direction_bitmask)
{
	struct stm32_gpio_priv *priv;
	unsigned int i;
	int ret;

	if (!handle || !handle->priv)
		return -EINVAL;

	priv = handle->priv;

	/* Apply the pins one by one, in ascending order, to keep the old port state */
	for (i = 0; i < 16; i++) {
		enum stm32_hal_gpio_mode mode;

		mode = (direction_bitmask & (1ULL << i)) ?
		       STM32_HAL_GPIO_INPUT : STM32_HAL_GPIO_OUTPUT_PP;

		ret = stm32_hal_gpio_set_mode(priv->port, (uint16_t)(1U << i),
					      mode);
		if (ret)
			return ret;
	}

	return 0;
}

static int stm32_capi_gpio_port_get_direction(struct capi_gpio_port_handle *handle,
					      uint64_t *direction_bitmask)
{
	struct stm32_gpio_priv *priv;

	if (!handle || !handle->priv || !direction_bitmask)
		return -EINVAL;

	priv = handle->priv;

	/* Output, alternate and analog pins read as 0 */
	*direction_bitmask = stm32_hal_gpio_get_input_mask(priv->port);

	return 0;
}

static int stm32_capi_gpio_port_set_value(struct capi_gpio_port_handle *handle,
					  uint64_t value_bitmask)
{
	struct stm32_gpio_priv *priv;

	if (!handle || !handle->priv)
		return -EINVAL;

	priv = handle->priv;
	stm32_hal_gpio_write_port(priv->port, (uint16_t)value_bitmask);

	return 0;
}

static int stm32_capi_gpio_port_get_value(struct capi_gpio_port_handle *handle,
					  uint64_t *value_bitmask)
{
	struct stm32_gpio_priv *priv;

	if (!handle || !handle->priv || !value_bitmask)
		return -EINVAL;

	priv = handle->priv;
	*value_bitmask = (uint64_t)stm32_hal_gpio_read_port(priv->port);

	return 0;
}

static int stm32_capi_gpio_port_set_raw_value(struct capi_gpio_port_handle *handle,
					      uint64_t value_bitmask)
{
	return stm32_capi_gpio_port_set_value(handle, value_bitmask);
}

static int stm32_capi_gpio_port_get_raw_value(struct capi_gpio_port_handle *handle,
					      uint64_t *value_bitmask)
{
	return stm32_capi_gpio_port_get_value(handle, value_bitmask);
}

static int stm32_capi_gpio_port_toggle(struct capi_gpio_port_handle *handle,
					uint64_t pins_bitmask)
{
	struct stm32_gpio_priv *priv;

	if (!handle || !handle->priv)
		return -EINVAL;

	priv = handle->priv;
	stm32_hal_gpio_toggle_pins(priv->port, (uint16_t)pins_bitmask);

	return 0;
}

static int stm32_capi_gpio_pin_set_direction(struct capi_gpio_pin *pin,
					     uint8_t direction)
{
	struct stm32_gpio_priv *priv;
	enum stm32_hal_gpio_mode mode;

	if (!pin || !pin->port_handle || !pin->port_handle->priv)
		return -EINVAL;

	priv = pin->port_handle->priv;

	mode = (direction == CAPI_GPIO_INPUT) ?
	       STM32_HAL_GPIO_INPUT : STM32_HAL_GPIO_OUTPUT_PP;

	return stm32_hal_gpio_set_mode(priv->port,
				       (uint16_t)(1U << pin->number), mode);
}

static int stm32_capi_gpio_pin_get_direction(struct capi_gpio_pin *pin,
					     uint8_t *direction)
{
	struct stm32_gpio_priv *priv;
	uint16_t inputs;

	if (!pin || !pin->port_handle || !pin->port_handle->priv || !direction)
		return -EINVAL;

	priv = pin->port_handle->priv;
	inputs = stm32_hal_gpio_get_input_mask(priv->port);

	if (inputs & (uint16_t)(1U << pin->number))
		*direction = CAPI_GPIO_INPUT;
	else
		*direction = CAPI_GPIO_OUTPUT;

	return 0;
}

static int stm32_capi_gpio_pin_set_value(struct capi_gpio_pin *pin,
					 uint8_t value)
{
	struct stm32_gpio_priv *priv;

	if (!pin || !pin->port_handle || !pin->port_handle->priv)
		return -EINVAL;

	priv = pin->port_handle->priv;

	stm32_hal_gpio_write_pins(priv->port, (uint16_t)(1U << pin->number),
				  value == CAPI_GPIO_HIGH);

	return 0;
}

static int stm32_capi_gpio_pin_get_value(struct capi_gpio_pin *pin,
					 uint8_t *value)
{
	struct stm32_gpio_priv *priv;

	if (!pin || !pin->port_handle || !pin->port_handle->priv || !value)
		return -EINVAL;

	priv = pin->port_handle->priv;

	if (stm32_hal_gpio_read_pins(priv->port, (uint16_t)(1U << pin->number)))
		*value = CAPI_GPIO_HIGH;
	else
		*value = CAPI_GPIO_LOW;

	return 0;
}

static int stm32_capi_gpio_pin_set_raw_value(struct capi_gpio_pin *pin,
					     uint8_t value)
{
	return stm32_capi_gpio_pin_set_value(pin, value);
}

static int stm32_capi_gpio_pin_get_raw_value(struct capi_gpio_pin *pin,
					     uint8_t *value)
{
	return stm32_capi_gpio_pin_get_value(pin, value);
}

static int stm32_capi_gpio_pin_toggle(struct capi_gpio_pin *pin)
{
	struct stm32_gpio_priv *priv;

	if (!pin || !pin->port_handle || !pin->port_handle->priv)
		return -EINVAL;

	priv = pin->port_handle->priv;

	stm32_hal_gpio_toggle_pins(priv->port, (uint16_t)(1U << pin->number));

	return 0;
}

const struct capi_gpio_ops stm32_capi_gpio_ops = {
	.port_init = stm32_capi_gpio_port_init,
	.port_deinit = stm32_capi_gpio_port_deinit,
	.port_set_direction = stm32_capi_gpio_port_set_direction,
	.port_get_direction = stm32_capi_gpio_port_get_direction,
	.port_set_value = stm32_capi_gpio_port_set_value,
	.port_get_value = stm32_capi_gpio_port_get_value,
	.port_set_raw_value = stm32_capi_gpio_port_set_raw_value,
	.port_get_raw_value = stm32_capi_gpio_port_get_raw_value,
	.port_toggle = stm32_capi_gpio_port_toggle,
	.pin_set_direction = stm32_capi_gpio_pin_set_direction,
	.pin_get_direction = stm32_capi_gpio_pin_get_direction,
	.pin_set_value = stm32_capi_gpio_pin_set_value,
	.pin_get_value = stm32_capi_gpio_pin_get_value,
	.pin_set_raw_value = stm32_capi_gpio_pin_set_raw_value,
	.pin_get_raw_value = stm32_capi_gpio_pin_get_raw_value,
	.pin_toggle = stm32_capi_gpio_pin_toggle,
};
