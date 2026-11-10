/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   stm32_capi_time.c
 * @brief  CAPI time backend for STM32: delays and uptime.
 *
 * The build skips this file when CONFIG_FREERTOS is set.
 */

#include <stdbool.h>
#include <stdint.h>
#include "stm32_capi_hal.h"
#include "capi_time.h"

/**
 * @brief Wait at least the given number of microseconds.
 *
 * Cores with a DWT cycle counter (Cortex-M3/M4/M7) use it for a precise delay.
 * Other cores use HAL_Delay, which has a 1 ms resolution.
 *
 * @param us - Minimum number of microseconds to wait.
 */
#if defined(DWT)
#pragma GCC push_options
#pragma GCC optimize ("O3")
void capi_wait_us_impl(uint32_t us)
{
	static bool dwt_initialised = false;
	volatile uint32_t cycles = (SystemCoreClock / 1000000U) * us;
	volatile uint32_t start;

	if (!dwt_initialised) {
		CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
#ifdef STM32F7
		/* Unlock the DWT registers */
		DWT->LAR = 0xC5ACCE55;
#endif
		DWT->CTRL |= 1;
		dwt_initialised = true;
	}

	start = DWT->CYCCNT;
	while (DWT->CYCCNT - start < cycles)
		;
}
#pragma GCC pop_options
#else
void capi_wait_us_impl(uint32_t us)
{
	if (us)
		HAL_Delay((us - 1) / 1000 + 1);
}
#endif

/**
 * @brief Wait at least the given number of milliseconds.
 *
 * HAL_Delay uses SysTick. Its accuracy is +/- 1 ms.
 *
 * @param ms - Minimum number of milliseconds to wait.
 */
void capi_wait_ms_impl(uint32_t ms)
{
	HAL_Delay(ms);
}

/**
 * @brief Get the time since boot in microseconds.
 *
 * The milliseconds come from HAL_GetTick (SysTick).
 * Cores with a DWT cycle counter get the sub-millisecond part from CYCCNT.
 * On other cores, this part is zero.
 *
 * @param us - Pointer to the uptime in microseconds.
 * @return 0 in case of success.
 */
int capi_uptime_impl(uint64_t *us)
{
	uint32_t ms = HAL_GetTick();
	uint64_t result = (uint64_t)ms * 1000U;

#if defined(DWT)
	{
		uint32_t cycles_per_ms = SystemCoreClock / 1000U;
		uint32_t cyccnt = DWT->CYCCNT;
		uint32_t frac_us = (cyccnt % cycles_per_ms) / (SystemCoreClock / 1000000U);

		result += frac_us;
	}
#endif

	*us = result;

	return 0;
}
