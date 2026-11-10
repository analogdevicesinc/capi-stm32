/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   hal2/stm32_hal_port_i2c.c
 * @brief  I2C porting layer for the new HAL.
 *
 * See platform/stm32_hal_port.h for the purpose of this layer.
 *
 * Both HALs shift the address left by one and clear the R/W bit. The
 * differences are: the master calls are HAL_I2C_MASTER_*, the register
 * address width is an enum, and the init has two steps. The init function
 * checks that the generated mx_i2c<n>.c ran. The SPI file does the same.
 */

#include "stm32_hal_port.h"

#include <errno.h>

/** Convert a HAL status to the errno value of the port layer. */
static int i2c_status_to_errno(hal_status_t status)
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
static uint32_t i2c_timeout(uint32_t timeout_ms)
{
	return (timeout_ms == STM32_HAL_WAIT_FOREVER) ? HAL_MAX_DELAY :
	       timeout_ms;
}

/** Convert a register address width in bytes to the HAL constant. */
static int i2c_memadd_size(uint8_t mem_addr_len, hal_i2c_mem_addr_size_t *size)
{
	switch (mem_addr_len) {
	case 1:
		*size = HAL_I2C_MEM_ADDR_8BIT;
		return 0;
	case 2:
		*size = HAL_I2C_MEM_ADDR_16BIT;
		return 0;
	default:
		return -EINVAL;
	}
}

/** Convert a 7-bit address to the 8-bit form, as the classic HAL needs. */
static uint32_t i2c_device_addr(uint16_t addr)
{
	return (uint32_t)addr << 1;
}

int stm32_hal_i2c_init(stm32_hal_i2c_t *hi2c)
{
	if (!hi2c)
		return -EINVAL;

	switch (HAL_I2C_GetState(hi2c)) {
	case HAL_I2C_STATE_IDLE:
		return 0;
	case HAL_I2C_STATE_RESET:
	case HAL_I2C_STATE_INIT:
		/* The peripheral has no init, or has no configuration */
		return -EIO;
	default:
		return -EBUSY;
	}
}

int stm32_hal_i2c_deinit(stm32_hal_i2c_t *hi2c)
{
	if (!hi2c)
		return -EINVAL;

	/* HAL_I2C_DeInit cannot fail */
	HAL_I2C_DeInit(hi2c);

	return 0;
}

int stm32_hal_i2c_write(stm32_hal_i2c_t *hi2c, uint16_t addr, uint8_t *buf,
			uint32_t len, uint32_t timeout_ms)
{
	if (!hi2c || (!buf && len))
		return -EINVAL;

	return i2c_status_to_errno(HAL_I2C_MASTER_Transmit(hi2c,
				   i2c_device_addr(addr), buf, len,
				   i2c_timeout(timeout_ms)));
}

int stm32_hal_i2c_read(stm32_hal_i2c_t *hi2c, uint16_t addr, uint8_t *buf,
		       uint32_t len, uint32_t timeout_ms)
{
	if (!hi2c || (!buf && len))
		return -EINVAL;

	return i2c_status_to_errno(HAL_I2C_MASTER_Receive(hi2c,
				   i2c_device_addr(addr), buf, len,
				   i2c_timeout(timeout_ms)));
}

int stm32_hal_i2c_mem_write(stm32_hal_i2c_t *hi2c, uint16_t addr,
			    uint16_t mem_addr, uint8_t mem_addr_len,
			    uint8_t *buf, uint32_t len, uint32_t timeout_ms)
{
	hal_i2c_mem_addr_size_t mem_addr_size;
	int ret;

	if (!hi2c || (!buf && len))
		return -EINVAL;

	ret = i2c_memadd_size(mem_addr_len, &mem_addr_size);
	if (ret)
		return ret;

	return i2c_status_to_errno(HAL_I2C_MASTER_MemWrite(hi2c,
				   i2c_device_addr(addr), mem_addr,
				   mem_addr_size, buf, len,
				   i2c_timeout(timeout_ms)));
}

int stm32_hal_i2c_mem_read(stm32_hal_i2c_t *hi2c, uint16_t addr,
			   uint16_t mem_addr, uint8_t mem_addr_len,
			   uint8_t *buf, uint32_t len, uint32_t timeout_ms)
{
	hal_i2c_mem_addr_size_t mem_addr_size;
	int ret;

	if (!hi2c || (!buf && len))
		return -EINVAL;

	ret = i2c_memadd_size(mem_addr_len, &mem_addr_size);
	if (ret)
		return ret;

	return i2c_status_to_errno(HAL_I2C_MASTER_MemRead(hi2c,
				   i2c_device_addr(addr), mem_addr,
				   mem_addr_size, buf, len,
				   i2c_timeout(timeout_ms)));
}
