// Copyright 2024 Robert Bosch GmbH
//
// SPDX-License-Identifier: Apache-2.0

#include <assert.h>
#include <dlfcn.h>
#include <string.h>
#include <dse/testing.h>
#include <dse/logger.h>
#include <dse/clib/util/yaml.h>
#include <dse/modelc/model.h>
#include <dse/modelc/schema.h>
#include <dse/network/network.h>
#include <dse/ncodec/codec.h>


#define UNUSED(x)     ((void)x)
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

typedef struct SRMap {
    bool     active;
    uint32_t vector_index;
    size_t   signal_index;
} SRMap;


typedef struct {
    ModelDesc     model;
    /* Runnable model object. */
    Network       network;
    /* Ticks. */
    bool          init_tick_done;
    double        last_tick;
    /* Signal vectors. */
    SignalVector* sv_signal;
    SignalVector* sv_network;
    uint32_t      sv_network_index;
    NCODEC*       network_codec;
    SRMap*        __sr_map;
} NetworkModelDesc;


ModelDesc* model_create(ModelDesc* model)
{
    /* Extend the ModelDesc object (using a shallow copy). */
    NetworkModelDesc* m = calloc(1, sizeof(NetworkModelDesc));
    memcpy(m, model, sizeof(ModelDesc));

    /* Locate SignalVectors. */
    SignalVector* sv = m->model.sv;
    while (sv && sv->name) {
        if (strcmp(sv->alias, "signal_channel") == 0) m->sv_signal = sv;
        if (strcmp(sv->alias, "network_channel") == 0) m->sv_network = sv;
        /* Next signal vector. */
        sv++;
    }
    if (m->sv_signal == NULL) log_fatal("Signal channel not found!");
    if (m->sv_network == NULL) log_fatal("Network channel not found!");
    if (m->sv_network->is_binary == false)
        log_fatal("Network channel is not binary!");

    /* Initialise the Network objects. */
    dse_yaml_get_string(
        m->model.mi->spec, "metadata/network", &m->network.name);
    log_notice("Network: %s", m->network.name);

    int rc = network_load(&m->network, m->model.mi);
    if (rc) log_fatal("Network load failed!");
    network_schedule_reset(&m->network);

    /* Locate the Network signal. */
    const char* network_signal = NULL;
    for (uint32_t i = 0; i < m->sv_network->count; i++) {
        const char* name =
            m->sv_network->annotation(m->sv_network, i, "network");
        if (name == NULL) continue;
        if (strcmp(name, m->network.name) == 0) {
            network_signal = m->sv_network->signal[i];
            m->sv_network_index = i;
            break;
        }
    }
    if (network_signal == NULL) {
        log_error("Searched for signal annotation 'network' with value '%s' on "
                  "network SignalVector.",
            m->network.name);
        log_error("Check ModelInstance metadata/network (currently: %s)",
            m->network.name);
        log_error("Check SignalGroup[Network] signal/annotation/network for "
                  "the network signal");
        log_fatal("Network signal not found!");
    }
    log_notice(
        "  network signal: %s (index=%d)", network_signal, m->sv_network_index);
    log_notice(
        "  signal mimetype: %s", m->sv_network->mime_type[m->sv_network_index]);

    /* Locate the Network Codec. */
    m->network_codec = m->sv_network->codec(m->sv_network, m->sv_network_index);
    if (m->network_codec == NULL) log_fatal("Unable to locate NCodec object!");

    /* Print the parsed network. */
    log_notice("  Network Configuration:");
    uint32_t sig_idx = 0;
    for (NetworkMessage* msg = m->network.messages; msg->name; msg++) {
        log_notice("    %s [frame_id 0x%x, len %d]", msg->name, msg->frame_id,
            msg->buffer_len);
        for (NetworkSignal* sig = msg->signals;
             sig->name && sig_idx < m->network.signal_count; sig++) {
            const char* signal_name = m->network.signal_name[sig_idx++];
            log_notice("        %s [%s]", signal_name, sig->name);
        }
    }

    /* Create the SignalVector mapping. */
    log_notice("  SignalVector<->Network Mapping:");
    m->__sr_map = calloc(m->sv_signal->count, sizeof(SRMap));
    for (uint32_t sv_idx = 0; sv_idx < m->sv_signal->count; sv_idx++) {
        for (size_t sig_idx = 0; sig_idx < m->network.signal_count; sig_idx++) {
            const char* sv_sig_name = m->sv_signal->signal[sv_idx];
            const char* nt_sig_name = m->network.signal_name[sig_idx];
            log_debug("mapping attempt (signal/network): %s and %s",
                sv_sig_name, nt_sig_name);
            if (strcmp(sv_sig_name, nt_sig_name) != 0) continue;
            /* Mapping found. */
            m->__sr_map[sv_idx].active = true;
            m->__sr_map[sv_idx].vector_index = sv_idx;
            m->__sr_map[sv_idx].signal_index = sig_idx;
            log_notice("    [%d]:[%d] %s->%s", sv_idx, sig_idx, sv_sig_name,
                nt_sig_name);
            break;
        }
    }

    /* Set the SignalVector initial value. */
    MarshalItem* mi_p = m->network.marshal_list;
    while (mi_p->signal) {
        size_t sig_idx = mi_p->signal_vector_index;
        m->network.signal_vector[sig_idx] = mi_p->signal->init_value;
        log_debug("signal: %s init_value %f", mi_p->signal->name,
            mi_p->signal->init_value);
        /* Next item? */
        mi_p++;
    }
    for (uint32_t i = 0; i < m->sv_signal->count; i++) {
        if (m->__sr_map[i].active == false) continue;
        uint32_t sv_idx = m->__sr_map[i].vector_index;
        size_t   sig_idx = m->__sr_map[i].signal_index;
        m->sv_signal->scalar[sv_idx] = m->network.signal_vector[sig_idx];
    }

    /* Trigger checksum calculation. */
    network_marshal_signals_to_messages(&m->network, m->network.marshal_list);
    network_pack_messages(&m->network);
    for (NetworkMessage* msg = m->network.messages; msg->name; msg++) {
        msg->needs_tx = false;
        log_debug("message: %s checksum %d", msg->name, msg->buffer_checksum);
    }

    /* Return the extended object. */
    return (ModelDesc*)m;
}


