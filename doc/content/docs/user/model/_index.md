---
title: "FSIL Network Model"
linkTitle: "Network Model"
weight: 70
---

## Toolchains

### Set the latest Docker Images that are available on ghcr.io to build the Network Model:

```bash
$ export GCC_BUILDER_IMAGE=ghcr.io/boschglobal/dse-gcc-builder:main
$ export GCC_TESTER_IMAGE=ghcr.io/boschglobal/dse-python-builder:main
```

## Build

### Build DSE Operational Tools with Local Build Environment:

```bash
# Environment setup.
$ export AR_USER=<user>
$ export AR_TOKEN=<token>
$ export DSE_DOCKER_REPO=artifactory.boschdevcloud.com/lab000141-emthacks-docker-local
$ export DSE_PYPI_REPO=artifactory.boschdevcloud.com/artifactory/api/pypi/lab000141-emthacks-pypi-local/simple
$ export PIP_EXTRA_INDEX_URL=https://$AR_USER:$AR_TOKEN@$DSE_PYPI_REPO
$ docker login -u $AR_USER -p $AR_TOKEN $DSE_DOCKER_REPO

# Python package.
$ pip install dse.opstools
$ pip list | grep opstools
dse.opstools                           0.1.81

# Build.
$ git clone https://github.boschdevcloud.com/bios-emthacks/dse.opstools.git
$ cd dse.opstools
$ make
$ make install
```

### Build Network with Local Build Environment:
```bash
# Get the repo.
$ git clone https://github.com/boschglobal/dse.network.git
$ cd dse.network
```

## Generate header files

### Install cantools in Local Build Environment:
```bash
$ python3 -m pip install cantools
```

###  Use the generate_c_source command to generate header files from the DBC file:

```bash
$ python -m cantools generate_c_source /path/to/stub.dbc

Successfully generated ./stub.h and ./stub.c.
```

The generated code will contain message structures, encode and decode functions(signal functions), pack and unpack functions(message functions) and frame annotations.
The maximum signal size is 64 bits.

## Configure the YAML
### 1. Use the cantools parser to generate the YAML from generated header:

Provide the sample CAN DBC and generated header file which can be found in dse/network/examples/stub, as input. This will generate a Message Library and a Function Library. The Message Library has message and signal related functions and the Function Library has PDU level functions such as CRC and alive counter.
These libraries can then be configured as part of simulation.

```bash
# Generate the Network configuration (network.yaml).
$ dse.codegen cantools-network \
    --input stub.h \
    --output network.yaml \
    --dbc stub.dbc \
    --message_lib examples/stub/lib/message.so \
    --function_lib examples/stub/lib/function.so \
    --node_id 2 \
    --interface_id 3 \
    --bus_id 4

input: stub.h
output: network.yaml
Labels: {}
conversion finished
```
### 2. Patch functions into the YAML

```bash
# For example, create a patcher YAML, patch.yaml to patch the functions into the YAML:

---
- message: function_example
  functions:
    encode:
      - function: counter_inc_uint8
        annotations:
          position: 1
      - function: crc_generate
        annotations:
          position: 0
    decode:
      - function: crc_validate
        annotations:
          position: 0

# Patch the functions:
$ dse.codegen patch --input /path/to/input/network.yaml --output network.yaml  --patcher patch.yaml

Patching Completed
```


## Build the Network model


### Check that all Sandbox files are generated after building the Network model:

```bash
# Generated in dse.network\tests\cmocka\build\_out\examples\stub\data:

0 build/_out/examples/stub/data/model.yaml
8.0K build/_out/examples/stub/data/network.yaml
4.0K build/_out/examples/stub/data/simulation.yaml
28K build/_out/examples/stub/lib/function.so
28K build/_out/examples/stub/lib/function__ut.so
64K build/_out/examples/stub/lib/message.so
```

## Verify the working of the Network model

###  Check if the Network model starts by adjusting the LOG level to LOG_SIMBUS in tests:

```bash
# Logger is set in tests\cmocka\network\__test__.c

uint8_t __log_level__ = LOG_QUIET; /* LOG_ERROR LOG_INFO LOG_DEBUG LOG_TRACE */
```
Check that all the network signals are loaded correctly.


```bash
# Refer to the logger level given below and set for appropriate logging information:

typedef enum LoggerLevel {
    LOG_TRACE = 0,
    LOG_DEBUG,
    LOG_SIMBUS, /* Log SimBus messages. */
    LOG_INFO,
    LOG_NOTICE, /* Application level messages (always printed). */
    LOG_QUIET,  /* Only prints errors, used in unit tests. */
    LOG_ERROR,  /* May print errno, if set.*/
    LOG_FATAL,  /* Will print errno and then call exit().*/
} LoggerLevel;
```

Set the logging level to LOG_DEBUG to see information on message parsing, signal encoding, and decoding.
