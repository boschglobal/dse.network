// Copyright 2024 Robert Bosch GmbH
//
// SPDX-License-Identifier: Apache-2.0


#include <dse/testing.h>
#include <dse/network/network.h>
#include <dse/logger.h>


#define UNUSED(x) ((void)x)


typedef struct NetworkMock {
    Network*           network;
    ModelInstanceSpec* model_instance;
} NetworkMock;

typedef struct InstanceDataMock {
    uint8_t position;  // Annotation: position
} InstanceDataMock;


void test_function_encode(void** state)
{
    NetworkMock* mock = *state;
    Network*     network = mock->network;

    network_load(mock->network, mock->model_instance);
    assert_non_null(network);
    assert_non_null(network->messages);
    assert_non_null(network->signal_vector);
    assert_non_null(network->signal_count);
    assert_non_null(network->marshal_list);

    /* Set the initial condition. */
    network->signal_vector[4] = 0;
    network->signal_vector[5] = 1;
    network->signal_vector[6] = 5;
    network->signal_vector[7] = 50;
    network_marshal_signals_to_messages(network, network->marshal_list);
    network_pack_messages(network);
    NetworkMessage* nm_p = &network->messages[2];
    void*           payload = nm_p->payload;
    assert_int_equal(((uint8_t*)payload)[(0) / sizeof(uint8_t)], 0);
    assert_int_equal(((uint8_t*)payload)[(1) / sizeof(uint8_t)], 1);
    assert_int_equal(((uint8_t*)payload)[(2) / sizeof(uint8_t)], 5);
    assert_int_equal(((uint8_t*)payload)[(3) / sizeof(uint8_t)], 50);
    assert_null(nm_p->encode_functions[0].data);
    assert_null(nm_p->encode_functions[1].data);

    /* Call the message encode functions. */
    network_function_apply_encode(network);
    network_marshal_messages_to_signals(network, network->marshal_list, false);

    /* Check the packet and signals are modified. */
    assert_int_equal(((uint8_t*)payload)[(0) / sizeof(uint8_t)], 57);
    assert_int_equal(((uint8_t*)payload)[(1) / sizeof(uint8_t)], 2);
    assert_int_equal(((uint8_t*)payload)[(2) / sizeof(uint8_t)], 5);
    assert_int_equal(((uint8_t*)payload)[(3) / sizeof(uint8_t)], 50);
    assert_double_equal(network->signal_vector[4], 57.0, 0.0);
    assert_double_equal(network->signal_vector[5], 2.0, 0.0);
    assert_double_equal(network->signal_vector[6], 5.0, 0.0);
    assert_double_equal(network->signal_vector[7], 50.0, 0.0);

    /* Check the allocated data. */
    assert_non_null(nm_p->encode_functions[0].data);
    assert_non_null(nm_p->encode_functions[1].data);
    InstanceDataMock* idm0 = nm_p->encode_functions[0].data;
    assert_int_equal(idm0->position, 1);
    InstanceDataMock* idm1 = nm_p->encode_functions[1].data;
    assert_int_equal(idm1->position, 0);

    /* Call the message encode functions second time, check no alloc. */
    network_function_apply_encode(network);
    assert_ptr_equal(nm_p->encode_functions[0].data, idm0);
    assert_ptr_equal(nm_p->encode_functions[1].data, idm1);

    network_unload(mock->network);
}


