// Copyright 2024 Robert Bosch GmbH
//
// SPDX-License-Identifier: Apache-2.0

#include <stdlib.h>
#include <errno.h>
#include <dse/testing.h>
#include <dse/network/network.h>
#include "function.h"


/**
crc_generate
============

Calculate a CRC based on the message packet. The CRC is written into the
specified position in the message packet.

The CRC algorithm is a simple summation of all bytes in the message packet.

> Note: in the encode path (TX), changes to the counter are not reflected in
the corresponding signal.

Parameters
----------
data (void**)
: Pointer reference for instance data.

payload (uint8_t*)
: The payload that this function will modify.

payload_len (size_t)
: The length of the payload.

Returns
-------
0
: CRC generated.

ENOMEM
: Instance data could not be established.

EPROTO
: A required annotation was not located.

Annotations
-----------
position
: The position of the CRC in the message packet.
 */
int crc_generate(NetworkFunction* function, uint8_t* payload, size_t payload_len)
{
    if (payload == NULL || function == NULL) return EINVAL;
    InstanceData* inst = function->data;
    /* Function instance configuration. */
    if (inst == NULL) {
        inst = alloc_inst_data(&function->data);
        if (inst == NULL) return ENOMEM;
        /* Mandatory annotations. */
        const char* value = network_function_annotation(function, "position");
        if (value == NULL) return EPROTO;
        inst->position = strtoul(value, NULL, 10);
    }
    /* CRC calculation. */
    uint8_t* buffer = payload;
    uint8_t  crc = 0;
    for (uint8_t i = 0; i < payload_len; i++) {
        if (i == inst->position) continue;
        crc += buffer[i];
    }
    buffer[inst->position] = crc;

    return 0;
}


/**
crc_validate
============

Validate the CRC of a message packet. The CRC is included in the message packet.

> Note: in the decode path (RX), bad messages (function returns EBADMSG) will
not change corresponding signals.

Parameters
----------
data (void**)
: Pointer reference for instance data.

payload (uint8_t*)
: The payload that this function will modify.

payload_len (size_t)
: The length of the payload.

Returns
-------
0
: The CRC passed validation.

EBADMSG
: The CRC failed validation. The message will not be decoded.

ENOMEM
: Instance data could not be established.

EPROTO
: A required annotation was not located.

Annotations
-----------
position
: The position of the CRC in the message packet.
 */
int crc_validate(NetworkFunction* function, uint8_t* payload, size_t payload_len)
{
    if (payload == NULL || function == NULL) return EINVAL;
    InstanceData* inst = function->data;
    /* Function instance configuration. */
    if (inst == NULL) {
        inst = alloc_inst_data(&function->data);
        if (inst == NULL) return ENOMEM;
        /* Mandatory annotations. */
        const char* value = network_function_annotation(function, "position");
        if (value == NULL) return EPROTO;
        inst->position = strtoul(value, NULL, 10);
    }
    /* CRC validation. */
    uint8_t* buffer = payload;
    uint8_t  crc = buffer[inst->position];
    for (uint8_t i = payload_len; i > 0; --i) {
        if (i == inst->position) continue;
        crc -= buffer[i];
    }
    if (crc != 0) return EBADMSG;

    return 0;
}
