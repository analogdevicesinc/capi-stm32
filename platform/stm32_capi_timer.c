/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   stm32_capi_timer.c
 * @brief  CAPI timer backend for STM32. It provides the PWM channels.
 */

#include "stm32_capi_timer.h"


#include "capi_alloc.h"
#include <errno.h>

/* Private handle stored in capi_timer_handle->priv */
struct stm32_timer_priv {
	TIM_HandleTypeDef *htim;
};

static int stm32_capi_timer_init(struct capi_timer_handle **handle,
				 const struct capi_timer_config *config)
{
	struct capi_timer_handle *h;
	struct stm32_timer_priv *priv;
	struct stm32_timer_extra_config *extra;

	if (!handle || !config || !config->extra)
		return -EINVAL;

	extra = (struct stm32_timer_extra_config *)config->extra;

	if (*handle == NULL) {
		h = (struct capi_timer_handle *)capi_calloc(1, sizeof(*h));
		if (!h)
			return -ENOMEM;
		h->init_allocated = true;
	} else {
		h = *handle;
		h->init_allocated = false;
	}

	priv = (struct stm32_timer_priv *)capi_calloc(1, sizeof(*priv));
	if (!priv) {
		if (h->init_allocated)
			capi_free(h);
		return -ENOMEM;
	}

	priv->htim = extra->htim;
	h->priv = priv;
	h->ops = config->ops;
	*handle = h;

	return 0;
}

static int stm32_capi_timer_deinit(struct capi_timer_handle *handle)
{
	struct stm32_timer_priv *priv;

	if (!handle)
		return -EINVAL;

	priv = handle->priv;
	if (priv) {
		HAL_TIM_Base_Stop(priv->htim);
		HAL_TIM_Base_DeInit(priv->htim);
		capi_free(priv);
	}

	if (handle->init_allocated)
		capi_free(handle);

	return 0;
}

static int stm32_capi_timer_start(struct capi_timer_handle *handle)
{
	struct stm32_timer_priv *priv;

	if (!handle || !handle->priv)
		return -EINVAL;

	priv = handle->priv;
	if (HAL_TIM_Base_Start(priv->htim) != HAL_OK)
		return -EIO;

	return 0;
}

static int stm32_capi_timer_stop(struct capi_timer_handle *handle)
{
	struct stm32_timer_priv *priv;

	if (!handle || !handle->priv)
		return -EINVAL;

	priv = handle->priv;
	if (HAL_TIM_Base_Stop(priv->htim) != HAL_OK)
		return -EIO;

	return 0;
}

static int stm32_capi_timer_counter_get(struct capi_timer_handle *handle,
					uint32_t *counter)
{
	struct stm32_timer_priv *priv;

	if (!handle || !handle->priv || !counter)
		return -EINVAL;

	priv = handle->priv;
	*counter = __HAL_TIM_GET_COUNTER(priv->htim);

	return 0;
}

static int stm32_capi_timer_channel_enable(struct capi_timer_handle *handle,
					   uint32_t chan)
{
	struct stm32_timer_priv *priv;
	uint32_t hal_chan;

	if (!handle || !handle->priv)
		return -EINVAL;

	priv = handle->priv;
	hal_chan = chan * 4;

	if (HAL_TIM_PWM_Start(priv->htim, hal_chan) != HAL_OK)
		return -EIO;

	return 0;
}

static int stm32_capi_timer_channel_disable(struct capi_timer_handle *handle,
					    uint32_t chan)
{
	struct stm32_timer_priv *priv;
	uint32_t hal_chan;

	if (!handle || !handle->priv)
		return -EINVAL;

	priv = handle->priv;
	hal_chan = chan * 4;

	if (HAL_TIM_PWM_Stop(priv->htim, hal_chan) != HAL_OK)
		return -EIO;

	return 0;
}

static int stm32_capi_timer_channel_compare_set(struct capi_timer_handle *handle,
						uint32_t chan, uint32_t compare)
{
	struct stm32_timer_priv *priv;
	uint32_t hal_chan;

	if (!handle || !handle->priv)
		return -EINVAL;

	priv = handle->priv;
	hal_chan = chan * 4;

	__HAL_TIM_SET_COMPARE(priv->htim, hal_chan, compare);

	return 0;
}

static int stm32_capi_timer_channel_compare_get(struct capi_timer_handle *handle,
						uint32_t chan, uint32_t *compare)
{
	struct stm32_timer_priv *priv;
	uint32_t hal_chan;

	if (!handle || !handle->priv || !compare)
		return -EINVAL;

	priv = handle->priv;
	hal_chan = chan * 4;

	*compare = __HAL_TIM_GET_COMPARE(priv->htim, hal_chan);

	return 0;
}

const struct capi_timer_ops stm32_capi_timer_ops = {
	.init = stm32_capi_timer_init,
	.deinit = stm32_capi_timer_deinit,
	.start = stm32_capi_timer_start,
	.stop = stm32_capi_timer_stop,
	.counter_get = stm32_capi_timer_counter_get,
	.channel_enable = stm32_capi_timer_channel_enable,
	.channel_disable = stm32_capi_timer_channel_disable,
	.channel_compare_set = stm32_capi_timer_channel_compare_set,
	.channel_compare_get = stm32_capi_timer_channel_compare_get,
};

