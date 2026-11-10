/*
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef STM32_HAL_PORT_H_
#define STM32_HAL_PORT_H_

/**
 * @file   stm32_hal_port.h
 * @brief  Porting layer of capi-stm32 over the two ST HAL generations.
 *
 * ST has two HAL APIs that do not match. The classic HAL (STM32 families up to
 * H5) uses handle structs with an Init member, weak global callbacks and
 * __HAL_RCC_* macros. The new HAL (STM32C5) uses opaque handles,
 * Set/GetConfig calls, per-handle callbacks and RCC functions.
 *
 * Both HALs have functions with the same name and a different signature, for
 * example HAL_GPIO_Init and HAL_UART_Init. A shim cannot reuse these names.
 * A macro rename would break the generated mx_*.c files, which call them.
 *
 * So this layer has its own names. Each stm32_capi_*.c driver uses only this
 * header, and compiles with either HAL. The implementations are in
 * platform/hal1/ and platform/hal2/. The build selects one with
 * CONFIG_STM32_HAL2 (see CMakeLists.txt).
 *
 * Add a function here only if the two HALs differ for it. The drivers call
 * the common parts directly: CMSIS NVIC, HAL_Delay, HAL_GetTick and the
 * HAL_OK, HAL_ERROR, HAL_BUSY and HAL_TIMEOUT codes. Each new function needs
 * two implementations that stay in agreement.
 */

#include <stdbool.h>
#include <stdint.h>

#include "stm32_capi_hal.h"
#include "capi_spi.h"
#include "capi_uart.h"

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/** @brief Timeout value that waits without limit, for any blocking transfer. */
#define STM32_HAL_WAIT_FOREVER		0xFFFFFFFFU

/*
 * Peripherals that the HAL of this build provides
 *
 * A HAL does not compile every peripheral. The classic HAL sets
 * HAL_<p>_MODULE_ENABLED in the hal_conf.h of the board. The new HAL always
 * sets USE_HAL_<p>_MODULE to 1 or 0. CubeMX2 takes the value from the
 * components of the .ioc2. A board with a console UART only has no
 * hal_spi_handle_t.
 *
 * Each section below depends on its flag, so the header parses in any file.
 * A driver for a missing peripheral does not build. CONFIG_<P>_STM32 and the
 * HAL module must agree.
 */
#ifdef CONFIG_STM32_HAL2
#define STM32_HAL_PORT_HAS_GPIO		USE_HAL_GPIO_MODULE
#define STM32_HAL_PORT_HAS_UART		USE_HAL_UART_MODULE
#define STM32_HAL_PORT_HAS_SPI		USE_HAL_SPI_MODULE
#define STM32_HAL_PORT_HAS_I2C		USE_HAL_I2C_MODULE
#else
#ifdef HAL_GPIO_MODULE_ENABLED
#define STM32_HAL_PORT_HAS_GPIO		1
#else
#define STM32_HAL_PORT_HAS_GPIO		0
#endif
#ifdef HAL_UART_MODULE_ENABLED
#define STM32_HAL_PORT_HAS_UART		1
#else
#define STM32_HAL_PORT_HAS_UART		0
#endif
#ifdef HAL_SPI_MODULE_ENABLED
#define STM32_HAL_PORT_HAS_SPI		1
#else
#define STM32_HAL_PORT_HAS_SPI		0
#endif
#ifdef HAL_I2C_MODULE_ENABLED
#define STM32_HAL_PORT_HAS_I2C		1
#else
#define STM32_HAL_PORT_HAS_I2C		0
#endif
#endif /* CONFIG_STM32_HAL2 */

/* GPIO */

#if STM32_HAL_PORT_HAS_GPIO

/**
 * @brief GPIO port, in the form that the active HAL uses.
 *
 * The classic HAL uses a pointer to the register block. The new HAL uses
 * hal_gpio_t, an enum with the same base addresses as values. You can convert
 * one to the other, but you cannot assign one to the other.
 *
 * Drivers must not dereference a port. Pass it to the functions below. They
 * replace direct access to port->MODER and port->ODR.
 */
#ifdef CONFIG_STM32_HAL2
typedef hal_gpio_t stm32_hal_gpio_port_t;
#else
typedef GPIO_TypeDef *stm32_hal_gpio_port_t;
#endif

