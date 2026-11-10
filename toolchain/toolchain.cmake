# Copyright (c) 2026 Analog Devices, Inc.
# SPDX-License-Identifier: BSD-3-Clause

message(STATUS "STM32 Platform")

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# CMake includes this file again for try_compile(). It does not pass cache
# variables such as TARGET to those projects. Pass TARGET, so that the MCU flag
# selection below works in the compiler checks.
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES TARGET)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")
find_package(STM32CubeMX REQUIRED)
# CubeIDE is required only for its GCC. Without it, flash.cmake uses the OpenOCD of the system.
if(USE_VENDOR_TOOLCHAIN)
    find_package(STM32CubeIDE REQUIRED)
else()
    find_package(STM32CubeIDE)
endif()
# Not REQUIRED: only boards with an .ioc2 file need STM32CubeMX2. For them,
# capi_stm32_config_sdk_cubemx2() gives an error if it is missing.
find_package(STM32CubeMX2)

if(USE_VENDOR_TOOLCHAIN)
    file(GLOB _cubeide_gcc_bin
        "${CUBEIDE_DIR}/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.*/tools/bin"
    )
    if(_cubeide_gcc_bin)
        list(GET _cubeide_gcc_bin 0 CROSS_COMPILER_BIN)
        message(STATUS "Using CubeIDE-bundled GCC: ${CROSS_COMPILER_BIN}")
    else()
        message(FATAL_ERROR
            "USE_VENDOR_TOOLCHAIN=ON but could not locate the CubeIDE-bundled GCC "
            "under ${CUBEIDE_DIR}/plugins -- refusing to silently fall back to "
            "whatever arm-none-eabi-gcc happens to be on PATH, which may not match "
            "the toolchain this platform was verified against. Fix CUBEIDE_DIR, or "
            "configure with -DUSE_VENDOR_TOOLCHAIN=OFF to use PATH deliberately.")
    endif()

    find_program(CMAKE_C_COMPILER   arm-none-eabi-gcc HINTS ${CROSS_COMPILER_BIN})
    find_program(CMAKE_CXX_COMPILER arm-none-eabi-g++ HINTS ${CROSS_COMPILER_BIN})
    find_program(CMAKE_ASM_COMPILER arm-none-eabi-gcc HINTS ${CROSS_COMPILER_BIN})
    find_program(CMAKE_LINKER       arm-none-eabi-ld  HINTS ${CROSS_COMPILER_BIN})
    find_program(CMAKE_SIZE         arm-none-eabi-size HINTS ${CROSS_COMPILER_BIN})
    find_program(CMAKE_OBJCOPY      arm-none-eabi-objcopy HINTS ${CROSS_COMPILER_BIN})

    # Report a missing compiler here, with the search directory. CMake gives a
    # generic error later.
    if(NOT CMAKE_C_COMPILER)
        message(FATAL_ERROR
            "arm-none-eabi-gcc not found (searched CROSS_COMPILER_BIN=${CROSS_COMPILER_BIN} and PATH). "
            "Install the ARM GCC toolchain or STM32CubeIDE. "
            "To use a compiler already on PATH instead, configure with -DUSE_VENDOR_TOOLCHAIN=OFF.")
    endif()
else()
    set(CMAKE_C_COMPILER arm-none-eabi-gcc)
    set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
    set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
    set(CMAKE_LINKER arm-none-eabi-ld)
    set(CMAKE_SIZE arm-none-eabi-size)
    set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
endif()

# Find OpenOCD. Order: -DOPENOCD_PATH, the one in CubeIDE, the one in PATH.
if(NOT OPENOCD_PATH)
    if(CUBEIDE_DIR)
        file(GLOB_RECURSE _cubeide_openocd ${CUBEIDE_DIR}/plugins/*/openocd)
        foreach(_candidate ${_cubeide_openocd})
            if(NOT IS_DIRECTORY "${_candidate}")
                set(OPENOCD_PATH "${_candidate}")
                break()
            endif()
        endforeach()
    endif()
    if(OPENOCD_PATH)
        file(GLOB_RECURSE _cubeide_scripts ${CUBEIDE_DIR}/plugins/*/mem_helper.tcl)
        if(_cubeide_scripts)
            list(GET _cubeide_scripts 0 _scripts_file)
            cmake_path(GET _scripts_file PARENT_PATH OPENOCD_SCRIPTS)
        endif()
        message(STATUS "Found CubeIDE-bundled OpenOCD: ${OPENOCD_PATH}")
    endif()
endif()

