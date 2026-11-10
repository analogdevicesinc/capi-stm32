/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   stm32_capi_spi.c
 * @brief  CAPI SPI backend for STM32.
 *
 * The backend uses the porting layer in stm32_hal_port.h.
 */

#include "stm32_capi_spi.h"


#include "capi_gpio.h"
#include "capi_alloc.h"
#include <errno.h>

/*
 * Per-device clock format.
 * A controller holds one CPOL/CPHA pair. Devices on one bus can need
 * different pairs.
 * For example, an ADXL355 needs mode 0 and an AD74413R needs mode 1.
 * The backend applies the format of the addressed device before each transfer.
 * It caches the last format. A transfer to the same device then costs one
 * comparison.
 * On the classic HAL, a format change runs a full HAL_SPI_Init.
 * The capi-maxim backend does the same.
 */

/* Private handle stored in capi_spi_controller_handle->priv */
struct stm32_spi_priv {
	stm32_hal_spi_t *hspi;
	enum capi_spi_mode format_mode;
	bool format_valid;
};

/*
 * Load the clock format of the device if the peripheral does not have it.
 * Deassert CS and keep the bus idle first. The HALs cannot change the format
 * during a transaction.
 */
static int stm32_spi_select_format(struct stm32_spi_priv *priv,
				   const struct capi_spi_device *device)
{
	int ret;

	if (priv->format_valid && priv->format_mode == device->mode)
		return 0;

	ret = stm32_hal_spi_set_format(priv->hspi, device->mode,
				       device->max_speed_hz);
	if (ret) {
		/* The loaded format is unknown now */
		priv->format_valid = false;
		return ret;
	}

	priv->format_mode = device->mode;
	priv->format_valid = true;

	return 0;
}

static int stm32_capi_spi_init(struct capi_spi_controller_handle **handle,
			       const struct capi_spi_config *config)
{
	struct capi_spi_controller_handle *spi_handle;
	struct stm32_spi_priv *priv;
	struct stm32_spi_extra_config *extra;
	int ret;

	if (!handle || !config || !config->extra)
		return -EINVAL;

	extra = (struct stm32_spi_extra_config *)config->extra;
	/* Use the handle, or call the getter that the board names */
	stm32_hal_spi_t *hspi = extra->hspi;

	if (!hspi && extra->hspi_get)
		hspi = extra->hspi_get();

	if (!hspi)
		return -EINVAL;

	if (*handle == NULL) {
		spi_handle = (struct capi_spi_controller_handle *)capi_calloc(1,
				sizeof(*spi_handle));
		if (!spi_handle)
			return -ENOMEM;

		priv = (struct stm32_spi_priv *)capi_calloc(1, sizeof(*priv));
		if (!priv) {
			capi_free(spi_handle);
			return -ENOMEM;
		}

		spi_handle->priv = priv;
		spi_handle->init_allocated = true;
	} else {
		spi_handle = *handle;
		spi_handle->init_allocated = false;

		if (!spi_handle->priv)
			return -EINVAL;

		priv = spi_handle->priv;
	}

	spi_handle->ops = config->ops;
	priv->hspi = hspi;

	ret = stm32_hal_spi_init(priv->hspi);
	if (ret) {
		if (spi_handle->init_allocated) {
			capi_free(priv);
			capi_free(spi_handle);
		}
		return -EIO;
	}

	*handle = spi_handle;

	return 0;
}

static int stm32_capi_spi_deinit(struct capi_spi_controller_handle *handle)
{
	struct stm32_spi_priv *priv;

	if (!handle)
		return -EINVAL;

	priv = handle->priv;

	if (priv)
		stm32_hal_spi_deinit(priv->hspi);

	if (handle->init_allocated) {
		capi_free(priv);
		capi_free(handle);
	}

	return 0;
}

static int stm32_capi_spi_transceive(struct capi_spi_device *device,
				     struct capi_spi_transfer *transfer)
{
	struct stm32_spi_priv *priv;
	uint32_t timeout;
	uint16_t size;
	int ret;

	if (!device || !device->controller || !device->controller->priv || !transfer)
		return -EINVAL;

	priv = device->controller->priv;
	timeout = (transfer->timeout > 0) ? (uint32_t)transfer->timeout :
		  STM32_HAL_WAIT_FOREVER;

	ret = stm32_spi_select_format(priv, device);
	if (ret)
		return ret;

	if (transfer->tx_buf && transfer->rx_buf) {
		size = (transfer->tx_size > transfer->rx_size) ?
			transfer->tx_size : transfer->rx_size;
		return stm32_hal_spi_write_read(priv->hspi,
						(uint8_t *)transfer->tx_buf,
						transfer->rx_buf,
						size, timeout);
	}

	if (transfer->tx_buf)
		return stm32_hal_spi_write(priv->hspi,
					   (uint8_t *)transfer->tx_buf,
					   transfer->tx_size, timeout);

	if (transfer->rx_buf)
		return stm32_hal_spi_read(priv->hspi, transfer->rx_buf,
					  transfer->rx_size, timeout);

	return -EINVAL;
}

static int stm32_capi_spi_set_cs(struct capi_spi_device *device,
				 enum capi_spi_cs_control cs)
{
	if (!device)
		return -EINVAL;

	/* Only a GPIO chip select is possible */
	if (!device->cs_gpio)
		return 0;

	switch (cs) {
	case CAPI_SPI_CS_MANUAL_ASSERT:
		capi_gpio_pin_set_raw_value(device->cs_gpio, CAPI_GPIO_LOW);
		break;
	case CAPI_SPI_CS_MANUAL_DEASSERT:
		capi_gpio_pin_set_raw_value(device->cs_gpio, CAPI_GPIO_HIGH);
		break;
	case CAPI_SPI_CS_AUTO:
	default:
		break;
	}

	return 0;
}

const struct capi_spi_ops stm32_capi_spi_ops = {
	.init = stm32_capi_spi_init,
	.deinit = stm32_capi_spi_deinit,
	.transceive = stm32_capi_spi_transceive,
	.transceive_async = NULL,
	.read_command = NULL,
	.read_command_async = NULL,
	.abort_async = NULL,
	.register_callback = NULL,
	.set_cs = stm32_capi_spi_set_cs,
	.isr = NULL,
};

