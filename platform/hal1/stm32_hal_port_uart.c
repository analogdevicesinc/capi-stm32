/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   hal1/stm32_hal_port_uart.c
 * @brief  UART porting layer for the classic HAL.
 *
 * See platform/stm32_hal_port.h for the purpose of this layer.
 *
 * The classic HAL reports UART events with weak global functions, such as
 * HAL_UART_TxCpltCallback. All UARTs share them. Only the handle pointer shows
 * which UART caused the event. This file keeps a table that maps each handle
 * to its context. The new HAL does not need this table.
 */

#include "stm32_hal_port.h"

#include <errno.h>

/* Number of UART instances in the table. H7 and H5 have 10 at most. */
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
static int uart_status_to_errno(HAL_StatusTypeDef status)
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

	return 0;
}

int stm32_hal_uart_set_line_config(stm32_hal_uart_t *huart,
				   const struct capi_uart_line_config *cfg)
{
	if (!huart || !cfg)
		return -EINVAL;

	/* Write to the live handle. Mode, OverSampling, prescaler and FIFO */
	/* thresholds keep the values of the generated init */
	huart->Init.BaudRate = cfg->baudrate;

	switch (cfg->size) {
#ifdef UART_WORDLENGTH_7B
	case CAPI_UART_DATA_BITS_7:
		huart->Init.WordLength = UART_WORDLENGTH_7B;
		break;
#endif
	case CAPI_UART_DATA_BITS_8:
		huart->Init.WordLength = UART_WORDLENGTH_8B;
		break;
	default:
		return -EINVAL;
	}

	switch (cfg->parity) {
	case CAPI_UART_PARITY_NONE:
		huart->Init.Parity = UART_PARITY_NONE;
		break;
	case CAPI_UART_PARITY_ODD:
		huart->Init.Parity = UART_PARITY_ODD;
		break;
	case CAPI_UART_PARITY_EVEN:
		huart->Init.Parity = UART_PARITY_EVEN;
		break;
	default:
		return -EINVAL;
	}

	huart->Init.StopBits = (cfg->stop_bits == CAPI_UART_STOP_2_BIT) ?
			       UART_STOPBITS_2 : UART_STOPBITS_1;

	switch (cfg->flow_control) {
	case CAPI_UART_FLOW_CONTROL_NONE:
		huart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
		break;
	case CAPI_UART_FLOW_CONTROL_RTS:
		huart->Init.HwFlowCtl = UART_HWCONTROL_RTS;
		break;
	case CAPI_UART_FLOW_CONTROL_CTS:
		huart->Init.HwFlowCtl = UART_HWCONTROL_CTS;
		break;
	case CAPI_UART_FLOW_CONTROL_RTSCTS:
		huart->Init.HwFlowCtl = UART_HWCONTROL_RTS_CTS;
		break;
	default:
		return -EINVAL;
	}

	if (HAL_UART_Init(huart) != HAL_OK)
		return -EIO;

	return 0;
}

int stm32_hal_uart_get_line_config(stm32_hal_uart_t *huart,
				   struct capi_uart_line_config *cfg)
{
	if (!huart || !cfg)
		return -EINVAL;

	cfg->baudrate = huart->Init.BaudRate;

	switch (huart->Init.WordLength) {
#ifdef UART_WORDLENGTH_7B
	case UART_WORDLENGTH_7B:
		cfg->size = CAPI_UART_DATA_BITS_7;
		break;
#endif
	case UART_WORDLENGTH_8B:
	default:
		cfg->size = CAPI_UART_DATA_BITS_8;
		break;
	}

	switch (huart->Init.Parity) {
	case UART_PARITY_ODD:
		cfg->parity = CAPI_UART_PARITY_ODD;
		break;
	case UART_PARITY_EVEN:
		cfg->parity = CAPI_UART_PARITY_EVEN;
		break;
	default:
		cfg->parity = CAPI_UART_PARITY_NONE;
		break;
	}

