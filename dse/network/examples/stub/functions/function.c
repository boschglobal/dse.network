// Copyright 2024 Robert Bosch GmbH
//
// SPDX-License-Identifier: Apache-2.0

#include <stdlib.h>
#include <errno.h>
#include <dse/testing.h>
#include "function.h"


InstanceData* alloc_inst_data(void** data)
{
    InstanceData* inst = *data;
    if (inst == NULL) {
        /* First call, create instance data. */
        inst = calloc(1, sizeof(InstanceData));
        *data = inst;
    }
    return inst;
}
