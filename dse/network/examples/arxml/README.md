<!--
Copyright 2024 Robert Bosch GmbH

SPDX-License-Identifier: Apache-2.0
-->

# Tutorial : ARXML

Tutorial example showing how to configure and use a Network Model with ARXML as an input.


## Usage

```bash
# Define a shell function for the Simer tool.
$ export SIMER_IMAGE=ghcr.io/boschglobal/dse-simer:latest
$ simer() { ( cd "$1" && shift && docker run -it --rm -v $(pwd):/sim $SIMER_IMAGE "$@"; ) }

# Download and run the tutorial simulation ...
$ export NETWORK_URL=https://github.com/boschglobal/dse.network/releases/download/v1.0.3/Network-1.0.3-linux-amd64.zip
$ curl -fSL -o /tmp/network.zip $NETWORK_URL; unzip -d /tmp /tmp/network.zip
$ simer /tmp/Network-1.0.3-linux-amd64/examples/arxml

# Or build locally and then run the tutorial simulation ..
$ git clone https://github.com/boschglobal/dse.network.git
$ cd dse.network
$ make
$ cd dse/network/build/_out/examples/arxml
$ curl https://raw.githubusercontent.com/nikidimitrow/Learning-AUTOSAR-fundamental/master/BasicsOfAUTOSAR/MyECU.ecuc.arxml --output stub.arxml
$ task --force generate ARXML=stub.arxml 
$ task set-mimetype \
    OUTDIR=stub/stub.dbc \
    SIGNAL=can \
    MIMETYPE='application/x-automotive-bus; interface=stream; type=frame; bus=can; schema=fbs; bus_id=1; node_id=2; interface_id=1'
$ simer . -endtime 0.050
```

Documentation for the `simer` tool is available here : https://boschglobal.github.io/dse.doc/docs/user/simer
