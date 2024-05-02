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


static NetworkMessage* _find_mux_message(
    Network* n, uint32_t mux_id, uint32_t frame_id)
{
    if (mux_id == 0) return NULL;
    for (NetworkMessage* nm = n->messages; nm && nm->name; nm++) {
        if (nm->frame_id == frame_id) {
            if (nm->mux_id == mux_id) {
                return nm;
                break;
            }
        }
    }
    return NULL;
}

static void _process_message(
    Network* n, NetworkMessage* nm, NCodecCanMessage* msg)
{
    nm->unpack_func(nm->buffer, msg->buffer, msg->len);
    if (nm->mux_signal && nm->mux_signal->mux_mi) {
        /* This is a container message, also process the contained message. */
        MarshalItem* mi = nm->mux_signal->mux_mi;
        network_marshal_messages_to_signals(n, mi, true);
        uint32_t        mux_id = n->signal_vector[mi->signal_vector_index];
        /* Locate the mux message. */
        NetworkMessage* mux_message =
            _find_mux_message(n, mux_id, msg->frame_id);
        if (mux_message) {
            _process_message(n, mux_message, msg);
        }
    }

    /* Calculate the checksum for the payload. */
    uint32_t payload_checksum =
        simbus_generate_uid_hash(nm->buffer, nm->buffer_len);
    nm->update_signals = false;
    if (payload_checksum == nm->buffer_checksum) {
        log_debug("filtered on checksum %d", payload_checksum);
        return;
    }
    nm->buffer_checksum = payload_checksum;
    nm->update_signals = true;
    log_debug("decode path checksum %d", payload_checksum);
}

static void _process_can_frame(Network* n, NCodecCanMessage* msg)
{
    /* Find the matching message. */
    NetworkMessage* message = NULL;
    NetworkMessage* nm = n->messages;
    while (nm->frame_id) {
        if (nm->frame_id == msg->frame_id) {
            message = nm;
            break;
        }
        /* Next message. */
        nm++;
    }
    if (message) {
        _process_message(n, message, msg);
    } else {
        log_debug("Network does not have frame_id : %d", msg->frame_id);
    }
}

void network_decode_from_bus(Network* n, void* nc)
{
    assert(n);
    assert(nc);

    while (1) {
        NCodecCanMessage msg = {};
        if (ncodec_read(nc, &msg) < 0) break;
        _process_can_frame(n, &msg);
    }
    ncodec_truncate(nc);
}


void network_encode_to_bus(Network* n, void* nc)
{
    assert(n);
    assert(nc);

    for (NetworkMessage* nm = n->messages; nm->name; nm++) {
        /* Check if this message should be TXed? */
        if (nm->needs_tx == false) continue;
        /* Message TX and and clear the needs_tx flag. */
        int rc = ncodec_write(nc, &(struct NCodecCanMessage){
                                      .frame_id = nm->frame_id,
                                      .frame_type = nm->frame_type,
                                      .buffer = (uint8_t*)nm->payload,
                                      .len = nm->payload_len,
                                  });
        if (rc < 0) log_error("Unable to write CAN Frame to ncodec object!");
        nm->needs_tx = false;
    }
    ncodec_flush(nc);
}
