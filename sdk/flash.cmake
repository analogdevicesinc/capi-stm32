# Copyright (c) 2026 Analog Devices, Inc.
# SPDX-License-Identifier: BSD-3-Clause

# Flash and debug targets for STM32 boards.
# Provides capi_stm32_add_flash_target(TARGET). It writes openocd.cfg and adds
# the flash, reset, erase and debug_server targets.
# OpenOCD comes from CubeIDE (CUBEIDE_DIR) or from the system.
# The OpenOCD settings come from TARGET and the .ioc file.

function(capi_stm32_add_flash_target TARGET_NAME)
    # capi calls only capi_<platform>_add_flash_target(). Set up
    # the .hex and .bin files here, before the early return for a missing
    # OpenOCD. The post build step runs once.
    capi_stm32_post_build(${TARGET_NAME})

    # Find OpenOCD. OPENOCD_PATH from the toolchain has priority.
    if(NOT DEFINED OPENOCD_PATH OR OPENOCD_PATH STREQUAL "")
        if(DEFINED CUBEIDE_DIR)
            file(GLOB _ocd_bin
                "${CUBEIDE_DIR}/plugins/com.st.stm32cube.ide.mcu.externaltools.openocd.*/tools/bin/openocd")
            if(_ocd_bin)
                list(GET _ocd_bin -1 OPENOCD_PATH)
            endif()
        endif()
        if(NOT OPENOCD_PATH)
            find_program(OPENOCD_PATH openocd)
        endif()
    endif()

    if(NOT OPENOCD_PATH)
        message(WARNING "OpenOCD not found — flash/debug targets unavailable.")
        return()
    endif()

    # OpenOCD scripts directory. It must match the binary.
    # OPENOCD_SCRIPTS from the toolchain has priority. Else use the binary path.
    set(_ocd_scripts "")
    if(DEFINED OPENOCD_SCRIPTS AND NOT OPENOCD_SCRIPTS STREQUAL "")
        set(_ocd_scripts "${OPENOCD_SCRIPTS}")
    else()
        get_filename_component(_ocd_dir "${OPENOCD_PATH}" DIRECTORY)
        if(IS_DIRECTORY "${_ocd_dir}/../share/openocd/scripts")
            set(_ocd_scripts "${_ocd_dir}/../share/openocd/scripts")
        elseif(DEFINED CUBEIDE_DIR)
            file(GLOB _ocd_script_dirs
                "${CUBEIDE_DIR}/plugins/com.st.stm32cube.ide.mcu.debug.openocd*/resources/openocd/st_scripts")
            if(_ocd_script_dirs)
                list(GET _ocd_script_dirs -1 _ocd_scripts)
            endif()
        endif()
    endif()

    # Interface and target
    if(NOT DEFINED STM32_OPENOCD_INTERFACE)
        set(STM32_OPENOCD_INTERFACE "interface/stlink-dap.cfg")
    endif()
    if(NOT DEFINED STM32_OPENOCD_TARGET)
        string(REGEX MATCH "^stm32([a-z][0-9])" _match "${TARGET}")
        if(_match)
            set(STM32_OPENOCD_TARGET "target/stm32${CMAKE_MATCH_1}x.cfg")
        else()
            message(FATAL_ERROR "Cannot derive OpenOCD target from TARGET='${TARGET}'. Set STM32_OPENOCD_TARGET manually.")
        endif()
    endif()

    # Chip name
    if(NOT DEFINED STM32_OPENOCD_CHIPNAME)
        if(DEFINED OPENOCD_CHIPNAME AND NOT OPENOCD_CHIPNAME STREQUAL "")
            set(STM32_OPENOCD_CHIPNAME "${OPENOCD_CHIPNAME}")
        else()
            string(TOUPPER "${TARGET}" _upper_target)
            set(STM32_OPENOCD_CHIPNAME "${_upper_target}xx")
        endif()
    endif()

    # Debug probe selection.
    # OpenOCD uses the first ST-LINK that it finds. With more than one board,
    # this can be the wrong one. The probe connects and the SWD DP answers, but
    # the target part of the script fails, and it looks like a broken target.
    # Configure with -DSTM32_OPENOCD_ADAPTER_SERIAL=<serial> to select a probe.
    # `udevadm info -q property -n /dev/ttyACM<n> | grep ID_SERIAL_SHORT` gives
    # the serial of the probe of a virtual COM port.
    set(_adapter_serial_cfg "")
    if(DEFINED STM32_OPENOCD_ADAPTER_SERIAL AND NOT STM32_OPENOCD_ADAPTER_SERIAL STREQUAL "")
        set(_adapter_serial_cfg "adapter serial ${STM32_OPENOCD_ADAPTER_SERIAL}")
    endif()

    # H7 dual-core devices need AP_NUM
    set(_ap_num "")
    if("${TARGET}" MATCHES "^stm32h7")
        set(_ap_num "set AP_NUM 1")
    endif()

    # Internal flash loader, for families whose target script has no built-in
    # flash driver.
    # The scripts of older families declare the flash bank with a driver in the
    # OpenOCD binary ("flash bank ... stm32h7x ..."). The scripts of newer
    # families (STM32C5) use "stldr". They read the programming algorithm from a
    # .stldr file of STM32CubeProgrammer, one for each device ID. If
    # INTERNAL_FLASH_LOADERS is not set before the target script, the script
    # prints "No internal flash loaders provided" and adds no flash bank. Then
    # `program` has nothing to write to. A board that needs it sets its device
    # ID in board.cmake.
    set(_stldr_cfg "")
    if(DEFINED STM32_OPENOCD_STLDR_ID AND NOT STM32_OPENOCD_STLDR_ID STREQUAL "")
        if(NOT DEFINED STM32_OPENOCD_STLDR OR STM32_OPENOCD_STLDR STREQUAL "")
            # The loaders are in a CubeIDE plugin next to the OpenOCD plugin.
            # Take the plugins directory from the OpenOCD binary. CUBEIDE_DIR is
            # not in the scope of this function.
            string(REGEX REPLACE "(.*/plugins)/.*" "\\1"
                   _plugins_dir "${OPENOCD_PATH}")
            file(GLOB _stldr_candidates
                "${_plugins_dir}/*/tools/bin/FlashLoader/${STM32_OPENOCD_STLDR_ID}.stldr")
            if(_stldr_candidates)
                list(GET _stldr_candidates -1 STM32_OPENOCD_STLDR)
            endif()
        endif()
        if(STM32_OPENOCD_STLDR)
            set(_stldr_cfg "set INTERNAL_FLASH_LOADERS {${STM32_OPENOCD_STLDR}}")
        else()
            message(WARNING
                "${STM32_OPENOCD_STLDR_ID}.stldr (internal flash loader for ${TARGET}) "
                "not found beside ${OPENOCD_PATH} -- flashing will fail with "
                "\"No internal flash loaders provided\". Configure with "
                "-DSTM32_OPENOCD_STLDR=<path>/${STM32_OPENOCD_STLDR_ID}.stldr; it ships "
                "under STM32CubeProgrammer's tools/bin/FlashLoader/.")
        endif()
    endif()

    # Write openocd.cfg
    set(_ocd_cfg "${CMAKE_BINARY_DIR}/openocd.cfg")
    file(WRITE "${_ocd_cfg}"
"source [find ${STM32_OPENOCD_INTERFACE}]\n\
${_adapter_serial_cfg}\n\
set CHIPNAME ${STM32_OPENOCD_CHIPNAME}\n\
adapter speed 8000\n\
${_ap_num}\n\
${_stldr_cfg}\n\
source [find ${STM32_OPENOCD_TARGET}]\n")

    set(_scripts_flag "")
    if(_ocd_scripts)
        set(_scripts_flag -s ${_ocd_scripts})
    endif()

    set(_elf "$<TARGET_FILE:${TARGET_NAME}>")

    add_custom_target(flash
        COMMAND "${OPENOCD_PATH}"
            ${_scripts_flag}
            -f ${_ocd_cfg}
            -c "program ${_elf} verify reset exit"
        DEPENDS ${TARGET_NAME}
        COMMENT "Flashing ${TARGET_NAME} via OpenOCD..."
        VERBATIM
    )

    add_custom_target(reset
        COMMAND "${OPENOCD_PATH}"
            ${_scripts_flag}
            -f ${_ocd_cfg}
            -c "init" -c "reset run" -c "exit"
        COMMENT "Resetting board..."
        VERBATIM
    )

    add_custom_target(erase
        COMMAND "${OPENOCD_PATH}"
            ${_scripts_flag}
            -f ${_ocd_cfg}
            -c "init" -c "halt" -c "flash erase_sector 0 0 last" -c "exit"
        COMMENT "Erasing flash..."
        VERBATIM
    )

    # Debug server. OpenOCD keeps running for GDB.
    add_custom_target(debug_server
        COMMAND "${OPENOCD_PATH}"
            ${_scripts_flag}
            -f ${_ocd_cfg}
        COMMENT "Starting OpenOCD debug server..."
        VERBATIM
    )

    find_program(_gdb NAMES arm-none-eabi-gdb gdb-multiarch)
    if(_gdb)
        set(_gdbinit "${CMAKE_BINARY_DIR}/gdbinit")
        file(WRITE "${_gdbinit}"
"target extended-remote :3333\n\
monitor reset halt\n\
load\n\
monitor reset init\n")

        add_custom_target(debug
            COMMAND ${_gdb}
                -x ${_gdbinit}
                ${_elf}
            DEPENDS ${TARGET_NAME}
            COMMENT "Launching GDB debug session..."
            VERBATIM
        )
    endif()

    message(STATUS "Flash targets: flash, reset, erase, debug_server (${OPENOCD_PATH})")
