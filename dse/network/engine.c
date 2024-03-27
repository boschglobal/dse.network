// Copyright 2024 Robert Bosch GmbH
//
// SPDX-License-Identifier: Apache-2.0

#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <dlfcn.h>
#include <dse/testing.h>
#include <dse/logger.h>
#include <dse/network/network.h>
#include <dse/modelc/schema.h>
#include <dse/clib/util/yaml.h>


#define UNUSED(x)     ((void)x)
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))


int network_load_marshal_lists(
    Network* network, ModelInstanceSpec* model_instance)
{
    UNUSED(model_instance);
    assert(network);
    assert(network->name);

    HashList m_list;
    hashlist_init(&m_list, 100);

    NetworkMessage* nm = network->messages;
    while (nm && nm->name) {
        if (nm->buffer_len == 0) {
            /* Next message. */
            log_error("Message buffer_len not set!");
            nm++;
            continue;
        }
        /* Signals. */
        NetworkSignal* ns = nm->signals;
        while (ns && ns->name) {
            /* Create an MI object. */
            MarshalItem* mi = calloc(1, sizeof(MarshalItem));
            mi->signal = ns;
            mi->message = nm;
            log_debug("MarshalItem, name: %s", ns->signal_name);
            hashlist_append(&m_list, mi);

            /* Next signal. */
            ns++;
        }
        /* Next message. */
        nm++;
    }

    /* Convert the HashList to a NULL terminated list. */
    size_t count = hashlist_length(&m_list);
    network->marshal_list = calloc(count + 1, sizeof(MarshalItem));
    for (uint32_t i = 0; i < count; i++) {
        memcpy(&network->marshal_list[i], hashlist_at(&m_list, i),
            sizeof(MarshalItem));
        free(hashlist_at(&m_list, i));
    }
    hashlist_destroy(&m_list);

    return 0;
}


int network_get_signal_names(
    MarshalItem* ml, const char*** signal_names, size_t* signal_count)
{
    int          count = 0;
    const char** names = NULL;
    MarshalItem* mi_p;

    /* Default the return values (passed by reference). */
    *signal_names = NULL;
    *signal_count = 0;

    /* Count the signals first. */
    mi_p = ml;
    while (mi_p->signal) {
        count++;
        /* Next item? */
        mi_p++;
    }

    /* Allocate the return values, caller to free. */
    names = calloc(count + 1, sizeof(const char*));
    mi_p = ml;
    int    name_idx = 0;
    size_t sv_offset = 0;
    while (mi_p->signal) {
        NetworkSignal* ns = mi_p->signal;
        names[name_idx] = ns->signal_name;
        mi_p->signal_vector_index = sv_offset;
        name_idx++;
        sv_offset++;

        /* Next item? */
        mi_p++;
    }
    *signal_names = names;
    *signal_count = count;

    return 0;
}


