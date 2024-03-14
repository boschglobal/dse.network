// Copyright 2024 Robert Bosch GmbH
//
// SPDX-License-Identifier: Apache-2.0

#include <stddef.h>
#include <string.h>
#include <dse/modelc/model.h>
#include <dse/logger.h>

#define SAFETY_BRAKE_PEDAL_ID 42

typedef struct {
    ModelDesc model;
    /* Signal Interface. */
    double* pedal_fault;
    double* pedal_pos_ac;
    double* check_engine_set;
    double* check_engine_clear;
    /* State. */
    bool pedal_fault_active;
    double last_sample;
    int alive_counter;
} SafetyModelDesc;

static inline double* _index(SafetyModelDesc* m, const char* v, const char* s)
{
    ModelSignalIndex idx = m->model.index((ModelDesc*)m, v, s);
    if (idx.scalar == NULL) log_fatal("Signal not found (%s:%s)", v, s);
    return idx.scalar;
}


ModelDesc* model_create(ModelDesc* model)
{
    /* Extend the ModelDesc object (using a shallow copy). */
    SafetyModelDesc* m = calloc(1, sizeof(SafetyModelDesc));
    memcpy(m, model, sizeof(ModelDesc));

    /* Index the signals. */
    m->pedal_pos_ac = _index(m, "physical", "BrakePedalPos_AC");
    m->pedal_fault = _index(m, "model", "BrakePedalFault");
    m->check_engine_set = _index(m, "vehicle", "CheckEngineSet");
    m->check_engine_clear = _index(m, "vehicle", "CheckEngineClear");

    /* Return the extended object. */
    return (ModelDesc*)m;
}


int model_step(ModelDesc* model, double* model_time, double stop_time)
{
    SafetyModelDesc* m = (SafetyModelDesc*)model;
    int ac_sample = (int)*m->pedal_pos_ac;
    if (ac_sample == m->alive_counter) {
        /* No change observed. */
        if ((*model_time - m->last_sample) > 0.1000) {
            /* Pedal fault detected (> 100 ms). */
            *m->pedal_fault = 1;
        } else {
            *m->pedal_fault = 0;
        }
    } else {
        m->alive_counter = ac_sample;
        m->last_sample = *model_time;
        *m->pedal_fault = 0;
    }

    /* Indicate the fault condition to the vehicle (HMI). */
    *m->check_engine_set = 0;
    *m->check_engine_clear = 0;
    if ((bool)*m->pedal_fault != m->pedal_fault_active) {
        /* Fault state change. */
        if ((bool)*m->pedal_fault) {
            *m->check_engine_set = SAFETY_BRAKE_PEDAL_ID;
        } else {
            *m->check_engine_clear = SAFETY_BRAKE_PEDAL_ID;
        }
    }
    m->pedal_fault_active = (bool)*m->pedal_fault;

    /* Complete the step. */
    *model_time = stop_time;
    return 0;
}
