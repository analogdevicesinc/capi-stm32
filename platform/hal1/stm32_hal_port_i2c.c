/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   hal1/stm32_hal_port_i2c.c
 * @brief  I2C porting layer for the classic HAL.
 *
 * See platform/stm32_hal_port.h for the purpose of this layer.
 *
 * The layer hides two HAL conventions from the driver. It shifts the address
 * left by one, because the HAL needs the 8-bit form with the R/W bit cleared.
 * It converts the register address width from bytes to an I2C_MEMADD_SIZE_*
 * constant.
 */

#include "stm32_hal_port.h"

#include <errno.h>

/** Convert a HAL status to the errno value of the port layer. */
static int i2c_status_to_errno(HAL_StatusTypeDef status)
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
static uint32_t i2c_timeout(uint32_t timeout_ms)
{
	return (timeout_ms == STM32_HAL_WAIT_FOREVER) ? HAL_MAX_DELAY :
	       timeout_ms;
}

/** Convert a register address width in bytes to the HAL constant. */
static int i2c_memadd_size(uint8_t mem_addr_len, uint16_t *size)
{
	switch (mem_addr_len) {
	case 1:
		*size = I2C_MEMADD_SIZE_8BIT;
		return 0;
	case 2:
		*size = I2C_MEMADD_SIZE_16BIT;
		return 0;
	default:
		return -EINVAL;
	}
}

int stm32_hal_i2c_init(stm32_hal_i2c_t *hi2c)
{
	if (!hi2c)
		return -EINVAL;

	return i2c_status_to_errno(HAL_I2C_Init(hi2c));
}

int stm32_hal_i2c_deinit(stm32_hal_i2c_t *hi2c)
{
	if (!hi2c)
		return -EINVAL;

	return i2c_status_to_errno(HAL_I2C_DeInit(hi2c));
}

int stm32_hal_i2c_write(stm32_hal_i2c_t *hi2c, uint16_t addr, uint8_t *buf,
			uint32_t len, uint32_t timeout_ms)
{
	if (!hi2c || (!buf && len))
		return -EINVAL;

	return i2c_status_to_errno(HAL_I2C_Master_Transmit(hi2c,
				   (uint16_t)(addr << 1), buf, (uint16_t)len,
				   i2c_timeout(timeout_ms)));
}

int stm32_hal_i2c_read(stm32_hal_i2c_t *hi2c, uint16_t addr, uint8_t *buf,
		       uint32_t len, uint32_t timeout_ms)
{
	if (!hi2c || (!buf && len))
		return -EINVAL;

	return i2c_status_to_errno(HAL_I2C_Master_Receive(hi2c,
				   (uint16_t)(addr << 1), buf, (uint16_t)len,
				   i2c_timeout(timeout_ms)));
}

int stm32_hal_i2c_mem_write(stm32_hal_i2c_t *hi2c, uint16_t addr,
			    uint16_t mem_addr, uint8_t mem_addr_len,
			    uint8_t *buf, uint32_t len, uint32_t timeout_ms)
{
	uint16_t mem_addr_size;
	int ret;

	if (!hi2c || (!buf && len))
		return -EINVAL;

	ret = i2c_memadd_size(mem_addr_len, &mem_addr_size);
	if (ret)
		return ret;

	return i2c_status_to_errno(HAL_I2C_Mem_Write(hi2c,
				   (uint16_t)(addr << 1), mem_addr,
				   mem_addr_size, buf, (uint16_t)len,
				   i2c_timeout(timeout_ms)));
}

int stm32_hal_i2c_mem_read(stm32_hal_i2c_t *hi2c, uint16_t addr,
			   uint16_t mem_addr, uint8_t mem_addr_len,
			   uint8_t *buf, uint32_t len, uint32_t timeout_ms)
{
	uint16_t mem_addr_size;
	int ret;

	if (!hi2c || (!buf && len))
		return -EINVAL;

	ret = i2c_memadd_size(mem_addr_len, &mem_addr_size);
	if (ret)
		return ret;

	return i2c_status_to_errno(HAL_I2C_Mem_Read(hi2c,
				   (uint16_t)(addr << 1), mem_addr,
				   mem_addr_size, buf, (uint16_t)len,
				   i2c_timeout(timeout_ms)));
}