void test_function_decode(void** state)
{
    NetworkMock* mock = *state;
    Network*     network = mock->network;

    network_load(mock->network, mock->model_instance);
    assert_non_null(network);
    assert_non_null(network->messages);
    assert_non_null(network->signal_vector);
    assert_non_null(network->signal_count);
    assert_non_null(network->marshal_list);

    /* Set the initial condition. */
    network->signal_vector[4] = 56;
    network->signal_vector[5] = 1;
    network->signal_vector[6] = 5;
    network->signal_vector[7] = 50;
    network_marshal_signals_to_messages(network, network->marshal_list);
    network_pack_messages(network);
    NetworkMessage* nm_p = &network->messages[2];
    void*           payload = nm_p->payload;
    size_t          length = nm_p->payload_len;
    assert_int_equal(((uint8_t*)payload)[(0) / sizeof(uint8_t)], 56);
    assert_int_equal(((uint8_t*)payload)[(1) / sizeof(uint8_t)], 1);
    assert_int_equal(((uint8_t*)payload)[(2) / sizeof(uint8_t)], 5);
    assert_int_equal(((uint8_t*)payload)[(3) / sizeof(uint8_t)], 50);
    assert_null(nm_p->decode_functions[0].data);

    /* Clear the signal vector. */
    network->signal_vector[4] = 0;
    network->signal_vector[5] = 0;
    network->signal_vector[6] = 0;
    network->signal_vector[7] = 0;

    /* Call the message decode functions (fake call to network_decode_from_bus). */
    nm_p->unpack_func(nm_p->buffer, payload, length);
    nm_p->update_signals = true;
    network_function_apply_decode(network);
    network_marshal_messages_to_signals(network, network->marshal_list, false);

    /* Check the signals are set. */
    assert_true(nm_p->update_signals);
    assert_double_equal(network->signal_vector[4], 56.0, 0.0);
    assert_double_equal(network->signal_vector[5], 1.0, 0.0);
    assert_double_equal(network->signal_vector[6], 5.0, 0.0);
    assert_double_equal(network->signal_vector[7], 50.0, 0.0);

    /* Check the allocated data. */
    assert_non_null(nm_p->decode_functions[0].data);
    InstanceDataMock* idm0 = nm_p->decode_functions[0].data;
    assert_int_equal(idm0->position, 0);

    /* Call the message decode functions second time, check no alloc. */
    network_function_apply_decode(network);
    assert_ptr_equal(nm_p->decode_functions[0].data, idm0);

    network_unload(mock->network);
}


void test_function_decode_EBADMSG(void** state)
{
    NetworkMock* mock = *state;
    Network*     network = mock->network;

    network_load(mock->network, mock->model_instance);
    assert_non_null(network);
    assert_non_null(network->messages);
    assert_non_null(network->signal_vector);
    assert_non_null(network->signal_count);
    assert_non_null(network->marshal_list);

    /* Set the initial condition. */
    network->signal_vector[4] = 56;
    network->signal_vector[5] = 1;
    network->signal_vector[6] = 5;
    network->signal_vector[7] = 50;
    network_marshal_signals_to_messages(network, network->marshal_list);
    network_pack_messages(network);
    NetworkMessage* nm_p = &network->messages[2];
    void*           payload = nm_p->payload;
    size_t          length = nm_p->payload_len;
    assert_int_equal(((uint8_t*)payload)[(0) / sizeof(uint8_t)], 56);
    assert_int_equal(((uint8_t*)payload)[(1) / sizeof(uint8_t)], 1);
    assert_int_equal(((uint8_t*)payload)[(2) / sizeof(uint8_t)], 5);
    assert_int_equal(((uint8_t*)payload)[(3) / sizeof(uint8_t)], 50);
    assert_null(nm_p->decode_functions[0].data);

    /* Clear the signal vector. */
    network->signal_vector[4] = 0;
    network->signal_vector[5] = 0;
    network->signal_vector[6] = 0;
    network->signal_vector[7] = 0;

    /* Inject a CRC fault, which should result in EBADMSG from decode function. */
    ((uint8_t*)payload)[(0) / sizeof(uint8_t)] = 58;

    /* Call the message decode functions (fake call to network_decode_from_bus). */
    nm_p->unpack_func(nm_p->buffer, payload, length);
    nm_p->update_signals = true;
    network_function_apply_decode(network);
    network_marshal_messages_to_signals(network, network->marshal_list, false);

    /* Check the signals are not set. */
    assert_false(nm_p->update_signals);
    assert_double_equal(network->signal_vector[4], 0.0, 0.0);
    assert_double_equal(network->signal_vector[5], 0.0, 0.0);
    assert_double_equal(network->signal_vector[6], 0.0, 0.0);
    assert_double_equal(network->signal_vector[7], 0.0, 0.0);

    /* Check the allocated data. */
    assert_non_null(nm_p->decode_functions[0].data);
    InstanceDataMock* idm0 = nm_p->decode_functions[0].data;
    assert_int_equal(idm0->position, 0);

    network_unload(mock->network);
}


extern int test_network_setup(void** state);
extern int test_network_teardown(void** state);


int run_function_tests(void)
{
    void* s = test_network_setup;
    void* t = test_network_teardown;

    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_function_encode, s, t),
        cmocka_unit_test_setup_teardown(test_function_decode, s, t),
        cmocka_unit_test_setup_teardown(test_function_decode_EBADMSG, s, t),
    };

    return cmocka_run_group_tests_name("FUNCTION", tests, NULL, NULL);
}
