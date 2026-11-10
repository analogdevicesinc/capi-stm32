/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   stm32_capi_init_cubemx2.c
 * @brief  Board init entry point for the STM32CubeMX2 backend.
 *
 * Classic CubeMX generates a main() that does the board init.
 * sdk/stm32_patch_cubemx.cmake renames it to stm32_init().
 * CubeMX2 generates the same init as mx_system_init() and a main() that
 * calls it. The build leaves out the generated main.c. stm32_init() calls
 * mx_system_init().
 * The build compiles this file only for the stm32cubemx2 target.
 */

#include "stm32_capi_hal.h"

int stm32_init(void)
{
	/* SYSTEM_OK is not zero. Each other value names the stage that failed. */
	/* Return only pass or fail to capi_platform_init() */
	return (mx_system_init() == SYSTEM_OK) ? 0 : -1;
}
