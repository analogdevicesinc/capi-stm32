/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   stm32_capi_tdm.c
 * @brief  CAPI I2S backend for STM32. It drives the SAI block as a TDM port.
 */

#include "stm32_capi_tdm.h"

#include "capi_alloc.h"
#include <errno.h>

/* Private handle stored in capi_i2s_handle->priv */
struct stm32_tdm_priv {
	SAI_HandleTypeDef *hsai;
};

static int stm32_capi_tdm_init(struct capi_i2s_handle **handle,
				const struct capi_i2s_config *config)
{
	struct capi_i2s_handle *i2s_handle;
	struct stm32_tdm_priv *priv;
	struct stm32_tdm_extra_config *extra;
	int ret;

	if (!handle || !config || !config->extra)
		return -EINVAL;

	extra = (struct stm32_tdm_extra_config *)config->extra;
	if (!extra->hsai)
		return -EINVAL;

	if (*handle == NULL) {
		i2s_handle = (struct capi_i2s_handle *)capi_calloc(1,
				sizeof(*i2s_handle));
		if (!i2s_handle)
			return -ENOMEM;

		priv = (struct stm32_tdm_priv *)capi_calloc(1, sizeof(*priv));
		if (!priv) {
			capi_free(i2s_handle);
			return -ENOMEM;
		}

		i2s_handle->priv = priv;
		i2s_handle->init_allocated = true;
	} else {
		i2s_handle = *handle;
		i2s_handle->init_allocated = false;

		if (!i2s_handle->priv)
			return -EINVAL;

		priv = i2s_handle->priv;
	}

	i2s_handle->ops = config->ops;
	priv->hsai = extra->hsai;

	ret = HAL_SAI_Init(priv->hsai);
	if (ret != HAL_OK) {
		if (i2s_handle->init_allocated) {
			capi_free(priv);
			capi_free(i2s_handle);
		}
		return -EIO;
	}

	*handle = i2s_handle;

	return 0;
}

static int stm32_capi_tdm_deinit(const struct capi_i2s_handle *handle)
{
	struct stm32_tdm_priv *priv;

	if (!handle)
		return -EINVAL;

	priv = handle->priv;

	if (priv)
		HAL_SAI_DeInit(priv->hsai);

	if (handle->init_allocated) {
		capi_free(priv);
		capi_free((void *)handle);
	}

	return 0;
}

static int stm32_capi_tdm_port_enable(const struct capi_i2s_handle *handle,
				       uint32_t port_no,
				       enum capi_i2s_direction direction)
{
	struct stm32_tdm_priv *priv;

	if (!handle || !handle->priv)
		return -EINVAL;

	priv = handle->priv;
	(void)port_no;
	(void)direction;

	__HAL_SAI_ENABLE(priv->hsai);

	return 0;
}

static int stm32_capi_tdm_port_disable(const struct capi_i2s_handle *handle,
					uint32_t port_no,
					enum capi_i2s_direction direction)
{
	struct stm32_tdm_priv *priv;

	if (!handle || !handle->priv)
		return -EINVAL;

	priv = handle->priv;
	(void)port_no;
	(void)direction;

	__HAL_SAI_DISABLE(priv->hsai);

	return 0;
}

const struct capi_i2s_ops stm32_capi_tdm_ops = {
	.init = stm32_capi_tdm_init,
	.deinit = stm32_capi_tdm_deinit,
	.get_config = NULL,
	.set_port_config = NULL,
	.get_port_config = NULL,
	.port_enable = stm32_capi_tdm_port_enable,
	.port_disable = stm32_capi_tdm_port_disable,
};
