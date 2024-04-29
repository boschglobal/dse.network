// Copyright 2024 Robert Bosch GmbH
//
// SPDX-License-Identifier: Apache-2.0

#include <stddef.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <dse/testing.h>
#include <dse/network/network.h>
#include <dse/modelc/schema.h>
#include <dse/clib/util/yaml.h>
#include <dse/logger.h>


#define UNUSED(x) ((void)x)


typedef struct NetworkMock {
    Network*           network;
    ModelInstanceSpec* model_instance;
} NetworkMock;


static void set_update_signals(MarshalItem* mi, bool value)
{
    while (mi && mi->signal) {
        if (mi->message) mi->message->update_signals = value;
        mi++;
    }
}


void test_engine_network_marshal_list(void** state)
{
    UNUSED(state);

    /* Get the Mock objects. */
    NetworkMock* mock = *state;

    assert_null(mock->network->marshal_list);

    /* Call load. */
    network_load(mock->network, mock->model_instance);

    assert_string_equal(mock->network->marshal_list[0].signal->name, "enable");
    assert_string_equal(
        mock->network->marshal_list[0].signal->signal_name, "enable");
    assert_string_equal(
        mock->network->marshal_list[1].signal->signal_name, "average_radius");
    assert_string_equal(
        mock->network->marshal_list[2].signal->signal_name, "temperature");
    assert_string_equal(
        mock->network->marshal_list[3].signal->signal_name, "radius");

    /* Check signal vector index. */
    assert_int_equal(mock->network->marshal_list[0].signal_vector_index, 0);
    assert_int_equal(mock->network->marshal_list[1].signal_vector_index, 1);
    assert_int_equal(mock->network->marshal_list[2].signal_vector_index, 2);
    assert_int_equal(mock->network->marshal_list[3].signal_vector_index, 3);

    network_unload(mock->network);
}


void test_engine_get_signal_name_vector(void** state)
{
    UNUSED(state);

    /* Get the Mock objects. */
    NetworkMock* mock = *state;

    assert_null(mock->network->marshal_list);

    /* Call load. */
    network_load(mock->network, mock->model_instance);

    assert_int_equal(mock->network->signal_count, 27);
    assert_string_equal(mock->network->signal_name[0], "enable");
    assert_string_equal(mock->network->signal_name[1], "average_radius");
    assert_string_equal(mock->network->signal_name[2], "temperature");
    assert_string_equal(mock->network->signal_name[3], "radius");
    assert_null(mock->network->signal_name[27]);
    assert_non_null(mock->network->signal_vector);

    /* Check signal vector index. */
    assert_int_equal(mock->network->marshal_list[0].signal_vector_index, 0);
    assert_int_equal(mock->network->marshal_list[1].signal_vector_index, 1);
    assert_int_equal(mock->network->marshal_list[2].signal_vector_index, 2);
    assert_int_equal(mock->network->marshal_list[3].signal_vector_index, 3);

    /* Cleanup */
    network_unload(mock->network);
}


void test_engine_marshal_network_signal_to_message(void** state)
{
    UNUSED(state);

    /* Get the Mock objects. */
    NetworkMock* mock = *state;
    Network*     network = mock->network;

    /* Call load. */
    network_load(mock->network, mock->model_instance);
    assert_non_null(network);
    assert_non_null(network->messages);
    assert_non_null(network->signal_vector);
    assert_non_null(network->signal_count);
    assert_non_null(network->marshal_list);

    /* Set signal values and marshal to the Model. */
    network->signal_vector[0] = 1;
    network->signal_vector[1] = 2;
    network->signal_vector[2] = 260;
    network->signal_vector[3] = 2;

    network_marshal_signals_to_messages(network, network->marshal_list);

    /* Check the message container. */
    void* message1 = network->messages[0].buffer;
    assert_int_equal(((int8_t*)message1)[(0) / sizeof(int8_t)], 1);
    assert_int_equal(((int8_t*)message1)[(1) / sizeof(int8_t)], 20);
    assert_int_equal(((int16_t*)message1)[(2) / sizeof(int16_t)], 1000);

    /* Check the message container. */
    void* message2 = network->messages[1].buffer;
    assert_int_equal(((int8_t*)message2)[(0) / sizeof(int8_t)], 20);

    /* Assuming out-of-range values. */
    network->signal_vector[0] = 5;
    network->signal_vector[1] = 90;
    network->signal_vector[2] = 2500;
    network->signal_vector[3] = 90;

    network_marshal_signals_to_messages(network, network->marshal_list);

    /* Ensure the message container is NOT modified. */
    assert_int_equal(((int8_t*)message1)[(0) / sizeof(int8_t)], 1);
    assert_int_equal(((int8_t*)message1)[(1) / sizeof(int8_t)], 20);
    assert_int_equal(((int16_t*)message1)[(2) / sizeof(int16_t)], 1000);
    assert_int_equal(((int8_t*)message2)[(0) / sizeof(int8_t)], 20);

    network_unload(mock->network);
}


