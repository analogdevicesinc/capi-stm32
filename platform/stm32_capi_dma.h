/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   stm32_capi_dma.h
 * @brief  Header for the STM32 CAPI DMA backend.
 */

#ifndef STM32_CAPI_DMA_H_
#define STM32_CAPI_DMA_H_

#include "stm32_capi_hal.h"
#include "capi_dma.h"

/**
 * @struct stm32_dma_extra_config
 * @brief STM32 specific DMA controller configuration.
 */
struct stm32_dma_extra_config {
	/** HAL handle of the DMA stream. The backend does not read it. */
	DMA_HandleTypeDef *hdma;
};

/**
 * @struct stm32_dma_chan_extra
 * @brief STM32 specific DMA transfer data, passed in capi_dma_transfer.extra.
 */
struct stm32_dma_chan_extra {
	/** HAL handle of the DMA stream for this channel */
	DMA_HandleTypeDef *hdma;
};

/**
 * @brief STM32 DMA operations for CAPI
 */
extern const struct capi_dma_ops stm32_capi_dma_ops;

#endif /* STM32_CAPI_DMA_H_ */
