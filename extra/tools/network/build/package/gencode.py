# Copyright 2024 Robert Bosch GmbH
#
# SPDX-License-Identifier: Apache-2.0

import os
import re
import json
import yaml
import argparse
import cantools
from cantools.database.can.c_source import camel_to_snake_case, generate
from cantools.subparsers.generate_c_source import _do_generate_c_source


def parse_arguments():
    parser = argparse.ArgumentParser(description="Generate C source from DBC file if the input file is of type DBC.")
    parser.add_argument("infile", help="Input file path [DBC]")
    parser.add_argument("output_directory", help="Output directory path")
    parser.add_argument("--encoding", default="utf-8", help="Encoding of the DBC file")
    parser.add_argument("--prune", action="store_true", help="Prune unused message and signal definitions")
    parser.add_argument("--no-strict", action="store_true", help="Disable strict checking")
    parser.add_argument("--database-name", help="Database name")
    parser.add_argument("--no-floating-point-numbers", action="store_true", help="Disable floating point numbers")
    parser.add_argument("--bit-fields", action="store_true", help="Enable bit fields")
    parser.add_argument("--use-float", action="store_true", help="Use float instead of double")
    parser.add_argument("--node", help="Node name")
    parser.add_argument("--keepdbcnames", action="store_true", help="Keep DBC names")
    parser.add_argument("--generate-fuzzer", action="store_true", help="Generate fuzzer code")
    parser.add_argument('--filter', nargs='*', help='Filters the frames based on frame ID.')
    parser.add_argument('--cycle_time', help='missing cycle time in dictionary format with key being frame ID . ex- "{ ""400"": ""20"", ""530"":""30"" }"')
    return parser.parse_args()

def scan_messages(dbc_file, out_path, filter, cycle_time):
    outfile = os.path.join(out_path, os.path.basename(os.path.splitext(dbc_file)[0]) + '.yaml')
    db = cantools.database.load_file(dbc_file)
    frames = {}
    cycle_time = json.loads(cycle_time) if cycle_time else {}
    for message in db.messages:
        if filter is not None and str(message.frame_id) in filter:
            continue
        isContainer = False
        if len(message.signal_groups) > 0 :
            isContainer = True
        messageName = camel_to_snake_case(message.name)
        frames[messageName] = {
            'frame_id': int(message.frame_id),
            'frame_length': int(message.length),
            'cycle_time_ms': int(message.cycle_time) if message.cycle_time else None,
            'is_can_fd': message.is_fd,
            'is_extended_frame': message.is_extended_frame,
            'is_container': isContainer
        }
        if str(message.frame_id) in cycle_time.keys():
            frames[messageName]['cycle_time_ms'] = int(cycle_time[str(message.frame_id)])
        if isContainer :
            for mux_id, signals in message.signal_tree[0]['Header_ID'].items():
                frames[messageName + '_' + hex(mux_id)] = {
                    'container': messageName,
                    'container_mux_id': int(mux_id),
                    'frame_id': int(message.frame_id),
                    'frame_length': int(message.length),
                    'cycle_time_ms': int(message.cycle_time) if message.cycle_time else None,
                    'is_can_fd': message.is_fd,
                    'is_extended_frame': message.is_extended_frame,
                    'signals' : [camel_to_snake_case(s) for s in signals]
        }

    with open(outfile, 'w') as f:
        yaml.dump({'frames': frames}, f)


def camel_to_snake_case(value):
    value = re.sub(r'(.)([A-Z][a-z]+)', r'\1_\2', value)
    value = re.sub(r'(_+)', '_', value)
    value = re.sub(r'([a-z0-9])([A-Z])', r'\1_\2', value).lower()

    return value


def main():
    args = parse_arguments()
    _do_generate_c_source(args)
    scan_messages(args.infile, args.output_directory, args.filter, args.cycle_time)

if __name__ == "__main__":
    main()
