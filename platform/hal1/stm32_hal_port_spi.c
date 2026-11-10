/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   hal1/stm32_hal_port_spi.c
 * @brief  SPI porting layer for the classic HAL.
 *
 * See platform/stm32_hal_port.h for the purpose of this layer.
 *
 * The CAPI SPI backend only blocks, so this file is a thin translation of the
 * handle type, the three transfer types and the HAL status to errno.
 */

#include "stm32_hal_port.h"

#include <errno.h>

/** Convert a HAL status to the errno value of the port layer. */
static int spi_status_to_errno(HAL_StatusTypeDef status)
{
	switch (status) {
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

/* HAL_MAX_DELAY has the same value as STM32_HAL_WAIT_FOREVER */
static uint32_t spi_timeout(uint32_t timeout_ms)
{
	return (timeout_ms == STM32_HAL_WAIT_FOREVER) ? HAL_MAX_DELAY :
	       timeout_ms;
}

int stm32_hal_spi_init(stm32_hal_spi_t *hspi)
{
	if (!hspi)
		return -EINVAL;

	return spi_status_to_errno(HAL_SPI_Init(hspi));
}

int stm32_hal_spi_set_format(stm32_hal_spi_t *hspi, enum capi_spi_mode mode,
			     uint32_t speed_hz)
{
	uint32_t polarity, phase;

	if (!hspi)
		return -EINVAL;

	switch (mode) {
	case CAPI_SPI_MODE_0:
		polarity = SPI_POLARITY_LOW;
		phase = SPI_PHASE_1EDGE;
		break;
	case CAPI_SPI_MODE_1:
		polarity = SPI_POLARITY_LOW;
		phase = SPI_PHASE_2EDGE;
		break;
	case CAPI_SPI_MODE_2:
		polarity = SPI_POLARITY_HIGH;
		phase = SPI_PHASE_1EDGE;
		break;
	case CAPI_SPI_MODE_3:
		polarity = SPI_POLARITY_HIGH;
		phase = SPI_PHASE_2EDGE;
		break;
	default:
		return -EINVAL;
	}

	/*
	 * This function does not use speed_hz. The HAL needs a prescaler, and
	 * the prescaler needs the kernel clock of the instance
	 * (HAL_RCC_GetPCLKxFreq, which changes with instance and family).
	 * A wrong guess can clock a device out of specification.
	 * The bus keeps the rate of the .ioc.
	 */
	(void)speed_hz;

	if (hspi->Init.CLKPolarity == polarity && hspi->Init.CLKPhase == phase)
		return 0;

	hspi->Init.CLKPolarity = polarity;
	hspi->Init.CLKPhase = phase;

	/* A write to Init does not change the peripheral. The HAL has no setter. */
	/* HAL_SPI_Init applies the format. The caller skips it for the same format. */
	return spi_status_to_errno(HAL_SPI_Init(hspi));
}

int stm32_hal_spi_deinit(stm32_hal_spi_t *hspi)
{
	if (!hspi)
		return -EINVAL;

	return spi_status_to_errno(HAL_SPI_DeInit(hspi));
}

int stm32_hal_spi_write_read(stm32_hal_spi_t *hspi, uint8_t *tx, uint8_t *rx,
			     uint32_t len, uint32_t timeout_ms)
{
	if (!hspi || !tx || !rx)
		return -EINVAL;

	return spi_status_to_errno(HAL_SPI_TransmitReceive(hspi, tx, rx,
				   (uint16_t)len, spi_timeout(timeout_ms)));
}

int stm32_hal_spi_write(stm32_hal_spi_t *hspi, uint8_t *tx, uint32_t len,
			uint32_t timeout_ms)
{
	if (!hspi || !tx)
		return -EINVAL;

	return spi_status_to_errno(HAL_SPI_Transmit(hspi, tx, (uint16_t)len,
				   spi_timeout(timeout_ms)));
}

int stm32_hal_spi_read(stm32_hal_spi_t *hspi, uint8_t *rx, uint32_t len,
		       uint32_t timeout_ms)
{
	if (!hspi || !rx)
		return -EINVAL;

	return spi_status_to_errno(HAL_SPI_Receive(hspi, rx, (uint16_t)len,
				   spi_timeout(timeout_ms)));
}
