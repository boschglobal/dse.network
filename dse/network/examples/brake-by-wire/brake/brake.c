// Copyright 2024 Robert Bosch GmbH
//
// SPDX-License-Identifier: Apache-2.0

#include <stddef.h>
#include <string.h>
#include <dse/modelc/model.h>
#include <dse/logger.h>


typedef struct {
    ModelDesc model;
    /* Signal Interface. */
    double*   pedal_fault;
    double*   pedal_linear;
    double*   pedal_pos;
    double*   brake_force;
} BrakeModelDesc;

static inline double* _index(BrakeModelDesc* m, const char* v, const char* s)
{
    ModelSignalIndex idx = m->model.index((ModelDesc*)m, v, s);
    if (idx.scalar == NULL) log_fatal("Signal not found (%s:%s)", v, s);
    return idx.scalar;
}


ModelDesc* model_create(ModelDesc* model)
{
    /* Extend the ModelDesc object (using a shallow copy). */
    BrakeModelDesc* m = calloc(1, sizeof(BrakeModelDesc));
    memcpy(m, model, sizeof(ModelDesc));

    /* Index the signals. */
    m->pedal_fault = _index(m, "model", "BrakePedalFault");
    m->pedal_linear = _index(m, "wire", "BrakePedal");
    m->pedal_pos = _index(m, "physical", "BrakePedalPos");
    m->brake_force = _index(m, "physical", "BrakeForce");

    /* Return the extended object. */
    return (ModelDesc*)m;
}


int model_step(ModelDesc* model, double* model_time, double stop_time)
{
    BrakeModelDesc* m = (BrakeModelDesc*)model;

    /* Calculate the brake request force. */
    double brake_request = 0;  // 0..1
    if (*m->pedal_fault) {
        // Fall back to linear pedal (0..5 volts -> 0..1).
        brake_request = (*m->pedal_linear / 5.0);
    } else {
        brake_request = *m->pedal_pos;
    }

    /* Simple linear model. */
    *m->brake_force = brake_request;

    /* Complete the step. */
    *model_time = stop_time;
    return 0;
}