int network_marshal_signals_to_messages(Network* network, MarshalItem* mi)
{
    if (network == NULL || mi == NULL) return 1;

    while (mi->signal) {
        if (strcmp(mi->signal->member_type, "uint8_t") == 0) {
            uint8_t _value = 0;
            _value = mi->signal->encode_func_int8(
                network->signal_vector[mi->signal_vector_index]);
            if (mi->signal->range_func_int8(_value)) {
                ((int8_t*)mi->message
                        ->buffer)[(mi->signal->buffer_offset) / sizeof(int8_t)] =
                    _value;

                log_debug("calling encode_func (%f -> %d): %d %s",
                    network->signal_vector[mi->signal_vector_index], _value,
                    ((uint8_t*)mi->message->buffer)[mi->signal->buffer_offset /
                                                         sizeof(uint8_t)],
                    mi->signal->name);
            }
        }
        if (strcmp(mi->signal->member_type, "uint16_t") == 0) {
            uint16_t _value = 0;
            _value = mi->signal->encode_func_int16(
                network->signal_vector[mi->signal_vector_index]);
            if (mi->signal->range_func_int16(_value)) {
                ((int16_t*)mi->message
                        ->buffer)[(mi->signal->buffer_offset) / sizeof(int16_t)] =
                    _value;

                log_debug("calling encode_func (%f -> %d): %d %s",
                    network->signal_vector[mi->signal_vector_index], _value,
                    ((uint16_t*)mi->message->buffer)[mi->signal->buffer_offset /
                                                          sizeof(uint16_t)],
                    mi->signal->name);
            }
        }

        if (strcmp(mi->signal->member_type, "uint32_t") == 0) {
            uint32_t _value = 0;
            _value = mi->signal->encode_func_int32(
                network->signal_vector[mi->signal_vector_index]);
            if (mi->signal->range_func_int32(_value)) {
                ((int32_t*)mi->message
                        ->buffer)[(mi->signal->buffer_offset) / sizeof(int32_t)] =
                    _value;

                log_debug("calling encode_func (%f -> %d): %d %s",
                    network->signal_vector[mi->signal_vector_index], _value,
                    ((uint32_t*)mi->message->buffer)[mi->signal->buffer_offset /
                                                          sizeof(uint32_t)],
                    mi->signal->name);
            }
        }

        if (strcmp(mi->signal->member_type, "uint64_t") == 0) {
            uint64_t _value = 0;
            _value = mi->signal->encode_func_int64(
                network->signal_vector[mi->signal_vector_index]);
            if (mi->signal->range_func_int64(_value)) {
                ((int64_t*)mi->message
                        ->buffer)[(mi->signal->buffer_offset) / sizeof(int64_t)] =
                    _value;

                log_debug("calling encode_func (%f -> %d): %d %s",
                    network->signal_vector[mi->signal_vector_index], _value,
                    ((uint64_t*)mi->message->buffer)[mi->signal->buffer_offset /
                                                          sizeof(uint64_t)],
                    mi->signal->name);
            }
        }

        if (strcmp(mi->signal->member_type, "int8_t") == 0) {
            int8_t _value = 0;
            _value = mi->signal->encode_func_int8(
                network->signal_vector[mi->signal_vector_index]);
            if (mi->signal->range_func_int8(_value)) {
                ((int8_t*)mi->message
                        ->buffer)[(mi->signal->buffer_offset) / sizeof(int8_t)] =
                    _value;
                log_debug("calling encode_func (%f -> %d): %d %s",
                    network->signal_vector[mi->signal_vector_index], _value,
                    ((int8_t*)mi->message
                            ->buffer)[mi->signal->buffer_offset / sizeof(int8_t)],
                    mi->signal->name);
            }
        }

        if (strcmp(mi->signal->member_type, "int16_t") == 0) {
            int16_t _value = 0;
            _value = mi->signal->encode_func_int16(
                network->signal_vector[mi->signal_vector_index]);
            if (mi->signal->range_func_int16(_value)) {
                ((int16_t*)mi->message
                        ->buffer)[(mi->signal->buffer_offset) / sizeof(int16_t)] =
                    _value;
                log_debug("calling encode_func (%f -> %d): %d %s",
                    network->signal_vector[mi->signal_vector_index], _value,
                    ((int16_t*)mi->message->buffer)[mi->signal->buffer_offset /
                                                         sizeof(int16_t)],
                    mi->signal->name);
            }
        }

        if (strcmp(mi->signal->member_type, "int32_t") == 0) {
            int32_t _value = 0;
            _value = mi->signal->encode_func_int32(
                network->signal_vector[mi->signal_vector_index]);
            if (mi->signal->range_func_int32(_value)) {
                ((int32_t*)mi->message
                        ->buffer)[(mi->signal->buffer_offset) / sizeof(int32_t)] =
                    _value;
                log_debug("calling encode_func (%f -> %d): %d %s",
                    network->signal_vector[mi->signal_vector_index], _value,
                    ((int32_t*)mi->message->buffer)[mi->signal->buffer_offset /
                                                         sizeof(int32_t)],
                    mi->signal->name);
            }
        }
        if (strcmp(mi->signal->member_type, "int64_t") == 0) {
            int16_t _value = 0;
            _value = mi->signal->encode_func_int64(
                network->signal_vector[mi->signal_vector_index]);
            if (mi->signal->range_func_int64(_value)) {
                ((int64_t*)mi->message
                        ->buffer)[(mi->signal->buffer_offset) / sizeof(int64_t)] =
                    _value;
                log_debug("calling encode_func (%f -> %d): %d %s",
                    network->signal_vector[mi->signal_vector_index], _value,
                    ((int64_t*)mi->message->buffer)[mi->signal->buffer_offset /
                                                         sizeof(int64_t)],
                    mi->signal->name);
            }
        }
        if (strcmp(mi->signal->member_type, "float") == 0) {
            float _value = 0;
            _value = mi->signal->encode_func_float(
                network->signal_vector[mi->signal_vector_index]);
            if (mi->signal->range_func_float(_value)) {
                ((float*)mi->message
                        ->buffer)[(mi->signal->buffer_offset) / sizeof(float)] =
                    _value;
                log_debug("calling encode_func (%f -> %d): %d %s",
                    network->signal_vector[mi->signal_vector_index], _value,
                    ((float*)mi->message
                            ->buffer)[mi->signal->buffer_offset / sizeof(float)],
                    mi->signal->name);
            }
        }
        if (strcmp(mi->signal->member_type, "double") == 0) {
            double _value = 0;
            _value = mi->signal->encode_func_double(
                network->signal_vector[mi->signal_vector_index]);
            if (mi->signal->range_func_double(_value)) {
                ((double*)mi->message
                        ->buffer)[(mi->signal->buffer_offset) / sizeof(double)] =
                    _value;
                log_debug("calling encode_func (%f -> %d): %d %s",
                    network->signal_vector[mi->signal_vector_index], _value,
                    ((double*)mi->message
                            ->buffer)[mi->signal->buffer_offset / sizeof(double)],
                    mi->signal->name);
            }
        }


        /* Next item. */
        mi++;
    }
    return 0;
}


