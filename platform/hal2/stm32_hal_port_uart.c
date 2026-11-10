/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   hal2/stm32_hal_port_uart.c
 * @brief  UART porting layer for the new HAL.
 *
 * See platform/stm32_hal_port.h for the purpose of this layer.
 *
 * Three hal_conf.h options change the HAL behavior. They are off by default.
 * CubeMX2 regenerates hal_conf.h from the .ioc2, so this file cannot expect
 * them to be on:
 *
 *   - USE_HAL_UART_REGISTER_CALLBACKS: when on, the events use callbacks that
 *     are registered for each handle, and the weak globals do not exist. When
 *     off, the HAL calls the weak globals of the classic HAL. Both paths end
 *     in the four uart_on_*() functions below.
 *   - USE_HAL_UART_USER_DATA: when on, the handle can carry the context. When
 *     off, this file keeps the same handle-to-context table as the classic HAL.
 *   - USE_HAL_UART_GET_LAST_ERRORS: when off, the HAL has no error word. An
 *     error event then has no flags, but the layer still reports it.
 */

#include "stm32_hal_port.h"

#include <errno.h>

/* Number of UART instances in the table. The classic HAL has the same size. */
#define STM32_HAL_UART_MAX_INSTANCES	12

struct uart_slot {
	stm32_hal_uart_t *huart;
	const struct stm32_hal_uart_events *events;
	void *ctx;
};

static struct uart_slot uart_slots[STM32_HAL_UART_MAX_INSTANCES];

static struct uart_slot *uart_slot_find(const stm32_hal_uart_t *huart)
{
	unsigned int i;

	/* A free slot has a NULL handle. Never match it. */
	if (!huart)
		return NULL;

	for (i = 0; i < STM32_HAL_UART_MAX_INSTANCES; i++) {
		if (uart_slots[i].huart == huart)
			return &uart_slots[i];
	}

	return NULL;
}

/** Convert a HAL status to the errno value of the port layer. */
static int uart_status_to_errno(hal_status_t status)
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

/* The four events. A registered callback or a weak global calls them. */

static void uart_on_tx_done(hal_uart_handle_t *huart)
{
	struct uart_slot *slot = uart_slot_find(huart);

	if (slot && slot->events->tx_done)
		slot->events->tx_done(slot->ctx);
}

static void uart_on_rx_done(hal_uart_handle_t *huart, uint32_t size_byte,
			    hal_uart_rx_event_types_t rx_event)
{
	struct uart_slot *slot = uart_slot_find(huart);

	/* The HAL reports the received size and the end event. The CAPI async */
	/* read ends on a fixed byte count, so the callers do not need them. */
	(void)size_byte;
	(void)rx_event;

	if (slot && slot->events->rx_done)
		slot->events->rx_done(slot->ctx);
}

static void uart_on_error(hal_uart_handle_t *huart)
{
	struct uart_slot *slot = uart_slot_find(huart);
	uint32_t flags = 0;
	uint32_t raw = 0;

	if (!slot || !slot->events->error)
		return;

#if defined(USE_HAL_UART_GET_LAST_ERRORS) && (USE_HAL_UART_GET_LAST_ERRORS == 1)
	raw = HAL_UART_GetLastErrorCodes(huart);

	if (raw & HAL_UART_RECEIVE_ERROR_PE)
		flags |= STM32_HAL_UART_ERR_PARITY;
	if (raw & HAL_UART_RECEIVE_ERROR_FE)
		flags |= STM32_HAL_UART_ERR_FRAMING;
	if (raw & HAL_UART_RECEIVE_ERROR_ORE)
		flags |= STM32_HAL_UART_ERR_OVERRUN;
#endif /* USE_HAL_UART_GET_LAST_ERRORS */

	slot->events->error(slot->ctx, flags, raw);
}

static void uart_on_abort_done(hal_uart_handle_t *huart)
{
	struct uart_slot *slot = uart_slot_find(huart);

	if (slot && slot->events->abort_done)
		slot->events->abort_done(slot->ctx);
}

#if !defined(USE_HAL_UART_REGISTER_CALLBACKS) || (USE_HAL_UART_REGISTER_CALLBACKS == 0)

/* Overrides of the weak callbacks, for a build without registration */

void HAL_UART_TxCpltCallback(hal_uart_handle_t *huart)
{
	uart_on_tx_done(huart);
}

