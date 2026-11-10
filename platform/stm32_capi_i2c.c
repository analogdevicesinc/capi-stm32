/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   stm32_capi_i2c.c
 * @brief  CAPI I2C backend for STM32.
 *
 * The backend uses the porting layer in stm32_hal_port.h.
 */

#include "stm32_capi_i2c.h"


#include "capi_alloc.h"
#include <errno.h>

/* Transfer timeout. A stuck bus (SDA or SCL held low) returns -ETIMEDOUT */
#define STM32_CAPI_I2C_TIMEOUT_MS	100

/* Private handle stored in capi_i2c_controller_handle->priv */
struct stm32_i2c_priv {
	stm32_hal_i2c_t *hi2c;
};

static int stm32_capi_i2c_init(struct capi_i2c_controller_handle **handle,
			       const struct capi_i2c_config *config)
{
	struct capi_i2c_controller_handle *i2c_handle;
	struct stm32_i2c_priv *priv;
	struct stm32_i2c_extra_config *extra;
	int ret;

	if (!handle || !config || !config->extra)
		return -EINVAL;

	extra = (struct stm32_i2c_extra_config *)config->extra;
	/* Use the handle, or call the getter that the board names */
	stm32_hal_i2c_t *hi2c = extra->hi2c;

	if (!hi2c && extra->hi2c_get)
		hi2c = extra->hi2c_get();

	if (!hi2c)
		return -EINVAL;

	if (*handle == NULL) {
		i2c_handle = (struct capi_i2c_controller_handle *)capi_calloc(1,
				sizeof(*i2c_handle));
		if (!i2c_handle)
			return -ENOMEM;

		priv = (struct stm32_i2c_priv *)capi_calloc(1, sizeof(*priv));
		if (!priv) {
			capi_free(i2c_handle);
			return -ENOMEM;
		}

		i2c_handle->priv = priv;
		i2c_handle->init_allocated = true;
	} else {
		i2c_handle = *handle;
		i2c_handle->init_allocated = false;

		if (!i2c_handle->priv)
			return -EINVAL;

		priv = i2c_handle->priv;
	}

	i2c_handle->ops = config->ops;
	priv->hi2c = hi2c;

	ret = stm32_hal_i2c_init(priv->hi2c);
	if (ret) {
		if (i2c_handle->init_allocated) {
			capi_free(priv);
			capi_free(i2c_handle);
		}
		return -EIO;
	}

	*handle = i2c_handle;

	return 0;
}

static int stm32_capi_i2c_deinit(struct capi_i2c_controller_handle *handle)
{
	struct stm32_i2c_priv *priv;

	if (!handle)
		return -EINVAL;

	priv = handle->priv;

	if (priv)
		stm32_hal_i2c_deinit(priv->hi2c);

	if (handle->init_allocated) {
		capi_free(priv);
		capi_free(handle);
	}

	return 0;
}

/**
 * @brief Build the sub-address value from the CAPI bytes (big-endian).
 * @param transfer - The transfer. A length other than 1 reads two bytes.
 * @return The sub-address.
 */
static uint16_t stm32_i2c_sub_address(const struct capi_i2c_transfer *transfer)
{
	if (transfer->sub_address_len == 1)
		return transfer->sub_address[0];

	return ((uint16_t)transfer->sub_address[0] << 8) |
	       transfer->sub_address[1];
}

/**
 * @brief Get the sub-address width for the porting layer: 1 or 2 bytes.
 * @param transfer - The transfer. A longer sub-address is cut to two bytes.
 * @return The width in bytes.
 */
static uint8_t stm32_i2c_sub_address_len(const struct capi_i2c_transfer *transfer)
{
	return (transfer->sub_address_len == 1) ? 1 : 2;
}

static int stm32_capi_i2c_transmit(struct capi_i2c_device *device,
				   struct capi_i2c_transfer *transfer)
{
	struct stm32_i2c_priv *priv;

	if (!device || !device->controller || !device->controller->priv || !transfer)
		return -EINVAL;

	if (!transfer->buf && transfer->len)
		return -EINVAL;

	priv = device->controller->priv;

	if (transfer->sub_address && transfer->sub_address_len > 0 && transfer->len == 0) {
		/* A write of a sub-address only has no data phase */
		/* A memory write of size 0 is unreliable on STM32 */
		/* (HAL_I2C_ERROR_WRONGSTART on H7) */
		/* Send the sub-address bytes as the data instead */
		return stm32_hal_i2c_write(priv->hi2c, transfer->target_addr,
					   transfer->sub_address,
					   transfer->sub_address_len,
					   STM32_CAPI_I2C_TIMEOUT_MS);
	}

	if (transfer->sub_address && transfer->sub_address_len > 0)
		return stm32_hal_i2c_mem_write(priv->hi2c,
					       transfer->target_addr,
					       stm32_i2c_sub_address(transfer),
					       stm32_i2c_sub_address_len(transfer),
					       transfer->buf, transfer->len,
					       STM32_CAPI_I2C_TIMEOUT_MS);

	return stm32_hal_i2c_write(priv->hi2c, transfer->target_addr,
				   transfer->buf, transfer->len,
				   STM32_CAPI_I2C_TIMEOUT_MS);
}

static int stm32_capi_i2c_receive(struct capi_i2c_device *device,
				  struct capi_i2c_transfer *transfer)
{
	struct stm32_i2c_priv *priv;

	if (!device || !device->controller || !device->controller->priv || !transfer)
		return -EINVAL;

	if (!transfer->buf && transfer->len)
		return -EINVAL;

	priv = device->controller->priv;

	if (transfer->sub_address && transfer->sub_address_len > 0)
		return stm32_hal_i2c_mem_read(priv->hi2c,
					      transfer->target_addr,
					      stm32_i2c_sub_address(transfer),
					      stm32_i2c_sub_address_len(transfer),
					      transfer->buf, transfer->len,
					      STM32_CAPI_I2C_TIMEOUT_MS);

	return stm32_hal_i2c_read(priv->hi2c, transfer->target_addr,
				  transfer->buf, transfer->len,
				  STM32_CAPI_I2C_TIMEOUT_MS);
}

const struct capi_i2c_ops stm32_capi_i2c_ops = {
	.init = stm32_capi_i2c_init,
	.deinit = stm32_capi_i2c_deinit,
	.transmit = stm32_capi_i2c_transmit,
	.receive = stm32_capi_i2c_receive,
	.register_callback = NULL,
	.configure_bus_speed = NULL,
	.transmit_async = NULL,
	.receive_async = NULL,
	.recover_bus = NULL,
	.register_target = NULL,
	.unregister_target = NULL,
	.isr = NULL,
};

