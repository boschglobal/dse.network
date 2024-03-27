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


#define MSG_NAME             "scheduled_message"
#define MSG_FRAME_ID         0x1f6u
#define MSG_FRAME_SIG_OFFSET 0
#define MSG_CYCLE_TIME_MS    10
#define MSG_SIG_NAME         "Schedule_signal"  // uint8_t
#define MSG_SIG_IDX          3
#define NETWORK_NAME         "stub_inst"
#define NETWORK_SIG          "can"


void test_schedule__cycle_time(void** state)
{
    SimMock*   mock = *state;
    ModelMock* network_model = &mock->model[0];
    assert_non_null(network_model);

    /* 0-9.5ms */
    {
        for (uint32_t i = 0; i < 20; i++) {
            assert_int_equal(simmock_step(mock, true), 0);
            assert_int_equal(network_model->sv_network->length[0] > 0,
                false);  // can_bus has no data.
        }
    }
    /* 10ms */
    {
        for (uint32_t i = 0; i < 1; i++) {
            assert_int_equal(simmock_step(mock, true), 0);
        }
        SignalCheck s_checks[] = {
            { .index = MSG_SIG_IDX, .value = 0.0 },
        };
        FrameCheck f_checks[] = {
            { .frame_id = MSG_FRAME_ID,
                .offset = MSG_FRAME_SIG_OFFSET,
                .value = 0x00 },
        };
        simmock_print_scalar_signals(mock, LOG_DEBUG);
        simmock_print_network_frames(mock, LOG_DEBUG);
        assert_int_equal(network_model->sv_network->length[0] > 0, true);
        simmock_signal_check(
            mock, NETWORK_NAME, s_checks, ARRAY_SIZE(s_checks), NULL);
        simmock_frame_check(
            mock, NETWORK_NAME, NETWORK_SIG, f_checks, ARRAY_SIZE(f_checks));
    }
    /* 10-19.5ms */
    {
        for (uint32_t i = 0; i < 19; i++) {
            assert_int_equal(simmock_step(mock, true), 0);
            assert_int_equal(network_model->sv_network->length[0] > 0,
                false);  // can_bus has no data.
        }
    }
    /* 20ms */
    {
        for (uint32_t i = 0; i < 1; i++) {
            assert_int_equal(simmock_step(mock, true), 0);
        }
        SignalCheck s_checks[] = {
            { .index = MSG_SIG_IDX, .value = 0.0 },
        };
        FrameCheck f_checks[] = {
            { .frame_id = MSG_FRAME_ID,
                .offset = MSG_FRAME_SIG_OFFSET,
                .value = 0x00 },
        };
        simmock_print_scalar_signals(mock, LOG_DEBUG);
        simmock_print_network_frames(mock, LOG_DEBUG);
        assert_int_equal(network_model->sv_network->length[0] > 0, true);
        simmock_signal_check(
            mock, NETWORK_NAME, s_checks, ARRAY_SIZE(s_checks), NULL);
        simmock_frame_check(
            mock, NETWORK_NAME, NETWORK_SIG, f_checks, ARRAY_SIZE(f_checks));
    }
}


void test_schedule__signal_change(void** state)
{
    SimMock* mock = *state;
    ModelMock* network_model = &mock->model[0];
    assert_non_null(network_model);

    /* 0-5ms */
    {
        for (uint32_t i = 0; i < 10; i++) {
            assert_int_equal(simmock_step(mock, true), 0);
            assert_int_equal(network_model->sv_network->length[0] > 0, false);  // can_bus has no data.
        }
        /* Set a signal value. */
        assert_true(MSG_SIG_IDX < mock->sv_signal->count);
        mock->sv_signal->scalar[MSG_SIG_IDX] = 1;
    }
    /* 5-9.5ms */
    {
        for (uint32_t i = 0; i < 10; i++) {
            assert_int_equal(simmock_step(mock, true), 0);
            assert_int_equal(network_model->sv_network->length[0] > 0, false);  // can_bus has no data.
        }
    }
    /* 10ms */
    {
        for (uint32_t i = 0; i < 1; i++) {
            assert_int_equal(simmock_step(mock, true), 0);
        }
        SignalCheck s_checks[] = {
            { .index = MSG_SIG_IDX, .value = 1.0 },
        };
        FrameCheck f_checks[] = {
            { .frame_id = MSG_FRAME_ID, .offset = MSG_FRAME_SIG_OFFSET, .value = 0x01 },
        };
        simmock_print_scalar_signals(mock, LOG_DEBUG);
        simmock_print_network_frames(mock, LOG_DEBUG);
        assert_int_equal(network_model->sv_network->length[0] > 0, true);
        simmock_signal_check(mock, NETWORK_NAME, s_checks, ARRAY_SIZE(s_checks), NULL);
        simmock_frame_check(mock, NETWORK_NAME, NETWORK_SIG, f_checks, ARRAY_SIZE(f_checks));
    }
    /* 10-19.5ms */
    {
        for (uint32_t i = 0; i < 19; i++) {
            assert_int_equal(simmock_step(mock, true), 0);
            assert_int_equal(network_model->sv_network->length[0] > 0, false);  // can_bus has no data.
        }
    }
    /* 20ms */
    {
        for (uint32_t i = 0; i < 1; i++) {
            assert_int_equal(simmock_step(mock, true), 0);
        }
        SignalCheck s_checks[] = {
            { .index = MSG_SIG_IDX, .value = 1.0 },
        };
        FrameCheck f_checks[] = {
            { .frame_id = MSG_FRAME_ID, .offset = MSG_FRAME_SIG_OFFSET, .value = 0x01 },
        };
        simmock_print_scalar_signals(mock, LOG_DEBUG);
        simmock_print_network_frames(mock, LOG_DEBUG);
        assert_int_equal(network_model->sv_network->length[0] > 0, true);
        simmock_signal_check(mock, NETWORK_NAME, s_checks, ARRAY_SIZE(s_checks), NULL);
        simmock_frame_check(mock, NETWORK_NAME, NETWORK_SIG, f_checks, ARRAY_SIZE(f_checks));
    }
}


int run_schedule_tests(void)
{
    void* s = test_setup;
    void* t = test_teardown;

    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_schedule__cycle_time, s, t),
        cmocka_unit_test_setup_teardown(test_schedule__signal_change, s, t),
    };

    return cmocka_run_group_tests_name("SCHEDULE", tests, NULL, NULL);
}
