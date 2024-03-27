// Copyright 2024 Robert Bosch GmbH
//
// SPDX-License-Identifier: Apache-2.0

#include <assert.h>
#include <dlfcn.h>
#include <dse/testing.h>
#include <dse/logger.h>
#include <dse/clib/util/yaml.h>
#include <dse/modelc/model.h>
#include <dse/modelc/schema.h>
#include <dse/network/network.h>


#define UNUSED(x)     ((void)x)
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))


int network_load(Network *network, ModelInstanceSpec *model_instance)
{
    assert(network);
    network_parse(network, model_instance);
    network_load_message_lib(network, network->message_lib_path);
    network_load_function_lib(network, network->function_lib_path);
    network_load_function_funcs(network, network->messages);
    network_load_signal_funcs(network, network->messages, network->messages->signals);
    network_load_message_funcs(network, network->messages);
    network_load_marshal_lists(network, model_instance);
    network_get_signal_names(network->marshal_list, &network->signal_name, &network->signal_count);
    network->signal_vector = calloc(network->signal_count, sizeof(double));
    for (size_t i = 0; i < network->signal_count; i++) {
        log_info("Network Signal [%d] : %s", i, network->signal_name[i]);
    }

    return 0;
}


int network_unload(Network* network)
{
    assert(network);

    network_unload_parser(network);
    network_unload_marshal_lists(network);
    if (network) {
        if (network->signal_name) free(network->signal_name);
        if (network->signal_vector) free(network->signal_vector);
        if (network->schedule_list) free(network->schedule_list);
    }

    return 0;
}