void HAL_UART_RxCpltCallback(hal_uart_handle_t *huart, uint32_t size_byte,
			     hal_uart_rx_event_types_t rx_event)
{
	uart_on_rx_done(huart, size_byte, rx_event);
}

void HAL_UART_ErrorCallback(hal_uart_handle_t *huart)
{
	uart_on_error(huart);
}

void HAL_UART_AbortCpltCallback(hal_uart_handle_t *huart)
{
	uart_on_abort_done(huart);
}

#endif /* !USE_HAL_UART_REGISTER_CALLBACKS */

int stm32_hal_uart_set_events(stm32_hal_uart_t *huart,
			      const struct stm32_hal_uart_events *events,
			      void *ctx)
{
	struct uart_slot *slot;
	unsigned int i;

	if (!huart)
		return -EINVAL;

	if (!events) {
		slot = uart_slot_find(huart);
		if (slot) {
			slot->huart = NULL;
			slot->events = NULL;
			slot->ctx = NULL;
		}
		return 0;
	}

	slot = uart_slot_find(huart);
	if (!slot) {
		for (i = 0; i < STM32_HAL_UART_MAX_INSTANCES; i++) {
			if (!uart_slots[i].huart) {
				slot = &uart_slots[i];
				break;
			}
		}
	}

	if (!slot)
		return -ENOSPC;

	slot->huart = huart;
	slot->events = events;
	slot->ctx = ctx;

#if defined(USE_HAL_UART_REGISTER_CALLBACKS) && (USE_HAL_UART_REGISTER_CALLBACKS == 1)
	/*
	 * Register all four callbacks, also the ones that this event set does not use.
	 * The code reads the set when the event occurs.
	 * A later registration of the same handle then leaves no old callback.
	 */
	if (HAL_UART_RegisterTxCpltCallback(huart, uart_on_tx_done) != HAL_OK ||
	    HAL_UART_RegisterRxCpltCallback(huart, uart_on_rx_done) != HAL_OK ||
	    HAL_UART_RegisterErrorCallback(huart, uart_on_error) != HAL_OK ||
	    HAL_UART_RegisterAbortCpltCallback(huart,
					       uart_on_abort_done) != HAL_OK) {
		slot->huart = NULL;
		slot->events = NULL;
		slot->ctx = NULL;
		return -EIO;
	}
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */

	return 0;
}

int stm32_hal_uart_set_line_config(stm32_hal_uart_t *huart,
				   const struct capi_uart_line_config *cfg)
{
	hal_uart_config_t hal_cfg;

	if (!huart || !cfg)
		return -EINVAL;

	/* Read-modify-write. Direction, oversampling, prescaler and one-bit */
	/* sampling keep the values of the generated init. */
	HAL_UART_GetConfig(huart, &hal_cfg);

	hal_cfg.baud_rate = cfg->baudrate;

	switch (cfg->size) {
	case CAPI_UART_DATA_BITS_7:
		hal_cfg.word_length = HAL_UART_WORD_LENGTH_7_BIT;
		break;
	case CAPI_UART_DATA_BITS_8:
		hal_cfg.word_length = HAL_UART_WORD_LENGTH_8_BIT;
		break;
	default:
		return -EINVAL;
	}

	switch (cfg->parity) {
	case CAPI_UART_PARITY_NONE:
		hal_cfg.parity = HAL_UART_PARITY_NONE;
		break;
	case CAPI_UART_PARITY_ODD:
		hal_cfg.parity = HAL_UART_PARITY_ODD;
		break;
	case CAPI_UART_PARITY_EVEN:
		hal_cfg.parity = HAL_UART_PARITY_EVEN;
		break;
	default:
		return -EINVAL;
	}

	hal_cfg.stop_bits = (cfg->stop_bits == CAPI_UART_STOP_2_BIT) ?
			    HAL_UART_STOP_BIT_2 : HAL_UART_STOP_BIT_1;

	switch (cfg->flow_control) {
	case CAPI_UART_FLOW_CONTROL_NONE:
		hal_cfg.hw_flow_ctl = HAL_UART_HW_CONTROL_NONE;
		break;
	case CAPI_UART_FLOW_CONTROL_RTS:
		hal_cfg.hw_flow_ctl = HAL_UART_HW_CONTROL_RTS;
		break;
	case CAPI_UART_FLOW_CONTROL_CTS:
		hal_cfg.hw_flow_ctl = HAL_UART_HW_CONTROL_CTS;
		break;
	case CAPI_UART_FLOW_CONTROL_RTSCTS:
		hal_cfg.hw_flow_ctl = HAL_UART_HW_CONTROL_RTS_CTS;
		break;
	default:
		return -EINVAL;
	}

	return uart_status_to_errno(HAL_UART_SetConfig(huart, &hal_cfg));
}

