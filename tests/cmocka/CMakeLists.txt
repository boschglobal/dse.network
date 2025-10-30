# Copyright 2024 Robert Bosch GmbH
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.21)
cmake_policy(SET CMP0135 NEW)

# set(CMAKE_VERBOSE_MAKEFILE ON)

project(test_dse_network)

set(CMAKE_ENABLE_EXPORTS ON)

include(FetchContent)
include(GNUInstallDirs)
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/_out)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -ggdb")
list(APPEND C_CXX_WARNING_FLAGS
    -Wall
    -W
    -Wwrite-strings
    -Wno-missing-field-initializers
    -Wno-misleading-indentation
)
add_compile_options(${C_CXX_WARNING_FLAGS})


# External Project - yaml
# -----------------------
set(YAML_SOURCE_DIR "$ENV{EXTERNAL_BUILD_DIR}/yaml")
set(YAML_BINARY_DIR "$ENV{EXTERNAL_BUILD_DIR}/yaml")
find_library(YAML_LIB
    NAMES
        libyaml.a
    PATHS
        ${YAML_BINARY_DIR}
    REQUIRED
    NO_DEFAULT_PATH
)
add_library(yaml STATIC IMPORTED GLOBAL)
set_target_properties(yaml
    PROPERTIES
        IMPORTED_LOCATION "${YAML_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${YAML_BINARY_DIR}"
)


# External Project - DSE ModelC
# -----------------------------
FetchContent_Declare(dse_modelc
    URL                 ${ExternalProject__MODELC__URL}
    HTTP_USERNAME       ${ExternalProject__MODELC__USERNAME}
    HTTP_PASSWORD       ${ExternalProject__MODELC__PASSWORD}
    SOURCE_DIR          "$ENV{EXTERNAL_BUILD_DIR}/dse.modelc"
)
FetchContent_MakeAvailable(dse_modelc)
set(DSE_MODELC_SOURCE_DIR ${dse_modelc_SOURCE_DIR}/dse/modelc)
set(DSE_MODELC_SOURCE_FILES
    ${DSE_MODELC_SOURCE_DIR}/model/schema.c
)
set(DSE_MODELC_INCLUDE_DIR "${DSE_MODELC_SOURCE_DIR}/../..")


# External Project - DSE ModelC Library (for linking to mstep)
# -------------------------------------
set(MODELC_BINARY_DIR "$ENV{EXTERNAL_BUILD_DIR}/dse.modelc.lib")
find_library(MODELC_LIB
    NAMES
        libmodelc_bundled.a
    PATHS
        ${MODELC_BINARY_DIR}/lib
    REQUIRED
    NO_DEFAULT_PATH
)
add_library(modelc STATIC IMPORTED GLOBAL)
set_target_properties(modelc
    PROPERTIES
        IMPORTED_LOCATION "${MODELC_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${MODELC_BINARY_DIR}"
)
set(DSE_MODELC_LIB_INCLUDE_DIR "${MODELC_BINARY_DIR}/include")
set(DSE_MODELC_LIB_MOCK_SOURCE_DIR "${MODELC_BINARY_DIR}/mocks")
set(DSE_MODELC_LIB_MOCK_SOURCE_FILES
    ${DSE_MODELC_LIB_MOCK_SOURCE_DIR}/simmock.c
)


# External Project - DSE C Lib
# ----------------------------
FetchContent_Declare(dse_clib
    URL                 ${ExternalProject__CLIB__URL}
    HTTP_USERNAME       ${ExternalProject__CLIB__USERNAME}
    HTTP_PASSWORD       ${ExternalProject__CLIB__PASSWORD}
    SOURCE_DIR          "$ENV{EXTERNAL_BUILD_DIR}/dse.clib"
)
FetchContent_MakeAvailable(dse_clib)
set(DSE_CLIB_SOURCE_DIR ${dse_clib_SOURCE_DIR}/dse/clib)
set(DSE_CLIB_SOURCE_FILES
    ${DSE_CLIB_SOURCE_DIR}/util/yaml.c
    ${DSE_CLIB_SOURCE_DIR}/collections/hashmap.c
    ${DSE_CLIB_SOURCE_DIR}/collections/set.c
)
set(DSE_CLIB_INCLUDE_DIR "${DSE_CLIB_SOURCE_DIR}/../..")



