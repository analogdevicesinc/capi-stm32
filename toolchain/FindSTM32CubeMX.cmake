# Copyright (c) 2026 Analog Devices, Inc.
# SPDX-License-Identifier: BSD-3-Clause

find_program(
  STM32CUBEMX_EXECUTABLE
  NAMES STM32CubeMX stm32cubemx
  HINTS
    "$ENV{STM32CUBEMX}"
  PATHS
    "/usr/local/bin"
    "/usr/bin"
    "/opt/st"
    "/opt/STMicroelectronics"
    "/usr/local/STMicroelectronics"
    "$ENV{HOME}/st"
    "$ENV{HOME}/STMicroelectronics"
    "$ENV{HOME}/STM32CubeMX"
    "C:/ST"
    "$ENV{ProgramFiles}/STMicroelectronics"
    "$ENV{ProgramFiles\(x86\)}/STMicroelectronics"
    "$ENV{ProgramFiles}/STMicroelectronics/STM32Cube/STM32CubeMX"
    "/opt/STMicroelectronics/STM32Cube/STM32CubeMX"
    "/usr/local/STMicroelectronics/STM32Cube/STM32CubeMX"
    "$ENV{HOME}/STMicroelectronics/STM32Cube/STM32CubeMX"
    "$ENV{HOME}/STM32CubeMX"
  PATH_SUFFIXES
    "STM32Cube/STM32CubeMX"
    "STM32CubeMX"
    "bin"
    ""
  DOC "STM32CubeMX executable"
)

get_filename_component(
  CUBEMX_DIR
  ${STM32CUBEMX_EXECUTABLE}
  DIRECTORY
)

# The bundled JRE is in a "jre" directory next to the STM32CubeMX launcher.
# Only old releases need this java, because their STM32CubeMX is a .jar file.
# New releases have a native launcher with its own JRE.
find_program(
  STM32CUBEMX_JAVA
  NAMES java
  HINTS
    "${CUBEMX_DIR}/jre/bin"
  DOC "STM32CubeMX java executable"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  STM32CubeMX
  FOUND_VAR STM32CUBEMX_FOUND
  REQUIRED_VARS STM32CUBEMX_EXECUTABLE
)

# Find how to start STM32CubeMX. Old releases are a Java archive, started with
# "java -jar". New releases are a native launcher (ELF, PE or Mach-O), started
# directly. Both have the name "STM32CubeMX" without an extension, so check the
# magic bytes of the file.
if(STM32CUBEMX_EXECUTABLE AND NOT DEFINED STM32CUBEMX_LAUNCH_COMMAND)
  file(READ "${STM32CUBEMX_EXECUTABLE}" _cubemx_magic HEX LIMIT 4)
  string(TOLOWER "${_cubemx_magic}" _cubemx_magic)
  # 504b: ZIP/jar.  7f454c46: ELF.  4d5a: PE (.exe).  cafe/feca/cffa: Mach-O.
  if(_cubemx_magic MATCHES "^504b")
    if(NOT STM32CUBEMX_JAVA)
      message(WARNING "STM32CubeMX is a Java archive but no bundled java was "
        "found near ${STM32CUBEMX_EXECUTABLE}; CubeMX generation will fail.")
    endif()
    set(STM32CUBEMX_LAUNCH_COMMAND
      "${STM32CUBEMX_JAVA}" -jar "${STM32CUBEMX_EXECUTABLE}"
      CACHE STRING "Command used to launch STM32CubeMX")
  else()
    set(STM32CUBEMX_LAUNCH_COMMAND
      "${STM32CUBEMX_EXECUTABLE}"
      CACHE STRING "Command used to launch STM32CubeMX")
  endif()
endif()

mark_as_advanced(STM32CUBEMX_EXECUTABLE STM32CUBEMX_JAVA STM32CUBEMX_LAUNCH_COMMAND)
