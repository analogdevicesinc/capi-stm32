# Copyright (c) 2026 Analog Devices, Inc.
# SPDX-License-Identifier: BSD-3-Clause

file(READ ${STM32_TEMPLATE_FILE} OLD_CONTENT)

# Find the .ioc file
if(STM32_IOC_PATH)
        if(IS_ABSOLUTE ${STM32_IOC_PATH})
                set(IOC_SOURCE "${STM32_IOC_PATH}")
        else()
                set(IOC_SOURCE "${SOURCE_DIR}/${STM32_IOC_PATH}")
        endif()
else()
        set(IOC_SOURCE "${SOURCE_DIR}/${BOARD}.ioc")
endif()

# Copy the .ioc file to the CubeMX output directory <BOARD>_build/<IOC_NAME>/.
# CubeMX then finds a project and shows no "project does not exist" dialog.
# CubeMX loads the file in place, so it needs no "project path" command.
get_filename_component(IOC_FILENAME "${IOC_SOURCE}" NAME)
get_filename_component(IOC_NAME "${IOC_SOURCE}" NAME_WE)
set(PROJECT_DIR "${STM32_TARGET_BUILD}/${BOARD}_build/${IOC_NAME}")
file(MAKE_DIRECTORY "${PROJECT_DIR}")
file(COPY_FILE "${IOC_SOURCE}" "${PROJECT_DIR}/${IOC_FILENAME}")

# Patch the copy to suppress the "migrate to newer version?" dialog
file(READ "${PROJECT_DIR}/${IOC_FILENAME}" IOC_CONTENT)
string(REPLACE "ProjectManager.AskForMigrate=true" "ProjectManager.AskForMigrate=false" IOC_CONTENT "${IOC_CONTENT}")
file(WRITE "${PROJECT_DIR}/${IOC_FILENAME}" "${IOC_CONTENT}")

set(CUBEMX_CONFIG_NAME "config load ${PROJECT_DIR}/${IOC_FILENAME}\n")
file(WRITE "${STM32_TARGET_BUILD}/stm32cubemx_config.cubemx" "${CUBEMX_CONFIG_NAME}${OLD_CONTENT}")