/**
 * @brief Name a GPIO port by its letter. STM32_HAL_GPIO_PORT(B) is port B.
 *
 * The HALs spell a port differently: GPIOB is a pointer and HAL_GPIOB is an
 * enumerator. A BSP that uses one spelling builds with one HAL only. This macro
 * takes the letter and builds with both.
 *
 * In both cases the value is the port base address. A cast to an integer
 * identifier gives the same result.
 */
#ifdef CONFIG_STM32_HAL2
#define STM32_HAL_GPIO_PORT(letter)	HAL_GPIO##letter
#else
#define STM32_HAL_GPIO_PORT(letter)	GPIO##letter
#endif

/** @brief Pin direction and drive. The CAPI GPIO operations need only these. */
enum stm32_hal_gpio_mode {
	/** Digital input, no pull resistor. */
	STM32_HAL_GPIO_INPUT,
	/** Push-pull output, low speed. */
	STM32_HAL_GPIO_OUTPUT_PP,
};

/**
 * @brief Enable the clock of a GPIO port.
 * @param port - The port.
 * @return 0 in case of success, -EINVAL if the MCU has no such port.
 */
int stm32_hal_gpio_clk_enable(stm32_hal_gpio_port_t port);

/**
 * @brief Set the same mode on one or more pins of a port.
 * @param port - The port.
 * @param pins - Bitmask of pins (bit n is pin n).
 * @param mode - Direction and drive to apply.
 * @return 0 in case of success, negative error code otherwise.
 */
int stm32_hal_gpio_set_mode(stm32_hal_gpio_port_t port, uint16_t pins,
			    enum stm32_hal_gpio_mode mode);

/**
 * @brief Get the pins of a port that are inputs.
 *
 * Pins in output, alternate-function or analog mode read as 0.
 *
 * @param port - The port.
 * @return Bitmask with a 1 for each input pin.
 */
uint16_t stm32_hal_gpio_get_input_mask(stm32_hal_gpio_port_t port);

/**
 * @brief Drive the selected pins high or low. Other pins do not change.
 * @param port - The port.
 * @param pins - Bitmask of pins to drive.
 * @param set - true to drive high, false to drive low.
 */
void stm32_hal_gpio_write_pins(stm32_hal_gpio_port_t port, uint16_t pins,
			       bool set);

/**
 * @brief Read the input level of the selected pins.
 * @param port - The port.
 * @param pins - Bitmask of pins to read.
 * @return true if any selected pin is high.
 */
bool stm32_hal_gpio_read_pins(stm32_hal_gpio_port_t port, uint16_t pins);

/**
 * @brief Invert the output level of the selected pins.
 * @param port - The port.
 * @param pins - Bitmask of pins to toggle.
 */
void stm32_hal_gpio_toggle_pins(stm32_hal_gpio_port_t port, uint16_t pins);

/**
 * @brief Drive the 16 output pins of a port in one write.
 *
 * This is not a read-modify-write. Every pin takes its level from @p value.
 *
 * @param port - The port.
 * @param value - Bit n is the level of pin n.
 */
void stm32_hal_gpio_write_port(stm32_hal_gpio_port_t port, uint16_t value);

/**
 * @brief Read the 16 input pins of a port in one read.
 * @param port - The port.
 * @return Bit n is the level of pin n.
 */
uint16_t stm32_hal_gpio_read_port(stm32_hal_gpio_port_t port);

#endif /* STM32_HAL_PORT_HAS_GPIO */

/* UART */

#if STM32_HAL_PORT_HAS_UART

/**
 * @brief UART handle, in the form that the active HAL uses.
 *
 * The generated init code owns this object, and a BSP points to it
 * (`.huart = &huart3`). Drivers use it only through a pointer.
 * Drivers must not read its members. The Init member of the classic HAL has no
 * equivalent in the new HAL, which has only Get/SetConfig.
 */
#ifdef CONFIG_STM32_HAL2
typedef hal_uart_handle_t stm32_hal_uart_t;
#else
typedef UART_HandleTypeDef stm32_hal_uart_t;
#endif

/** @name Line error flags for stm32_hal_uart_events::error. @{ */
#define STM32_HAL_UART_ERR_PARITY	(1U << 0)
#define STM32_HAL_UART_ERR_FRAMING	(1U << 1)
#define STM32_HAL_UART_ERR_OVERRUN	(1U << 2)
/** @} */