endfunction()

# Post build: write the .hex and .bin files and print the size
function(capi_stm32_post_build TARGET_NAME)
    # An SDK or capi_stm32_add_flash_target() calls this function. Add the
    # commands once.
    get_target_property(_stm32_post_build_done ${TARGET_NAME} CAPI_STM32_POST_BUILD)
    if(_stm32_post_build_done)
        return()
    endif()
    set_target_properties(${TARGET_NAME} PROPERTIES CAPI_STM32_POST_BUILD TRUE)

    find_program(_objcopy arm-none-eabi-objcopy)
    find_program(_size arm-none-eabi-size)

    if(_objcopy)
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${_objcopy} -O ihex $<TARGET_FILE:${TARGET_NAME}>
                    $<TARGET_FILE_DIR:${TARGET_NAME}>/$<TARGET_FILE_BASE_NAME:${TARGET_NAME}>.hex
            COMMAND ${_objcopy} -O binary $<TARGET_FILE:${TARGET_NAME}>
                    $<TARGET_FILE_DIR:${TARGET_NAME}>/$<TARGET_FILE_BASE_NAME:${TARGET_NAME}>.bin
            COMMENT "Generating .hex and .bin"
        )
    endif()

    if(_size)
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${_size} --format=berkeley $<TARGET_FILE:${TARGET_NAME}>
            COMMENT "Binary size:"
        )
    endif()
endfunction()
