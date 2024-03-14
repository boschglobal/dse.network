# Copyright 2024 Robert Bosch GmbH
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.21)

project(BrakeByWire)
set(EXAMPLE_PATH  "examples/brake-by-wire")
set(MODELS_PATH   "${EXAMPLE_PATH}/models")
set(NETWORKS_PATH "${EXAMPLE_PATH}/networks")
set(CMAKE_SHARED_LIBRARY_PREFIX "")


# Brake Model
# ===========
add_library(brake SHARED
    brake/brake.c
)
target_include_directories(brake
    PRIVATE
        ${DSE_MODELC_INCLUDE_DIR}
)
install(TARGETS brake
    LIBRARY DESTINATION
        ${MODELS_PATH}/brake/lib
    COMPONENT
        brake
)
install(
    FILES
        brake/model.yaml
    DESTINATION
        ${MODELS_PATH}/brake/data
    COMPONENT
        brake
)


# Driver Model
# ============
add_library(driver SHARED
    driver/driver.c
)
target_include_directories(driver
    PRIVATE
        ${DSE_MODELC_INCLUDE_DIR}
)
install(TARGETS driver
    LIBRARY DESTINATION
        ${MODELS_PATH}/driver/lib
    COMPONENT
        driver
)
install(
    FILES
        driver/model.yaml
    DESTINATION
        ${MODELS_PATH}/driver/data
    COMPONENT
        driver
)


# Pedal Model
# ===========
add_library(pedal SHARED
    pedal/pedal.c
)
target_include_directories(pedal
    PRIVATE
        ${DSE_MODELC_INCLUDE_DIR}
)
install(TARGETS pedal
    LIBRARY DESTINATION
        ${MODELS_PATH}/pedal/lib
    COMPONENT
        pedal
)
install(
    FILES
        pedal/model.yaml
    DESTINATION
        ${MODELS_PATH}/pedal/data
    COMPONENT
        pedal
)


# Safety Model
# ============
add_library(safety SHARED
    safety/safety.c
)
target_include_directories(safety
    PRIVATE
        ${DSE_MODELC_INCLUDE_DIR}
)
install(TARGETS safety
    LIBRARY DESTINATION
        ${MODELS_PATH}/safety/lib
    COMPONENT
        safety
)
install(
    FILES
        safety/model.yaml
    DESTINATION
        ${MODELS_PATH}/safety/data
    COMPONENT
        safety
)


# Simulation Files
# ================
install(
    FILES
        simulation.yaml
    DESTINATION
        ${EXAMPLE_PATH}
    COMPONENT
        simulation
)
install(
    FILES
        model.yaml
    DESTINATION
        ${MODELS_PATH}/network/data
    COMPONENT
        simulation
)



# Network Generation
# ==================

# Functions
# -----------
# TODO functions

# Network Brake
# -------------
install(
    FILES
        network/brake/brake.dbc
    DESTINATION
        ${NETWORKS_PATH}/brake
    COMPONENT
        network_brake
)

# Network Vehicle
# ---------------
install(
    FILES
        network/vehicle/vehicle.dbc
    DESTINATION
        ${NETWORKS_PATH}/vehicle
    COMPONENT
        network_vehicle
)
