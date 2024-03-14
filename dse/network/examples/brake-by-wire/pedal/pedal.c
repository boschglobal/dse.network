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
    double* pedal_linear;
    double* pedal_pos;
    double* brake_force;
    /* Pedal Interface (HMI). */
    double* hmi_pedal_pos;
    double* hmi_pedal_force;
} PedalModelDesc;

static inline double* _index(PedalModelDesc* m, const char* v, const char* s)
{
    ModelSignalIndex idx = m->model.index((ModelDesc*)m, v, s);
    if (idx.scalar == NULL) log_fatal("Signal not found (%s:%s)", v, s);
    return idx.scalar;
}


ModelDesc* model_create(ModelDesc* model)
{
    /* Extend the ModelDesc object (using a shallow copy). */
    PedalModelDesc* m = calloc(1, sizeof(PedalModelDesc));
    memcpy(m, model, sizeof(ModelDesc));

    /* Index the signals. */
    m->pedal_linear = _index(m, "wire", "BrakePedal");
    m->pedal_pos = _index(m, "physical", "BrakePedalPos");
    m->brake_force = _index(m, "physical", "BrakeForce");
    m->hmi_pedal_pos = _index(m, "hmi", "BrakePedalPos");
    m->hmi_pedal_force = _index(m, "hmi", "BrakePedalForce");

    /* Return the extended object. */
    return (ModelDesc*)m;
}


int model_step(ModelDesc* model, double* model_time, double stop_time)
{
    PedalModelDesc* m = (PedalModelDesc*)model;

    /* Simple mapping to/from Pedal Interface (HMI). */
    *m->pedal_pos = *m->hmi_pedal_pos;
    *m->hmi_pedal_force = *m->brake_force;

    /* Pedal movement is duplicated to wire interface (0..1 -> 0..5 volts). */
    *m->pedal_linear = *m->hmi_pedal_pos * 5.0;

    /* Complete the step. */
    *model_time = stop_time;
    return 0;
}
