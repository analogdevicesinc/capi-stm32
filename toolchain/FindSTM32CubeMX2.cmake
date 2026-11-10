# Copyright (c) 2026 Analog Devices, Inc.
# SPDX-License-Identifier: BSD-3-Clause

# Find the CLI of STM32CubeMX2.
# STM32CubeMX2 is a separate tool, not a new version of STM32CubeMX. It has
# another project format (.ioc2 is JSON, .ioc is INI), another generator and
# another device database. STM32C5 boards need it. The build finds both tools
# independently.
# Always use the `cube` wrapper. Do not start bin.js or the Electron binary of
# the bundle, because they need the environment that the wrapper sets.
# Sets STM32CUBEMX2_CUBE_EXECUTABLE and STM32CUBEMX2_FOUND.

find_program(
  STM32CUBEMX2_CUBE_EXECUTABLE
  NAMES cube
  HINTS
    "$ENV{STM32CUBEMX2}"
    "$ENV{HOME}/.local/stm32cube/bin"
    "$ENV{HOME}/st/STM32CubeMX2/.bin"
    "$ENV{HOME}/STM32CubeMX2/.bin"
    "/opt/st/STM32CubeMX2/.bin"
  PATH_SUFFIXES
    "bin"
    ".bin"
    ""
  DOC "STM32CubeMX2 'cube' CLI wrapper"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  STM32CubeMX2
  FOUND_VAR STM32CUBEMX2_FOUND
  REQUIRED_VARS STM32CUBEMX2_CUBE_EXECUTABLE
)
