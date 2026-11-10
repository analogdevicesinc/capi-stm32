/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file   stm32_capi_uart.c
 * @brief  CAPI UART backend for STM32.
 *
 * This file also provides the newlib system calls for stdio.
 */

#include "stm32_capi_uart_priv.h"
#include "capi_alloc.h"
#include <errno.h>
#include <sys/stat.h>
#include <stdio.h>

/* The porting layer passes the CAPI handle as context to the event callbacks */

static void stm32_capi_uart_on_tx_done(void *ctx)
{
	struct capi_uart_handle *handle = ctx;
	struct stm32_uart_priv_handle *priv = handle->priv;

	if (priv->callback)
		priv->callback(CAPI_UART_EVENT_TX_DONE, priv->callback_arg, 0);
}

static void stm32_capi_uart_on_rx_done(void *ctx)
{
	struct capi_uart_handle *handle = ctx;
	struct stm32_uart_priv_handle *priv = handle->priv;

	if (priv->callback)
		priv->callback(CAPI_UART_EVENT_RX_DONE, priv->callback_arg, 0);
}

static void stm32_capi_uart_on_error(void *ctx, uint32_t flags, uint32_t raw)
{
	struct capi_uart_handle *handle = ctx;
	struct stm32_uart_priv_handle *priv = handle->priv;

	if (flags & STM32_HAL_UART_ERR_PARITY)
		priv->last_line_status |= CAPI_UART_LINE_STAT_PARITY_ERROR;
	if (flags & STM32_HAL_UART_ERR_FRAMING)
		priv->last_line_status |= CAPI_UART_LINE_STAT_FRAMING_ERROR;
	if (flags & STM32_HAL_UART_ERR_OVERRUN)
		priv->last_line_status |= CAPI_UART_LINE_STAT_OVERRUN_ERROR;

	priv->last_interrupt_reason = CAPI_UART_INTR_RX_LINE_STATUS;

	if (priv->callback)
		priv->callback(CAPI_UART_EVENT_INTERRUPT, priv->callback_arg,
			       (int)raw);
}

static void stm32_capi_uart_on_abort_done(void *ctx)
{
	struct capi_uart_handle *handle = ctx;
	struct stm32_uart_priv_handle *priv = handle->priv;

	if (priv->callback)
		priv->callback(CAPI_UART_EVENT_TX_ABORTED, priv->callback_arg,
			       0);
}

static const struct stm32_hal_uart_events stm32_capi_uart_events = {
	.tx_done = stm32_capi_uart_on_tx_done,
	.rx_done = stm32_capi_uart_on_rx_done,
	.error = stm32_capi_uart_on_error,
	.abort_done = stm32_capi_uart_on_abort_done,
};

/**
 * @brief Initialize the STM32 UART peripheral.
 * @param handle - Pointer to the UART handle to allocate and initialize.
 * @param config - Pointer to the UART configuration structure.
 * @return 0 in case of success, negative error code otherwise.
 */
static int stm32_capi_uart_init(struct capi_uart_handle **handle,
				const struct capi_uart_config *config)
{
	struct capi_uart_handle *uart_handle;
	struct stm32_uart_priv_handle *uart_priv_handle;
	struct stm32_uart_extra_config *uart_extra_config;
	struct capi_uart_line_config line_config;
	struct capi_uart_line_config current_config;
	int ret = 0;

	if (!handle || !config || !config->extra || !config->line_config)
		return -EINVAL;

	if (*handle == NULL) {
		uart_handle = (struct capi_uart_handle *)capi_calloc(1, sizeof(*uart_handle));
		if (!uart_handle)
			return -ENOMEM;

		uart_priv_handle = (struct stm32_uart_priv_handle *)capi_calloc(1,
				   sizeof(*uart_priv_handle));
		if (!uart_priv_handle) {
			capi_free(uart_handle);
			return -ENOMEM;
		}

		uart_handle->priv = uart_priv_handle;
		uart_handle->init_allocated = true;
	} else {
		uart_handle = *handle;
		uart_handle->init_allocated = false;

		if (!uart_handle->priv)
			return -EINVAL;

		uart_priv_handle = uart_handle->priv;
	}
	uart_handle->ops = config->ops;

	uart_extra_config = config->extra;

	/* Use the handle, or call the getter that the board names */
	uart_priv_handle->huart = uart_extra_config->huart;
	if (!uart_priv_handle->huart && uart_extra_config->huart_get)
		uart_priv_handle->huart = uart_extra_config->huart_get();

	if (!uart_priv_handle->huart) {
		ret = -EINVAL;
		goto error;
	}

	/*
	 * Init does not change flow control, Mode or OverSampling.
	 * They keep the values of the generated init.
	 * Only set_line_config() changes the flow control.
	 */
	line_config = *config->line_config;
	ret = stm32_hal_uart_get_line_config(uart_priv_handle->huart,
					     &current_config);
	if (ret)
		goto error;

	line_config.flow_control = current_config.flow_control;

	ret = stm32_hal_uart_set_line_config(uart_priv_handle->huart,
					     &line_config);
	if (ret)
		goto error;

	*handle = uart_handle;

	stm32_hal_uart_set_events(uart_priv_handle->huart,
				  &stm32_capi_uart_events, uart_handle);

	return 0;
error:
	if (uart_handle->init_allocated) {
		capi_free(uart_priv_handle);
		capi_free(uart_handle);
	}

	return ret;
}

