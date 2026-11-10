/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   stm32_capi_xspi.c
 * @brief  CAPI SPI backend for the STM32 XSPI (OctoSPI) peripheral.
 *
 * The backend sends single-line commands.
 */

#include "stm32_capi_xspi.h"

#include "capi_alloc.h"
#include <errno.h>
#include <string.h>

/* Private handle stored in capi_spi_controller_handle->priv */
struct stm32_xspi_priv {
	XSPI_HandleTypeDef *hxspi;
};

static int stm32_capi_xspi_init(struct capi_spi_controller_handle **handle,
				 const struct capi_spi_config *config)
{
	struct capi_spi_controller_handle *spi_handle;
	struct stm32_xspi_priv *priv;
	struct stm32_xspi_extra_config *extra;

	if (!handle || !config || !config->extra)
		return -EINVAL;

	extra = (struct stm32_xspi_extra_config *)config->extra;
	if (!extra->hxspi)
		return -EINVAL;

	if (*handle == NULL) {
		spi_handle = (struct capi_spi_controller_handle *)capi_calloc(1,
				sizeof(*spi_handle));
		if (!spi_handle)
			return -ENOMEM;

		priv = (struct stm32_xspi_priv *)capi_calloc(1, sizeof(*priv));
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
	priv->hxspi = (XSPI_HandleTypeDef *)extra->hxspi;

	/* The generated init code must initialize the XSPI peripheral */
	*handle = spi_handle;

	return 0;
}

static int stm32_capi_xspi_deinit(struct capi_spi_controller_handle *handle)
{
	struct stm32_xspi_priv *priv;

	if (!handle)
		return -EINVAL;

	priv = handle->priv;

	if (priv && priv->hxspi)
		HAL_XSPI_DeInit(priv->hxspi);

	if (handle->init_allocated) {
		capi_free(priv);
		capi_free(handle);
	}

	return 0;
}

static int stm32_capi_xspi_transceive(struct capi_spi_device *device,
				       struct capi_spi_transfer *transfer)
{
	struct stm32_xspi_priv *priv;
	XSPI_RegularCmdTypeDef cmd = {0};
	uint32_t timeout;
	int ret;

	if (!device || !device->controller || !device->controller->priv || !transfer)
		return -EINVAL;

	priv = device->controller->priv;
	timeout = (transfer->timeout > 0) ? (uint32_t)transfer->timeout : HAL_MAX_DELAY;

	/* Send a single-line command. The first tx_buf byte is the instruction. */
	/* The other tx_buf bytes are the data. */
	cmd.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
	cmd.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
	cmd.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
	cmd.AddressMode = HAL_XSPI_ADDRESS_NONE;
	cmd.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
	cmd.DummyCycles = 0;
	cmd.DataMode = HAL_XSPI_DATA_NONE;
	cmd.DQSMode = HAL_XSPI_DQS_DISABLE;

	if (transfer->tx_buf && transfer->tx_size > 0) {
		cmd.Instruction = transfer->tx_buf[0];

		if (transfer->tx_size > 1) {
			cmd.DataMode = HAL_XSPI_DATA_1_LINE;
			cmd.DataLength = transfer->tx_size - 1;
		}

		ret = HAL_XSPI_Command(priv->hxspi, &cmd, timeout);
		if (ret != HAL_OK)
			goto err;

		if (transfer->tx_size > 1) {
			ret = HAL_XSPI_Transmit(priv->hxspi,
						(uint8_t *)&transfer->tx_buf[1],
						timeout);
			if (ret != HAL_OK)
				goto err;
		}
	}

	if (transfer->rx_buf && transfer->rx_size > 0) {
		/* Send a new read command with the same instruction byte */
		if (!transfer->tx_buf || transfer->tx_size == 0)
			return -EINVAL;

		cmd.DataMode = HAL_XSPI_DATA_1_LINE;
		cmd.DataLength = transfer->rx_size;

		ret = HAL_XSPI_Command(priv->hxspi, &cmd, timeout);
		if (ret != HAL_OK)
			goto err;

		ret = HAL_XSPI_Receive(priv->hxspi, transfer->rx_buf, timeout);
		if (ret != HAL_OK)
			goto err;
	}

	return 0;

err:
	switch (ret) {
	case HAL_BUSY:
		return -EBUSY;
	case HAL_TIMEOUT:
		return -ETIMEDOUT;
	default:
		return -EIO;
	}
}

const struct capi_spi_ops stm32_capi_xspi_ops = {
	.init = stm32_capi_xspi_init,
	.deinit = stm32_capi_xspi_deinit,
	.transceive = stm32_capi_xspi_transceive,
	.transceive_async = NULL,
	.read_command = NULL,
	.read_command_async = NULL,
	.abort_async = NULL,
	.register_callback = NULL,
	.set_cs = NULL,
	.isr = NULL,
};
