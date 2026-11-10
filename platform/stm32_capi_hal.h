/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   stm32_capi_hal.h
 * @brief  Entry point to the HAL headers that the code generator creates.
 */

#ifndef _STM32_HAL_H
#define _STM32_HAL_H

#include "main.h"

/**
 * @brief Start the HAL, set the clocks and initialize the board peripherals.
 * @return 0 in case of success, negative error code otherwise.
 */
int stm32_init(void);

#endif
