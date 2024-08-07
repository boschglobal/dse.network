# Copyright 2024 Robert Bosch GmbH
#
# SPDX-License-Identifier: Apache-2.0

# List Open Source Software project here, setting External Project URLs
# according. This list may be used to generate archives of OSS Code. All
# OSS Projects can be listed here, even if they are not used by the
# External Projects, to maintain an accurate inventory of OSS Projects.

set(ExternalProject__YAML__URL       https://github.com/yaml/libyaml/archive/0.2.5.tar.gz)
set(ExternalProject__CLIB__URL       https://github.com/boschglobal/dse.clib/archive/refs/tags/v$ENV{DSE_CLIB_VERSION}.zip)
set(ExternalProject__MODELC__URL     https://github.com/boschglobal/dse.modelc/archive/refs/tags/v$ENV{DSE_MODELC_VERSION}.zip)
set(ExternalProject__MODELC_LIB__URL https://github.com/boschglobal/dse.modelc/releases/download/v$ENV{DSE_MODELC_VERSION}/ModelC-$ENV{DSE_MODELC_VERSION}-$ENV{PACKAGE_ARCH}.zip)
set(ExternalProject__CANTOOLS__URL   https://github.com/cantools/cantools/archive/refs/tags/39.4.5.tar.gz)
set(ExternalProject__DLFCNWIN32__URL https://github.com/dlfcn-win32/dlfcn-win32/archive/refs/tags/v1.3.0.tar.gz)
