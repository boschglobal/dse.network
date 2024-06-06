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


int network_load_marshal_lists(Network* n)
{
    assert(n);
    assert(n->name);

    HashList m_list;
    hashlist_init(&m_list, 100);

    for (NetworkMessage* nm = n->messages; nm && nm->name; nm++) {
        if (nm->buffer_len == 0) {
            /* Next message. */
            log_error("Message buffer_len not set!");
            nm++;
            continue;
        }
        /* Signals. */
        for (NetworkSignal* ns = nm->signals; ns && ns->name; ns++) {
            /* Create an MI object. */
            MarshalItem* mi = calloc(1, sizeof(MarshalItem));
            mi->signal = ns;
            mi->message = nm;
            log_debug("MarshalItem, name: %s", ns->signal_name);
            hashlist_append(&m_list, mi);
        }
    }

    /* Convert the HashList to a NULL terminated list. */
    size_t count = hashlist_length(&m_list);
    n->marshal_list = calloc(count + 1, sizeof(MarshalItem));
    for (uint32_t i = 0; i < count; i++) {
        memcpy(
            &n->marshal_list[i], hashlist_at(&m_list, i), sizeof(MarshalItem));
        free(hashlist_at(&m_list, i));
    }
    hashlist_destroy(&m_list);

    /* Set any mux_mi references. */
    for (MarshalItem* mi = n->marshal_list; mi && mi->signal; mi++) {
        if (mi->signal->mux_signal) {
            mi->signal->mux_mi = mi;
            mi->message->mux_signal = mi->signal;
        }
    }

    return 0;
}


int network_get_signal_names(
    MarshalItem* ml, const char*** signal_names, size_t* signal_count)
{
    int          count = 0;
    const char** names = NULL;
    MarshalItem* mi;

    /* Default the return values (passed by reference). */
    *signal_names = NULL;
    *signal_count = 0;

    /* Count the signals first. */
    mi = ml;
    while (mi->signal) {
        count++;
        /* Next item? */
        mi++;
    }

    /* Allocate the return values, caller to free. */
    names = calloc(count + 1, sizeof(const char*));
    mi = ml;
    int    name_idx = 0;
    size_t sv_offset = 0;
    while (mi->signal) {
        NetworkSignal* ns = mi->signal;
        if (ns->internal) {
            /* Internal signal, prevent matching with ModelC signals.*/
            names[name_idx] = "";
        } else {
            names[name_idx] = ns->signal_name;
        }
        mi->signal_vector_index = sv_offset;
        name_idx++;
        sv_offset++;

        /* Next item? */
        mi++;
    }
    *signal_names = names;
    *signal_count = count;

    return 0;
}


