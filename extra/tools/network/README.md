<!--
Copyright 2024 Robert Bosch GmbH

SPDX-License-Identifier: Apache-2.0
-->

# Network Toolset

Containerised Network toolset.


## Usage

```bash
# Build the network examples.
$ make

# Change to the simulation root directory.
$ cd dse/network/build/_out/examples/brake-by-wire

# Generate network code and configuration files.
$ task generate \
    DBCFILE=networks/brake/brake.dbc \
    SIGNAL=can \
    MIMETYPE="application/x-automotive-bus; interface=stream; type=frame; bus=can; schema=fbs; bus_id=1; node_id=1; interface_id=1"
$ task generate \
    DBCFILE=networks/vehicle/vehicle.dbc \
    SIGNAL=can \
    MIMETYPE="application/x-automotive-bus; interface=stream; type=frame; bus=can; schema=fbs; bus_id=1; node_id=2; interface_id=1"

# Run the simulation.
$ simer . -endtime 0.04
```


## Integration Testing

### Building Artifacts

```bash
# Build the examples.
$ make cleanall
$ ls -R dse/network/build/_out/examples/brake-by-wire/networks/*
dse/network/build/_out/examples/brake-by-wire/networks/brake:
brake.dbc*

dse/network/build/_out/examples/brake-by-wire/networks/vehicle:
vehicle.dbc*

# Build the tool.
$ cd extra/tools/network
$ make

# Build the Network Toolset container (from repo root).
$ make tools

# Run the Task (which uses the Toolset container).
$ task --force generate DBCFILE=stub/stub.dbc
```


## Development

### Go Module Update (schema updates)

```bash
$ go clean -modcache
$ go mod tidy

$ export GOPRIVATE=github.com/boschglobal,github.boschdevcloud.com
$ go get github.com/boschglobal/dse.schemas/code/go/dse@v1.2.7
$ go get github.boschdevcloud.com/fsil/fsil.go/ast
```


### Go Module Vendor

> NOTE: Its not clear if this partial-vendoring strategy works (go build -mod=mod ...). See `GOFLAGS` to manipulate.

```bash
# Vendor the project.
$  go mod vendor

# Remove public modules.
$ rm -rf vendor/github.com
$ rm -rf vendor/golang.org/
$ rm -rf vendor/gopkg.in/
```
