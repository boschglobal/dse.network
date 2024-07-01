# Copyright 2024 Robert Bosch GmbH
#
# SPDX-License-Identifier: Apache-2.0

import os
import shutil
import argparse
import glob
from canmatrix.cli import convert


def parse_arguments():
    parser = argparse.ArgumentParser(description="Generate DBC files from an arxml" )
    parser.add_argument("arxml", help="ARXML file")
    parser.add_argument("output_directory", help="Output directory path")
    return parser.parse_args()


def convert_arxml(arxml_file, out_dir):
    # Convert the ARXML file.
    arxml_basename = os.path.basename(os.path.splitext(arxml_file)[0])
    dbc_base_filename = out_dir+os.path.sep+arxml_basename+'.dbc'
    convert.cli_convert([arxml_file, dbc_base_filename], standalone_mode=False)

    # Relocate the DBC files for later processing.
    for file in glob.glob(out_dir+os.path.sep+'*.dbc'):
        dbcname = os.path.basename(os.path.splitext(file)[0]).lower()
        os.makedirs(os.path.join(out_dir, dbcname), exist_ok=True)
        shutil.move(file, os.path.join('./', out_dir, dbcname, dbcname + '.dbc'))


def main():
    args = parse_arguments()
    convert_arxml(args.arxml, args.output_directory)


if __name__ == "__main__":
    main()