/**
 * @brief Asynchronous UART events. The layer calls them from interrupt context.
 *
 * The classic HAL uses weak global functions shared by all UARTs. Its
 * implementation of this layer keeps a table that maps handles to contexts.
 * The new HAL registers the callbacks for each handle. The drivers see the
 * same four events with both HALs.
 *
 * The layer does not call a member that is NULL.
 */
struct stm32_hal_uart_events {
	/** An asynchronous write finished. */
	void (*tx_done)(void *ctx);
	/** An asynchronous read finished. */
	void (*rx_done)(void *ctx);
	/**
	 * A line error occurred.
	 * @param flags - Bitmask of STM32_HAL_UART_ERR_*, the same for both HALs.
	 * @param raw - Error word of the HAL, unchanged. It has more detail.
	 */
	void (*error)(void *ctx, uint32_t flags, uint32_t raw);
	/** An abort finished. */
	void (*abort_done)(void *ctx);
};

/**
 * @brief Set the event callbacks of one UART and the context for them.
 * @param huart - The UART handle.
 * @param events - Callback set. The layer keeps a pointer to it, so use a
 *                 static one. NULL removes the callbacks.
 * @param ctx - Pointer that the layer passes to each callback.
 * @return 0 in case of success, negative error code otherwise.
 */
int stm32_hal_uart_set_events(stm32_hal_uart_t *huart,
			      const struct stm32_hal_uart_events *events,
			      void *ctx);

/**
 * @brief Apply a line configuration and initialize the peripheral again.
 *
 * This is a read-modify-write. Settings that CAPI does not have keep the value
 * of the generated init: direction, oversampling, prescaler, FIFO thresholds.
 *
 * @param huart - The UART handle.
 * @param cfg - Baud rate, word length, parity, stop bits and flow control.
 * @return 0 in case of success, -EINVAL for a setting that the MCU cannot
 *         make, negative error code otherwise.
 */
int stm32_hal_uart_set_line_config(stm32_hal_uart_t *huart,
				   const struct capi_uart_line_config *cfg);

/**
 * @brief Read the current line configuration of the peripheral.
 *
 * The function does not report settings that CAPI does not have. It reports a
 * word length or parity that CAPI cannot name as 8 bits or none. It does not
 * fail for them.
 *
 * @param huart - The UART handle.
 * @param cfg - Pointer to the configuration. Set on success.
 * @return 0 in case of success, negative error code otherwise.
 */
int stm32_hal_uart_get_line_config(stm32_hal_uart_t *huart,
				   struct capi_uart_line_config *cfg);

/**
 * @brief Release the peripheral.
 * @param huart - The UART handle.
 * @return 0 in case of success, negative error code otherwise.
 */
int stm32_hal_uart_deinit(stm32_hal_uart_t *huart);

/**
 * @brief Transmit and wait until it is done or the timeout expires.
 * @param huart - The UART handle.
 * @param buf - Data to send.
 * @param len - Number of bytes.
 * @param timeout_ms - Milliseconds to wait, or STM32_HAL_WAIT_FOREVER.
 * @return 0 in case of success, -EBUSY, -ETIMEDOUT or -EIO.
 */
int stm32_hal_uart_write(stm32_hal_uart_t *huart, const uint8_t *buf, uint32_t len,
			 uint32_t timeout_ms);

/**
 * @brief Receive and wait until it is done or the timeout expires.
 * @param huart - The UART handle.
 * @param buf - Buffer for the data.
 * @param len - Number of bytes.
 * @param timeout_ms - Milliseconds to wait, or STM32_HAL_WAIT_FOREVER.
 * @return 0 in case of success, -EBUSY, -ETIMEDOUT or -EIO.
 */
int stm32_hal_uart_read(stm32_hal_uart_t *huart, uint8_t *buf, uint32_t len,
			uint32_t timeout_ms);

/**
 * @brief Start an interrupt-driven transmit. The tx_done event ends it.
 * @param huart - The UART handle.
 * @param buf - Data to send. It must stay valid until the event.
 * @param len - Number of bytes.
 * @return 0 in case of success, -EBUSY or -EIO.
 */
