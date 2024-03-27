<!--
Copyright 2024 Robert Bosch GmbH

SPDX-License-Identifier: Apache-2.0
-->

# Dynamic Simulation Environment - Network Model

[![CI](https://github.com/boschglobal/dse.network/actions/workflows/ci.yaml/badge.svg)](https://github.com/boschglobal/dse.network/actions/workflows/ci.yaml)
[![Super Linter](https://github.com/boschglobal/dse.network/actions/workflows/super-linter.yml/badge.svg)](https://github.com/boschglobal/dse.network/actions/workflows/super-linter.yml)
![GitHub](https://img.shields.io/github/license/boschglobal/dse.network)


## Introduction

Network Model of the Dynamic Simulation Environment (DSE) Core Platform.


### Project Structure

```
L- dse/network  Network Model source code.
L- extra        Build infrastructure.
  L- docker     Containerised tools.
L- licenses     Third Party Licenses.
L- tests        Unit and integration tests.
```


## Usage

### Toolchains

The Network Model is built using containerised toolchains. Those are
available from the DSE C Library and can be built as follows:

```bash
$ git clone https://github.com/boschglobal/dse.clib.git
$ cd dse.clib
$ make docker
```

Alternatively, the latest Docker Images are available on ghcr.io and can be
used as follows:

```bash
$ export GCC_BUILDER_IMAGE=ghcr.io/boschglobal/dse-gcc-builder:main
```


### Build

```bash
# Get the repo.
$ git clone https://github.com/boschglobal/dse.network.git
$ cd dse.network

# Optionally set builder images.
$ export GCC_BUILDER_IMAGE=ghcr.io/boschglobal/dse-gcc-builder:main

# Build.
$ make

# Run tests.
$ make test

# Remove (clean) temporary build artifacts.
$ make clean
$ make cleanall
```


### Example

The Network Model includes a sample Network (CAN database) which can be used to
generate a Message Library. This Message Library is then loaded by the
Network Model and together they provide a connection between the Signal and
Network interfaces of a Simulation.

The following commands illustrate the process of generating a Message Library.

```bash
# Get the example from repo.
$ git clone https://github.com/boschglobal/dse.network.git
$ cd dse.network/dse/network/examples/stub

# Generate C files (stub.h & stub.c).
$ python -m cantools generate_c_source stub.dbc

# Generate the Network configuration (network.yaml).
$ dse.codegen cantools-network \
    --input stub.h \
    --output network.yaml \
    --dbc stub.dbc \
    --message_lib examples/stub/lib/message.so \
    --node_id 2 \
    --interface_id 3 \
    --bus_id 4

# Generate a Signal Group (signalgroup.yaml) to represent Signals in the Simulation.
$ dse.convert signalgroup \
    --input network.yaml \
    --output signalgroup.ymal \
    --name "signal" \
    --labels "{'channel': 'signal_vector'}"
```

After those steps the Message Library can be configured as a part of a
Simulation. The user documentation contains full details of how to build,
configure and use the Network Model. The `dse/network/examples/stub` folder
includes a complete example configuration.


## Contribute

Please refer to the [CONTRIBUTING.md](./CONTRIBUTING.md) file.


## License

Dynamic Simulation Environment Network Model is open-sourced under the Apache-2.0 license.
See the [LICENSE](LICENSE) and [NOTICE](./NOTICE) files for details.


### Third Party Licenses

[Third Party Licenses](licenses/)