/**
 * @brief Deinitialize the STM32 UART peripheral and free the handle.
 * @param handle - Pointer to the UART handle.
 * @return 0 in case of success, negative error code otherwise.
 */
static int stm32_capi_uart_deinit(struct capi_uart_handle *handle)
{
	struct stm32_uart_priv_handle *uart_priv_handle;

	if (!handle)
		return -EINVAL;

	uart_priv_handle = handle->priv;

	if (uart_priv_handle) {
		stm32_hal_uart_set_events(uart_priv_handle->huart, NULL, NULL);
		stm32_hal_uart_deinit(uart_priv_handle->huart);
	}

	if (handle->init_allocated) {
		capi_free(uart_priv_handle);
		capi_free(handle);
	}

	return 0;
}

/**
 * @brief Get the current UART line configuration.
 * @param handle - Pointer to the UART handle.
 * @param line_config - Pointer to the structure to fill.
 * @return 0 in case of success, negative error code otherwise.
 */
static int stm32_capi_uart_get_line_config(struct capi_uart_handle *handle,
		struct capi_uart_line_config *line_config)
{
	struct stm32_uart_priv_handle *priv;
	int ret;

	if (!handle || !handle->priv || !line_config)
		return -EINVAL;

	priv = handle->priv;

	ret = stm32_hal_uart_get_line_config(priv->huart, line_config);
	if (ret)
		return ret;

	/* Report the settings that the STM32 UART does not have as inactive */
	line_config->address_mode = CAPI_UART_ADDRESS_MODE_DISABLED;
	line_config->device_address = 0;
	line_config->sticky_parity = false;
	line_config->loopback = false;

	return 0;
}

/**
 * @brief Set the UART line configuration.
 * @param handle - Pointer to the UART handle.
 * @param line_config - Pointer to the new line configuration.
 * @return 0 in case of success, negative error code otherwise.
 */
static int stm32_capi_uart_set_line_config(struct capi_uart_handle *handle,
		struct capi_uart_line_config *line_config)
{
	struct stm32_uart_priv_handle *priv;

	if (!handle || !handle->priv || !line_config)
		return -EINVAL;

	priv = handle->priv;

	return stm32_hal_uart_set_line_config(priv->huart, line_config);
}

/**
 * @brief Transmit data and wait until it is sent.
 * @param handle - Pointer to the UART handle.
 * @param buf - Pointer to the data buffer to transmit.
 * @param len - Number of bytes to transmit.
 * @return 0 in case of success, negative error code otherwise.
 */
static int stm32_capi_uart_transmit(struct capi_uart_handle *handle,
				    const uint8_t *buf, uint32_t len)
{
	struct stm32_uart_priv_handle *uart_priv_handle;

	if (!handle || !handle->priv || !buf)
		return -EINVAL;

	if (!len)
		return 0;

	uart_priv_handle = handle->priv;

	return stm32_hal_uart_write(uart_priv_handle->huart, buf, len,
				    STM32_HAL_WAIT_FOREVER);
}

