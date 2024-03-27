// Copyright 2024 Robert Bosch GmbH
//
// SPDX-License-Identifier: Apache-2.0

#include <dse/testing.h>
#include <dse/logger.h>
#include <dse/network/network.h>
#include <dse/modelc/schema.h>
#include <dse/clib/util/yaml.h>


/*
    YAML Schema concept.
    Model - defines the Network model (i.e location of binary). ONCE ONLY.
    Model Instance - defines _an_ instance of a Network model using LABELS (i.e.
   for channel mapping). MANY Network YAML - found based on Kind and on LABELS
*/


/*
GOAL : parse the YAML file into our Network structures.
Write the code to create this kind of structure:
    Network network = { .name = "stub" };
    NetworkMessage network_message = { .name =
"stub_example_message_t" }; NetworkSignal network_signals = [ {
.name = "enable" }, { .name = "average_radius" }, { .name = "temperature" }, {
.name = NULL },
    ];
*/

#define UNUSED(x) ((void)x)


typedef struct NetworkMock {
    Network*           network;
    ModelInstanceSpec* model_instance;
} NetworkMock;


int test_network_setup(void** state)
{
    NetworkMock* mock = calloc(1, sizeof(NetworkMock));
    assert_non_null(mock);
    mock->network = calloc(1, sizeof(Network));
    mock->model_instance = calloc(1, sizeof(ModelInstanceSpec));
    mock->network->name = "stub";

    /* Load the mock objects. */
    mock->model_instance->name = (char*)"stub_inst";

    /* Load the docs. */
    YamlDocList* doc_list = NULL;
    const char*  yaml_files[] = {
        "data/model.yaml",
        "examples/stub/data/network.yaml",
        "examples/stub/data/simulation.yaml",
        NULL,
    };
    for (const char** _ = yaml_files; *_ != NULL; _++) {
        doc_list = dse_yaml_load_file(*_, doc_list);
    }
    mock->model_instance->yaml_doc_list = doc_list;

    *state = mock;

    return 0;
}


int test_network_teardown(void** state)
{
    NetworkMock* mock = *state;

    if (mock && mock->network) {
        free(mock->network);
    }
    if (mock && mock->model_instance) {
        dse_yaml_destroy_doc_list(mock->model_instance->yaml_doc_list);
        free(mock->model_instance);
    }
    if (mock) free(mock);

    return 0;
}


void test_network_parse_database(void** state)
{
    UNUSED(state);

    /* Get the Mock objects. */
    NetworkMock* mock = *state;

    /* Initialize members of mock->network. */
    assert_null(mock->network->message_lib_handle);
    assert_null(mock->network->doc);
    assert_null(mock->network->messages);
    assert_non_null(mock->network->name);

    /* Call parse directly. */
    network_parse(mock->network, mock->model_instance);

    /* Check the Network YAML was parsed correctly. */
    assert_non_null(mock->network->doc);
    assert_string_equal(mock->network->name, "stub");
    assert_string_equal(
        mock->network->message_lib_path, "examples/stub/lib/message.so");
    assert_string_equal(
        mock->network->function_lib_path, "examples/stub/lib/function__ut.so");
    assert_int_equal(mock->network->node_id, 2);
    assert_int_equal(mock->network->interface_id, 3);
    assert_int_equal(mock->network->bus_id, 4);

    network_unload_parser(mock->network);
}


void test_network_parse_messages(void** state)
{
    UNUSED(state);

    /* Get the Mock objects. */
    NetworkMock* mock = *state;

    /* Initial conditions. */
    assert_null(mock->network->message_lib_handle);
    assert_null(mock->network->doc);
    assert_null(mock->network->messages);
    assert_non_null(mock->network->name);

    /* Call parse directly. */
    network_parse(mock->network, mock->model_instance);

    assert_string_equal(mock->network->name, "stub");

    assert_string_equal(mock->network->messages[0].name, "example_message");
    assert_int_equal(mock->network->messages[0].buffer_len, 8);
    assert_non_null(mock->network->messages[0].buffer);
    assert_int_equal(mock->network->messages[0].frame_id, 496);
    assert_int_equal(mock->network->messages[0].frame_type, 0);
    assert_int_equal(mock->network->messages[0].payload_len, 8);
    assert_non_null(mock->network->messages[0].payload);
    assert_int_equal(mock->network->messages[6].cycle_time_ms, 10);

    assert_string_equal(mock->network->messages[1].name, "example_message2");
    assert_string_equal(mock->network->messages[2].name, "function_example");
    assert_string_equal(mock->network->messages[3].name, "unsigned_types");
    assert_string_equal(mock->network->messages[4].name, "signed_types");
    assert_string_equal(mock->network->messages[5].name, "float_types");
    assert_string_equal(mock->network->messages[6].name, "scheduled_message");

    assert_null(mock->network->messages[7].name);  // Null terminating message.

    network_unload_parser(mock->network);
}