int stm32_hal_uart_get_line_config(stm32_hal_uart_t *huart,
				   struct capi_uart_line_config *cfg)
{
	hal_uart_config_t hal_cfg;

	if (!huart || !cfg)
		return -EINVAL;

	HAL_UART_GetConfig(huart, &hal_cfg);

	cfg->baudrate = hal_cfg.baud_rate;

	switch (hal_cfg.word_length) {
	case HAL_UART_WORD_LENGTH_7_BIT:
		cfg->size = CAPI_UART_DATA_BITS_7;
		break;
	case HAL_UART_WORD_LENGTH_8_BIT:
	default:
		cfg->size = CAPI_UART_DATA_BITS_8;
		break;
	}

	switch (hal_cfg.parity) {
	case HAL_UART_PARITY_ODD:
		cfg->parity = CAPI_UART_PARITY_ODD;
		break;
	case HAL_UART_PARITY_EVEN:
		cfg->parity = CAPI_UART_PARITY_EVEN;
		break;
	default:
		cfg->parity = CAPI_UART_PARITY_NONE;
		break;
	}

	cfg->stop_bits = (hal_cfg.stop_bits == HAL_UART_STOP_BIT_2) ?
			 CAPI_UART_STOP_2_BIT : CAPI_UART_STOP_1_BIT;

	switch (hal_cfg.hw_flow_ctl) {
	case HAL_UART_HW_CONTROL_RTS:
		cfg->flow_control = CAPI_UART_FLOW_CONTROL_RTS;
		break;
	case HAL_UART_HW_CONTROL_CTS:
		cfg->flow_control = CAPI_UART_FLOW_CONTROL_CTS;
		break;
	case HAL_UART_HW_CONTROL_RTS_CTS:
		cfg->flow_control = CAPI_UART_FLOW_CONTROL_RTSCTS;
		break;
	default:
		cfg->flow_control = CAPI_UART_FLOW_CONTROL_NONE;
		break;
	}

	return 0;
}

int stm32_hal_uart_deinit(stm32_hal_uart_t *huart)
{
	if (!huart)
		return -EINVAL;

	/* HAL_UART_DeInit cannot fail */
	HAL_UART_DeInit(huart);

	return 0;
}

int stm32_hal_uart_write(stm32_hal_uart_t *huart, const uint8_t *buf, uint32_t len,
			 uint32_t timeout_ms)
{
	if (!huart || !buf)
		return -EINVAL;

	/* HAL_MAX_DELAY has the same value as STM32_HAL_WAIT_FOREVER */
	if (timeout_ms == STM32_HAL_WAIT_FOREVER)
		timeout_ms = HAL_MAX_DELAY;

	return uart_status_to_errno(HAL_UART_Transmit(huart, buf, len,
				    timeout_ms));
}

int stm32_hal_uart_read(stm32_hal_uart_t *huart, uint8_t *buf, uint32_t len,
			uint32_t timeout_ms)
{
	if (!huart || !buf)
		return -EINVAL;

	if (timeout_ms == STM32_HAL_WAIT_FOREVER)
		timeout_ms = HAL_MAX_DELAY;

	return uart_status_to_errno(HAL_UART_Receive(huart, buf, len,
				    timeout_ms));
}

int stm32_hal_uart_write_async(stm32_hal_uart_t *huart, const uint8_t *buf,
			       uint32_t len)
{
	if (!huart || !buf)
		return -EINVAL;

	return uart_status_to_errno(HAL_UART_Transmit_IT(huart, buf, len));
}

int stm32_hal_uart_read_async(stm32_hal_uart_t *huart, uint8_t *buf,
			      uint32_t len)
{
	if (!huart || !buf)
		return -EINVAL;

	return uart_status_to_errno(HAL_UART_Receive_IT(huart, buf, len));
}

void stm32_hal_uart_irq_handler(stm32_hal_uart_t *huart)
{
	if (!huart)
		return;

	HAL_UART_IRQHandler(huart);
}
