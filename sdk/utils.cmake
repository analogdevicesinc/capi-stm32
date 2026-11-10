# Copyright (c) 2026 Analog Devices, Inc.
# SPDX-License-Identifier: BSD-3-Clause

# Build utilities for the capi-stm32 SDK scripts

# Run a command. Print its output. Stop with a FATAL_ERROR if it fails.
# Arguments: WHAT (text for the error message), WORKING_DIRECTORY, COMMAND.

function(capi_stm32_run_checked)
    cmake_parse_arguments(ARG "" "WHAT;WORKING_DIRECTORY" "COMMAND" ${ARGN})

    if(NOT ARG_COMMAND)
        message(FATAL_ERROR "capi_stm32_run_checked: COMMAND is required")
    endif()
    if(NOT ARG_WHAT)
        set(ARG_WHAT "running command")
    endif()

    set(_wd_arg "")
    if(ARG_WORKING_DIRECTORY)
        set(_wd_arg WORKING_DIRECTORY "${ARG_WORKING_DIRECTORY}")
    endif()

    execute_process(
        COMMAND ${ARG_COMMAND}
        ${_wd_arg}
        RESULT_VARIABLE _result
        OUTPUT_VARIABLE _output
        ERROR_VARIABLE _error
    )

    if(_output)
        message(STATUS "${_output}")
    endif()

    if(NOT _result EQUAL 0)
        if(_error)
            message(STATUS "${_error}")
        endif()
        message(FATAL_ERROR
            "Failed while ${ARG_WHAT} (exit code ${_result}).\n"
            "Command: ${ARG_COMMAND}\n"
            "See the output above for details.")
    elseif(_error)
        message(STATUS "${_error}")
    endif()
endfunction()
