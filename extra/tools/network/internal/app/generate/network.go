// Copyright 2024 Robert Bosch GmbH
//
// SPDX-License-Identifier: Apache-2.0

package generate

import (
	"flag"
	"fmt"
	"os"
	"slices"
	"strconv"
	"strings"

	"gopkg.in/yaml.v3"

	"golang.org/x/text/cases"
	"golang.org/x/text/language"

	"github.boschdevcloud.com/fsil/fsil.go/ast"
	"github.com/boschglobal/dse.schemas/code/go/dse/kind"
)

type GenNetworkCommand struct {
	name string
	fs   *flag.FlagSet

	dbcName      string
	headerFile   string
	metadataFile string
	messageLib   string
	functionLib  string
	outputFile   string
	signalStyle  string

	styles []string
}

func NewGenNetworkCommand(name string) *GenNetworkCommand {
	c := &GenNetworkCommand{
		name:   name,
		fs:     flag.NewFlagSet(name, flag.ExitOnError),
		styles: []string{"PascalCase", "camelCase", "snake_case"},
	}
	c.fs.StringVar(&c.dbcName, "name", "", "network name; typically the name (lower case, without extension) of the can dbc file")
	c.fs.StringVar(&c.headerFile, "header", "", "generated header file")
	c.fs.StringVar(&c.metadataFile, "metadata", "", "generated frame metadata file")
	c.fs.StringVar(&c.outputFile, "network", "", "path to write the generated network file")
	c.fs.StringVar(&c.messageLib, "message_lib", "", "path to message library")
	c.fs.StringVar(&c.functionLib, "function_lib", "", "path to function library")
	c.fs.StringVar(&c.signalStyle, "style", "PascalCase", "select from: PascalCase, camelCase, snake_case")
	return c
}

func (c GenNetworkCommand) Name() string {
	return c.name
}

func (c GenNetworkCommand) FlagSet() *flag.FlagSet {
	return c.fs
}

func (c *GenNetworkCommand) Parse(args []string) error {
	if err := c.fs.Parse(args); err != nil {
		return err
	}
	if slices.Contains(c.styles, c.signalStyle) == false {
		return fmt.Errorf("Parameter signalStyle not one of: %s", c.styles)
	}
	return nil
}

func (c *GenNetworkCommand) Run() error {
	fmd, err := loadFrameMetadata(c.metadataFile)
	if err != nil {
		return err
	}
	idx, err := loadAst(c.headerFile)
	if err != nil {
		return err
	}

	// Construct the network configuration.
	annotations := kind.Annotations{
		"message_lib": c.messageLib,
	}
	if c.functionLib != "" {
		annotations["function_lib"] = c.functionLib
	}
	net := kind.Network{
		Kind: "Network",
		Metadata: &kind.ObjectMetadata{
			Name:        stringPtr(c.dbcName),
			Annotations: &annotations,
		},
	}
	// Messages (based on structNames, not Typedefs).
	for structName, signalList := range idx.Structs {
		name := getMessageName(structName, c.dbcName)
		message := kind.NetworkMessage{Message: name}
		annotations := kind.Annotations{}
		frameInfo := findFrameInfo(fmd, structName)
		if frameInfo != nil {
			annotations["frame_id"] = frameInfo.FrameId
			annotations["frame_length"] = frameInfo.FrameLength
			annotations["frame_type"] = strconv.Itoa(getFrameType(frameInfo))
			if frameInfo.CycleTime != "" {
				annotations["cycle_time_ms"] = frameInfo.CycleTime
			}
		}
		annotations["struct_name"] = structName
		// Signals (field/member names of the struct).
		signals := []kind.NetworkSignal{}
		offset := 0
		for _, s := range signalList {
			name := getSignalName(s.Name, c.signalStyle)
			signal := kind.NetworkSignal{Signal: name}
			typeSize, err := getTypeSize(s.TypeName)
			if err != nil {
				return err
			}
			// Calculate the offset (type alignment).
			if offset%typeSize != 0 {
				offset = ((offset / typeSize) * typeSize) + typeSize
			}
			annotations := kind.Annotations{}
			annotations["struct_member_name"] = s.Name
			annotations["struct_member_offset"] = strconv.Itoa(offset)
			annotations["struct_member_primitive_type"] = s.TypeName
			signal.Annotations = &annotations
			signals = append(signals, signal)
			// Set offset for next iteration.
			offset += typeSize
		}
		message.Signals = &signals
		// Calculate the final type size/offset.
		if offset%8 != 0 {
			offset = ((offset / 8) * 8) + 8
		}
		annotations["struct_size"] = strconv.Itoa(offset)
		message.Annotations = &annotations
		net.Spec.Messages = append(net.Spec.Messages, message)
	}

	// Write the network configuration file.
	fmt.Fprintf(flag.CommandLine.Output(), "Writing file: %s\n", c.outputFile)
	return writeYaml(&net, c.outputFile, false)
}

