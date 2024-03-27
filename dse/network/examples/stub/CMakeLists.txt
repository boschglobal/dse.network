# Copyright 2024 Robert Bosch GmbH
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.21)

set(MODEL_PATH "examples/stub")


# Targets
# =======

# Message Library
# ---------------
add_library(message
    SHARED
        stub.c
)
set_target_properties(message
    PROPERTIES PREFIX ""
)

# Function Library
# ----------------
add_library(function
    SHARED
        functions/function.c
        functions/counters.c
        functions/crc.c
)
set_target_properties(function
    PROPERTIES PREFIX ""
)
target_include_directories(function
    PRIVATE
        ${DSE_CLIB_INCLUDE_DIR}
        ${DSE_MODELC_INCLUDE_DIR}
        ../../../..
)

# Function Library (with UNIT_TESTING)
# ----------------
add_library(function__ut
    SHARED
        functions/function.c
        functions/counters.c
        functions/crc.c
)
set_target_properties(function__ut
    PROPERTIES PREFIX ""
)
target_include_directories(function__ut
    PRIVATE
        ${DSE_CLIB_INCLUDE_DIR}
        ${DSE_MODELC_INCLUDE_DIR}
        ../../../..
)
target_compile_definitions(function__ut
    PUBLIC CMOCKA_TESTING
)


# Install
# =======
install(
    TARGETS
        message
        function
        function__ut
    LIBRARY DESTINATION
        ${MODEL_PATH}/lib
)
install(
    FILES
        network.yaml
        simulation.yaml
    DESTINATION
        ${MODEL_PATH}/data
)

install(
    FILES
        ../../model.yaml
    DESTINATION
        data
)
