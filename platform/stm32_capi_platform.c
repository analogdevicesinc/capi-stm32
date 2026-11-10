/*
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   stm32_capi_platform.c
 * @brief  CAPI platform hooks for STM32: init, core clock, tick and stdio.
 */

#include <errno.h>

#include "stm32_capi_hal.h"
#include "capi_platform.h"
#include "capi_uart.h"
#include "stm32_capi_uart.h"

extern uint32_t SystemCoreClock;

int capi_platform_init(void)
{
	return stm32_init();
}

int capi_platform_get_clock_hz(uint64_t *clock_hz)
{
	if (!clock_hz)
		return -EINVAL;

	*clock_hz = SystemCoreClock;

	return 0;
}

void capi_platform_tick(void)
{
	HAL_IncTick();
}

int capi_stdio_init(struct capi_uart_handle *handle)
{
	return stm32_uart_stdio_enable(handle);
}