type FrameInfo struct {
	FrameId       string `yaml:"frame_id"`
	FrameLength   string `yaml:"frame_length"`
	CycleTime     string `yaml:"cycle_time_ms"`
	CanFD         bool   `yaml:"is_can_fd"`
	ExtendedFrame bool   `yaml:"is_extended_frame"`
}

type FrameMetadata struct {
	Frames map[string]FrameInfo `yaml:"frames"`
}

func loadFrameMetadata(path string) (*FrameMetadata, error) {
	y, err := os.ReadFile(path)
	if err != nil {
		return nil, fmt.Errorf("Unable to open frame metadata: %s (%w)", path, err)
	}
	fmd := FrameMetadata{}
	err = yaml.Unmarshal(y, &fmd)
	if err != nil {
		return nil, fmt.Errorf("Unable to unmarshal frame metadata (%w)", err)
	}
	return &fmd, nil
}

func findFrameInfo(fmd *FrameMetadata, name string) *FrameInfo {
	caser := cases.Title(language.Und)
	parts := []string{}
	for _, part := range strings.Split(name, "_") {
		part = caser.String(part)
		parts = append(parts, part)
	}
	frameName := strings.Join(parts[1:len(parts)-1], "")
	if info, ok := fmd.Frames[frameName]; ok {
		return &info
	}
	return nil
}

func getMessageName(structName string, dbcName string) string {
	dbcName = strings.ToLower(dbcName)
	name := structName[:len(structName)-2]
	if strings.Index(name, dbcName+"_") == 0 {
		name = strings.Replace(name, dbcName+"_", "", 1)
	}
	return name
}

func getSignalName(name string, style string) string {
	caser := cases.Title(language.Und)
	parts := []string{}
	switch style {
	case "PascalCase":
		for _, part := range strings.Split(name, "_") {
			part = caser.String(part)
			parts = append(parts, part)
		}
		name = strings.Join(parts, "")
	case "camelCase":
		for i, part := range strings.Split(name, "_") {
			if i != 0 {
				part = caser.String(part)
			}
			parts = append(parts, part)
		}
		name = strings.Join(parts, "")
	case "snake_case":
		for _, part := range strings.Split(name, "_") {
			parts = append(parts, part)
		}
		name = strings.Join(parts, "_")
	}

	return name
}

func getFrameType(f *FrameInfo) int {
	frameType := 0
	if f.CanFD {
		frameType += 2
	}
	if f.ExtendedFrame {
		frameType += 1
	}
	return frameType
}

func loadAst(path string) (*ast.Index, error) {
	index := ast.Index{}
	ast := ast.Ast{
		Path: path,
	}
	err := ast.Load()
	if err != nil {
		return nil, err
	}
	ast.Parse(&index)
	return &index, nil
}

var dataTypeSize map[string]int

func init() {
	dataTypeSize = map[string]int{
		"float":        4,
		"double":       8,
		"int":          4,
		"char":         1,
		"short":        2,
		"unsigned int": 4,
		"long":         4,
		"uint8_t":      1,
		"uint16_t":     2,
		"uint32_t":     4,
		"uint64_t":     8,
		"int8_t":       1,
		"int16_t":      2,
		"int32_t":      4,
		"int64_t":      8,
	}
}

func getTypeSize(t string) (int, error) {
	if size, ok := dataTypeSize[t]; ok {
		return size, nil
	}
	return 0, fmt.Errorf("Unknown type size: %s", t)
}
