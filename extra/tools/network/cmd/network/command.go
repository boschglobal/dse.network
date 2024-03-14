// Copyright 2024 Robert Bosch GmbH
//
// SPDX-License-Identifier: Apache-2.0

package main

import (
	"flag"
	"fmt"
	"os"
	"reflect"
)

var usage = `
Network Tool (Network Model of the Dynamic Simulation Environment)

  network <command> [command options,]

Examples:
  network gen-network -name can -header can.h -metadata can.yaml -network network.yaml
  network patch-network -network network.yaml -signal_patch spatch.csv -function_patch fpatch.yaml
  network get-signalgroup \
    -name can \
    -network network.yaml \
    -signalgroup signalgroup.yaml \
    -signal can \
    -mimetype 'application/x-automotive-bus; interface=stream; type=frame; bus=can; schema=fbs; bus_id=1; node_id=1; interface_id=1'

Commands:
`

func PrintUsage() {
	fmt.Fprintf(flag.CommandLine.Output(), usage)

	for _, cmd := range cmds {
		if cmd.Name() == "help" {
			continue
		}

		fmt.Fprintf(flag.CommandLine.Output(), "  %s\n", cmd.Name())
		fmt.Fprintf(flag.CommandLine.Output(), "    Options:\n")
		cmd.FlagSet().VisitAll(func(f *flag.Flag) {
			fmt.Fprintf(flag.CommandLine.Output(), "      -%s %s\n", f.Name, reflect.TypeOf(f.Value))
			fmt.Fprintf(flag.CommandLine.Output(), "          %s", f.Usage)
			if f.DefValue != "" {
				fmt.Fprintf(flag.CommandLine.Output(), "  (default: %s)", f.DefValue)
			}
			fmt.Fprintf(flag.CommandLine.Output(), "\n")
		})
	}
}

type HelpCommand struct {
	name string
	fs   *flag.FlagSet
}

func NewHelpCommand(name string) *HelpCommand {
	c := &HelpCommand{
		name: name,
		fs:   flag.NewFlagSet(name, flag.ExitOnError),
	}
	return c
}

func (c HelpCommand) Name() string {
	return c.name
}

func (c HelpCommand) FlagSet() *flag.FlagSet {
	return c.fs
}

func (c *HelpCommand) Parse(args []string) error {
	return nil
}

func (c *HelpCommand) Run() error {
	PrintUsage()
	return nil
}

type CommandRunner interface {
	Name() string
	FlagSet() *flag.FlagSet
	Parse([]string) error
	Run() error
}

func DispatchCommand(name string) error {
	var cmd CommandRunner
	for _, c := range cmds {
		if c.Name() == name {
			cmd = c
			break
		}
	}
	if cmd == nil {
		return fmt.Errorf("Unknown command: %s", name)
	}

	if cmd.Name() != "help" {
		fmt.Fprintf(flag.CommandLine.Output(), "Running command %s ...\n", cmd.Name())
	}
	cmd.Parse(os.Args[2:])
	return cmd.Run()
}