/**
 * @brief Receive data and wait until it arrives.
 * @param handle - Pointer to the UART handle.
 * @param buf - Pointer to the buffer to store received data.
 * @param len - Number of bytes to receive.
 * @return 0 in case of success, negative error code otherwise.
 */
static int stm32_capi_uart_receive(struct capi_uart_handle *handle,
				   uint8_t *buf, uint32_t len)
{
	struct stm32_uart_priv_handle *uart_priv_handle;

	if (!handle || !handle->priv || !buf)
		return -EINVAL;

	if (!len)
		return 0;

	uart_priv_handle = handle->priv;

	return stm32_hal_uart_read(uart_priv_handle->huart, buf, len,
				   STM32_HAL_WAIT_FOREVER);
}

/**
 * @brief Register a callback for the UART async events.
 * @param handle - Pointer to the UART handle.
 * @param callback - Callback function.
 * @param callback_arg - Argument for the callback.
 * @return 0 in case of success, negative error code otherwise.
 */
static int stm32_capi_uart_register_callback(struct capi_uart_handle *handle,
		capi_uart_callback const callback,
		void *const callback_arg)
{
	struct stm32_uart_priv_handle *priv;

	if (!handle || !handle->priv)
		return -EINVAL;

	priv = handle->priv;
	priv->callback = callback;
	priv->callback_arg = callback_arg;

	return 0;
}

/**
 * @brief Start an interrupt-driven transmit.
 * @param handle - Pointer to the UART handle.
 * @param buf - Pointer to the data buffer to transmit.
 * @param len - Number of bytes to transmit.
 * @return 0 in case of success, negative error code otherwise.
 */
static int stm32_capi_uart_transmit_async(struct capi_uart_handle *handle,
		const uint8_t *buf, uint32_t len)
{
	struct stm32_uart_priv_handle *priv;

	if (!handle || !handle->priv || !buf)
		return -EINVAL;

	if (!len)
		return 0;

	priv = handle->priv;

	return stm32_hal_uart_write_async(priv->huart, buf, len);
}

/**
 * @brief Start an interrupt-driven receive.
 * @param handle - Pointer to the UART handle.
 * @param buf - Pointer to the buffer for incoming data.
 * @param len - Number of bytes to receive.
 * @return 0 in case of success, negative error code otherwise.
 */
static int stm32_capi_uart_receive_async(struct capi_uart_handle *handle,
		uint8_t *buf, uint32_t len)
{
	struct stm32_uart_priv_handle *priv;

	if (!handle || !handle->priv || !buf)
		return -EINVAL;

	if (!len)
		return 0;

	priv = handle->priv;

	return stm32_hal_uart_read_async(priv->huart, buf, len);
}

/**
 * @brief Get the reason of the last UART interrupt.
 * @param handle - Pointer to the UART handle.
 * @param reason - Pointer to the interrupt reason.
 * @return 0 in case of success, negative error code otherwise.
 */
static int stm32_capi_uart_get_interrupt_reason(
	struct capi_uart_handle *handle,
	enum capi_uart_interrupt_reason *reason)
{
	struct stm32_uart_priv_handle *priv;

	if (!handle || !handle->priv || !reason)
		return -EINVAL;

	priv = handle->priv;
	*reason = priv->last_interrupt_reason;

	return 0;
}

/**
 * @brief Get the line status flags. A read clears them.
 * @param handle - Pointer to the UART handle.
 * @param status_flags - Pointer to a bitmask of capi_uart_line_status_flags.
 * @return 0 in case of success, negative error code otherwise.
 */
static int stm32_capi_uart_get_line_status(struct capi_uart_handle *handle,
		uint32_t *status_flags)
{
	struct stm32_uart_priv_handle *priv;

	if (!handle || !handle->priv || !status_flags)
		return -EINVAL;

	priv = handle->priv;
	*status_flags = priv->last_line_status;
	priv->last_line_status = 0;

	return 0;
}

/**
 * @brief UART ISR entry point. It calls the porting layer.
 * @param handle - Pointer to the UART handle.
 */
