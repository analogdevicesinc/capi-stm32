/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   stm32_capi_gpdma.h
 * @brief  Header for the STM32 CAPI GPDMA backend (STM32H5).
 */

#ifndef STM32_CAPI_GPDMA_H_
#define STM32_CAPI_GPDMA_H_

#include "stm32_capi_hal.h"
#include "capi_dma.h"

/**
 * @struct stm32_gpdma_extra_config
 * @brief STM32 specific GPDMA controller configuration.
 */
struct stm32_gpdma_extra_config {
	/** HAL handle of the GPDMA channel. The backend does not read it. */
	DMA_HandleTypeDef *hdma;
};

/**
 * @struct stm32_gpdma_chan_extra
 * @brief STM32 specific GPDMA transfer data, passed in capi_dma_transfer.extra.
 */
struct stm32_gpdma_chan_extra {
	/** HAL handle of the GPDMA channel for this transfer */
	DMA_HandleTypeDef *hdma;
};

/**
 * @brief STM32 GPDMA operations for CAPI
 */
extern const struct capi_dma_ops stm32_capi_gpdma_ops;

#endif /* STM32_CAPI_GPDMA_H_ */
