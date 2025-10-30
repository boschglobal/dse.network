// Copyright 2024 Robert Bosch GmbH
//
// SPDX-License-Identifier: Apache-2.0

#include <stddef.h>
#include <string.h>
#include <dse/modelc/model.h>
#include <dse/logger.h>


#define UNUSED(x) ((void)x)


typedef struct {
    ModelDesc model;
    /* Pedal Interface (HMI). */
    double*   hmi_pedal_pos;
    double*   hmi_pedal_force;
    /* Warning Lights. */
    double*   hmi_check_engine;
    /* Vehicle Network. */
    double*   check_engine_set;
    double*   check_engine_clear;
} DriverModelDesc;

static inline double* _index(DriverModelDesc* m, const char* v, const char* s)
{
    ModelSignalIndex idx = m->model.index((ModelDesc*)m, v, s);
    if (idx.scalar == NULL) log_fatal("Signal not found (%s:%s)", v, s);
    return idx.scalar;
}


ModelDesc* model_create(ModelDesc* model)
{
    /* Extend the ModelDesc object (using a shallow copy). */
    DriverModelDesc* m = calloc(1, sizeof(DriverModelDesc));
    memcpy(m, model, sizeof(ModelDesc));

    /* Index the signals. */
    m->hmi_pedal_pos = _index(m, "hmi", "BrakePedalPos");
    m->hmi_pedal_force = _index(m, "hmi", "BrakePedalForce");
    m->hmi_check_engine = _index(m, "hmi", "CheckEngine");
    m->check_engine_set = _index(m, "vehicle", "CheckEngineSet");
    m->check_engine_clear = _index(m, "vehicle", "CheckEngineClear");

    /* Return the extended object. */
    return (ModelDesc*)m;
}


int model_step(ModelDesc* model, double* model_time, double stop_time)
{
    DriverModelDesc* m = (DriverModelDesc*)model;
    UNUSED(m);

    // TODO operate the brake pedal

    // TODO manage the check engine lamp

    // TODO emit a log file for graphical presentation ??? also for other
    // signals ??


    /* Complete the step. */
    *model_time = stop_time;
    return 0;
}
