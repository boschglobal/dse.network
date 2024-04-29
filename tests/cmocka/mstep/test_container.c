// Copyright 2024 Robert Bosch GmbH
//
// SPDX-License-Identifier: Apache-2.0

#include <dse/testing.h>
#include <dse/logger.h>
#include <dse/modelc/model.h>
#include <dse/network/network.h>
#include <dse/ncodec/codec.h>
#include <dse/mocks/simmock.h>


static int test_setup(void** state)
{
    const char* inst_names[] = {
        "stub_inst",
    };
    char* argv[] = {
        (char*)"test_network",
        (char*)"--name=stub_inst",
        (char*)"--logger=5",  // 1=debug, 5=QUIET (commit with 5!)
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


#define MSG_FRAME_ID          600
#define MSG_HEADER_ID_OFFSET  0
#define MSG_HEADER_DLC_OFFSET 3
#define MSG_SIGNAL_OFFSET     4
#define NETWORK_NAME          "stub_inst"
#define NETWORK_SIG           "can"


void test_container__601_tx(void** state)
{
    SimMock*   mock = *state;
    ModelMock* network_model = &mock->model[0];
    ModelDesc* m = network_model->mi->model_desc;
    assert_non_null(m);
    ModelSignalIndex foo_idx =
        m->index((ModelDesc*)m, "signal_channel", "foo_double");
    assert_non_null(foo_idx.scalar);
    assert_ptr_equal(foo_idx.sv, network_model->sv_signal);
    simmock_print_scalar_signals(mock, LOG_DEBUG);

    /* Step the model. */
    assert_int_equal(simmock_step(mock, true), 0);
    assert_int_equal(network_model->sv_network->length[0] > 0,
        false);  // can_bus has no data.

    /* Set a signal, step model. */
    mock->sv_signal->scalar[foo_idx.signal] = 10.0;
    assert_int_equal(simmock_step(mock, true), 0);


    /* Check network. */
    SignalCheck s_checks[] = {
        { .index = foo_idx.signal, .value = 10.0 },
    };
    FrameCheck f_checks[] = {
        { .frame_id = MSG_FRAME_ID,
            .offset = MSG_HEADER_ID_OFFSET + 0,
            .value = 0x59 },
        { .frame_id = MSG_FRAME_ID,
            .offset = MSG_HEADER_ID_OFFSET + 1,
            .value = 0x02 },
        { .frame_id = MSG_FRAME_ID,
            .offset = MSG_HEADER_ID_OFFSET + 2,
            .value = 0x00 },
        { .frame_id = MSG_FRAME_ID,
            .offset = MSG_HEADER_DLC_OFFSET,
            .value = 0x2a },
        { .frame_id = MSG_FRAME_ID,
            .offset = MSG_SIGNAL_OFFSET + 6,
            .value = 0x24 },
        { .frame_id = MSG_FRAME_ID,
            .offset = MSG_SIGNAL_OFFSET + 7,
            .value = 0x40 },
    };
    simmock_print_scalar_signals(mock, LOG_DEBUG);
    simmock_print_network_frames(mock, LOG_DEBUG);
    assert_int_equal(network_model->sv_network->length[0] > 0, true);
    simmock_signal_check(
        mock, NETWORK_NAME, s_checks, ARRAY_SIZE(s_checks), NULL);
    simmock_frame_check(
        mock, NETWORK_NAME, NETWORK_SIG, f_checks, ARRAY_SIZE(f_checks));
}


void test_container__601_rx(void** state)
{
    SimMock*   mock = *state;
    ModelMock* network_model = &mock->model[0];
    ModelDesc* m = network_model->mi->model_desc;
    assert_non_null(m);
    ModelSignalIndex foo_idx =
        m->index((ModelDesc*)m, "signal_channel", "foo_double");
    assert_non_null(foo_idx.scalar);
    assert_ptr_equal(foo_idx.sv, network_model->sv_signal);
    simmock_print_scalar_signals(mock, LOG_DEBUG);

    /* Set a signal, step model. */
    mock->sv_signal->scalar[foo_idx.signal] = 10.0;
    assert_int_equal(simmock_step(mock, true), 0);
    assert_int_equal(network_model->sv_network->length[0] > 0, true);
    simmock_print_network_frames(mock, LOG_DEBUG);

    /* Step the model. */
    assert_int_equal(simmock_step(mock, true), 0);
    assert_int_equal(network_model->sv_network->length[0] > 0, false);

    /* Inject a frame, step model. */
    uint8_t m601_msg[] = {
        0x59, 0x02, 0x00,                                // header_id
        0x2a,                                            // header_dlc
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x40,  // foo_double (20.0)
    };
    simmock_write_frame(mock->sv_network_tx, "can", m601_msg, sizeof(m601_msg),
        600, CAN_EXTENDED_FRAME);
    assert_int_equal(mock->sv_network_tx->length[0] > sizeof(m601_msg), true);
    assert_int_equal(simmock_step(mock, true), 0);
    // assert_int_equal(network_model->sv_network->length[0] > 0, true);

    /* Check signal. */
    SignalCheck s_checks[] = {
        { .index = foo_idx.signal, .value = 20.0 },
    };
    simmock_print_scalar_signals(mock, LOG_DEBUG);
    simmock_print_network_frames(mock, LOG_DEBUG);
    simmock_signal_check(
        mock, NETWORK_NAME, s_checks, ARRAY_SIZE(s_checks), NULL);
}


int run_container_tests(void)
{
    void* s = test_setup;
    void* t = test_teardown;

    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_container__601_tx, s, t),
        cmocka_unit_test_setup_teardown(test_container__601_rx, s, t),
    };

    return cmocka_run_group_tests_name("CONTAINER", tests, NULL, NULL);
}
