// Copyright 2024 Robert Bosch GmbH
//
// SPDX-License-Identifier: Apache-2.0

#ifndef DSE_NETWORK_FUNCTION_H_
#define DSE_NETWORK_FUNCTION_H_

#include <dse/platform.h>
#include <dse/network/network.h>


/**
Example Network Functions
=========================

Example implementation of Network Functions.
*/


typedef struct InstanceData {
    uint8_t position;  // Annotation: position
} InstanceData;


DLL_PRIVATE InstanceData* alloc_inst_data(void** data);

/* counters.c */
DLL_PUBLIC int counter_inc_uint8(
    NetworkFunction* function, uint8_t* payload, size_t payload_len);
DLL_PUBLIC int crc_generate(
    NetworkFunction* function, uint8_t* payload, size_t payload_len);

/* crc.c */
DLL_PUBLIC int crc_validate(
    NetworkFunction* function, uint8_t* payload, size_t payload_len);

#endif  // DSE_NETWORK_FUNCTION_H_