if(NOT OPENOCD_PATH)
    find_program(OPENOCD_PATH
        NAMES openocd
        PATH_SUFFIXES bin
        DOC "Path to OpenOCD executable"
    )
    if(OPENOCD_PATH)
        cmake_path(GET OPENOCD_PATH PARENT_PATH _ocd_bin_dir)
        cmake_path(GET _ocd_bin_dir PARENT_PATH _ocd_prefix)
        cmake_path(SET _ocd_scripts NORMALIZE "${_ocd_prefix}/share/openocd/scripts")
        if(EXISTS "${_ocd_scripts}")
            set(OPENOCD_SCRIPTS "${_ocd_scripts}")
        endif()
        message(STATUS "Found system OpenOCD: ${OPENOCD_PATH}")
    else()
        message(STATUS "OpenOCD not found")
    endif()
endif()

set(CMAKE_EXECUTABLE_SUFFIX_ASM ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_C ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_CXX ".elf")

# Set the MCU flags from TARGET
if("${TARGET}" MATCHES "^stm32f7")
    set(STM32_MCU_FLAGS "-mthumb -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-d16")
    message(STATUS "Detected Cortex-M7 MCU: ${TARGET}")
elseif("${TARGET}" MATCHES "^stm32h7")
    set(STM32_MCU_FLAGS "-mthumb -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-d16")
    message(STATUS "Detected Cortex-M7 MCU: ${TARGET}")
elseif("${TARGET}" MATCHES "^stm32f4")
    set(STM32_MCU_FLAGS "-mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16")
    message(STATUS "Detected Cortex-M4 MCU: ${TARGET}")
elseif("${TARGET}" MATCHES "^stm32l4")
    set(STM32_MCU_FLAGS "-mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16")
    message(STATUS "Detected Cortex-M4 MCU: ${TARGET}")
elseif("${TARGET}" MATCHES "^stm32h5")
    set(STM32_MCU_FLAGS "-mthumb -mcpu=cortex-m33 -mfloat-abi=hard -mfpu=fpv5-sp-d16")
    message(STATUS "Detected Cortex-M33 MCU: ${TARGET}")
elseif("${TARGET}" MATCHES "^stm32c5")
    set(STM32_MCU_FLAGS "-mthumb -mcpu=cortex-m33 -mfloat-abi=hard -mfpu=fpv5-sp-d16")
    message(STATUS "Detected Cortex-M33 MCU: ${TARGET}")
else()
    # Stop with an error. A guess can give wrong -mcpu and -mfpu flags.
    message(FATAL_ERROR "Unknown or unset STM32 TARGET '${TARGET}'. Set TARGET in the board preset (e.g. stm32f756).")
endif()

# Flags for all build types
set(CMAKE_C_FLAGS "${STM32_MCU_FLAGS} -ffunction-sections -fdata-sections -MD" CACHE STRING "C compiler flags" FORCE)
set(CMAKE_CXX_FLAGS "${STM32_MCU_FLAGS} -ffunction-sections -fdata-sections -MD" CACHE STRING "C++ compiler flags" FORCE)
set(CMAKE_ASM_FLAGS "${STM32_MCU_FLAGS} -x assembler-with-cpp" CACHE STRING "ASM compiler flags" FORCE)

# Debug: full debug information, no optimization
set(CMAKE_C_FLAGS_DEBUG "-g3 -O0 -DDEBUG" CACHE STRING "C compiler flags for Debug" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0 -DDEBUG" CACHE STRING "C++ compiler flags for Debug" FORCE)
set(CMAKE_ASM_FLAGS_DEBUG "-g3" CACHE STRING "ASM compiler flags for Debug" FORCE)

# Release: optimize for speed, no assertions
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "C compiler flags for Release" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "C++ compiler flags for Release" FORCE)
set(CMAKE_ASM_FLAGS_RELEASE "" CACHE STRING "ASM compiler flags for Release" FORCE)

# RelWithDebInfo: optimize, with debug information
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG" CACHE STRING "C compiler flags for RelWithDebInfo" FORCE)
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG" CACHE STRING "C++ compiler flags for RelWithDebInfo" FORCE)
set(CMAKE_ASM_FLAGS_RELWITHDEBINFO "-g" CACHE STRING "ASM compiler flags for RelWithDebInfo" FORCE)

# MinSizeRel: optimize for size
set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG" CACHE STRING "C compiler flags for MinSizeRel" FORCE)
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG" CACHE STRING "C++ compiler flags for MinSizeRel" FORCE)
set(CMAKE_ASM_FLAGS_MINSIZEREL "" CACHE STRING "ASM compiler flags for MinSizeRel" FORCE)

# Linker flags for all build types
set(CMAKE_EXE_LINKER_FLAGS "${STM32_MCU_FLAGS} -specs=nosys.specs -Wl,--gc-sections ${MCU_LINKER_FLAGS} --entry=Reset_Handler" CACHE STRING "Linker flags for MCU" FORCE)

if (NOT PROBE)
    set(PROBE "openocd")
endif()
