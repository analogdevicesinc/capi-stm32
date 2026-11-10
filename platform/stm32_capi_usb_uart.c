/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   stm32_capi_usb_uart.c
 * @brief  CAPI UART backend for the STM32 USB CDC virtual COM port (stub).
 */

#include "stm32_capi_usb_uart.h"

#include "capi_alloc.h"
#include <errno.h>

/*
 * This backend is a stub. A full backend needs the STM32 USB Device
 * middleware (usbd_cdc_if.h), which each project provides.
 */

/* Private handle stored in capi_uart_handle->priv */
struct stm32_usb_uart_priv {
	bool initialized;
	uint8_t cdc_index;
};

static int stm32_capi_usb_uart_init(struct capi_uart_handle **handle,
				     const struct capi_uart_config *config)
{
	struct capi_uart_handle *uart_handle;
	struct stm32_usb_uart_priv *priv;
	struct stm32_usb_uart_extra_config *extra;

	if (!handle || !config)
		return -EINVAL;

	if (*handle == NULL) {
		uart_handle = (struct capi_uart_handle *)capi_calloc(1,
				sizeof(*uart_handle));
		if (!uart_handle)
			return -ENOMEM;

		priv = (struct stm32_usb_uart_priv *)capi_calloc(1, sizeof(*priv));
		if (!priv) {
			capi_free(uart_handle);
			return -ENOMEM;
		}

		uart_handle->priv = priv;
		uart_handle->init_allocated = true;
	} else {
		uart_handle = *handle;
		uart_handle->init_allocated = false;

		if (!uart_handle->priv)
			return -EINVAL;

		priv = uart_handle->priv;
	}

	uart_handle->ops = config->ops;

	if (config->extra) {
		extra = (struct stm32_usb_uart_extra_config *)config->extra;
		priv->cdc_index = extra->cdc_index;
	}

	priv->initialized = true;
	*handle = uart_handle;

	return 0;
}

static int stm32_capi_usb_uart_deinit(struct capi_uart_handle *handle)
{
	struct stm32_usb_uart_priv *priv;

	if (!handle)
		return -EINVAL;

	priv = handle->priv;

	if (priv)
		priv->initialized = false;

	if (handle->init_allocated) {
		capi_free(priv);
		capi_free(handle);
	}

	return 0;
}

static int stm32_capi_usb_uart_transmit(struct capi_uart_handle *handle,
					 uint8_t *buf, uint32_t len)
{
	struct stm32_usb_uart_priv *priv;

	if (!handle || !handle->priv)
		return -EINVAL;

	priv = handle->priv;
	if (!priv->initialized)
		return -ENODEV;

	if (!buf || !len)
		return -EINVAL;

	/* A transmit needs the USB middleware (CDC_Transmit_FS) */
	(void)buf;
	(void)len;

	return -ENOSYS;
}

static int stm32_capi_usb_uart_receive(struct capi_uart_handle *handle,
					uint8_t *buf, uint32_t len)
{
	struct stm32_usb_uart_priv *priv;

	if (!handle || !handle->priv)
		return -EINVAL;

	priv = handle->priv;
	if (!priv->initialized)
		return -ENODEV;

	if (!buf || !len)
		return -EINVAL;

	/* A receive needs the callbacks of the USB middleware */
	(void)buf;
	(void)len;

	return -ENOSYS;
}

const struct capi_uart_ops stm32_capi_usb_uart_ops = {
	.init = stm32_capi_usb_uart_init,
	.deinit = stm32_capi_usb_uart_deinit,
	.get_line_config = NULL,
	.set_line_config = NULL,
	.enable_fifo = NULL,
	.flush_tx_fifo = NULL,
	.flush_rx_fifo = NULL,
	.get_rx_fifo_count = NULL,
	.get_tx_fifo_count = NULL,
	.transmit = stm32_capi_usb_uart_transmit,
	.receive = stm32_capi_usb_uart_receive,
	.register_callback = NULL,
	.transmit_async = NULL,
	.receive_async = NULL,
	.get_interrupt_reason = NULL,
	.get_line_status = NULL,
	.transmit_9bit = NULL,
	.receive_9bit = NULL,
	.set_flow_control_state = NULL,
	.get_flow_control_state = NULL,
	.isr = NULL,
	.read_byte = NULL,
	.write_byte = NULL,
	.set_irq_tx = NULL,
	.irq_tx_ready = NULL,
	.irq_tx_complete = NULL,
	.set_irq_rx = NULL,
	.irq_rx_ready = NULL,
	.set_irq_err = NULL,
	.is_irq_pending = NULL,
};
