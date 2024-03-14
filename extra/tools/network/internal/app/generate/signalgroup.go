// Copyright 2024 Robert Bosch GmbH
//
// SPDX-License-Identifier: Apache-2.0

package generate

import (
	"flag"
	"fmt"
	"os"

	"github.com/boschglobal/dse.schemas/code/go/dse/kind"
	"gopkg.in/yaml.v3"
)

type GenSignalGroupCommand struct {
	commandName string
	fs          *flag.FlagSet

	name            string
	inputFile       string
	outputFile      string
	networkSignal   string
	networkMimetype string
}

func NewGenSignalGroupCommand(name string) *GenSignalGroupCommand {
	c := &GenSignalGroupCommand{commandName: name, fs: flag.NewFlagSet(name, flag.ExitOnError)}
	c.fs.StringVar(&c.name, "name", "", "network name; typically the name (lower case, without extension) of the can dbc file")
	c.fs.StringVar(&c.inputFile, "network", "", "generated network file")
	c.fs.StringVar(&c.outputFile, "signalgroup", "", "path to write generated signal group file")
	c.fs.StringVar(&c.networkSignal, "signal", "", "network signal name")
	c.fs.StringVar(&c.networkMimetype, "mimetype", "", "network signal MIME type")
	return c
}

func (c GenSignalGroupCommand) Name() string {
	return c.commandName
}

func (c GenSignalGroupCommand) FlagSet() *flag.FlagSet {
	return c.fs
}

func (c *GenSignalGroupCommand) Parse(args []string) error {
	return c.fs.Parse(args)
}

func (c *GenSignalGroupCommand) Run() error {
	// Load the Network.
	inputYaml, err := os.ReadFile(c.inputFile)
	if err != nil {
		return fmt.Errorf("Unable to open input file: %s (%w)", c.inputFile, err)
	}
	network := kind.Network{}
	if err := yaml.Unmarshal(inputYaml, &network); err != nil {
		return fmt.Errorf("Unable to unmarshal network yaml: %s (%w)", c.inputFile, err)
	}
	// Create/truncate the output file.
	fmt.Fprintf(flag.CommandLine.Output(), "Creating file: %s\n", c.outputFile)
	err = os.WriteFile(c.outputFile, []byte(""), 0644)
	if err != nil {
		return fmt.Errorf("Error writing yaml: %v", err)
	}
	// Generate the signal groups.
	if err := c.generateSignalVector(&network); err != nil {
		return err
	}
	if err := c.generateNetworkVector(&network); err != nil {
		return err
	}
	return nil
}

func (c *GenSignalGroupCommand) generateSignalVector(network *kind.Network) error {
	// Build the SignalGroup.
	signalGroup := kind.SignalGroup{
		Kind: "SignalGroup",
		Metadata: &kind.ObjectMetadata{
			Name: stringPtr(c.name),
			Labels: &kind.Labels{
				"channel": "signal_vector",
			},
		},
	}
	signals := []kind.Signal{}
	for _, m := range network.Spec.Messages {
		if m.Signals == nil {
			continue
		}
		for _, s := range *m.Signals {
			signal := kind.Signal{Signal: s.Signal}
			signals = append(signals, signal)
		}
	}
	signalGroup.Spec.Signals = signals

	// Write the SignalGroup.
	fmt.Fprintf(flag.CommandLine.Output(), "Appending file: %s\n", c.outputFile)
	return writeYaml(&signalGroup, c.outputFile, true)
}

func (c *GenSignalGroupCommand) generateNetworkVector(network *kind.Network) error {
	// Build the SignalGroup.
	signalGroup := kind.SignalGroup{
		Kind: "SignalGroup",
		Metadata: &kind.ObjectMetadata{
			Name: stringPtr(c.name),
			Labels: &kind.Labels{
				"channel": "network_vector",
			},
			Annotations: &kind.Annotations{
				"vector_type": "binary",
			},
		},
	}
	signals := []kind.Signal{
		{
			Signal: c.networkSignal,
			Annotations: &kind.Annotations{
				"network":   c.name,
				"mime_type": c.networkMimetype,
			},
		},
	}
	signalGroup.Spec.Signals = signals

	// Write the SignalGroup.
	fmt.Fprintf(flag.CommandLine.Output(), "Appending file: %s\n", c.outputFile)
	return writeYaml(&signalGroup, c.outputFile, true)
}
