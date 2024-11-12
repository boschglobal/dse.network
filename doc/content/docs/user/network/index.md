---
title: "Network - Network Model"
linkTitle: "Network"
weight: 800
tags:
- Network
- Model
github_repo: "https://github.com/boschglobal/dse.network"
github_subdir: "doc"
---

## Synposis

Network Model of a Communication Stack representing the connection between Physical Signals and Network Messages.

```bash
# Build a network model.
$ cd path/to/simulation;
$ task generate \
    DBCFILE=networks/can_1/can_1.dbc \
    SIGNAL=can_1 \
    MIMETYPE="application/x-automotive-bus; interface=stream; type=frame; bus=can; schema=fbs; bus_id=1; node_id=1; interface_id=0"
$ cd -

# Run the simulation.
$ simer path/to/simulation -stepsize 0.0005 -endtime 0.04
```


## Simulation Setup

### Structure

#### Network Example Simulation

```text
# Example Source Code:
L- dse/network/examples/brake-by-wire
  L- simulation.yaml          Stack definitions.
  L- model.yaml               Model definitions.
  L- brake/                   Brake model source code.
  L- driver/                  Driver model source code.
  L- network/
    L- brake/brake.dbc        Brake network definition.
    L- vehicle/vehicle.dbc    Vehicle network definition.
  L- pedal/                   Pedal model source code.
  L- safety/                  Safety model source code.
  L- CMakeLists.txt           Build script.

# Packaged Simulation:
L- dse/network/build/_out/examples/brake-by-wire    <== simulation path
  L- simulation.yaml          Simulation definition.
  L- models
    L- brake                  Brake model.
    L- driver                 Driver model.
    L- network
      L- data/model.yaml      Network model definition (common to all networks).
      L- lib/network.so       Network model library (common to all networks).
    L- pedal                  Pedal model.
    L- safety                 Safety model.
  L- networks
    L- brake
      L- brake.dbc            Brake network definition.
      L- model.yaml           Model definition.
      L- network.yaml         Network definition.
      L- signalgroup.yaml     Signal definitions.
      L- message.so           Network network library.
    L- vehicle
      L- vehicle.dbc          Vehicle network definition.
      L- model.yaml           Model definition.
      L- network.yaml         Network definition.
      L- signalgroup.yaml     Signal definitions.
      L- message.so           Vehicle network library.
```


## Network Setup

### CAN DBC Conversion

> DOC: Provide description of network Taskfile workflows.


### Network Functions

> DOC: Provide description of network functions.


### Network Codec

> DOC: Provide description of network codec configuration.


## Network Operation

### Network Tracing

Network messages can be traced by setting environment variables for a specific model.

```bash
# Trace for the MIMEtype:
# application/x-automotive-bus; interface=stream; type=frame; bus=can; schema=fbs; bus_id=1; node_id=2; interface_id=3
#   bus=CAN
#   bus_id=1

# Trace individual frames:
$ simer path/to/simulation \
    -env network_inst:NCODEC_TRACE_CAN_1=0x3ea,0x3eb \
    -stepsize 0.0005 -endtime 0.04

# Trace all frames:
$ simer path/to/simulation \
    -env network_inst:NCODEC_TRACE_CAN_1=* \
    -stepsize 0.0005 -endtime 0.04
```
