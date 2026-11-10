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

## Notes

- NUCLEO-C5A3ZG flashing needs the `0x45A.stldr` loader of STM32CubeIDE, or
  `-DSTM32_OPENOCD_STLDR=<path>`.
- If STM32CubeMX2 stops with `Project Initialization error`, stop the old
  STM32CubeMX2 processes and run again.
- With more than one ST-LINK, set `-DSTM32_OPENOCD_ADAPTER_SERIAL=<serial>`.
- NUCLEO-C5A3ZG has no DMA and no GPIO IRQ backend yet.

## License

BSD-3-Clause.
