// Copyright 2024 Robert Bosch GmbH
//
// SPDX-License-Identifier: Apache-2.0

#include <dse/testing.h>
#include <dse/logger.h>
#include <dse/modelc/model.h>
#include <dse/network/network.h>
#include <dse/ncodec/codec.h>
#include <dse/mocks/simmock.h>


#define GENERAL_BUFFER_LEN 255
#define UNUSED(x)          ((void)x)
#define ARRAY_SIZE(x)      (sizeof(x) / sizeof(x[0]))


static int test_setup(void** state)
{
    const char* inst_names[] = {
        "stub_inst",
    };
    char* argv[] = {
        (char*)"test_mstep",
        (char*)"--name=stub_inst",
        (char*)"--logger=5",  // QUIET
        (char*)"examples/stub/data/simulation.yaml",
        (char*)"../../../../tests/cmocka/mstep/model_mstep.yaml",
        (char*)"../../../../tests/cmocka/mstep/network_mstep.yaml",
    };
    SimMock* mock = simmock_alloc(inst_names, ARRAY_SIZE(inst_names));
    simmock_configure(mock, argv, ARRAY_SIZE(argv), ARRAY_SIZE(inst_names));
    simmock_load(mock);
    simmock_load_model_check(mock->model, true, true, true);
    simmock_setup(mock, "signal", "network");

    /* Return the mock. */
    *state = mock;
    return 0;
}


static int test_teardown(void** state)
{
    SimMock* mock = *state;

    simmock_exit(mock, true);

    simmock_free(mock);

    return 0;
}


void test_mstep(void** state)
{
    UNUSED(state);

    SimMock*   mock = *state;
    /* Check the state of the Network stub. */
    ModelMock* model = &mock->model[0];
    assert_non_null(model);

    assert_non_null(model->sv_signal);
    assert_string_equal(model->sv_signal->name, "signal");
    assert_int_equal(model->sv_signal->count, 7);
    assert_non_null(model->sv_signal->scalar);
    assert_non_null(model->sv_network);
    assert_string_equal(model->sv_network->name, "network");
    assert_int_equal(model->sv_network->count, 1);
    assert_non_null(model->sv_network->binary);
    assert_non_null(model->sv_network->length);
    assert_non_null(model->sv_network->buffer_size);

    /* Check the initial values. */
    assert_double_equal(model->sv_signal->scalar[0], 1.0, 0.0);
    assert_double_equal(model->sv_signal->scalar[1], 0.0, 0.0);
    assert_double_equal(model->sv_signal->scalar[2], 265.0, 0.0);
    assert_null(model->sv_network->binary[0]);
    assert_int_equal(model->sv_network->length[0], 0);
    assert_int_equal(model->sv_network->buffer_size[0], 0);

    /* Step the model - ensure no can_tx based on setting initial values. */
    int rc = modelc_step(model->mi, mock->step_size);
    assert_int_equal(rc, 0);
    assert_double_equal(model->sv_signal->scalar[0], 1.0, 0.0);
    assert_double_equal(model->sv_signal->scalar[1], 0.0, 0.0);
    assert_double_equal(model->sv_signal->scalar[2], 265.0, 0.0);
    assert_null(model->sv_network->binary[0]);
    assert_int_equal(model->sv_network->length[0], 0);
    assert_int_equal(model->sv_network->buffer_size[0], 0);
    signal_reset(model->sv_network, 0);

    /* Step the model - set signals and check for can_tx. */
    model->sv_signal->scalar[0] = 2;
    model->sv_signal->scalar[1] = 1;
    model->sv_signal->scalar[2] = 260;
    rc = modelc_step(model->mi, mock->step_size);
    assert_int_equal(rc, 0);
    assert_double_equal(model->sv_signal->scalar[0], 2.0, 0.0);
    assert_double_equal(model->sv_signal->scalar[1], 1.0, 0.0);
    assert_double_equal(model->sv_signal->scalar[2], 260.0, 0.0);
    assert_non_null(model->sv_network->binary[0]);
    assert_int_equal(model->sv_network->length[0], 0x62);
    assert_int_equal(model->sv_network->buffer_size[0], 0x62);
    /* Copy the CAN packet for later use, change frame_id. */
    /*{
        uint8_t *buffer = model->sv_network->binary[0];
        uint32_t length = model->sv_network->length[0];
        for (uint32_t i = 0; i< length;i+=8) printf("%02x %02x %02x %02x %02x
    %02x %02x %02x\n", buffer[i+0], buffer[i+1], buffer[i+2], buffer[i+3],
            buffer[i+4], buffer[i+5], buffer[i+6], buffer[i+7]);
    }*/
    size_t  C_len = 0x62;
    uint8_t C_buf[0x62];
    memcpy(C_buf, model->sv_network->binary[0], C_len);
    C_buf[53] = 0x42;
    signal_reset(model->sv_network, 0);

    /* Step the model - set can_rx and check for signals. */
    model->sv_signal->scalar[0] = 1;
    model->sv_signal->scalar[1] = 0;
    model->sv_signal->scalar[2] = 265;
    modelc_step(model->mi, mock->step_size);
    assert_int_equal(rc, 0);
    assert_double_equal(model->sv_signal->scalar[0], 1.0, 0.0);
    assert_double_equal(model->sv_signal->scalar[1], 0.0, 0.0);
    assert_double_equal(model->sv_signal->scalar[2], 265.0, 0.0);
    signal_reset(model->sv_network, 0);

    /* Step the model - inject previous CAN packet. */
    signal_append(model->sv_network, 0, C_buf, C_len);
    rc = modelc_step(model->mi, mock->step_size);
    assert_int_equal(rc, 0);
    assert_double_equal(model->sv_signal->scalar[0], 2.0, 0.0);
    assert_double_equal(model->sv_signal->scalar[1], 1.0, 0.0);
    assert_double_equal(model->sv_signal->scalar[2], 260.0, 0.0);
    assert_null(model->sv_network->binary[0]);
    assert_int_equal(model->sv_network->length[0], 0);
    assert_int_equal(model->sv_network->buffer_size[0], 0);
    signal_reset(model->sv_network, 0);
}