# Set the project paths
# =====================
set(DSE_NETWORK_SOURCE_DIR ../../dse/network)
set(DSE_NETWORK_SOURCE_FILES
    ${DSE_NETWORK_SOURCE_DIR}/loader.c
    ${DSE_NETWORK_SOURCE_DIR}/parser.c
    ${DSE_NETWORK_SOURCE_DIR}/engine.c
    ${DSE_NETWORK_SOURCE_DIR}/network.c
    ${DSE_NETWORK_SOURCE_DIR}/encoder.c
    ${DSE_NETWORK_SOURCE_DIR}/function.c
    ${DSE_NETWORK_SOURCE_DIR}/schedule.c
)
set(DSE_NETWORK_INCLUDE_DIR "${DSE_NETWORK_SOURCE_DIR}/../..")


# Targets
# =======

# Target - Stub
# -------------
add_subdirectory(../../dse/network build)


# Target - Network (TDD)
# ----------------------
add_executable(test_network
    network/__test__.c
    network/test_loader.c
    network/test_parser.c
    network/test_engine.c
    network/test_function.c

    ${DSE_NETWORK_SOURCE_FILES}
    ${DSE_CLIB_SOURCE_FILES}
    ${DSE_MODELC_SOURCE_FILES}
)
target_include_directories(test_network
    PRIVATE
        ${DSE_NETWORK_INCLUDE_DIR}
        ${DSE_MODELC_INCLUDE_DIR}
        ${DSE_MODELC_LIB_INCLUDE_DIR}
        ${DSE_CLIB_INCLUDE_DIR}
        ${YAML_SOURCE_DIR}/include
        ./
)
target_compile_definitions(test_network
    PUBLIC
        CMOCKA_TESTING
)
target_link_libraries(test_network
    PUBLIC
        -Wl,-Bstatic modelc -Wl,-Bdynamic ${CMAKE_DL_LIBS}
    PRIVATE
        cmocka
        yaml
        dl
        m
)
install(TARGETS test_network)


# Target - MSTEP
# --------------
add_executable(test_mstep
    mstep/__test__.c
    mstep/test_mstep.c
    mstep/test_schedule.c
    mstep/test_container.c
    ${DSE_MODELC_LIB_MOCK_SOURCE_FILES}
    ${DSE_NETWORK_SOURCE_FILES}
    ${DSE_MODELC_SOURCE_FILES}
    ${DSE_CLIB_SOURCE_FILES}
    ${DSE_NETWORK_SOURCE_DIR}/model.c
)
target_include_directories(test_mstep
    PRIVATE
        ${DSE_NETWORK_INCLUDE_DIR}
        ${DSE_CLIB_INCLUDE_DIR}
        ${DSE_MODELC_LIB_INCLUDE_DIR}
        ${DSE_MODELC_INCLUDE_DIR}
        ${YAML_SOURCE_DIR}/include
        ./
)
target_compile_definitions(test_mstep
    PUBLIC
        CMOCKA_TESTING
    PRIVATE
        PLATFORM_OS="${CDEF_PLATFORM_OS}"
        PLATFORM_ARCH="${CDEF_PLATFORM_ARCH}"
)
target_link_libraries(test_mstep
    PUBLIC
        -Wl,-Bstatic modelc -Wl,-Bdynamic ${CMAKE_DL_LIBS}
    PRIVATE
        cmocka
        dl
        rt
        m
)
install(TARGETS test_mstep)
