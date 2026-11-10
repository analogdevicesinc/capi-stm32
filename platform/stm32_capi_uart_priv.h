/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   stm32_capi_uart_priv.h
 * @brief  Private handle data of the STM32 CAPI UART backend.
 */

#ifndef _STM32_CAPI_UART_PRIV_H_
#define _STM32_CAPI_UART_PRIV_H_

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

#include "stm32_capi_uart.h"

/**
 * @struct stm32_uart_priv_handle
 * @brief STM32 UART private handle data
 */
struct stm32_uart_priv_handle {
	/** UART handle, in the form that the active HAL uses */
	stm32_hal_uart_t *huart;
	/** Callback that the user registers for async events */
	capi_uart_callback callback;
	/** Argument that the driver passes to the callback */
	void *callback_arg;
	/** Reason of the last interrupt, for get_interrupt_reason */
	enum capi_uart_interrupt_reason last_interrupt_reason;
	/** Line status flags, for get_line_status. A read clears them. */
	uint32_t last_line_status;
};

/** Statically allocate a UART handle with an STM32 private handle */
#define CAPI_UART_HANDLE_STM32_INIT() \
	(&(struct capi_uart_handle){ \
		.ops = NULL, \
		.init_allocated = false, \
		.priv = &(struct stm32_uart_priv_handle){0}})

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* _STM32_CAPI_UART_PRIV_H_ */
