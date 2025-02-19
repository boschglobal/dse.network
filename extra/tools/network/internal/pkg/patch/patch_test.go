
package patch

import (
	"io/ioutil"
	"os"
	"testing"

	"github.com/boschglobal/dse.schemas/code/go/dse/kind"
	"github.com/stretchr/testify/assert"
	"gopkg.in/yaml.v3"
)

func TestPatchNetworkCommand_Run(t *testing.T) {
	var functionPatch_file = "../../../test/testdata/function_patch.yaml"
	var signalPatch_file = "../../../test/testdata/signal_patch.csv"
	var network_file = "../../../test/testdata/network.yaml"

	// Copy the original file to a temporary location
	tempFile := network_file + ".temp"
	input, _ := ioutil.ReadFile(network_file)
	err := ioutil.WriteFile(tempFile, input, 0644)
	defer os.Remove(tempFile)

	cmd := NewPatchNetworkCommand("test")
	args := []string{"-network", tempFile, "-function_patch", functionPatch_file, "-signal_patch", signalPatch_file}


	// Parse arguments
	err = cmd.Parse(args)
	if err != nil {
		t.Errorf("Error parsing arguments: %v", err)
	}

	// Run the command
	err = cmd.Run()
	if err != nil {
		t.Errorf("Error running command: %v", err)
	}

	// Check if generated SignalGroup YAML exists
	_, err = os.Stat(network_file)
	if os.IsNotExist(err) {
		t.Errorf("Output Yaml file does not exist: %v", err)
	}

	// Read and parse the generated YAML file
	data, err := os.ReadFile(network_file)
	if err != nil {
		t.Fatalf("Failed to read generated YAML file: %v", err)
	}

	var generatedYAML kind.Network
	if err := yaml.Unmarshal(data, &generatedYAML); err != nil {
		t.Fatalf("Failed to parse generated YAML: %v", err)
	}

	// Extract annotations and verify
	assert.Equal(t, string(generatedYAML.Kind), "Network", "kind should match")
	assert.Equal(t, *generatedYAML.Metadata.Name, "stub", "metadata/name should match")
	assert.Equal(t, (*generatedYAML.Metadata.Annotations)["function_lib"], "examples/stub/lib/function__ut.so", "metadata/annotations/function_lib should match")
	assert.Equal(t, (*generatedYAML.Metadata.Annotations)["message_lib"], "examples/stub/lib/message.so", "metadata/annotations/message_lib should match")
	assert.Equal(t, (*generatedYAML.Metadata.Annotations)["bus_id"], 4, "metadata/annotations/bus_id should match")
	assert.Equal(t, (*generatedYAML.Metadata.Annotations)["interface_id"], 3, "metadata/annotations/interface_id should match")
	assert.Equal(t, (*generatedYAML.Metadata.Annotations)["node_id"], 2, "metadata/annotations/node_id should match")
}

func TestPatchedNetworkFunctions(t *testing.T) {
	var functionPatch_file = "../../../test/testdata/function_patch.yaml"
	var signalPatch_file = "../../../test/testdata/signal_patch.csv"
	var network_file = "../../../test/testdata/network.yaml"

	// Copy the original file to a temporary location
	tempFile := network_file + ".temp"
	input, _ := ioutil.ReadFile(network_file)
	err := ioutil.WriteFile(tempFile, input, 0644)
	defer os.Remove(tempFile)

	cmd := NewPatchNetworkCommand("test")
	args := []string{"-network", tempFile, "-function_patch", functionPatch_file, "-signal_patch", signalPatch_file}

	// Parse arguments
	err = cmd.Parse(args)
	if err != nil {
		t.Errorf("Error parsing arguments: %v", err)
	}

	// Run the command
	err = cmd.Run()
	if err != nil {
		t.Errorf("Error running command: %v", err)
	}

	// Read and parse the generated YAML file
	data, err := os.ReadFile(tempFile)
	if err != nil {
		t.Fatalf("Failed to read generated YAML file: %v", err)
	}

	var generatedYAML kind.Network
	if err := yaml.Unmarshal(data, &generatedYAML); err != nil {
		t.Fatalf("Failed to parse generated YAML: %v", err)
	}

	test_data := []map[string]interface{}{
		{
			"message":      "example_message",
			"frame_id":     "0x1f0u",
			"frame_length": "8u",
			"struct_name":  "stub_example_message_t",
			"struct_size":  4,
		},
		{
			"message":      "function_example",
			"frame_id":     "0x1f2u",
			"frame_length": "8u",
			"struct_name":  "stub_function_example_t",
			"struct_size":  4,
			"functions": map[string][]map[string]interface{}{
				"decode": {
					{"function": "crc_validate", "annotations": map[string]interface{}{"position": 0}},
				},
				"encode": {
					{"function": "counter_inc_uint8", "annotations": map[string]interface{}{"position": 1}},
					{"function": "crc_generate", "annotations": map[string]interface{}{"position": 0}},
				},
			},
		},
	}

	for i, message := range generatedYAML.Spec.Messages {
		assert.Equal(t, message.Message, test_data[i]["message"], "message should match")
		assert.Equal(t, (*message.Annotations)["frame_id"], test_data[i]["frame_id"], "frame_id should match")
		assert.Equal(t, (*message.Annotations)["frame_length"], test_data[i]["frame_length"], "frame_length should match")
		assert.Equal(t, (*message.Annotations)["struct_name"], test_data[i]["struct_name"], "struct_name should match")
		assert.Equal(t, (*message.Annotations)["struct_size"], test_data[i]["struct_size"], "struct_size should match")

		// Check functions if they exist
		if functions, exists := test_data[i]["functions"].(map[string][]map[string]interface{}); exists {
			for functionType, expectedFunctions := range functions {
				var actualFunctions *[]kind.NetworkFunction
				if functionType == "decode" {
					actualFunctions = message.Functions.Decode
				} else if functionType == "encode" {
					actualFunctions = message.Functions.Encode
				}
				// Compare function names and annotations
				for j, expectedFunction := range expectedFunctions {
					assert.Equal(t, (*actualFunctions)[j].Function, expectedFunction["function"], "function at index %d should match", j)
				}
			}
		}
	}
}


