// Copyright 2024 Robert Bosch GmbH
//
// SPDX-License-Identifier: Apache-2.0

#include <stdlib.h>
#include <errno.h>
#include <dse/testing.h>
#include <dse/network/network.h>
#include "function.h"


#define UNUSED(x)                ((void)x)


/**
counter_inc_uint8
=================

Increment an 8-bit counter in the message packet.

> Note: in the encode path, changes to the counter are not reflected in
the corresponding signal. Subsequent calls to `network_message_recalculate`
may overwrite the modified counter.

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
: Counter incremented.

ENOMEM
: Instance data could not be established.

EPROTO
: A required annotation was not located.

Annotations
-----------
position
: The position of the counter in the message packet.
 */
int counter_inc_uint8(NetworkFunction* function, uint8_t* payload, size_t payload_len)
{
    UNUSED(payload_len);

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
    /* Counter increment. */
    uint8_t* buffer = payload;
    uint8_t  counter = buffer[inst->position];
    counter++;
    buffer[inst->position] = counter;

    return 0;
}