int network_marshal_signals_to_messages(Network* n, MarshalItem* marshal_list)
{
    if (n == NULL || marshal_list == NULL) return 1;
    for (MarshalItem* mi = marshal_list; mi && mi->signal; mi++) {
        double _signal_value = n->signal_vector[mi->signal_vector_index];
        if (mi->signal->internal && mi->message->container) {
            /* Internal signals on container messages take a constant value. */
            _signal_value = mi->signal->value;
        }

        if (strcmp(mi->signal->member_type, "uint8_t") == 0) {
            uint8_t _value = 0;
            _value = mi->signal->encode_func_int8(_signal_value);
            if (mi->signal->range_func_int8(_value)) {
                ((int8_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                               sizeof(int8_t)] = _value;

                log_debug("calling encode_func (%f -> %d): %d %s",
                    _signal_value, _value,
                    ((uint8_t*)mi->message->buffer)[mi->signal->buffer_offset /
                                                    sizeof(uint8_t)],
                    mi->signal->name);
            }
        } else if (strcmp(mi->signal->member_type, "uint16_t") == 0) {
            uint16_t _value = 0;
            _value = mi->signal->encode_func_int16(_signal_value);
            if (mi->signal->range_func_int16(_value)) {
                ((int16_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                sizeof(int16_t)] = _value;

                log_debug("calling encode_func (%f -> %d): %d %s",
                    _signal_value, _value,
                    ((uint16_t*)mi->message->buffer)[mi->signal->buffer_offset /
                                                     sizeof(uint16_t)],
                    mi->signal->name);
            }
        } else if (strcmp(mi->signal->member_type, "uint32_t") == 0) {
            uint32_t _value = 0;
            _value = mi->signal->encode_func_int32(_signal_value);
            if (mi->signal->range_func_int32(_value)) {
                ((int32_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                sizeof(int32_t)] = _value;

                log_debug("calling encode_func (%f -> %d): %d %s",
                    _signal_value, _value,
                    ((uint32_t*)mi->message->buffer)[mi->signal->buffer_offset /
                                                     sizeof(uint32_t)],
                    mi->signal->name);
            }
        } else if (strcmp(mi->signal->member_type, "uint64_t") == 0) {
            uint64_t _value = 0;
            _value = mi->signal->encode_func_int64(_signal_value);
            if (mi->signal->range_func_int64(_value)) {
                ((int64_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                sizeof(int64_t)] = _value;

                log_debug("calling encode_func (%f -> %d): %d %s",
                    _signal_value, _value,
                    ((uint64_t*)mi->message->buffer)[mi->signal->buffer_offset /
                                                     sizeof(uint64_t)],
                    mi->signal->name);
            }
        } else if (strcmp(mi->signal->member_type, "int8_t") == 0) {
            int8_t _value = 0;
            _value = mi->signal->encode_func_int8(_signal_value);
            if (mi->signal->range_func_int8(_value)) {
                ((int8_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                               sizeof(int8_t)] = _value;
                log_debug("calling encode_func (%f -> %d): %d %s",
                    _signal_value, _value,
                    ((int8_t*)mi->message->buffer)[mi->signal->buffer_offset /
                                                   sizeof(int8_t)],
                    mi->signal->name);
            }
        } else if (strcmp(mi->signal->member_type, "int16_t") == 0) {
            int16_t _value = 0;
            _value = mi->signal->encode_func_int16(_signal_value);
            if (mi->signal->range_func_int16(_value)) {
                ((int16_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                sizeof(int16_t)] = _value;
                log_debug("calling encode_func (%f -> %d): %d %s",
                    _signal_value, _value,
                    ((int16_t*)mi->message->buffer)[mi->signal->buffer_offset /
                                                    sizeof(int16_t)],
                    mi->signal->name);
            }
        } else if (strcmp(mi->signal->member_type, "int32_t") == 0) {
            int32_t _value = 0;
            _value = mi->signal->encode_func_int32(_signal_value);
            if (mi->signal->range_func_int32(_value)) {
                ((int32_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                sizeof(int32_t)] = _value;
                log_debug("calling encode_func (%f -> %d): %d %s",
                    _signal_value, _value,
                    ((int32_t*)mi->message->buffer)[mi->signal->buffer_offset /
                                                    sizeof(int32_t)],
                    mi->signal->name);
            }
        } else if (strcmp(mi->signal->member_type, "int64_t") == 0) {
            int16_t _value = 0;
            _value = mi->signal->encode_func_int64(_signal_value);
            if (mi->signal->range_func_int64(_value)) {
                ((int64_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                sizeof(int64_t)] = _value;
                log_debug("calling encode_func (%f -> %d): %d %s",
                    _signal_value, _value,
                    ((int64_t*)mi->message->buffer)[mi->signal->buffer_offset /
                                                    sizeof(int64_t)],
                    mi->signal->name);
            }
        } else if (strcmp(mi->signal->member_type, "float") == 0) {
            float _value = 0;
            _value = mi->signal->encode_func_float(_signal_value);
            if (mi->signal->range_func_float(_value)) {
                ((float*)mi->message
                        ->buffer)[(mi->signal->buffer_offset) / sizeof(float)] =
                    _value;
                log_debug("calling encode_func (%f -> %d): %d %s",
                    _signal_value, _value,
                    ((float*)mi->message->buffer)[mi->signal->buffer_offset /
                                                  sizeof(float)],
                    mi->signal->name);
            }
        } else if (strcmp(mi->signal->member_type, "double") == 0) {
            double _value = 0;
            _value = mi->signal->encode_func_double(_signal_value);
            if (mi->signal->range_func_double(_value)) {
                ((double*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                               sizeof(double)] = _value;
                log_debug("calling encode_func (%f -> %d): %d %s",
                    _signal_value, _value,
                    ((double*)mi->message->buffer)[mi->signal->buffer_offset /
                                                   sizeof(double)],
                    mi->signal->name);
            }
        } else {
            log_error("Unknown type: %s (frame_id=%d, message=%s, signal=%s)",
                mi->signal->member_type, mi->message->frame_id,
                mi->message->name, mi->signal->name);
        }
    }
    return 0;
}


int network_marshal_messages_to_signals(
    Network* n, MarshalItem* marshal_list, bool single)
{
    if (n == NULL || marshal_list == NULL) return 1;
    for (MarshalItem* mi = marshal_list; mi && mi->signal; mi++) {
        log_debug(
            "MI Signal: frame_id=%d, update_signals=%d, index=%d, type=%s",
            mi->message->frame_id, mi->message->update_signals,
            mi->signal_vector_index, mi->signal->member_type);
        if (mi->message->update_signals == false && single == false) {
            /* Next item (will be forced if single == true). */
            continue;
        }
        if (strcmp(mi->signal->member_type, "uint8_t") == 0) {
            if (mi->signal->range_func_int8(
                    ((int8_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                   sizeof(int8_t)])) {
                double _v = mi->signal->decode_func_int8(
                    ((int8_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                   sizeof(int8_t)]);
                n->signal_vector[mi->signal_vector_index] = _v;

                log_debug("calling decode_func (%d -> %f): %f %s",
                    ((uint8_t*)
                            mi->message->buffer)[(mi->signal->buffer_offset) /
                                                 sizeof(uint8_t)],
                    _v, n->signal_vector[mi->signal_vector_index],
                    mi->signal->name);
            }
        } else if (strcmp(mi->signal->member_type, "uint16_t") == 0) {
            if (mi->signal->range_func_int16((
                    (int16_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                   sizeof(int16_t)])) {
                double _v = mi->signal->decode_func_int16((
                    (int16_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                   sizeof(int16_t)]);
                n->signal_vector[mi->signal_vector_index] = _v;

                log_debug("calling decode_func (%d -> %f): %f %s",
                    ((uint16_t*)
                            mi->message->buffer)[(mi->signal->buffer_offset) /
                                                 sizeof(uint16_t)],
                    _v, n->signal_vector[mi->signal_vector_index],
                    mi->signal->name);
            }
        } else if (strcmp(mi->signal->member_type, "uint32_t") == 0) {
            if (mi->signal->range_func_int32((
                    (int32_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                   sizeof(int32_t)])) {
                double _v = mi->signal->decode_func_int32((
                    (int32_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                   sizeof(int32_t)]);
                n->signal_vector[mi->signal_vector_index] = _v;

                log_debug("calling decode_func (%d -> %f): %f %s",
                    ((uint32_t*)
                            mi->message->buffer)[(mi->signal->buffer_offset) /
                                                 sizeof(uint32_t)],
                    _v, n->signal_vector[mi->signal_vector_index],
                    mi->signal->name);
            }
        } else if (strcmp(mi->signal->member_type, "uint64_t") == 0) {
            if (mi->signal->range_func_int64((
                    (int64_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                   sizeof(int64_t)])) {
                double _v = mi->signal->decode_func_int64((
                    (int64_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                   sizeof(int64_t)]);
                n->signal_vector[mi->signal_vector_index] = _v;

                log_debug("calling decode_func (%d -> %f): %f %s",
                    ((uint64_t*)
                            mi->message->buffer)[(mi->signal->buffer_offset) /
                                                 sizeof(uint64_t)],
                    _v, n->signal_vector[mi->signal_vector_index],
                    mi->signal->name);
            }
        } else if (strcmp(mi->signal->member_type, "int8_t") == 0) {
            if (mi->signal->range_func_int8(
                    ((int8_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                   sizeof(int8_t)])) {
                double _v = mi->signal->decode_func_int8(
                    ((int8_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                   sizeof(int8_t)]);
                n->signal_vector[mi->signal_vector_index] = _v;

                log_debug("calling decode_func (%d -> %f): %f %s",
                    ((int8_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                   sizeof(int8_t)],
                    _v, n->signal_vector[mi->signal_vector_index],
                    mi->signal->name);
            }
        } else if (strcmp(mi->signal->member_type, "int16_t") == 0) {
            if (mi->signal->range_func_int16((
                    (int16_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                   sizeof(int16_t)])) {
                double _v = mi->signal->decode_func_int16((
                    (int16_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                   sizeof(int16_t)]);
                n->signal_vector[mi->signal_vector_index] = _v;

                log_debug("calling decode_func (%d -> %f): %f %s",
                    ((int16_t*)
                            mi->message->buffer)[(mi->signal->buffer_offset) /
                                                 sizeof(int16_t)],
                    _v, n->signal_vector[mi->signal_vector_index],
                    mi->signal->name);
            }
        } else if (strcmp(mi->signal->member_type, "int32_t") == 0) {
            if (mi->signal->range_func_int32((
                    (int32_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                   sizeof(int32_t)])) {
                double _v = mi->signal->decode_func_int32((
                    (int32_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                   sizeof(int32_t)]);
                n->signal_vector[mi->signal_vector_index] = _v;

                log_debug("calling decode_func (%d -> %f): %f %s",
                    ((int32_t*)
                            mi->message->buffer)[(mi->signal->buffer_offset) /
                                                 sizeof(int32_t)],
                    _v, n->signal_vector[mi->signal_vector_index],
                    mi->signal->name);
            }
        } else if (strcmp(mi->signal->member_type, "int64_t") == 0) {
            if (mi->signal->range_func_int64((
                    (int64_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                   sizeof(int64_t)])) {
                double _v = mi->signal->decode_func_int64((
                    (int64_t*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                   sizeof(int64_t)]);
                n->signal_vector[mi->signal_vector_index] = _v;

                log_debug("calling decode_func (%d -> %f): %f %s",
                    ((int64_t*)
                            mi->message->buffer)[(mi->signal->buffer_offset) /
                                                 sizeof(int64_t)],
                    _v, n->signal_vector[mi->signal_vector_index],
                    mi->signal->name);
            }
        } else if (strcmp(mi->signal->member_type, "float") == 0) {
            if (mi->signal->range_func_float(
                    ((float*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                  sizeof(float)])) {
                double _v = mi->signal->decode_func_float((
                    (float*)mi->message
                        ->buffer)[(mi->signal->buffer_offset) / sizeof(float)]);
                n->signal_vector[mi->signal_vector_index] = _v;

                log_debug("calling decode_func (%d -> %f): %f %s",
                    ((float*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                  sizeof(float)],
                    _v, n->signal_vector[mi->signal_vector_index],
                    mi->signal->name);
            }
        } else if (strcmp(mi->signal->member_type, "double") == 0) {
            if (mi->signal->range_func_double(
                    ((double*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                   sizeof(double)])) {
                double _v = mi->signal->decode_func_double(
                    ((double*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                   sizeof(double)]);
                n->signal_vector[mi->signal_vector_index] = _v;

                log_debug("calling decode_func (%d -> %f): %f %s",
                    ((double*)mi->message->buffer)[(mi->signal->buffer_offset) /
                                                   sizeof(double)],
                    _v, n->signal_vector[mi->signal_vector_index],
                    mi->signal->name);
            }
        } else {
            log_error("Unknown type: %s (frame_id=%d, message=%s, signal=%s)",
                mi->signal->member_type, mi->message->frame_id,
                mi->message->name, mi->signal->name);
        }

        /* When single is true, process the single MI _ONLY_. */
        if (single) return 0;
    }

    /* Reset the message processing flags. */
    for (NetworkMessage* nm = n->messages; nm && nm->name; nm++) {
        nm->update_signals = false;
    }

    return 0;
}


void network_pack_messages(Network* n)
{
    assert(n);

    /* Loop over messages and call pack_func. */
    for (NetworkMessage* nm = n->messages; nm && nm->name; nm++) {
        if (nm->pack_func) {
            nm->pack_func(nm->payload, nm->buffer, nm->payload_len);

            /* Calculate checksum for the current buffer. */
            uint32_t payload_checksum =
                simbus_generate_uid_hash((uint8_t*)nm->buffer, nm->buffer_len);

            /* Check if the payload_checksum is different. */
            if (payload_checksum != nm->buffer_checksum) {
                if (nm->cycle_time_ms) {
                } else {
                    nm->needs_tx = true;
                    log_debug("encode path checksum %u", payload_checksum);
                    nm->buffer_checksum = payload_checksum;
                }
            } else {
                nm->needs_tx = false;
            }
        }
    }
}


void network_unpack_messages(Network* n)
{
    assert(n);

    /* Loop over messages and call unpack_func. */
    for (NetworkMessage* nm = n->messages; nm && nm->name; nm++) {
        if (nm->unpack_func) {
            nm->unpack_func(nm->buffer, nm->payload, nm->payload_len);
        }
    }
}


int network_unload_marshal_lists(Network* n)
{
    if (n) {
        if (n->marshal_list) free(n->marshal_list);
    }

    return 0;
}
