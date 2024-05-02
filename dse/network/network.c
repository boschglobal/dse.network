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


int network_load(Network* n, ModelInstanceSpec* mi)
{
    assert(n);

    network_parse(n, mi);
    network_load_message_lib(n, n->message_lib_path);
    network_load_function_lib(n, n->function_lib_path);
    network_load_function_funcs(n);
    network_load_signal_funcs(n);
    network_load_message_funcs(n);
    network_load_marshal_lists(n);
    network_get_signal_names(
        n->marshal_list, &n->signal_name, &n->signal_count);
    n->signal_vector = calloc(n->signal_count, sizeof(double));
    for (size_t i = 0; i < n->signal_count; i++) {
        log_info("Network Signal [%d] : %s", i, n->signal_name[i]);
    }

    return 0;
}


int network_unload(Network* n)
{
    assert(n);

    network_unload_parser(n);
    network_unload_marshal_lists(n);
    if (n) {
        if (n->signal_name) free(n->signal_name);
        if (n->signal_vector) free(n->signal_vector);
        if (n->schedule_list) free(n->schedule_list);
    }

    return 0;
}
