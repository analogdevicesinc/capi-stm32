/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   stm32_capi_dma.c
 * @brief  CAPI DMA backend for the STM32 DMA controller.
 *
 * The backend calls the HAL DMA functions.
 */

#include "stm32_capi_dma.h"


#include "capi_alloc.h"
#include <errno.h>

/* Private handle stored in capi_dma_handle->priv */
struct stm32_dma_priv {
	uint32_t num_chans;
};

/* Private data stored in capi_dma_chan->extra */
struct stm32_dma_chan_priv {
	DMA_HandleTypeDef *hdma;
	capi_dma_xfer_complete_cb complete_cb;
	void *complete_ctx;
	capi_dma_error_cb error_cb;
	void *error_ctx;
};

static int stm32_capi_dma_init(struct capi_dma_handle **handle,
			       const struct capi_dma_config *config)
{
	struct capi_dma_handle *h;
	struct stm32_dma_priv *priv;

	if (!handle || !config)
		return -EINVAL;

	if (*handle == NULL) {
		h = (struct capi_dma_handle *)capi_calloc(1, sizeof(*h));
		if (!h)
			return -ENOMEM;
		h->init_allocated = true;
	} else {
		h = *handle;
		h->init_allocated = false;
	}

	priv = (struct stm32_dma_priv *)capi_calloc(1, sizeof(*priv));
	if (!priv) {
		if (h->init_allocated)
			capi_free(h);
		return -ENOMEM;
	}

	priv->num_chans = config->num_chans;
	h->priv = priv;
	h->ops = config->ops;
	*handle = h;

	return 0;
}

static int stm32_capi_dma_deinit(struct capi_dma_handle *handle)
{
	if (!handle)
		return -EINVAL;

	if (handle->priv)
		capi_free(handle->priv);

	if (handle->init_allocated)
		capi_free(handle);

	return 0;
}

static int stm32_capi_dma_init_chan(struct capi_dma_handle *handle,
				    struct capi_dma_chan **chan, uint32_t id)
{
	struct capi_dma_chan *ch;
	struct stm32_dma_chan_priv *priv;

	if (!handle || !chan)
		return -EINVAL;

	ch = (struct capi_dma_chan *)capi_calloc(1, sizeof(*ch));
	if (!ch)
		return -ENOMEM;

	priv = (struct stm32_dma_chan_priv *)capi_calloc(1, sizeof(*priv));
	if (!priv) {
		capi_free(ch);
		return -ENOMEM;
	}

	ch->handle = handle;
	ch->id = id;
	ch->extra = priv;
	*chan = ch;

	return 0;
}

static int stm32_capi_dma_deinit_chan(struct capi_dma_chan *chan)
{
	struct stm32_dma_chan_priv *priv;

	if (!chan)
		return -EINVAL;

	priv = chan->extra;
	if (priv) {
		if (priv->hdma)
			HAL_DMA_DeInit(priv->hdma);
		capi_free(priv);
	}

	capi_free(chan);

	return 0;
}

static int stm32_capi_dma_config_xfer(struct capi_dma_chan *chan,
				      struct capi_dma_transfer *xfer)
{
	struct stm32_dma_chan_priv *priv;
	struct stm32_dma_chan_extra *extra;

	if (!chan || !chan->extra || !xfer)
		return -EINVAL;

	priv = chan->extra;

	extra = (struct stm32_dma_chan_extra *)xfer->extra;
	if (!extra || !extra->hdma)
		return -EINVAL;

	priv->hdma = extra->hdma;
	chan->xfer = xfer;

	if (HAL_DMA_Init(priv->hdma) != HAL_OK)
		return -EIO;

	return 0;
}

static int stm32_capi_dma_xfer_start(struct capi_dma_chan *chan)
{
	struct stm32_dma_chan_priv *priv;
	struct capi_dma_transfer *xfer;

	if (!chan || !chan->extra || !chan->xfer)
		return -EINVAL;

	priv = chan->extra;
	xfer = chan->xfer;

	if (HAL_DMA_Start(priv->hdma, (uint32_t)xfer->src,
			  (uint32_t)xfer->dst, xfer->length) != HAL_OK)
		return -EIO;

	return 0;
}

static int stm32_capi_dma_xfer_abort(struct capi_dma_chan *chan)
{
	struct stm32_dma_chan_priv *priv;

	if (!chan || !chan->extra)
		return -EINVAL;

	priv = chan->extra;

	if (HAL_DMA_Abort(priv->hdma) != HAL_OK)
		return -EIO;

	return 0;
}

static bool stm32_capi_dma_chan_is_completed(const struct capi_dma_chan *chan)
{
	struct stm32_dma_chan_priv *priv;

	if (!chan || !chan->extra)
		return true;

	priv = (struct stm32_dma_chan_priv *)chan->extra;

	return (HAL_DMA_GetState(priv->hdma) == HAL_DMA_STATE_READY);
}

const struct capi_dma_ops stm32_capi_dma_ops = {
	.init = stm32_capi_dma_init,
	.deinit = stm32_capi_dma_deinit,
	.init_chan = stm32_capi_dma_init_chan,
	.deinit_chan = stm32_capi_dma_deinit_chan,
	.config_xfer = stm32_capi_dma_config_xfer,
	.xfer_start = stm32_capi_dma_xfer_start,
	.xfer_abort = stm32_capi_dma_xfer_abort,
	.chan_is_completed = stm32_capi_dma_chan_is_completed,
};

