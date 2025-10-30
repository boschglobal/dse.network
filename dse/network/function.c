// Copyright 2024 Robert Bosch GmbH
//
// SPDX-License-Identifier: Apache-2.0

#include <stddef.h>
#include <assert.h>
#include <dse/testing.h>
#include <dse/logger.h>
#include <dse/network/network.h>


const char* network_function_annotation(NetworkFunction* nf, const char* name)
{
    if (nf->annotations == NULL) return NULL;
    return dse_yaml_get_scalar(nf->annotations, name);
}


int network_function_apply_encode(Network* n)
{
    assert(n);

    bool net_off = false;
    if (n->netoff_value && *(n->netoff_value) != 0.0) {
        net_off = true;
    }
    for (NetworkMessage* nm = n->messages; nm && nm->name; nm++) {
        if (net_off) nm->needs_tx = false;  // Force to false if network is off.
        if (nm->needs_tx == false) continue;

        uint32_t payload_checksum =
            simbus_generate_uid_hash(nm->payload, nm->payload_len);

        for (NetworkFunction* nf = nm->encode_functions; nf && nf->name; nf++) {
            if (nf->function) {
                int rc = nf->function(nf, nm->payload, nm->payload_len);
                if (rc)
                    log_fatal("error from message function (rc=%d): %s:%s", rc,
                        nm->name, nf->name);
            }
        }

        if (simbus_generate_uid_hash(nm->payload, nm->payload_len) !=
            payload_checksum) {
            /* Trigger update of signals based on changed payload. */
            nm->update_signals = true;
            nm->unpack_func(nm->buffer, nm->payload, nm->payload_len);
            /* Set the buffer checksum to prevent subsequent Tx. */
            nm->buffer_checksum =
                simbus_generate_uid_hash(nm->buffer, nm->buffer_len);
        }
    }

    return 0;
}


int network_function_apply_decode(Network* n)
{
    assert(n);

    for (NetworkMessage* nm = n->messages; nm && nm->name; nm++) {
        if (nm->update_signals == false) {
            /* No incoming message, don't call functions. */
            continue;
        }
        for (NetworkFunction* nf = nm->decode_functions; nf && nf->name; nf++) {
            if (nf->function) {
                int rc = nf->function(nf, nm->payload, nm->payload_len);
                switch (rc) {
                case 0:
                    break;
                case EBADMSG:
                    nm->update_signals = false;
                    break;
                default:
                    log_fatal("error from message function (rc=%d): %s:%s", rc,
                        nm->name, nf->name);
                }
            }
        }
    }

    return 0;
}
