// Copyright 2024 Robert Bosch GmbH
//
// SPDX-License-Identifier: Apache-2.0

#include <stddef.h>
#include <assert.h>
#include <dse/testing.h>
#include <dse/logger.h>
#include <dse/network/network.h>


const char* network_function_annotation(
    NetworkFunction* function, const char* name)
{
    if (function->annotations == NULL) return NULL;
    return dse_yaml_get_scalar(function->annotations, name);
}


int network_function_apply_encode(Network* network)
{
    assert(network);

    for (NetworkMessage* nm_p = network->messages; nm_p->name; nm_p++) {
        if (nm_p->needs_tx == false) continue;

        uint32_t payload_checksum =
        simbus_generate_uid_hash(nm_p->payload, nm_p->payload_len);

        for (NetworkFunction* nt_f = nm_p->encode_functions; nt_f->name;
             nt_f++) {
            if (nt_f->function) {
                int rc = nt_f->function(nt_f, nm_p->payload, nm_p->payload_len);
                if (rc)
                    log_fatal("error from message function (rc=%d): %s:%s", rc,
                        nm_p->name, nt_f->name);
            }
        }

        if (simbus_generate_uid_hash(nm_p->payload, nm_p->payload_len) != payload_checksum) {
            /* Trigger update of signals based on changed payload. */
            nm_p->update_signals = true;
			nm_p->unpack_func(nm_p->buffer, nm_p->payload, nm_p->payload_len);
        }
    }

    return 0;
}


int network_function_apply_decode(Network* network)
{
    assert(network);

    for (NetworkMessage* nm_p = network->messages; nm_p->name; nm_p++) {
        if (nm_p->update_signals == false) {
            /* No incoming message, don't call functions. */
            continue;
        }
        for (NetworkFunction* nt_f = nm_p->decode_functions; nt_f->name;
             nt_f++) {
            if (nt_f->function) {
                int rc = nt_f->function(nt_f, nm_p->payload, nm_p->payload_len);
                switch (rc) {
                case 0:
                    break;
                case EBADMSG:
                    nm_p->update_signals = false;
                    break;
                default:
                    log_fatal("error from message function (rc=%d): %s:%s", rc,
                        nm_p->name, nt_f->name);
                }
            }
        }
    }

    return 0;
}
