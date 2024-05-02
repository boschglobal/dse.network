// Copyright 2024 Robert Bosch GmbH
//
// SPDX-License-Identifier: Apache-2.0

#include <dse/testing.h>
#include <dse/logger.h>
#include <dse/network/network.h>
#include <dse/modelc/schema.h>


#define UNUSED(x)     ((void)x)
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))


void test_network_load_message_lib(void** state)
{
    UNUSED(state);
    Network network;
    void*   handle;
    assert_null(handle);
    handle = network_load_message_lib(&network, "examples/stub/lib/message.so");
    assert_non_null(handle);
}


void test_network_load_message_funcs(void** state)
{
    UNUSED(state);
    /* Initialize messages with signals array. */
    NetworkMessage network_message[] = {
        { .name = "example_message" },
        { .name = "example_message2" },
        { .name = "function_example" },
        { .name = "unsigned_types" },
        { .name = "signed_types" },
        { .name = "float_types" },
        // Container messages.
        { .name = "mux_message" },
        { .name = "mux_message_601", .container = "mux_message" },
        { .name = "mux_message_602", .container = "mux_message" },
        // End.
        { .name = NULL },
    };
    Network        network = { .name = "stub", .messages = network_message };

    /* Load the DLL. */
    void* handle =
        network_load_message_lib(&network, "examples/stub/lib/message.so");
    assert_non_null(handle);
    int i;
    /* Load the message functions. */
    for (i = 0; network_message[i].name != NULL; i++) {
        assert_null(network_message[i].pack_func);
        assert_null(network_message[i].unpack_func);
    }
    int rc = network_load_message_funcs(&network);
    assert_int_equal(rc, 0);
    for (i = 0; network_message[i].name != NULL; i++) {
        assert_non_null(network_message[i].pack_func);
        assert_non_null(network_message[i].unpack_func);
    }
}


void test_network_load_signal_funcs(void** state)
{
    UNUSED(state);
    Network       network = { .name = "stub" };
    NetworkSignal network_message1_signals[] = {
        { .name = "enable", .member_type = "uint8_t" },
        { .name = "average_radius", .member_type = "uint8_t" },
        { .name = "temperature", .member_type = "int16_t" },
        { .name = NULL },
    };
    NetworkSignal network_message2_signals[] = {
        { .name = "radius", .member_type = "uint8_t" },
        { .name = NULL },
    };
    NetworkSignal network_function_signals[] = {
        { .name = "crc", .member_type = "uint8_t" },
        { .name = "alive", .member_type = "uint8_t" },
        { .name = "foo", .member_type = "uint8_t" },
        { .name = "bar", .member_type = "uint8_t" },
        { .name = NULL },
    };
    NetworkSignal network_unsigned_signals[] = {
        { .name = "u_int8_signal", .member_type = "uint8_t" },
        { .name = "u_int16_signal", .member_type = "uint16_t" },
        { .name = "u_int32_signal", .member_type = "uint32_t" },
        { .name = "u_int64_signal", .member_type = "uint64_t" },
        { .name = NULL },
    };
    NetworkSignal network_signed_signals[] = {
        { .name = "int8_signal", .member_type = "int8_t" },
        { .name = "int16_signal", .member_type = "int16_t" },
        { .name = "int32_signal", .member_type = "int32_t" },
        { .name = "int64_signal", .member_type = "int64_t" },
        { .name = NULL },
    };
    NetworkSignal network_float_signals[] = {
        { .name = "double_signal", .member_type = "int64_t" },
        { .name = "float_signal", .member_type = "int32_t" },
        { .name = NULL },
    };


    /* Initialize messages with signals array. */
    NetworkMessage network_message[] = {
        { .name = "example_message", .signals = network_message1_signals },
        { .name = "example_message2", .signals = network_message2_signals },
        { .name = "function_example", .signals = network_function_signals },
        { .name = "unsigned_types", .signals = network_unsigned_signals },
        { .name = "signed_types", .signals = network_signed_signals },
        { .name = "float_types", .signals = network_float_signals },
        { .name = NULL },
    };
    network.messages = &network_message[0];


    /* Load the DLL. */
    void* handle =
        network_load_message_lib(&network, "examples/stub/lib/message.so");
    assert_non_null(handle);
    int i, j;
    for (i = 0; network_message1_signals[i].member_type != NULL; i++) {
        /* Assertions for null function pointers before loading. */
        assert_null(network_message1_signals[i].encode_func_int8);
        assert_null(network_message1_signals[i].decode_func_int8);
        assert_null(network_message1_signals[i].range_func_int8);
        assert_null(network_message1_signals[i].encode_func_int16);
        assert_null(network_message1_signals[i].decode_func_int16);
        assert_null(network_message1_signals[i].range_func_int16);
    }
    for (j = 0; network_message2_signals[j].member_type != NULL; j++) {
        /* Assertions for null function pointers before loading. */
        assert_null(network_message2_signals[j].encode_func_int8);
        assert_null(network_message2_signals[j].decode_func_int8);
        assert_null(network_message2_signals[j].range_func_int8);
        assert_null(network_message2_signals[j].encode_func_int16);
        assert_null(network_message2_signals[j].decode_func_int16);
        assert_null(network_message2_signals[j].range_func_int16);
    }

    assert_int_equal(i, ARRAY_SIZE(network_message1_signals) - 1);
    assert_int_equal(j, ARRAY_SIZE(network_message2_signals) - 1);

    int rc = network_load_signal_funcs(&network);
    assert_int_equal(rc, 0);

    for (i = 0; network_message1_signals[i].member_type != NULL; i++) {
        /* Assertions for non-null function pointers after loading. */
        if (strcmp(network_message1_signals[i].member_type, "uint8_t") == 0) {
            assert_non_null(network_message1_signals[i].encode_func_int8);
            assert_non_null(network_message1_signals[i].decode_func_int8);
            assert_non_null(network_message1_signals[i].range_func_int8);
        } else if (strcmp(network_message1_signals[i].member_type, "int16_t") ==
                   0) {
            assert_non_null(network_message1_signals[i].encode_func_int16);
            assert_non_null(network_message1_signals[i].decode_func_int16);
            assert_non_null(network_message1_signals[i].range_func_int16);
        }
    }
    for (i = 0; network_message2_signals[i].member_type != NULL; i++) {
        /* Assertions for non-null function pointers after loading. */
        if (strcmp(network_message2_signals[i].member_type, "uint8_t") == 0) {
            assert_non_null(network_message2_signals[i].encode_func_int8);
            assert_non_null(network_message2_signals[i].decode_func_int8);
            assert_non_null(network_message2_signals[i].range_func_int8);
        } else if (strcmp(network_message2_signals[i].member_type, "int16_t") ==
                   0) {
            assert_non_null(network_message2_signals[i].encode_func_int16);
            assert_non_null(network_message2_signals[i].decode_func_int16);
            assert_non_null(network_message2_signals[i].range_func_int16);
        }
    }
}


