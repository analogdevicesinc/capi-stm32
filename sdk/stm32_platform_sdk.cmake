# Copyright (c) 2026 Analog Devices, Inc.
# SPDX-License-Identifier: BSD-3-Clause

# Capture this file's directory at include-time (before any function calls)
set(_CAPI_STM32_SDK_DIR "${CMAKE_CURRENT_LIST_DIR}")

include("${_CAPI_STM32_SDK_DIR}/utils.cmake")
include("${_CAPI_STM32_SDK_DIR}/stm32_cubemx2_sdk.cmake")

# The CubeMX2 generator and the EXTI table script need Python3_EXECUTABLE.
# capi sets it only on the configure that creates its venv.
# So find Python here.
if(NOT Python3_EXECUTABLE)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
endif()

# Find the project file. Use CONFIG_STM32_IOC_PATH, or else ${BOARD}.ioc or
# ${BOARD}.ioc2. The extension selects the backend: .ioc is classic
# STM32CubeMX and .ioc2 is STM32CubeMX2. A board has one of them.
# Without an explicit path, .ioc has priority if it exists.
# Sets IOC_FILE in the caller's scope.
function(capi_stm32_resolve_ioc_file)
        if(DEFINED CONFIG_STM32_IOC_PATH AND NOT "${CONFIG_STM32_IOC_PATH}" STREQUAL "")
                if(IS_ABSOLUTE "${CONFIG_STM32_IOC_PATH}")
                        set(IOC_FILE "${CONFIG_STM32_IOC_PATH}" PARENT_SCOPE)
                else()
                        set(IOC_FILE "${CMAKE_SOURCE_DIR}/${CONFIG_STM32_IOC_PATH}" PARENT_SCOPE)
                endif()
        elseif(NOT EXISTS "${CMAKE_SOURCE_DIR}/${BOARD}.ioc"
               AND EXISTS "${CMAKE_SOURCE_DIR}/${BOARD}.ioc2")
                set(IOC_FILE "${CMAKE_SOURCE_DIR}/${BOARD}.ioc2" PARENT_SCOPE)
        else()
                set(IOC_FILE "${CMAKE_SOURCE_DIR}/${BOARD}.ioc" PARENT_SCOPE)
        endif()
endfunction()

# Derive OPENOCD variables from the .ioc file, falling back to the TARGET
# variable when the .ioc is unavailable or incomplete.
# Sets OPENOCD_INTERFACE, OPENOCD_CHIPNAME, and OPENOCD_TARGETCFG in the
# caller's scope.
function(capi_stm32_resolve_openocd_config IOC_FILE)
        if(NOT DEFINED OPENOCD_INTERFACE)
                set(OPENOCD_INTERFACE "interface/cmsis-dap.cfg" PARENT_SCOPE)
        endif()

        if(NOT DEFINED GDB_PORT)
                set(GDB_PORT 3333)
        endif()

        set(OPENOCD_EXTRA_COMMANDS "\
adapter speed 8000\n\
gdb_port ${GDB_PORT}" PARENT_SCOPE)
        set(OPENOCD_AP_NUM_LINE "" PARENT_SCOPE)

        # Halt the target on GDB attach, to start from a known state
        set(OPENOCD_POST_COMMANDS "\
$_TARGETNAME configure -event gdb-attach { reset halt }" PARENT_SCOPE)

        # Read the settings from the .ioc file
        if(EXISTS "${IOC_FILE}")
                file(READ "${IOC_FILE}" IOC_CONTENT)

                # The chip name is Mcu.CPN, or else TARGET. Set it always.
                # generate_openocd_config() writes no openocd.cfg if CHIPNAME,
                # INTERFACE or TARGETCFG is not set.
                set(_chipname "${TARGET}")
                string(REGEX MATCH "Mcu\\.CPN=([^\r\n]+)" _match "${IOC_CONTENT}")
                if(_match)
                        set(_chipname "${CMAKE_MATCH_1}")
                endif()

                string(REGEX MATCH "Mcu\\.Family=([^\r\n]+)" _match "${IOC_CONTENT}")
                if(_match)
                        set(OPENOCD_CHIPNAME "${_chipname}" PARENT_SCOPE)
                        string(TOLOWER "${CMAKE_MATCH_1}" _family)
                        set(OPENOCD_TARGETCFG "target/${_family}x.cfg" PARENT_SCOPE)
                        if(_family MATCHES "h7")
                                set(OPENOCD_AP_NUM_LINE "set AP_NUM 1" PARENT_SCOPE)
                                set(OPENOCD_INTERFACE "interface/stlink-dap.cfg" PARENT_SCOPE)
                                set(OPENOCD_POST_COMMANDS "" PARENT_SCOPE)
                        endif()
                        message(STATUS "Extracted OpenOCD config from .ioc: chip=${_chipname}, target=target/${_family}x.cfg")
                        return()
                endif()
        endif()

        # Else derive the settings from TARGET (stm32f756 gives stm32f7)
        set(OPENOCD_CHIPNAME "${TARGET}" PARENT_SCOPE)
        string(REGEX MATCH "^(stm32[a-z][0-9])" _family "${TARGET}")
        if(_family)
                set(OPENOCD_TARGETCFG "target/${_family}x.cfg" PARENT_SCOPE)
                message(STATUS "Using fallback OpenOCD config: chip=${TARGET}, target=target/${_family}x.cfg")
        else()
                set(OPENOCD_TARGETCFG "target/${TARGET}.cfg" PARENT_SCOPE)
                message(STATUS "Using fallback OpenOCD config: chip=${TARGET}, target=target/${TARGET}.cfg")
        endif()