int model_step(ModelDesc* model, double* model_time, double stop_time)
{
    NetworkModelDesc* m = (NetworkModelDesc*)model;

    /* RX: SignalVector -> Network. */
    for (uint32_t i = 0; i < m->sv_signal->count; i++) {
        if (m->__sr_map[i].active == false) continue;
        uint32_t sv_idx = m->__sr_map[i].vector_index;
        size_t   sig_idx = m->__sr_map[i].signal_index;
        m->network.signal_vector[sig_idx] = m->sv_signal->scalar[sv_idx];
        log_trace("RX signals.signal_vector[%d] = %f", sig_idx,
            m->sv_signal->scalar[sv_idx]);
    }
    network_decode_from_bus(&m->network, m->network_codec);
    network_function_apply_decode(&m->network);
    network_marshal_messages_to_signals(&m->network, m->network.marshal_list);
    m->sv_network->release(m->sv_network, m->sv_network_index);


    /* The network tasks are organised on a 1 ms schedule and need to be
    ticked at that cadence, even if the task themselves are on a slacker
    schedule (e.g. 5 ms). */


    /* The initial tick should occur only once. */
    if (*model_time == 0.0) {
        if (m->init_tick_done == false) {
            log_trace("Tick at model_time %f", *model_time);
            network_schedule_tick(&m->network);
            m->init_tick_done = true;
        }
    }

    /* How many ticks should occur in this step? Due to floating point
    accuracy, push the ticks a little (1.01) to be sure that the int
    conversion rounds down to 1 (and not
     from 0.999999 to 0). */

    int ticks = (((*model_time - m->last_tick) / 0.001) * 1.01);
    if (ticks) {
        for (int t = 0; t < ticks; t++) {
            log_trace("Tick at model_time %f", *model_time);
            network_schedule_tick(&m->network);
            m->last_tick = *model_time;
        }
    }

    network_marshal_signals_to_messages(&m->network, m->network.marshal_list);
    network_pack_messages(&m->network);
    network_function_apply_encode(&m->network);
    network_encode_to_bus(&m->network, m->network_codec);
    network_marshal_messages_to_signals(&m->network, m->network.marshal_list);

    /* TX: Network->SignalVector. */
    for (uint32_t i = 0; i < m->sv_signal->count; i++) {
        if (m->__sr_map[i].active == false) continue;
        uint32_t sv_idx = m->__sr_map[i].vector_index;
        size_t   sig_idx = m->__sr_map[i].signal_index;
        m->sv_signal->scalar[sv_idx] = m->network.signal_vector[sig_idx];
        log_trace(
            "TX sv.scalar[%d] = %f", sv_idx, m->network.signal_vector[sig_idx]);
    }

    /* Advance the model time. */
    *model_time = stop_time;
    return 0;
}


void model_destroy(ModelDesc* model)
{
    NetworkModelDesc* m = (NetworkModelDesc*)model;
    if (m->__sr_map) free(m->__sr_map);
    network_unload(&m->network);
}