int network_marshal_messages_to_signals(Network* network, MarshalItem* mi)
{
    if (network == NULL || mi == NULL) return 1;
    while (mi->signal) {
        log_trace("MI Signal: frame_id=%d, update_signals=%d, index=%d, type=%s",
        mi->message->frame_id, mi->message->update_signals, mi->signal_vector_index, mi->signal->member_type);
        if (mi->message->update_signals == false) {
            /* Next item. */
            mi++;
            continue;
        }
        if (strcmp(mi->signal->member_type, "uint8_t") == 0) {
            if (mi->signal->range_func_int8(
                    ((int8_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                        sizeof(int8_t)])) {
                double _v = mi->signal->decode_func_int8((
                    (int8_t*)mi->message
                        ->buffer)[(mi->signal->buffer_offset) / sizeof(int8_t)]);
                network->signal_vector[mi->signal_vector_index] = _v;

                log_debug("calling decode_func (%d -> %f): %f %s",
                    ((uint8_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                         sizeof(uint8_t)],
                    _v, network->signal_vector[mi->signal_vector_index],
                    mi->signal->name);
            }
        }
        if (strcmp(mi->signal->member_type, "uint16_t") == 0) {
            if (mi->signal->range_func_int16(
                    ((int16_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                         sizeof(int16_t)])) {
                double _v = mi->signal->decode_func_int16((
                    (int16_t*)mi->message
                        ->buffer)[(mi->signal->buffer_offset) / sizeof(int16_t)]);
                network->signal_vector[mi->signal_vector_index] = _v;

                log_debug("calling decode_func (%d -> %f): %f %s",
                    ((uint16_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                          sizeof(uint16_t)],
                    _v, network->signal_vector[mi->signal_vector_index],
                    mi->signal->name);
            }
        }
        if (strcmp(mi->signal->member_type, "uint32_t") == 0) {
            if (mi->signal->range_func_int32(
                    ((int32_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                         sizeof(int32_t)])) {
                double _v = mi->signal->decode_func_int32((
                    (int32_t*)mi->message
                        ->buffer)[(mi->signal->buffer_offset) / sizeof(int32_t)]);
                network->signal_vector[mi->signal_vector_index] = _v;

                log_debug("calling decode_func (%d -> %f): %f %s",
                    ((uint32_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                          sizeof(uint32_t)],
                    _v, network->signal_vector[mi->signal_vector_index],
                    mi->signal->name);
            }
        }
        if (strcmp(mi->signal->member_type, "uint64_t") == 0) {
            if (mi->signal->range_func_int64(
                    ((int64_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                         sizeof(int64_t)])) {
                double _v = mi->signal->decode_func_int64((
                    (int64_t*)mi->message
                        ->buffer)[(mi->signal->buffer_offset) / sizeof(int64_t)]);
                network->signal_vector[mi->signal_vector_index] = _v;

                log_debug("calling decode_func (%d -> %f): %f %s",
                    ((uint64_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                          sizeof(uint64_t)],
                    _v, network->signal_vector[mi->signal_vector_index],
                    mi->signal->name);
            }
        }
        if (strcmp(mi->signal->member_type, "int8_t") == 0) {
            if (mi->signal->range_func_int8(
                    ((int8_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                        sizeof(int8_t)])) {
                double _v = mi->signal->decode_func_int8((
                    (int8_t*)mi->message
                        ->buffer)[(mi->signal->buffer_offset) / sizeof(int8_t)]);
                network->signal_vector[mi->signal_vector_index] = _v;

                log_debug("calling decode_func (%d -> %f): %f %s",
                    ((int8_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                        sizeof(int8_t)],
                    _v, network->signal_vector[mi->signal_vector_index],
                    mi->signal->name);
            }
        }
        if (strcmp(mi->signal->member_type, "int16_t") == 0) {
            if (mi->signal->range_func_int16(
                    ((int16_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                         sizeof(int16_t)])) {
                double _v = mi->signal->decode_func_int16((
                    (int16_t*)mi->message
                        ->buffer)[(mi->signal->buffer_offset) / sizeof(int16_t)]);
                network->signal_vector[mi->signal_vector_index] = _v;

                log_debug("calling decode_func (%d -> %f): %f %s",
                    ((int16_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                         sizeof(int16_t)],
                    _v, network->signal_vector[mi->signal_vector_index],
                    mi->signal->name);
            }
        }
        if (strcmp(mi->signal->member_type, "int32_t") == 0) {
            if (mi->signal->range_func_int32(
                    ((int32_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                         sizeof(int32_t)])) {
                double _v = mi->signal->decode_func_int32((
                    (int32_t*)mi->message
                        ->buffer)[(mi->signal->buffer_offset) / sizeof(int32_t)]);
                network->signal_vector[mi->signal_vector_index] = _v;

                log_debug("calling decode_func (%d -> %f): %f %s",
                    ((int32_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                         sizeof(int32_t)],
                    _v, network->signal_vector[mi->signal_vector_index],
                    mi->signal->name);
            }
        }
        if (strcmp(mi->signal->member_type, "int64_t") == 0) {
            if (mi->signal->range_func_int64(
                    ((int64_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                         sizeof(int64_t)])) {
                double _v = mi->signal->decode_func_int64((
                    (int64_t*)mi->message
                        ->buffer)[(mi->signal->buffer_offset) / sizeof(int64_t)]);
                network->signal_vector[mi->signal_vector_index] = _v;

                log_debug("calling decode_func (%d -> %f): %f %s",
                    ((int64_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                         sizeof(int64_t)],
                    _v, network->signal_vector[mi->signal_vector_index],
                    mi->signal->name);
            }
        }
        if (strcmp(mi->signal->member_type, "float") == 0) {
            if (mi->signal->range_func_float((
                    (float*)mi->message
                        ->buffer)[(mi->signal->buffer_offset) / sizeof(float)])) {
                double _v = mi->signal->decode_func_float((
                    (float*)mi->message
                        ->buffer)[(mi->signal->buffer_offset) / sizeof(float)]);
                network->signal_vector[mi->signal_vector_index] = _v;

                log_debug("calling decode_func (%d -> %f): %f %s",
                    ((float*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                       sizeof(float)],
                    _v, network->signal_vector[mi->signal_vector_index],
                    mi->signal->name);
            }
        }
        if (strcmp(mi->signal->member_type, "double") == 0) {
            if (mi->signal->range_func_double(
                    ((double*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                        sizeof(double)])) {
                double _v = mi->signal->decode_func_double((
                    (double*)mi->message
                        ->buffer)[(mi->signal->buffer_offset) / sizeof(double)]);
                network->signal_vector[mi->signal_vector_index] = _v;

                log_debug("calling decode_func (%d -> %f): %f %s",
                    ((double*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                        sizeof(double)],
                    _v, network->signal_vector[mi->signal_vector_index],
                    mi->signal->name);
            }
        }
        /* Next item. */
        mi++;
    }
    return 0;
}


void network_pack_messages(Network* network)
{
    assert(network);

    /* Loop over messages and call pack_func. */
    NetworkMessage* nm_p = network->messages;
    while (nm_p->name) {
        if (nm_p->pack_func) {
            nm_p->pack_func(nm_p->payload, nm_p->buffer, nm_p->payload_len);

            /* Calculate checksum for the current buffer. */
            uint32_t payload_checksum = simbus_generate_uid_hash(
                (uint8_t*)nm_p->buffer, nm_p->buffer_len);

            /* Check if the payload_checksum is different. */
            if (payload_checksum != nm_p->buffer_checksum) {
                if(nm_p->cycle_time_ms) {
                } else {
                nm_p->needs_tx = true;
                log_debug("encode path checksum %u", payload_checksum);
                nm_p->buffer_checksum = payload_checksum;
                }
            } else {
                nm_p->needs_tx = false;
            }
        }

        /* Next message. */
        nm_p++;
    }
}


void network_unpack_messages(Network* network)
{
    assert(network);

    /* Loop over messages and call unpack_func. */
    NetworkMessage* nm_p = network->messages;
    while (nm_p->name) {
        if (nm_p->unpack_func) {
            nm_p->unpack_func(nm_p->buffer, nm_p->payload, nm_p->payload_len);
        }

        /* Next message. */
        nm_p++;
    }
}


int network_unload_marshal_lists(Network* network)
{
    if (network) {
        if (network->marshal_list) free(network->marshal_list);
    }

    return 0;
}