endfunction()

# Check if the CubeMX project needs a new generation. Compare the sha1 of the
# .ioc file and the CubeMX path with the stamp of the last generation.
# The generated directory must also exist. Else a failed run with a stamp
# looks up to date and breaks add_subdirectory().
# Sets NEED_REGEN (TRUE or FALSE) in the caller's scope.
function(capi_stm32_check_cubemx_stale IOC_FILE IOC_BUILD_DIR GENERATED_DIR)
        # The stamp is in the output directory of this .ioc file. A project can
        # use several .ioc files for one BOARD. Each file keeps its own
        # stamp, so a change of file does not force a slow regeneration.
        set(STAMP_FILE "${IOC_BUILD_DIR}/.cubemx_stamp")
        set(NEED_REGEN TRUE PARENT_SCOPE)

        if(EXISTS "${IOC_FILE}")
                file(SHA1 "${IOC_FILE}" _ioc_hash)
        else()
                set(_ioc_hash "MISSING")
        endif()
        set(_current "${_ioc_hash}:${STM32CUBEMX_EXECUTABLE}")

        if(EXISTS "${STAMP_FILE}" AND IS_DIRECTORY "${GENERATED_DIR}")
                file(READ "${STAMP_FILE}" _stored)
                string(STRIP "${_stored}" _stored)
                if("${_current}" STREQUAL "${_stored}")
                        set(NEED_REGEN FALSE PARENT_SCOPE)
                        return()
                endif()
                message(STATUS "CubeMX regeneration needed (IOC or CubeMX changed)")
        endif()
endfunction()

# Write the stamp file after a successful CubeMX generation. There is one stamp
# for each .ioc file (in IOC_BUILD_DIR).
function(capi_stm32_write_cubemx_stamp IOC_FILE IOC_BUILD_DIR)
        if(EXISTS "${IOC_FILE}")
                file(SHA1 "${IOC_FILE}" _ioc_hash)
        else()
                set(_ioc_hash "MISSING")
        endif()
        file(WRITE "${IOC_BUILD_DIR}/.cubemx_stamp" "${_ioc_hash}:${STM32CUBEMX_EXECUTABLE}")
endfunction()