void test_network_parse_signals(void** state)
{
    UNUSED(state);

    /* Get the Mock objects. */
    NetworkMock* mock = *state;

    /* Initial conditions. */
    assert_null(mock->network->message_lib_handle);
    assert_null(mock->network->doc);
    assert_null(mock->network->messages);
    assert_non_null(mock->network->name);

    /* Call parse directly. */
    network_parse(mock->network, mock->model_instance);

    assert_string_equal(
        mock->network->messages->signals[0].signal_name, "enable");
    assert_string_equal(mock->network->messages->signals[0].name, "enable");
    assert_int_equal(mock->network->messages->signals[0].buffer_offset, 0);
    assert_string_equal(
        mock->network->messages->signals[0].member_type, "uint8_t");
    assert_double_equal(
        mock->network->messages->signals[0].init_value, 0.0, 0.0);

    assert_string_equal(
        mock->network->messages->signals[1].signal_name, "average_radius");
    assert_string_equal(
        mock->network->messages->signals[1].name, "average_radius");
    assert_int_equal(mock->network->messages->signals[1].buffer_offset, 1);
    assert_string_equal(
        mock->network->messages->signals[1].member_type, "uint8_t");
    assert_double_equal(
        mock->network->messages->signals[1].init_value, 1.0, 0.0);


    assert_string_equal(
        mock->network->messages->signals[2].signal_name, "temperature");
    assert_string_equal(
        mock->network->messages->signals[2].name, "temperature");
    assert_int_equal(mock->network->messages->signals[2].buffer_offset, 2);
    assert_string_equal(
        mock->network->messages->signals[2].member_type, "int16_t");
    assert_double_equal(
        mock->network->messages->signals[2].init_value, 265.0, 0.0);

    assert_null(
        mock->network->messages->signals[3].name);  // Null terminating signal.

    network_unload_parser(mock->network);
}


static int count_f(NetworkFunction* function)
{
    int count = 0;
    while (function && function->name) {
        count++;
        function++;
    }
    return count;
}

void test_network_parse_functions(void** state)
{
    UNUSED(state);

    /* Get the Mock objects. */
    NetworkMock* mock = *state;

    /* Initial conditions. */
    assert_null(mock->network->message_lib_handle);
    assert_null(mock->network->doc);
    assert_null(mock->network->messages);
    assert_non_null(mock->network->name);

    /* Call parse directly. */
    network_parse(mock->network, mock->model_instance);

    /* Select the message (as basis for tests). */
    NetworkMessage* message = &mock->network->messages[2];
    assert_string_equal(message->name, "function_example");
    NetworkFunction* functions;

    /* Encode Functions. */
    functions = message->encode_functions;
    assert_non_null(functions);
    assert_int_equal(count_f(functions), 2);
    assert_string_equal(functions[0].name, "counter_inc_uint8");
    assert_non_null(functions[0].annotations);
    assert_null(functions[0].function);
    assert_null(functions[0].data);
    assert_string_equal(
        dse_yaml_get_scalar(functions[0].annotations, "position"), "1");
    assert_string_equal(functions[1].name, "crc_generate");
    assert_non_null(functions[1].annotations);
    assert_null(functions[1].function);
    assert_null(functions[1].data);
    assert_string_equal(
        dse_yaml_get_scalar(functions[1].annotations, "position"), "0");

    /* Decode Functions. */
    functions = message->decode_functions;
    assert_non_null(functions);
    assert_int_equal(count_f(functions), 1);
    assert_string_equal(functions[0].name, "crc_validate");
    assert_non_null(functions[0].annotations);
    assert_null(functions[0].function);
    assert_null(functions[0].data);

    network_unload_parser(mock->network);
}


int run_parser_tests(void)
{
    void* s = test_network_setup;
    void* t = test_network_teardown;

    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_network_parse_database, s, t),
        cmocka_unit_test_setup_teardown(test_network_parse_messages, s, t),
        cmocka_unit_test_setup_teardown(test_network_parse_signals, s, t),
    };

    return cmocka_run_group_tests_name("PARSER", tests, NULL, NULL);
}
