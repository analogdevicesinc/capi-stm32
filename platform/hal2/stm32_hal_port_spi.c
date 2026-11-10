/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   hal2/stm32_hal_port_spi.c
 * @brief  SPI porting layer for the new HAL.
 *
 * See platform/stm32_hal_port.h for the purpose of this layer.
 *
 * The CAPI SPI backend only blocks, so most of this file is a thin
 * translation, as in the classic HAL. There are two differences:
 *
 *   - The new HAL has no HAL_SPI_Init. Init(handle, instance) binds a handle
 *     to a peripheral. SetConfig() applies the settings. The generated
 *     mx_spi<n>.c runs both before the driver. The init function only checks
 *     that they ran.
 *   - A transfer counts packets, not bytes. They are equal only for an 8-bit
 *     data width, which the byte-oriented CAPI API needs. The init function
 *     checks the width, to prevent a wrong transfer size.
 */

#include "stm32_hal_port.h"

#include <errno.h>

/** Convert a HAL status to the errno value of the port layer. */
static int spi_status_to_errno(hal_status_t status)
{
	switch (status) {
	case HAL_OK:
		return 0;
	case HAL_BUSY:
		return -EBUSY;
	case HAL_TIMEOUT:
		return -ETIMEDOUT;
	case HAL_INVALID_PARAM:
		return -EINVAL;
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

	switch (HAL_SPI_GetState(hspi)) {
	case HAL_SPI_STATE_IDLE:
		break;
	case HAL_SPI_STATE_RESET:
	case HAL_SPI_STATE_INIT:
		/* The generated init did not run. The cause can be an .ioc2 that has */
		/* the pins but not the SPI component. Then there is no mx_spi<n>.c. */
		return -EIO;
	default:
		/* The peripheral is configured and busy */
		return -EBUSY;
	}

	if (HAL_SPI_GetDataWidth(hspi) != HAL_SPI_DATA_WIDTH_8_BIT)
		return -ENOTSUP;

	return 0;
}

int stm32_hal_spi_set_format(stm32_hal_spi_t *hspi, enum capi_spi_mode mode,
			     uint32_t speed_hz)
{
	hal_spi_clock_polarity_t polarity;
	hal_spi_clock_phase_t phase;
	hal_status_t status;

	if (!hspi)
		return -EINVAL;

	switch (mode) {
	case CAPI_SPI_MODE_0:
		polarity = HAL_SPI_CLOCK_POLARITY_LOW;
		phase = HAL_SPI_CLOCK_PHASE_1_EDGE;
		break;
	case CAPI_SPI_MODE_1:
		polarity = HAL_SPI_CLOCK_POLARITY_LOW;
		phase = HAL_SPI_CLOCK_PHASE_2_EDGE;
		break;
	case CAPI_SPI_MODE_2:
		polarity = HAL_SPI_CLOCK_POLARITY_HIGH;
		phase = HAL_SPI_CLOCK_PHASE_1_EDGE;
		break;
	case CAPI_SPI_MODE_3:
		polarity = HAL_SPI_CLOCK_POLARITY_HIGH;
		phase = HAL_SPI_CLOCK_PHASE_2_EDGE;
		break;
	default:
		return -EINVAL;
	}

	/*
	 * This function does not use speed_hz. The HAL setter needs a prescaler,
	 * and the prescaler needs the kernel clock of the instance.
	 * A wrong guess can clock a device out of specification.
	 * The bus keeps the rate of the generated init.
	 */
	(void)speed_hz;

	/* This HAL has setters, so no re-init is needed */
	/* It cannot read the format back, so the caller caches it */
	status = HAL_SPI_SetClockPolarity(hspi, polarity);
	if (status != HAL_OK)
		return spi_status_to_errno(status);

	return spi_status_to_errno(HAL_SPI_SetClockPhase(hspi, phase));
}

int stm32_hal_spi_deinit(stm32_hal_spi_t *hspi)
{
	if (!hspi)
		return -EINVAL;

	/* HAL_SPI_DeInit cannot fail */
	HAL_SPI_DeInit(hspi);

	return 0;
}

int stm32_hal_spi_write_read(stm32_hal_spi_t *hspi, uint8_t *tx, uint8_t *rx,
			     uint32_t len, uint32_t timeout_ms)
{
	if (!hspi || !tx || !rx)
		return -EINVAL;

	return spi_status_to_errno(HAL_SPI_TransmitReceive(hspi, tx, rx, len,
				   spi_timeout(timeout_ms)));
}

int stm32_hal_spi_write(stm32_hal_spi_t *hspi, uint8_t *tx, uint32_t len,
			uint32_t timeout_ms)
{
	if (!hspi || !tx)
		return -EINVAL;

	return spi_status_to_errno(HAL_SPI_Transmit(hspi, tx, len,
				   spi_timeout(timeout_ms)));
}

int stm32_hal_spi_read(stm32_hal_spi_t *hspi, uint8_t *rx, uint32_t len,
		       uint32_t timeout_ms)
{
	if (!hspi || !rx)
		return -EINVAL;

	return spi_status_to_errno(HAL_SPI_Receive(hspi, rx, len,
				   spi_timeout(timeout_ms)));
}