	cfg->stop_bits = (huart->Init.StopBits == UART_STOPBITS_2) ?
			 CAPI_UART_STOP_2_BIT : CAPI_UART_STOP_1_BIT;

	switch (huart->Init.HwFlowCtl) {
	case UART_HWCONTROL_RTS:
		cfg->flow_control = CAPI_UART_FLOW_CONTROL_RTS;
		break;
	case UART_HWCONTROL_CTS:
		cfg->flow_control = CAPI_UART_FLOW_CONTROL_CTS;
		break;
	case UART_HWCONTROL_RTS_CTS:
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

	return uart_status_to_errno(HAL_UART_DeInit(huart));
}

int stm32_hal_uart_write(stm32_hal_uart_t *huart, const uint8_t *buf, uint32_t len,
			 uint32_t timeout_ms)
{
	if (!huart || !buf)
		return -EINVAL;

	/* HAL_MAX_DELAY has the same value as STM32_HAL_WAIT_FOREVER */
	if (timeout_ms == STM32_HAL_WAIT_FOREVER)
		timeout_ms = HAL_MAX_DELAY;

	return uart_status_to_errno(HAL_UART_Transmit(huart, buf, (uint16_t)len,
				    timeout_ms));
}

int stm32_hal_uart_read(stm32_hal_uart_t *huart, uint8_t *buf, uint32_t len,
			uint32_t timeout_ms)
{
	if (!huart || !buf)
		return -EINVAL;

	if (timeout_ms == STM32_HAL_WAIT_FOREVER)
		timeout_ms = HAL_MAX_DELAY;

	return uart_status_to_errno(HAL_UART_Receive(huart, buf, (uint16_t)len,
				    timeout_ms));
}

int stm32_hal_uart_write_async(stm32_hal_uart_t *huart, const uint8_t *buf,
			       uint32_t len)
{
	if (!huart || !buf)
		return -EINVAL;

	return uart_status_to_errno(HAL_UART_Transmit_IT(huart, buf,
				    (uint16_t)len));
}

int stm32_hal_uart_read_async(stm32_hal_uart_t *huart, uint8_t *buf,
			      uint32_t len)
{
	if (!huart || !buf)
		return -EINVAL;

	return uart_status_to_errno(HAL_UART_Receive_IT(huart, buf,
				    (uint16_t)len));
}

void stm32_hal_uart_irq_handler(stm32_hal_uart_t *huart)
{
	if (!huart)
		return;

	HAL_UART_IRQHandler(huart);
}

/*
 * Overrides of the weak HAL callbacks. HAL_UART_IRQHandler calls them.
 * Each one finds the table slot of the handle and forwards the event.
 */

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	struct uart_slot *slot = uart_slot_find(huart);

	if (slot && slot->events->tx_done)
		slot->events->tx_done(slot->ctx);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	struct uart_slot *slot = uart_slot_find(huart);

	if (slot && slot->events->rx_done)
		slot->events->rx_done(slot->ctx);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	struct uart_slot *slot = uart_slot_find(huart);
	uint32_t raw;
	uint32_t flags = 0;

	if (!slot || !slot->events->error)
		return;

	raw = huart->ErrorCode;

	if (raw & HAL_UART_ERROR_PE)
		flags |= STM32_HAL_UART_ERR_PARITY;
	if (raw & HAL_UART_ERROR_FE)
		flags |= STM32_HAL_UART_ERR_FRAMING;
	if (raw & HAL_UART_ERROR_ORE)
		flags |= STM32_HAL_UART_ERR_OVERRUN;

	slot->events->error(slot->ctx, flags, raw);
}

void HAL_UART_AbortCpltCallback(UART_HandleTypeDef *huart)
{
	struct uart_slot *slot = uart_slot_find(huart);

	if (slot && slot->events->abort_done)
		slot->events->abort_done(slot->ctx);
}