void test_engine_marshal_network_message_to_signal(void** state)
{
    UNUSED(state);

    /* Get the Mock objects. */
    NetworkMock* mock = *state;
    Network*     network = mock->network;


    /* Call load. */
    network_load(mock->network, mock->model_instance);
    assert_non_null(network);
    assert_non_null(network->messages);
    // assert_non_null(network->messages[0].buffer);
    assert_non_null(network->signal_vector);
    assert_non_null(network->signal_count);
    assert_non_null(network->marshal_list);

    /* Set signal values and marshal from the Model. */
    void* message1 = network->messages[0].buffer;
    ((int8_t*)message1)[(0) / sizeof(int8_t)] = 1;
    ((int8_t*)message1)[(1) / sizeof(int8_t)] = 20;
    ((int16_t*)message1)[(2) / sizeof(int16_t)] = 1000;

    /* Set signal values and marshal from the Model. */
    void* message2 = network->messages[1].buffer;
    ((int8_t*)message2)[(0) / sizeof(int8_t)] = 20;

    set_update_signals(network->marshal_list, true);
    network_marshal_messages_to_signals(network, network->marshal_list, false);

    /* Check the signal vector. */
    assert_int_equal(network->signal_vector[0], 1);
    assert_int_equal(network->signal_vector[1], 2);
    assert_int_equal(network->signal_vector[2], 260);
    assert_int_equal(network->signal_vector[3], 2);

    /* Assuming out-of-range values. */
    ((int8_t*)message1)[(0) / sizeof(int8_t)] = 10;
    ((int8_t*)message1)[(1) / sizeof(int8_t)] = 70;
    ((int16_t*)message1)[(2) / sizeof(int16_t)] = 3000;
    ((int8_t*)message1)[(3) / sizeof(int8_t)] = 70;

    set_update_signals(network->marshal_list, true);
    network_marshal_messages_to_signals(network, network->marshal_list, false);

    /* Ensure the signal vector is NOT modified. */
    assert_int_equal(network->signal_vector[0], 1);
    assert_int_equal(network->signal_vector[1], 2);
    assert_int_equal(network->signal_vector[2], 260);
    assert_int_equal(network->signal_vector[1], 2);

    network_unload(mock->network);
}

void test_engine_marshal_network_signal_to_buffer(void** state)
{
    UNUSED(state);

    /* Get the Mock objects. */
    NetworkMock* mock = *state;
    Network*     network = mock->network;

    /* Call load. */
    network_load(mock->network, mock->model_instance);
    assert_non_null(network);
    assert_non_null(network->messages);
    assert_non_null(network->signal_vector);
    assert_non_null(network->signal_count);
    assert_non_null(network->marshal_list);

    /* Set signal values and marshal to the Model. */
    network->signal_vector[0] = 1;
    network->signal_vector[1] = 2;
    network->signal_vector[2] = 260;
    network->signal_vector[3] = 2;

    /* Call Pack and Encode. */
    network_marshal_signals_to_messages(network, network->marshal_list);
    network_pack_messages(network);

    uint8_t expected_message1[] = { 0xA8, 0x7D, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00 };
    assert_memory_equal(network->messages[0].payload, expected_message1,
        sizeof(expected_message1));
    uint8_t expected_message2[] = { 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00 };
    assert_memory_equal(network->messages[1].payload, expected_message2,
        sizeof(expected_message2));

    network_unload(mock->network);
}


void test_engine_marshal_network_buffer_to_signal(void** state)
{
    UNUSED(state);

    /* Get the Mock objects. */
    NetworkMock* mock = *state;
    Network*     network = mock->network;

    /* Call load. */
    network_load(mock->network, mock->model_instance);
    assert_non_null(network);
    assert_non_null(network->messages);
    assert_non_null(network->signal_vector);
    assert_non_null(network->signal_count);
    assert_non_null(network->marshal_list);

    void* message1 = network->messages[0].buffer;
    ((int8_t*)message1)[(0) / sizeof(int8_t)] = 1;
    ((int8_t*)message1)[(1) / sizeof(int8_t)] = 20;
    ((int16_t*)message1)[(2) / sizeof(int16_t)] = 1000;
    void* message2 = network->messages[1].buffer;
    ((int8_t*)message2)[(0) / sizeof(int8_t)] = 20;

    /* Call Unpack and Decode. */
    set_update_signals(network->marshal_list, true);
    network_marshal_messages_to_signals(network, network->marshal_list, false);

    assert_int_equal(network->messages->payload_len, 8);

    /* Check the signal vector. */
    assert_int_equal(network->signal_vector[0], 1);
    assert_int_equal(network->signal_vector[1], 2);
    assert_int_equal(network->signal_vector[2], 260);
    assert_int_equal(network->signal_vector[3], 2);


    network_unload(mock->network);
}


