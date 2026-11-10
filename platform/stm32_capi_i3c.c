/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   stm32_capi_i3c.c
 * @brief  CAPI I3C backend for STM32. It wraps the HAL I3C controller calls.
 */

#include "stm32_capi_i3c.h"

#include "capi_alloc.h"
#include <errno.h>

/* Private handle stored in capi_i3c_controller_handle->priv */
struct stm32_i3c_priv {
	I3C_HandleTypeDef *hi3c;
};

static int stm32_capi_i3c_init(struct capi_i3c_controller_handle **handle,
				const struct capi_i3c_config *config)
{
	struct capi_i3c_controller_handle *i3c_handle;
	struct stm32_i3c_priv *priv;
	struct stm32_i3c_extra_config *extra;
	int ret;

	if (!handle || !config || !config->extra)
		return -EINVAL;

	extra = (struct stm32_i3c_extra_config *)config->extra;
	if (!extra->hi3c)
		return -EINVAL;

	if (*handle == NULL) {
		i3c_handle = (struct capi_i3c_controller_handle *)capi_calloc(1,
				sizeof(*i3c_handle));
		if (!i3c_handle)
			return -ENOMEM;

		priv = (struct stm32_i3c_priv *)capi_calloc(1, sizeof(*priv));
		if (!priv) {
			capi_free(i3c_handle);
			return -ENOMEM;
		}

		i3c_handle->priv = priv;
		i3c_handle->init_allocated = true;
	} else {
		i3c_handle = *handle;
		i3c_handle->init_allocated = false;

		if (!i3c_handle->priv)
			return -EINVAL;

		priv = i3c_handle->priv;
	}

	i3c_handle->ops = config->ops;
	priv->hi3c = extra->hi3c;

	ret = HAL_I3C_Init(priv->hi3c);
	if (ret != HAL_OK) {
		if (i3c_handle->init_allocated) {
			capi_free(priv);
			capi_free(i3c_handle);
		}
		return -EIO;
	}

	*handle = i3c_handle;

	return 0;
}

static int stm32_capi_i3c_deinit(struct capi_i3c_controller_handle *handle)
{
	struct stm32_i3c_priv *priv;

	if (!handle)
		return -EINVAL;

	priv = handle->priv;

	if (priv)
		HAL_I3C_DeInit(priv->hi3c);

	if (handle->init_allocated) {
		capi_free(priv);
		capi_free(handle);
	}

	return 0;
}

static int stm32_capi_i3c_transmit(struct capi_i3c_device *device,
				    struct capi_i3c_transfer *transfer)
{
	struct stm32_i3c_priv *priv;
	I3C_XferTypeDef xfer;
	int ret;

	if (!device || !device->controller || !device->controller->priv || !transfer)
		return -EINVAL;

	if (!transfer->buf && transfer->len)
		return -EINVAL;

	priv = device->controller->priv;

	xfer.CtrlBuf.pBuffer = NULL;
	xfer.CtrlBuf.Size = 0;
	xfer.StatusBuf.pBuffer = NULL;
	xfer.StatusBuf.Size = 0;
	xfer.TxBuf.pBuffer = transfer->buf;
	xfer.TxBuf.Size = transfer->len;
	xfer.RxBuf.pBuffer = NULL;
	xfer.RxBuf.Size = 0;

	ret = HAL_I3C_Ctrl_Transmit(priv->hi3c, &xfer, HAL_MAX_DELAY);

	switch (ret) {
	case HAL_OK:
		return 0;
	case HAL_BUSY:
		return -EBUSY;
	case HAL_TIMEOUT:
		return -ETIMEDOUT;
	default:
		return -EIO;
	}
}

static int stm32_capi_i3c_receive(struct capi_i3c_device *device,
				   struct capi_i3c_transfer *transfer)
{
	struct stm32_i3c_priv *priv;
	I3C_XferTypeDef xfer;
	int ret;

	if (!device || !device->controller || !device->controller->priv || !transfer)
		return -EINVAL;

	if (!transfer->buf && transfer->len)
		return -EINVAL;

	priv = device->controller->priv;

	xfer.CtrlBuf.pBuffer = NULL;
	xfer.CtrlBuf.Size = 0;
	xfer.StatusBuf.pBuffer = NULL;
	xfer.StatusBuf.Size = 0;
	xfer.TxBuf.pBuffer = NULL;
	xfer.TxBuf.Size = 0;
	xfer.RxBuf.pBuffer = transfer->buf;
	xfer.RxBuf.Size = transfer->len;

	ret = HAL_I3C_Ctrl_Receive(priv->hi3c, &xfer, HAL_MAX_DELAY);

	switch (ret) {
	case HAL_OK:
		return 0;
	case HAL_BUSY:
		return -EBUSY;
	case HAL_TIMEOUT:
		return -ETIMEDOUT;
	default:
		return -EIO;
	}
}

const struct capi_i3c_ops stm32_capi_i3c_ops = {
	.init = stm32_capi_i3c_init,
	.deinit = stm32_capi_i3c_deinit,
	.transmit = stm32_capi_i3c_transmit,
	.receive = stm32_capi_i3c_receive,
	.transmit_async = NULL,
	.receive_async = NULL,
	.register_callback = NULL,
	.do_daa = NULL,
	.do_daa_async = NULL,
	.add_static_device = NULL,
	.add_dynamic_device = NULL,
	.get_num_devices = NULL,
	.get_device_info = NULL,
	.register_target = NULL,
	.ibi_enable = NULL,
	.ibi_disable = NULL,
	.target_raise_ibi = NULL,
	.ibi_get_payload = NULL,
	.send_ccc = NULL,
	.recv_ccc = NULL,
	.hot_join_enable = NULL,
	.hot_join_disable = NULL,
	.transfer_batch = NULL,
	.isr = NULL,
};