int stm32_hal_uart_write_async(stm32_hal_uart_t *huart, const uint8_t *buf,
			       uint32_t len);

/**
 * @brief Start an interrupt-driven receive. The rx_done event ends it.
 * @param huart - The UART handle.
 * @param buf - Buffer for the data. It must stay valid until the event.
 * @param len - Number of bytes.
 * @return 0 in case of success, -EBUSY or -EIO.
 */
int stm32_hal_uart_read_async(stm32_hal_uart_t *huart, uint8_t *buf,
			      uint32_t len);

/**
 * @brief Handle a UART interrupt. It calls the event callbacks.
 * @param huart - The UART handle of the interrupt.
 */
void stm32_hal_uart_irq_handler(stm32_hal_uart_t *huart);

#endif /* STM32_HAL_PORT_HAS_UART */

/* SPI */

#if STM32_HAL_PORT_HAS_SPI

/**
 * @brief SPI handle, in the form that the active HAL uses.
 *
 * Drivers use it only through a pointer, as for stm32_hal_uart_t.
 */
#ifdef CONFIG_STM32_HAL2
typedef hal_spi_handle_t stm32_hal_spi_t;
#else
typedef SPI_HandleTypeDef stm32_hal_spi_t;
#endif

/**
 * @brief Initialize the peripheral with the settings of its handle.
 *
 * The CAPI SPI configuration has nothing to apply here. The function keeps
 * the settings of the generated init. The new HAL only checks that it ran.
 *
 * @param hspi - The SPI handle.
 * @return 0 in case of success, negative error code otherwise.
 */
int stm32_hal_spi_init(stm32_hal_spi_t *hspi);

/**
 * @brief Apply the clock format of one device to a shared controller.
 *
 * Devices on one bus can need different CPOL/CPHA pairs, and the controller
 * holds one pair. The generated init sets the pair of the .ioc.
 *
 * The transceive path calls this function only when the format differs from
 * the loaded one. CS is deasserted and the peripheral is idle at that time.
 * Neither HAL can change the format during a transaction.
 *
 * @param hspi - The SPI handle.
 * @param mode - CPOL/CPHA, as the CAPI enum.
 * @param speed_hz - Bit rate to apply. 0 keeps the current rate.
 * @return 0 in case of success, negative error code otherwise.
 */
int stm32_hal_spi_set_format(stm32_hal_spi_t *hspi, enum capi_spi_mode mode,
			     uint32_t speed_hz);

/**
 * @brief Release the peripheral.
 * @param hspi - The SPI handle.
 * @return 0 in case of success, negative error code otherwise.
 */
int stm32_hal_spi_deinit(stm32_hal_spi_t *hspi);

/**
 * @brief Full-duplex transfer. Wait until it is done or the timeout expires.
 * @param hspi - The SPI handle.
 * @param tx - Data to send.
 * @param rx - Buffer for the received data.
 * @param len - Number of bytes, the same in both directions.
 * @param timeout_ms - Milliseconds to wait, or STM32_HAL_WAIT_FOREVER.
 * @return 0 in case of success, -EBUSY, -ETIMEDOUT or -EIO.
 */
int stm32_hal_spi_write_read(stm32_hal_spi_t *hspi, uint8_t *tx, uint8_t *rx,
			     uint32_t len, uint32_t timeout_ms);

/**
 * @brief Transmit only. Wait until it is done or the timeout expires.
 * @param hspi - The SPI handle.
 * @param tx - Data to send.
 * @param len - Number of bytes.
 * @param timeout_ms - Milliseconds to wait, or STM32_HAL_WAIT_FOREVER.
 * @return 0 in case of success, -EBUSY, -ETIMEDOUT or -EIO.
 */
int stm32_hal_spi_write(stm32_hal_spi_t *hspi, uint8_t *tx, uint32_t len,
			uint32_t timeout_ms);

/**
 * @brief Receive only. Wait until it is done or the timeout expires.
 * @param hspi - The SPI handle.
 * @param rx - Buffer for the data.
 * @param len - Number of bytes.
 * @param timeout_ms - Milliseconds to wait, or STM32_HAL_WAIT_FOREVER.
 * @return 0 in case of success, -EBUSY, -ETIMEDOUT or -EIO.
 */