static int32_t _find_signal_idx(const char** h, const char* n)
{
    for (int32_t idx = 0; h[idx]; idx++) {
        if (strcmp(h[idx], n) == 0) return idx;
    }
    return -1;
}

static int32_t _find_message_idx(Network* net, const char* n)
{
    int32_t idx = 0;
    for (NetworkMessage* msg = net->messages; msg && msg->name; msg++) {
        if (strcmp(msg->name, n) == 0) return idx;
        idx++;
    }
    return -1;
}


void test_engine_marshal_container_message(void** state)
{
    UNUSED(state);

    /* Get the Mock objects. */
    NetworkMock* mock = *state;
    Network*     n = mock->network;

    /* Call load. */
    network_load(mock->network, mock->model_instance);
    assert_non_null(n);
    assert_non_null(n->messages);
    assert_non_null(n->signal_vector);
    assert_non_null(n->signal_count);
    assert_non_null(n->marshal_list);

    int32_t foo_idx = _find_signal_idx(n->signal_name, "foo_double");
    assert_in_range(foo_idx, 0, n->signal_count);
    int32_t m601_idx = _find_message_idx(n, "mux_message_601");
    assert_in_range(m601_idx, 0, 10);

    // TX Path
    n->signal_vector[foo_idx] = 10;

    /* Call Pack and Encode. */
    network_marshal_signals_to_messages(n, n->marshal_list);
    network_pack_messages(n);

    uint8_t m601_msg[] = {
        0x59, 0x02, 0x00,                                // header_id
        0x2a,                                            // header_dlc
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40,  // foo_double
    };
    uint8_t* _payload = n->messages[m601_idx].payload;
    // for (unsigned int i = 0; i < sizeof(m601_msg); i++) {
    //     log_error("  0x%02x  0x%02x", _payload[i], m601_msg[i]);
    // }
    assert_int_equal(n->messages[m601_idx].payload_len, 12);
    assert_memory_equal(_payload, m601_msg, sizeof(m601_msg));

    // RX Path
    _payload[10] = 0x34;  // eff. 10 -> 20

    set_update_signals(n->marshal_list, true);
    network_unpack_messages(n);
    network_marshal_messages_to_signals(n, n->marshal_list, false);

    assert_double_equal(n->signal_vector[foo_idx], 20, 0);


    network_unload(mock->network);
}


void test_engine_marshal_container_mux_signal(void** state)
{
    UNUSED(state);

    /* Get the Mock objects. */
    NetworkMock* mock = *state;
    Network*     n = mock->network;

    /* Call load. */
    network_load(mock->network, mock->model_instance);
    assert_non_null(n);
    assert_non_null(n->messages);
    assert_non_null(n->signal_vector);
    assert_non_null(n->signal_count);
    assert_non_null(n->marshal_list);

    int32_t m600_idx = _find_message_idx(n, "mux_message");
    assert_in_range(m600_idx, 0, 10);
    NetworkMessage* m600 = &n->messages[m600_idx];
    assert_non_null(m600);
    assert_non_null(m600->mux_signal);
    NetworkSignal* s600 = m600->mux_signal;
    assert_string_equal(s600->name, "header_id");
    assert_non_null(s600->mux_mi);
    MarshalItem* mi600 = s600->mux_mi;
    assert_ptr_equal(mi600->signal, s600);


    network_unload(mock->network);
}


extern int test_network_setup(void** state);
extern int test_network_teardown(void** state);


int run_engine_tests(void)
{
    void* s = test_network_setup;
    void* t = test_network_teardown;

    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_engine_network_marshal_list, s, t),
        cmocka_unit_test_setup_teardown(
            test_engine_get_signal_name_vector, s, t),
        cmocka_unit_test_setup_teardown(
            test_engine_marshal_network_signal_to_message, s, t),
        cmocka_unit_test_setup_teardown(
            test_engine_marshal_network_message_to_signal, s, t),
        cmocka_unit_test_setup_teardown(
            test_engine_marshal_network_signal_to_buffer, s, t),
        cmocka_unit_test_setup_teardown(
            test_engine_marshal_network_buffer_to_signal, s, t),
        cmocka_unit_test_setup_teardown(
            test_engine_marshal_container_message, s, t),
        cmocka_unit_test_setup_teardown(
            test_engine_marshal_container_mux_signal, s, t),
    };

    return cmocka_run_group_tests_name("ENGINE", tests, NULL, NULL);
}