function(capi_stm32_config_sdk BUILD_TARGET)
        capi_stm32_resolve_ioc_file()

        # Use the generator of the project file. The code below is the classic
        # STM32CubeMX backend.
        get_filename_component(_ioc_ext "${IOC_FILE}" LAST_EXT)
        if(_ioc_ext STREQUAL ".ioc2")
                # Do not set OpenOCD here. capi_stm32_resolve_openocd_config()
                # reads the INI keys of a classic .ioc only. flash.cmake derives
                # the settings from TARGET.
                capi_stm32_config_sdk_cubemx2("${BUILD_TARGET}" "${IOC_FILE}")
                return()
        endif()

        capi_stm32_resolve_openocd_config("${IOC_FILE}")

        get_filename_component(IOC_NAME "${IOC_FILE}" NAME_WE)
        set(EXTI_SCRIPT ${_CAPI_STM32_SDK_DIR}/scripts/exti_script.py)
        set(EXTI_GEN_FILE ${CMAKE_CURRENT_BINARY_DIR}/stm32_gpio_irq_generated.c)
        set(BOARD_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/${BOARD}_build")

        capi_stm32_check_cubemx_stale("${IOC_FILE}" "${BOARD_BUILD_DIR}/${IOC_NAME}"
                "${BOARD_BUILD_DIR}/${IOC_NAME}/cmake/stm32cubemx")

        if(NEED_REGEN)
                # Delete the old generation, so that CubeMX starts in a clean directory
                file(REMOVE_RECURSE "${BOARD_BUILD_DIR}/${IOC_NAME}")

                message(STATUS "Patching STM32CubeMX config file...")
                capi_stm32_run_checked(
                        WHAT "patching the STM32CubeMX config"
                        COMMAND ${CMAKE_COMMAND}
                        -D STM32_TEMPLATE_FILE=${_CAPI_STM32_SDK_DIR}/stm32_cubemx_template.cubemx
                        -D STM32_TARGET_BUILD=${CMAKE_CURRENT_BINARY_DIR}
                        -D STM32_IOC_PATH=${CONFIG_STM32_IOC_PATH}
                        -D SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
                        -D BOARD=${BOARD}
                        -P "${_CAPI_STM32_SDK_DIR}/stm32_patch_cubemx_config.cmake"
                )

                message(STATUS "Generating STM32CubeMX project...")
                execute_process(
                        COMMAND ${STM32CUBEMX_LAUNCH_COMMAND} -q "${CMAKE_CURRENT_BINARY_DIR}/stm32cubemx_config.cubemx"
                        RESULT_VARIABLE CUBEMX_RESULT
                        OUTPUT_VARIABLE CUBEMX_OUTPUT
                        ERROR_VARIABLE CUBEMX_ERROR
                )
                message(STATUS "CubeMX exit code: ${CUBEMX_RESULT}")
                if(CUBEMX_OUTPUT)
                        message(STATUS "CubeMX stdout:\n${CUBEMX_OUTPUT}")
                endif()
                if(CUBEMX_ERROR)
                        message(STATUS "CubeMX stderr:\n${CUBEMX_ERROR}")
                endif()
                if(NOT CUBEMX_RESULT EQUAL 0 OR
                   NOT EXISTS "${BOARD_BUILD_DIR}/${IOC_NAME}/cmake/stm32cubemx/CMakeLists.txt")
                        message(FATAL_ERROR
                                "STM32CubeMX project generation failed "
                                "(launch command: ${STM32CUBEMX_LAUNCH_COMMAND}). "
                                "See the CubeMX output above.")
                endif()

                message(STATUS "Patching STM32CubeMX generated CMakeLists.txt...")
                capi_stm32_run_checked(
                        WHAT "patching the STM32CubeMX-generated CMakeLists.txt"
                        COMMAND ${CMAKE_COMMAND}
                        -D CMAKE_FILE_TO_PATCH=${BOARD_BUILD_DIR}/${IOC_NAME}/cmake/stm32cubemx/CMakeLists.txt
                        -D STM32_PROJECT_BUILD=${BOARD_BUILD_DIR}/${IOC_NAME}
                        -D CAPI_STM32_BUILD_TARGET=${BUILD_TARGET}
                        -P "${_CAPI_STM32_SDK_DIR}/stm32_patch_cubemx.cmake"
                )

                if(CAPI_STM32_ENABLE_EXTI)
                        file(GLOB STARTUP_FILE ${BOARD_BUILD_DIR}/${IOC_NAME}/startup_*.s)
                        if(STARTUP_FILE)
                                capi_stm32_run_checked(
                                        WHAT "generating the STM32 GPIO IRQ table from ${STARTUP_FILE}"
                                        COMMAND ${Python3_EXECUTABLE} ${EXTI_SCRIPT} ${STARTUP_FILE} ${EXTI_GEN_FILE}
                                )
                        endif()
                endif()

                capi_stm32_write_cubemx_stamp("${IOC_FILE}" "${BOARD_BUILD_DIR}/${IOC_NAME}")
        endif()

        # CubeMX generates two CMakeLists.txt layouts:
        #
        # Linux: stm32cubemx is an INTERFACE library with includes and defines.
        #   STM32_Drivers and USB_Device_Library are OBJECT libraries.
        #   target_link_libraries(${BUILD_TARGET} ${MX_LINK_LIBS}) links them.
        #
        # Windows: stm32cubemx is an INTERFACE library with all sources
        #   (target_sources(stm32cubemx INTERFACE ...)). There are no OBJECT
        #   libraries and no MX_LINK_LIBS.
        #
        # Both layouts give duplicate symbols. On Linux, the OBJECT libraries
        # reach the executable twice. On Windows, the INTERFACE sources go to
        # each consumer of stm32cubemx.
        #
        # Use one pattern: the foreach below links the OBJECT libraries to the
        # executable only. stm32cubemx has includes and defines only.
        set(_cubemx_cmake "${BOARD_BUILD_DIR}/${IOC_NAME}/cmake/stm32cubemx/CMakeLists.txt")
        file(READ "${_cubemx_cmake}" _cubemx_content)
        set(_cubemx_modified FALSE)

        # Windows layout: move the INTERFACE sources to an OBJECT library
        if(_cubemx_content MATCHES "target_sources\\(stm32cubemx INTERFACE")
                string(REGEX REPLACE
                        "target_sources\\(stm32cubemx INTERFACE([^)]+)\\)"
                        "add_library(STM32_Drivers OBJECT)\ntarget_sources(STM32_Drivers PRIVATE\\1)\ntarget_link_libraries(STM32_Drivers PUBLIC stm32cubemx)"
                        _cubemx_content "${_cubemx_content}")
                set(_cubemx_modified TRUE)
        endif()

        # Linux layout: rename MX_LINK_LIBS, so that the generated
        # target_link_libraries() call does nothing. The foreach below links
        # the OBJECT libraries.
        if(_cubemx_content MATCHES "set\\(MX_LINK_LIBS[^_]")
                string(REPLACE "set(MX_LINK_LIBS" "set(_MX_LINK_LIBS_UNUSED"
                        _cubemx_content "${_cubemx_content}")
                set(_cubemx_modified TRUE)
        endif()

        if(_cubemx_modified)
                file(WRITE "${_cubemx_cmake}" "${_cubemx_content}")
        endif()

        if(NOT TARGET STM32_Drivers)
                add_subdirectory(${BOARD_BUILD_DIR}/${IOC_NAME}/cmake/stm32cubemx ${BOARD_BUILD_DIR}/${IOC_NAME}/cmake/stm32cubemx/build)
        endif()

        if(CAPI_STM32_ENABLE_EXTI)
        target_sources(${BUILD_TARGET} PRIVATE ${EXTI_GEN_FILE})
        endif()

        # Find the linker script of CubeMX in the output directory of this .ioc
        # file only. Another .ioc file of the same BOARD can leave its own
        # script in BOARD_BUILD_DIR. Check the result before -T. No match gives
        # a bare "-T" and an unclear linker error. More than one match passes
        # the extra scripts as input files and gives a wrong link.
        file(GLOB LINKER_SCRIPT_FILE ${BOARD_BUILD_DIR}/${IOC_NAME}/*_FLASH.ld)
        if(NOT LINKER_SCRIPT_FILE)
                message(FATAL_ERROR
                        "No CubeMX linker script (*_FLASH.ld) found under ${BOARD_BUILD_DIR}/${IOC_NAME}. "
                        "STM32CubeMX project generation may have produced an unexpected layout.")
        endif()
        list(LENGTH LINKER_SCRIPT_FILE _linker_script_count)
        if(_linker_script_count GREATER 1)
                message(FATAL_ERROR
                        "Multiple CubeMX linker scripts (*_FLASH.ld) found under ${BOARD_BUILD_DIR}/${IOC_NAME}: "
                        "${LINKER_SCRIPT_FILE}. Expected exactly one.")
        endif()

        # Link the CubeMX OBJECT libraries to the executable only. Their
        # objects then appear once on the link line.
        get_directory_property(_cubemx_targets
                DIRECTORY ${BOARD_BUILD_DIR}/${IOC_NAME}/cmake/stm32cubemx
                BUILDSYSTEM_TARGETS)
        foreach(_lib ${_cubemx_targets})
                get_target_property(_type ${_lib} TYPE)
                if(_type STREQUAL "OBJECT_LIBRARY")
                        target_link_libraries(${BUILD_TARGET} PUBLIC ${_lib})
                endif()
        endforeach()
        target_link_libraries(${BUILD_TARGET} PUBLIC stm32cubemx)
        target_compile_definitions(${BUILD_TARGET} PRIVATE -DSTM32_PLATFORM=1)
        target_link_options(${BUILD_TARGET} PRIVATE -T${LINKER_SCRIPT_FILE})
endfunction()
