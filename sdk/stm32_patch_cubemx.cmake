# Copyright (c) 2026 Analog Devices, Inc.
# SPDX-License-Identifier: BSD-3-Clause

# Patch the CMakeLists.txt and main.c that classic CubeMX generates.
# The caller runs this script with cmake -P.
# Input: CMAKE_FILE_TO_PATCH, STM32_PROJECT_BUILD, CAPI_STM32_BUILD_TARGET.

file(READ ${CMAKE_FILE_TO_PATCH} FILE_CONTENTS)

# Point CMAKE_SOURCE_DIR to the generated project directory
string(REPLACE "\${CMAKE_SOURCE_DIR}" "${STM32_PROJECT_BUILD}"
     PATCHED_CONTENTS "${FILE_CONTENTS}")

file(WRITE ${CMAKE_FILE_TO_PATCH} "${PATCHED_CONTENTS}")

file(READ ${CMAKE_FILE_TO_PATCH} FILE_CONTENTS)

string(REPLACE "\${CMAKE_PROJECT_NAME}" "${CAPI_STM32_BUILD_TARGET}"
     PATCHED_CONTENTS "${FILE_CONTENTS}")

file(WRITE ${CMAKE_FILE_TO_PATCH} "${PATCHED_CONTENTS}")

# Use the keyword form of target_link_libraries. CMake rejects a mix of forms.
file(READ ${CMAKE_FILE_TO_PATCH} FILE_CONTENTS)
string(REPLACE "target_link_libraries(${CAPI_STM32_BUILD_TARGET} \${MX_LINK_LIBS})"
               "target_link_libraries(${CAPI_STM32_BUILD_TARGET} PUBLIC \${MX_LINK_LIBS})"
     PATCHED_CONTENTS "${FILE_CONTENTS}")
file(WRITE ${CMAKE_FILE_TO_PATCH} "${PATCHED_CONTENTS}")

# Remove syscalls.c. It duplicates the symbols of stm32_capi_uart.c (stdio).
file(READ ${CMAKE_FILE_TO_PATCH} FILE_CONTENTS)
string(REGEX REPLACE "[^\n]*syscalls\\.c[^\n]*\n" ""
     PATCHED_CONTENTS "${FILE_CONTENTS}")
file(WRITE ${CMAKE_FILE_TO_PATCH} "${PATCHED_CONTENTS}")

# Move the CubeMX application sources (hal_msp, interrupt handlers, startup)
# from the build target to the STM32_Drivers OBJECT library.
#
# The HAL has weak stubs, such as HAL_UART_MspInit and SysTick_Handler, in
# that library. If the strong functions are in the same library, the linker
# always gets all objects, and the strong symbols win.
# In a static archive or another library, the linker can use the weak stubs.
file(READ ${CMAKE_FILE_TO_PATCH} FILE_CONTENTS)
string(REPLACE "target_sources(${CAPI_STM32_BUILD_TARGET} PRIVATE \${MX_Application_Src})"
               "target_sources(STM32_Drivers PRIVATE \${MX_Application_Src})"
     PATCHED_CONTENTS "${FILE_CONTENTS}")
file(WRITE ${CMAKE_FILE_TO_PATCH} "${PATCHED_CONTENTS}")


file(READ "${STM32_PROJECT_BUILD}/Core/Src/main.c" MAIN_FILE_CONTENTS)

# Rename main() to stm32_init(). The application provides main().
string(REPLACE "main(" "stm32_init("
     MAIN_PATCHED_CONTENTS "${MAIN_FILE_CONTENTS}")

# Replace the infinite loop with a return, so that stm32_init() returns
string(REPLACE "  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */" "  /* Initialization complete, return to caller */
  return 0;" MAIN_PATCHED_CONTENTS "${MAIN_PATCHED_CONTENTS}")

file(WRITE "${STM32_PROJECT_BUILD}/Core/Src/main.c" "${MAIN_PATCHED_CONTENTS}")

message(STATUS "Patched ${CMAKE_FILE_TO_PATCH} to use STM32_PROJECT_BUILD.")
