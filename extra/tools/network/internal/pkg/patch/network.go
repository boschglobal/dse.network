// Copyright 2024 Robert Bosch GmbH
//
// SPDX-License-Identifier: Apache-2.0

package patch

import (
	"encoding/csv"
	"flag"
	"fmt"
	"log/slog"
	"os"
	"unicode/utf8"

	"gopkg.in/yaml.v3"

	"github.com/boschglobal/dse.schemas/code/go/dse/kind"
)

type PatchNetworkCommand struct {
	name string
	fs   *flag.FlagSet

	networkFile       string
	functionPatchFile string
	signalPatchFile   string
	removeUnknown     bool
}

func NewPatchNetworkCommand(name string) *PatchNetworkCommand {
	c := &PatchNetworkCommand{name: name, fs: flag.NewFlagSet(name, flag.ExitOnError)}
	c.fs.StringVar(&c.networkFile, "network", "", "network file")
	c.fs.StringVar(&c.functionPatchFile, "function_patch", "", "path to function patch file, yaml format")
	c.fs.StringVar(&c.signalPatchFile, "signal_patch", "", "path to signal patch file, csv format")
	c.fs.BoolVar(&c.removeUnknown, "remove_unknown", false, "remove signals not included in signal patch file")
	return c
}

func (c PatchNetworkCommand) Name() string {
	return c.name
}

func (c PatchNetworkCommand) FlagSet() *flag.FlagSet {
	return c.fs
}

func (c *PatchNetworkCommand) Parse(args []string) error {
	return c.fs.Parse(args)
}

func (c *PatchNetworkCommand) Run() error {
	// Load the Network.
	networkYaml, err := os.ReadFile(c.networkFile)
	if err != nil {
		return fmt.Errorf("Unable to open network file: %s (%w)", c.networkFile, err)
	}
	network := kind.Network{}
	if err := yaml.Unmarshal(networkYaml, &network); err != nil {
		return fmt.Errorf("Unable to unmarshal network yaml: %s (%w)", c.networkFile, err)
	}

	// Patch the Network.
	if c.functionPatchFile != "" {
		if err := patchFunctions(&network, c.functionPatchFile); err != nil {
			return fmt.Errorf("Error with function patch: %v", err)
		}
	}
	if c.signalPatchFile != "" {
		if err := patchSignals(&network, c.signalPatchFile, c.removeUnknown); err != nil {
			return fmt.Errorf("Error with signal patch: %v", err)
		}
	}

	// Write the patched Network.
	y, err := yaml.Marshal(&network)
	if err != nil {
		return fmt.Errorf("Error marshalling yaml: %v", err)
	}
	err = os.WriteFile(c.networkFile, y, 0644)
	if err != nil {
		return fmt.Errorf("Error writing yaml: %v", err)
	}
	return nil
}

type FunctionPatch []struct {
	Message   string                `yaml:"message"`
	Functions kind.NetworkFunctions `yaml:"functions"`
}

func patchFunctions(network *kind.Network, patchFile string) error {
	// Load patch and create patch map.
	y, err := os.ReadFile(patchFile)
	if err != nil {
		return fmt.Errorf("Unable to open patch file: %s (%w)", patchFile, err)
	}
	fp := FunctionPatch{}
	err = yaml.Unmarshal(y, &fp)
	if err != nil {
		return fmt.Errorf("Unable to unmarshal patch file (%w)", err)
	}
	patchIndex := map[string]kind.NetworkFunctions{} // Message -> Functions
	for _, patch := range fp {
		patchIndex[patch.Message] = patch.Functions
	}

	// Apply the patch.
	for idx, m := range network.Spec.Messages {
		if functions, ok := patchIndex[m.Message]; ok {
			network.Spec.Messages[idx].Functions = &functions
			slog.Info(fmt.Sprintf("Patch Functions: message: %s", m.Message))
		}
	}
	return nil
}

func patchSignals(network *kind.Network, patchFile string, removeUnknown bool) error {
	// Load the CSV patch file.
	f, err := os.Open(patchFile)
	if err != nil {
		return fmt.Errorf("Unable to open patch file: %s (%w)", patchFile, err)
	}
	r := csv.NewReader(f)
	r.Comma = ','
	r.TrimLeadingSpace = true
	record, err := r.Read()
	if err != nil {
		return fmt.Errorf("Unable to read csv (%w)", err)
	}
	if record[0] != "source" || record[1] != "target" {
		return fmt.Errorf("Unexpected csv columns (%v)", record)
	}

	// Load the patch map.
	records, err := r.ReadAll()
	if err != nil {
		return fmt.Errorf("Unable to read csv (%w)", err)
	}
	patch := map[string]string{}
	for _, record := range records {
		if record[1] != "" {
			patch[record[0]] = record[1]
		} else {
			patch[record[0]] = record[0]
		}
	}

	// Apply the patch (remove if target=inactive or removeUnknown=true).
	for _, m := range network.Spec.Messages {
		if m.Signals == nil {
			continue
		}
		i := 0
		for _, s := range *m.Signals {
			if target, ok := patch[s.Signal]; ok {
				if target != "inactive" {
					slog.Info(fmt.Sprintf("Patch Signals: patch: %s -> %s", s.Signal, target))
					s.Signal = target
					(*m.Signals)[i] = s
					i++
					continue
				}
			} else {
				if removeUnknown == false {
					(*m.Signals)[i] = s
					i++
					continue
				}
			}
			slog.Info(fmt.Sprintf("Patch Signals: remove: %s", s.Signal))
		}
		*m.Signals = (*m.Signals)[:i]
	}

	return nil
}

func trimRune(s string) string {
	_, size := utf8.DecodeRuneInString(s)
	if size > 1 {
		return s[size:]
	}
	return s
}
