# capi-stm32

The STM32 platform for [CAPI](https://github.com/analogdevicesinc/capi).
It implements the CAPI interfaces on the STM32 HAL.

## CAPI setup

You need CMake 3.27+, Ninja, Python 3 and network access. Clone capi, this
repo and an application into one workspace:

```bash
mkdir ~/workspace && cd ~/workspace
git clone https://github.com/analogdevicesinc/capi
git clone https://github.com/analogdevicesinc/capi-stm32
git clone https://github.com/analogdevicesinc/capi-hello-world
```

```bash
export CAPI_WORKSPACE=~/workspace
export CAPI_DIR=~/workspace/capi
```

The application uses this checkout. To use a checkout in a different
directory, set `CAPI_STM32_DIR`.

## Platform setup

Install STM32CubeIDE and STM32CubeMX. NUCLEO-C5A3ZG also needs STM32CubeMX2.
If they are not in the default paths, set `STM32CUBEIDE`, `STM32CUBEMX` or
`STM32CUBEMX2`. Without STM32CubeIDE, install `arm-none-eabi-gcc` with newlib
and add `-DUSE_VENDOR_TOOLCHAIN=OFF` at configure.

## Build and flash

Run in `~/workspace/capi-hello-world`:

```bash
cmake --preset <board>
cmake --build build-stm32/<board> --target flash
```

| Board | MCU | Generator |
|---|---|---|
| `nucleo-h753zi` | STM32H753ZI | STM32CubeMX |
| `nucleo-c5a3zg` | STM32C5A3ZG | STM32CubeMX2 |
| `eval-adin1110ebz` | STM32L4S5QII | STM32CubeMX |

## Notes

- NUCLEO-C5A3ZG flashing needs the `0x45A.stldr` loader of STM32CubeIDE, or
  `-DSTM32_OPENOCD_STLDR=<path>`.
- If STM32CubeMX2 stops with `Project Initialization error`, stop the old
  STM32CubeMX2 processes and run again.
- With more than one ST-LINK, set `-DSTM32_OPENOCD_ADAPTER_SERIAL=<serial>`.
- NUCLEO-C5A3ZG has no DMA and no GPIO IRQ backend yet.
- EVAL-ADIN1110EBZ needs the STM32Cube FW_L4 V1.18.2 package in STM32CubeMX.
  Its console is USART1 on the board's FT232R USB UART. It has no on-board
  ST-LINK: connect one to the SWD header. The default OpenOCD interface,
  `interface/stlink-dap.cfg`, works with STM32CubeIDE's OpenOCD;
  `interface/stlink.cfg` does not. The ADIN1110 is on SPI2 at 25 MHz (PCLK1
  100 MHz and the .ioc's prescaler 4), with CS_N on PB12, RESET_N on PC7 and
  INT_N on PB11. PC8/PC9 read the SPI_CFG0/SPI_CFG1 straps, which select
  OPEN Alliance (SPI_CFG1 = 0) or generic SPI (SPI_CFG1 = 1); the ADIN1110
  driver in [no-OS-adin](https://github.com/analogdevicesinc/no-OS-adin)
  supports both.

## License

BSD-3-Clause.
