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
#include <dse/ncodec/codec.h>


#define UNUSED(x)     ((void)x)
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))


uint32_t simbus_generate_uid_hash(const uint8_t* key, size_t len)
{
    // FNV-1a hash (http://www.isthe.com/chongo/tech/comp/fnv/)
    uint32_t h = 2166136261UL; /* FNV_OFFSET 32 bit */
    for (size_t i = 0; i < len; ++i) {
        h = h ^ key[i];
        h = h * 16777619UL; /* FNV_PRIME 32 bit */
    }
    return h;
}


static void _process_message(
    Network* network, NetworkMessage* message, NCodecCanMessage* msg)
{
    message->unpack_func(message->buffer, msg->buffer, msg->len);
    if (message->mux_signal && message->mux_signal->mux_mi) {
        /* This is a container message, also process the contained message. */
        MarshalItem* mi = message->mux_signal->mux_mi;
        network_marshal_messages_to_signals(network, mi, true);
        uint32_t mux_id = network->signal_vector[mi->signal_vector_index];
        /* Locate the mux message. */
        NetworkMessage* mux_message = NULL;
        NetworkMessage* nm_p = network->messages;
        while (mux_id && nm_p->frame_id) {
            if (nm_p->frame_id == msg->frame_id) {
                if (nm_p->mux_id == mux_id) {
                    mux_message = nm_p;
                    break;
                }
            }
            /* Next message. */
            nm_p++;
        }
        if (mux_message) {
            _process_message(network, mux_message, msg);
        }
    }

    /* Calculate the checksum for the payload. */
    uint32_t payload_checksum =
        simbus_generate_uid_hash(message->buffer, message->buffer_len);
    message->update_signals = false;
    if (payload_checksum == message->buffer_checksum) {
        log_debug("filtered on checksum %d", payload_checksum);
        return;
    }
    message->buffer_checksum = payload_checksum;
    message->update_signals = true;
    log_debug("decode path checksum %d", payload_checksum);
}

static void _process_can_frame(Network* network, NCodecCanMessage* msg)
{
    /* Find the matching message. */
    NetworkMessage* message = NULL;
    NetworkMessage* nm_p = network->messages;
    while (nm_p->frame_id) {
        if (nm_p->frame_id == msg->frame_id) {
            message = nm_p;
            break;
        }
        /* Next message. */
        nm_p++;
    }
    if (message) {
        _process_message(network, message, msg);
    } else {
        log_debug("Network does not have frame_id : %d", msg->frame_id);
    }
}

void network_decode_from_bus(Network* network, void* nc)
{
    assert(network);
    assert(nc);

    while (1) {
        NCodecCanMessage msg = {};
        if (ncodec_read(nc, &msg) < 0) break;
        _process_can_frame(network, &msg);
    }
    ncodec_truncate(nc);
}


void network_encode_to_bus(Network* network, void* nc)
{
    assert(network);
    assert(nc);

    for (NetworkMessage* nm_p = network->messages; nm_p->name; nm_p++) {
        /* Check if this message should be TXed? */
        if (nm_p->needs_tx == false) continue;
        /* Message TX and and clear the needs_tx flag. */
        int rc = ncodec_write(nc, &(struct NCodecCanMessage){
                                      .frame_id = nm_p->frame_id,
                                      .frame_type = nm_p->frame_type,
                                      .buffer = (uint8_t*)nm_p->payload,
                                      .len = nm_p->payload_len,
                                  });
        if (rc < 0) log_error("Unable to write CAN Frame to ncodec object!");
        nm_p->needs_tx = false;
    }
    ncodec_flush(nc);
}