void test_mstep_message_function(void** state)
{
#define MSG_NF_NAME             "function_example"
#define MSG_NF_FRAME_ID         0x1f2
#define MSG_NF_FRAME_SIG_OFFSET 2
#define MSG_NF_SIG_NAME         "foo"  // uint8_t
#define MSG_NF_SIG_IDX          5
#define MSG_NF_ALIVE_NAME       "alive"  // uint8_t
#define MSG_NF_ALIVE_IDX        6
#define NETWORK_NAME            "stub_inst"
#define NETWORK_SIG             "can"

    SimMock*   mock = *state;
    ModelMock* network_model = &mock->model[0];
    assert_non_null(network_model);
    assert_true(MSG_NF_SIG_IDX < mock->sv_signal->count);
    assert_true(MSG_NF_ALIVE_IDX < mock->sv_signal->count);

    /* 0-5ms */
    {
        for (uint32_t i = 0; i < 10; i++) {
            assert_int_equal(simmock_step(mock, true), 0);
            assert_int_equal(network_model->sv_network->length[0] > 0,
                false);  // can_bus has no data.
        }
    }
    /* 5.5ms - Set a signal value. */
    {
        mock->sv_signal->scalar[MSG_NF_SIG_IDX] = 4;
        assert_int_equal(simmock_step(mock, true), 0);
        assert_int_equal(network_model->sv_network->length[0] > 0,
            true);  // can_bus has data.
        SignalCheck s_checks[] = {
            { .index = MSG_NF_SIG_IDX, .value = 4.0 },
            { .index = MSG_NF_ALIVE_IDX, .value = 1.0 },
        };
        simmock_print_scalar_signals(mock, LOG_DEBUG);
        simmock_signal_check(
            mock, NETWORK_NAME, s_checks, ARRAY_SIZE(s_checks), NULL);
    }
    /* 5.5-10ms - stable, no value change, no Tx*/
    {
        for (uint32_t i = 0; i < 9; i++) {
            assert_int_equal(simmock_step(mock, true), 0);
            assert_int_equal(network_model->sv_network->length[0] > 0,
                false);  // can_bus has no data.
            SignalCheck s_checks[] = {
                { .index = MSG_NF_SIG_IDX, .value = 4.0 },
                { .index = MSG_NF_ALIVE_IDX, .value = 1.0 },
            };
            simmock_print_scalar_signals(mock, LOG_DEBUG);
            simmock_signal_check(
                mock, NETWORK_NAME, s_checks, ARRAY_SIZE(s_checks), NULL);
        }
    }
    /* 10.5ms - Set a signal value. */
    {
        mock->sv_signal->scalar[MSG_NF_SIG_IDX] = 6;
        assert_int_equal(simmock_step(mock, true), 0);
        assert_int_equal(network_model->sv_network->length[0] > 0,
            true);  // can_bus has data.
        SignalCheck s_checks[] = {
            { .index = MSG_NF_SIG_IDX, .value = 6.0 },
            { .index = MSG_NF_ALIVE_IDX, .value = 2.0 },
        };
        simmock_print_scalar_signals(mock, LOG_DEBUG);
        simmock_signal_check(
            mock, NETWORK_NAME, s_checks, ARRAY_SIZE(s_checks), NULL);
    }
    /* 10.5-15ms - stable, no value change, no Tx*/
    {
        for (uint32_t i = 0; i < 9; i++) {
            assert_int_equal(simmock_step(mock, true), 0);
            assert_int_equal(network_model->sv_network->length[0] > 0,
                false);  // can_bus has no data.
            SignalCheck s_checks[] = {
                { .index = MSG_NF_SIG_IDX, .value = 6.0 },
                { .index = MSG_NF_ALIVE_IDX, .value = 2.0 },
            };
            simmock_print_scalar_signals(mock, LOG_DEBUG);
            simmock_signal_check(
                mock, NETWORK_NAME, s_checks, ARRAY_SIZE(s_checks), NULL);
        }
    }
}


void test_mstep_message_function_EBADMSG(void** state)
{
    UNUSED(state);
    skip();
}

int run_mstep_tests(void)
{
    void* s = test_setup;
    void* t = test_teardown;

    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_mstep, s, t),
        cmocka_unit_test_setup_teardown(test_mstep_message_function, s, t),
    };

    return cmocka_run_group_tests_name("MSTEP", tests, NULL, NULL);
}