func TestPatchedNetworkSignals(t *testing.T) {
	var functionPatch_file = "../../../test/testdata/function_patch.yaml"
	var signalPatch_file = "../../../test/testdata/signal_patch.csv"
	var network_file = "../../../test/testdata/network.yaml"

	// Copy the original file to a temporary location
	tempFile := network_file + ".temp"
	input, _ := ioutil.ReadFile(network_file)
	err := ioutil.WriteFile(tempFile, input, 0644)
	defer os.Remove(tempFile)

	cmd := NewPatchNetworkCommand("test")
	args := []string{"-network", tempFile, "-function_patch", functionPatch_file, "-signal_patch", signalPatch_file, "-remove_unknown"}

	// Parse arguments
	err = cmd.Parse(args)
	if err != nil {
		t.Errorf("Error parsing arguments: %v", err)
	}

	// Run the command
	err = cmd.Run()
	if err != nil {
		t.Errorf("Error running command: %v", err)
	}

	// Read and parse the generated YAML file
	data, err := os.ReadFile(tempFile)
	if err != nil {
		t.Fatalf("Failed to read generated YAML file: %v", err)
	}

	var generatedYAML kind.Network
	if err := yaml.Unmarshal(data, &generatedYAML); err != nil {
		t.Fatalf("Failed to parse generated YAML: %v", err)
	}

	// Test data for signals
	test_data := []map[string]interface{}{
		{
			"message":      "example_message",
			"signals": []map[string]interface{}{
				{
					"signal":                  "Enable",
					"struct_member_name":      "enable",
					"struct_member_offset":    0,
					"struct_member_primitive_type": "uint8_t",
				},
			},
		},
		{
			"message":      "function_example",
			"signals": []map[string]interface{}{
				{
					"signal":                  "new_Crc",
					"struct_member_name":      "crc",
					"struct_member_offset":    0,
					"struct_member_primitive_type": "uint16_t",
				},
				{
					"signal":                  "new_foo",
					"struct_member_name":      "foo",
					"struct_member_offset":    0,
					"struct_member_primitive_type": "uint16_t",
				},
				{
					"signal":                  "new_bar",
					"struct_member_name":      "bar",
					"struct_member_offset":    0,
					"struct_member_primitive_type": "uint16_t",
				},
				{
					"signal":                  "foobar",
					"struct_member_name":      "foobar",
					"struct_member_offset":    0,
					"struct_member_primitive_type": "uint16_t",
				},
			},
		},
	}

	// Iterate through signals
	for i, message := range generatedYAML.Spec.Messages {
		// Check signals for each message
		expectedSignals := test_data[i]["signals"].([]map[string]interface{})
		for i, signal := range *message.Signals {
			expectedSignal := expectedSignals[i]
			assert.Equal(t, signal.Signal, expectedSignal["signal"], "signal at index %d should match", i)
			assert.Equal(t, (*signal.Annotations)["struct_member_name"], expectedSignal["struct_member_name"], "struct_member_name at index %d should match", i)
			assert.Equal(t, (*signal.Annotations)["struct_member_offset"], expectedSignal["struct_member_offset"], "struct_member_offset at index %d should match", i)
			assert.Equal(t, (*signal.Annotations)["struct_member_primitive_type"], expectedSignal["struct_member_primitive_type"], "struct_member_primitive_type at index %d should match", i)
		}
		// Check the signal count when remove_unknown is true
		if i == 0 {
			assert.Equal(t, 0, len(*message.Signals), "Message %d: Expected number of expected signals to be 0, but got %d", i, len(expectedSignals))
		}
	}
}