static void stm32_capi_uart_isr(void *handle)
{
	struct capi_uart_handle *uart_handle = (struct capi_uart_handle *)handle;
	struct stm32_uart_priv_handle *priv;

	if (!uart_handle || !uart_handle->priv)
		return;

	priv = uart_handle->priv;

	stm32_hal_uart_irq_handler(priv->huart);
}

const struct capi_uart_ops stm32_capi_uart_ops = {
	.init = stm32_capi_uart_init,
	.deinit = stm32_capi_uart_deinit,
	.get_line_config = stm32_capi_uart_get_line_config,
	.set_line_config = stm32_capi_uart_set_line_config,
	.transmit = stm32_capi_uart_transmit,
	.receive = stm32_capi_uart_receive,
	.register_callback = stm32_capi_uart_register_callback,
	.transmit_async = stm32_capi_uart_transmit_async,
	.receive_async = stm32_capi_uart_receive_async,
	.get_interrupt_reason = stm32_capi_uart_get_interrupt_reason,
	.get_line_status = stm32_capi_uart_get_line_status,
	.isr = stm32_capi_uart_isr,
};

static struct capi_uart_handle *uart_stdio_handle = NULL;

/**
 * @brief Send stdout and read stdin through the STM32 UART.
 * @param handle - Pointer to an initialized UART handle.
 * @return 0 in case of success, negative error code otherwise.
 */
int stm32_uart_stdio_enable(struct capi_uart_handle *handle)
{
	if (!handle)
		return -EINVAL;

	uart_stdio_handle = handle;

	/* Disable stdout buffering to send each character at once */
	setvbuf(stdout, NULL, _IONBF, 0);

	return 0;
}

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

/**
 * @brief Check if a file descriptor refers to a terminal.
 * @param fd - File descriptor to check.
 * @return 1 if the descriptor is a terminal, 0 otherwise.
 */
int _isatty(int fd)
{
	if (fd >= STDIN_FILENO && fd <= STDERR_FILENO)
		return 1;

	errno = EBADF;
	return 0;
}

/**
 * @brief Write data to a file descriptor.
 * @param fd - File descriptor to write to.
 * @param ptr - Pointer to the data buffer.
 * @param len - Number of bytes to write.
 * @return Number of bytes written on success, -1 on error.
 */
int _write(int fd, char *ptr, int len)
{
	int ret;

	if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
		ret = stm32_capi_uart_transmit(uart_stdio_handle, (uint8_t *)ptr, len);
		if (ret < 0) {
			errno = -ret;
			return -1;
		}

		return len;
	}
	errno = EBADF;
	return -1;
}

/**
 * @brief Close a file descriptor.
 * @param fd - File descriptor to close.
 * @return 0 on success, -1 on error.
 */
int _close(int fd)
{
	if (fd >= STDIN_FILENO && fd <= STDERR_FILENO)
		return 0;

	errno = EBADF;
	return -1;
}

/**
 * @brief Reposition file descriptor offset.
 * @param fd - File descriptor.
 * @param ptr - Offset value.
 * @param dir - Seek direction.
 * @return -1 always, seeking is not supported.
 */
int _lseek(int fd, int ptr, int dir)
{
	(void) fd;
	(void) ptr;
	(void) dir;

	errno = EBADF;
	return -1;
}

/**
 * @brief Read data from a file descriptor.
 * @param fd - File descriptor to read from.
 * @param ptr - Pointer to the buffer for incoming data.
 * @param len - Number of bytes to read.
 * @return Number of bytes read on success, -1 on error.
 */
int _read(int fd, char *ptr, int len)
{
	int ret;

	if (fd == STDIN_FILENO) {
		ret = stm32_capi_uart_receive(uart_stdio_handle, (uint8_t *)ptr, 1);
		if (ret < 0) {
			errno = -ret;
			return -1;
		}

		return 1;
	}
	errno = EBADF;
	return -1;
}

/**
 * @brief Get file status.
 * @param fd - File descriptor.
 * @param st - Pointer to the stat structure to fill.
 * @return 0 on success, -1 on error.
 */
int _fstat(int fd, struct stat *st)
{
	if (fd >= STDIN_FILENO && fd <= STDERR_FILENO) {
		st->st_mode = S_IFCHR;
		return 0;
	}

	errno = EBADF;
	return 0;
}