void test_network_load_function_lib(void** state)
{
    UNUSED(state);
    Network network;
    void*   handle;
    assert_null(handle);
    handle =
        network_load_function_lib(&network, "examples/stub/lib/function.so");
    assert_non_null(handle);
}


void test_network_load_functions(void** state)
{
    UNUSED(state);
    Network         network = { .name = "my_database_name" };
    NetworkFunction encode_functions[] = {
        { .name = (char*)"counter_inc_uint8" },
        { .name = (char*)"crc_generate" },
        { .name = NULL },
    };
    NetworkFunction decode_functions[] = {
        { .name = (char*)"crc_validate" },
        { .name = NULL },
    };

    /* Initialize messages with signals array. */
    NetworkMessage network_message[] = {
        { .name = "example_message" },
        { .name = "example_message2" },
        {
            .name = "function_example",
            .encode_functions = encode_functions,
            .decode_functions = decode_functions,
        },
        { .name = NULL },
    };
    network.messages = &network_message[0];

    /* Load the DLL. */
    void* handle =
        network_load_function_lib(&network, "examples/stub/lib/function.so");
    assert_non_null(handle);
    assert_non_null(network.function_lib_handle);
    assert_ptr_equal(handle, network.function_lib_handle);

    /* Load the message functions. */
    int rc = network_load_function_funcs(&network);
    assert_int_equal(rc, 0);
    assert_null(network_message[0].encode_functions);
    assert_null(network_message[0].decode_functions);
    assert_null(network_message[1].encode_functions);
    assert_null(network_message[1].decode_functions);
    assert_non_null(network_message[2].encode_functions);
    assert_non_null(network_message[2].decode_functions);

    assert_non_null(network_message[2].encode_functions[0].function);
    assert_non_null(network_message[2].encode_functions[1].function);
    assert_null(network_message[2].encode_functions[2].function);

    assert_non_null(network_message[2].decode_functions[0].function);
    assert_null(network_message[2].decode_functions[1].function);
}


int run_loader_tests(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_network_load_message_lib),
        cmocka_unit_test(test_network_load_message_funcs),
        cmocka_unit_test(test_network_load_signal_funcs),
        cmocka_unit_test(test_network_load_functions),
    };

    return cmocka_run_group_tests_name("LOADER", tests, NULL, NULL);
}