int stm32_hal_spi_read(stm32_hal_spi_t *hspi, uint8_t *rx, uint32_t len,
		       uint32_t timeout_ms);

#endif /* STM32_HAL_PORT_HAS_SPI */

/* I2C */

#if STM32_HAL_PORT_HAS_I2C

/**
 * @brief I2C handle, in the form that the active HAL uses.
 *
 * Drivers use it only through a pointer, as for stm32_hal_uart_t.
 */
#ifdef CONFIG_STM32_HAL2
typedef hal_i2c_handle_t stm32_hal_i2c_t;
#else
typedef I2C_HandleTypeDef stm32_hal_i2c_t;
#endif

/**
 * @brief Initialize the peripheral with the settings of its handle.
 *
 * The new HAL only checks that the generated init ran.
 *
 * @param hi2c - The I2C handle.
 * @return 0 in case of success, negative error code otherwise.
 */
int stm32_hal_i2c_init(stm32_hal_i2c_t *hi2c);

/**
 * @brief Release the peripheral.
 * @param hi2c - The I2C handle.
 * @return 0 in case of success, negative error code otherwise.
 */
int stm32_hal_i2c_deinit(stm32_hal_i2c_t *hi2c);

/**
 * @brief Write to a target. Wait until it is done or the timeout expires.
 *
 * @param hi2c - The I2C handle.
 * @param addr - 7-bit target address. The layer converts it to the HAL form.
 * @param buf - Data to send.
 * @param len - Number of bytes.
 * @param timeout_ms - Milliseconds to wait, or STM32_HAL_WAIT_FOREVER.
 * @return 0 in case of success, -EBUSY, -ETIMEDOUT or -EIO.
 */
int stm32_hal_i2c_write(stm32_hal_i2c_t *hi2c, uint16_t addr, uint8_t *buf,
			uint32_t len, uint32_t timeout_ms);

/**
 * @brief Read from a target. Wait until it is done or the timeout expires.
 * @param hi2c - The I2C handle.
 * @param addr - 7-bit target address.
 * @param buf - Buffer for the data.
 * @param len - Number of bytes.
 * @param timeout_ms - Milliseconds to wait, or STM32_HAL_WAIT_FOREVER.
 * @return 0 in case of success, -EBUSY, -ETIMEDOUT or -EIO.
 */
int stm32_hal_i2c_read(stm32_hal_i2c_t *hi2c, uint16_t addr, uint8_t *buf,
		       uint32_t len, uint32_t timeout_ms);

/**
 * @brief Write to a register of a target (memory access).
 * @param hi2c - The I2C handle.
 * @param addr - 7-bit target address.
 * @param mem_addr - Register address in the target.
 * @param mem_addr_len - Width of @p mem_addr in bytes: 1 or 2.
 * @param buf - Data to send.
 * @param len - Number of bytes.
 * @param timeout_ms - Milliseconds to wait, or STM32_HAL_WAIT_FOREVER.
 * @return 0 in case of success, -EINVAL for an unsupported @p mem_addr_len,
 *         -EBUSY, -ETIMEDOUT or -EIO.
 */
int stm32_hal_i2c_mem_write(stm32_hal_i2c_t *hi2c, uint16_t addr,
			    uint16_t mem_addr, uint8_t mem_addr_len,
			    uint8_t *buf, uint32_t len, uint32_t timeout_ms);

/**
 * @brief Read a register of a target (memory access).
 * @param hi2c - The I2C handle.
 * @param addr - 7-bit target address.
 * @param mem_addr - Register address in the target.
 * @param mem_addr_len - Width of @p mem_addr in bytes: 1 or 2.
 * @param buf - Buffer for the data.
 * @param len - Number of bytes.
 * @param timeout_ms - Milliseconds to wait, or STM32_HAL_WAIT_FOREVER.
 * @return 0 in case of success, -EINVAL for an unsupported @p mem_addr_len,
 *         -EBUSY, -ETIMEDOUT or -EIO.
 */
int stm32_hal_i2c_mem_read(stm32_hal_i2c_t *hi2c, uint16_t addr,
			   uint16_t mem_addr, uint8_t mem_addr_len,
			   uint8_t *buf, uint32_t len, uint32_t timeout_ms);

#endif /* STM32_HAL_PORT_HAS_I2C */

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* STM32_HAL_PORT_H_ */
