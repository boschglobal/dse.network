# Copyright 2024 Robert Bosch GmbH
#
# SPDX-License-Identifier: Apache-2.0

import os
import argparse
import yaml


def parse_arguments():
    parser = argparse.ArgumentParser(description="Sets mimetype and network signal" )
    parser.add_argument("--output_directory", help="Output directory path containing network.yaml and signalgroup.yaml")
    parser.add_argument("--signal", help="name of the network signal")
    parser.add_argument("--mimetype", help="mimetype containing interface id, bus id and node id")
    return parser.parse_args()

def network(out_dir, mimetype):
    with open(out_dir + os.path.sep + 'network.yaml' , 'r') as file:
        data = yaml.safe_load(file)
        if 'metadata' not in data:
            data['metadata'] = {}
        if 'annotations' not in data['metadata']:
            data['metadata']['annotations'] = {}
        bus_id, node_id, interface_id = extract_values(mimetype)
        data['metadata']['annotations']['bus_id'] = bus_id
        data['metadata']['annotations']['node_id'] = node_id
        data['metadata']['annotations']['interface_id'] = interface_id
    with open(out_dir + os.path.sep + 'network.yaml' , 'w') as f:
        yaml.dump(data, f)

def signalgroup(out_dir, mimetype, signal):
    with open(out_dir + os.path.sep + 'signalgroup.yaml', 'r') as file:
        yaml_documents = list(yaml.safe_load_all(file))
    for idx, doc in enumerate(yaml_documents):
        if 'metadata' in doc and 'annotations' in doc['metadata'] and 'vector_type' in doc['metadata']['annotations']:
            if 'spec' in doc and 'signals' in doc['spec']:
                for item in doc['spec']['signals']:
                    if 'annotations' in item and 'mime_type' in item['annotations']:
                        item['annotations']['mime_type'] = mimetype
                    if 'signal' in item:
                        item['signal'] = signal
    with open(out_dir + os.path.sep + 'signalgroup.yaml', 'w') as f:
        yaml.safe_dump_all(yaml_documents, f)
            

def extract_values(mimetype):
    parts = mimetype.split(';')
    bus_id = None
    node_id = None
    interface_id = None
    for part in parts:
        if '=' in part:
            key, value = part.strip().split('=')
            if key.strip() == 'bus_id':
                bus_id = int(value.strip())
            elif key.strip() == 'node_id':
                node_id = int(value.strip())
            elif key.strip() == 'interface_id':
                interface_id = int(value.strip())
    return bus_id, node_id, interface_id


def main():
    args = parse_arguments()
    network(args.output_directory, args.mimetype)
    signalgroup(args.output_directory, args.mimetype, args.signal)


if __name__ == "__main__":
    main()
