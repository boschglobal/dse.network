#!/bin/bash

# Copyright 2024 Robert Bosch GmbH
#
# SPDX-License-Identifier: Apache-2.0


: "${GENCODE_EXE:=/usr/local/bin/gencode}"
: "${NETWORK_EXE:=/usr/local/bin/network}"

function print_usage () {
    echo "Network Tools"
    echo ""
    echo "  network <command> [command options,]"
    echo ""
    echo "Commands:"
    echo "  gen-code"
    echo "  [network]  (network commands)"
    echo ""
    echo "Example:"
    echo "  docker run --rm -v $(pwd):/sim network:test gen-code -h"
    echo "  docker run --rm -v $(pwd):/sim network:test help"
    exit 1
}

if [ $# -eq 0 ]; then print_usage; fi


echo "Command is $1"
if [ "$1" == "gen-code" ]; then
    CMD="$GENCODE_EXE"
else
    CMD="$NETWORK_EXE $1"
fi

# Run the command
if [ -z ${CMD+x} ]; then print_usage; fi
shift
echo $CMD "$@"
$CMD "$@"
