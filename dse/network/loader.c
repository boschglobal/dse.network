// Copyright 2024 Robert Bosch GmbH
//
// SPDX-License-Identifier: Apache-2.0

#include <stdarg.h>
#include <setjmp.h>
#include <dlfcn.h>
#include <dse/testing.h>
#include <dse/logger.h>
#include <dse/network/network.h>
#include <dse/modelc/schema.h>


#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))


void* network_load_message_lib(Network* network, const char* dll_path)
{
    char* dlerror_str;
    dlerror();
    network->message_lib_handle = dlopen(dll_path, RTLD_NOW | RTLD_LOCAL);
    dlerror_str = dlerror();
    if (dlerror_str) log_fatal(dlerror_str);

    return network->message_lib_handle;
}

int network_load_message_funcs(Network* network, NetworkMessage* nm_p)
{
    void* handle = network->message_lib_handle;
    char  func_name[1024];
    /* Loop over messages. */
    for (int m = 0; nm_p[m].name != NULL; m++) {
        // Pack
        snprintf(
            func_name, sizeof(func_name), "%s_%s_pack", network->name, nm_p[m].name);
        nm_p[m].pack_func = (PackFunc)dlsym(handle, func_name);
        // Unpack
        snprintf(func_name, sizeof(func_name), "%s_%s_unpack", network->name,
            nm_p[m].name);
        nm_p[m].unpack_func = (UnpackFunc)dlsym(handle, func_name);
    }

    return 0;
}

typedef struct {
    const char* name;
    void*       func;
} NetFunc_t;

static void __load_network_funcs(
    Network* network, NetworkMessage* net_msg, NetworkSignal* net_sig)
{
    NetFunc_t net_func[] = {
        { .name = "encode" },
        { .name = "decode" },
        { .name = "is_in_range" },
    };
    void* handle = network->message_lib_handle;

    for (uint32_t i = 0; i < ARRAY_SIZE(net_func); i++) {
        char func_name[1024];
        snprintf(func_name, sizeof(func_name), "%s_%s_%s_%s", network->name,
            net_msg->name, net_sig->name, net_func[i].name);
        net_func[i].func = dlsym(handle, func_name);
        if (net_func[i].func == NULL)
            log_error("Network function not loaded (%s)", func_name);
    }
    if ((net_func[0].func == NULL) || (net_func[1].func == NULL) ||
        (net_func[2].func == NULL)) {
        log_fatal(
            "Missing network functions or bad network signal configuration!");
    }

    if ((strcmp(net_sig->member_type, "int8_t") == 0) ||
        (strcmp(net_sig->member_type, "uint8_t") == 0)) {
        net_sig->encode_func_int8 = net_func[0].func;
        net_sig->decode_func_int8 = net_func[1].func;
        net_sig->range_func_int8 = net_func[2].func;
    }
    if ((strcmp(net_sig->member_type, "int16_t") == 0) ||
        (strcmp(net_sig->member_type, "uint16_t") == 0)) {
        net_sig->encode_func_int16 = net_func[0].func;
        net_sig->decode_func_int16 = net_func[1].func;
        net_sig->range_func_int16 = net_func[2].func;
    } else if ((strcmp(net_sig->member_type, "int32_t") == 0) ||
               (strcmp(net_sig->member_type, "uint32_t") == 0)) {
        net_sig->encode_func_int32 = net_func[0].func;
        net_sig->decode_func_int32 = net_func[1].func;
        net_sig->range_func_int32 = net_func[2].func;
    } else if ((strcmp(net_sig->member_type, "int64_t") == 0) ||
               (strcmp(net_sig->member_type, "uint64_t") == 0)) {
        net_sig->encode_func_int64 = net_func[0].func;
        net_sig->decode_func_int64 = net_func[1].func;
        net_sig->range_func_int64 = net_func[2].func;
    } else if (strcmp(net_sig->member_type, "float") == 0) {
        net_sig->encode_func_float = net_func[0].func;
        net_sig->decode_func_float = net_func[1].func;
        net_sig->range_func_float = net_func[2].func;
    } else if (strcmp(net_sig->member_type, "double") == 0) {
        net_sig->encode_func_double = net_func[0].func;
        net_sig->decode_func_double = net_func[1].func;
        net_sig->range_func_double = net_func[2].func;
    }
}


int network_load_signal_funcs(
    Network* network, NetworkMessage* nm_p, NetworkSignal* ns_p)
{
    /* Loop over messages. */
    for (int m = 0; nm_p[m].name != NULL; m++) {
        ns_p = nm_p[m].signals;
        /* Loop over signals within each message. */
        for (int i = 0; ns_p[i].name != NULL; i++) {
            __load_network_funcs(network, &nm_p[m], &ns_p[i]);
        }
    }

    return 0;
}


void* network_load_function_lib(Network* network, const char* dll_path)
{
    char* dlerror_str;
    dlerror();
    network->function_lib_handle = dlopen(dll_path, RTLD_NOW | RTLD_LOCAL);
    dlerror_str = dlerror();
    if (dlerror_str) log_fatal(dlerror_str);

    return network->function_lib_handle;
}


int network_load_function_funcs(Network* network, NetworkMessage* nm_p)
{
    if (network->function_lib_handle == NULL) return 1;

    while (nm_p && nm_p->name) {
        /* Encode functions. */
        NetworkFunction* ef = nm_p->encode_functions;
        while (ef && ef->name) {
            ef->function = dlsym(network->function_lib_handle, ef->name);
            if (ef->function == NULL) {
                log_fatal("Could not load encode function %s for message %s",
                    ef->name, nm_p->name);
            }
            /* Next function; */
            ef++;
        }
        /* Decode functions. */
        NetworkFunction* df = nm_p->decode_functions;
        while (df && df->name) {
            df->function = dlsym(network->function_lib_handle, df->name);
            if (df->function == NULL) {
                log_fatal("Could not load decode function %s for message %s",
                    df->name, nm_p->name);
            }
            /* Next function; */
            df++;
        }

        /* Next message. */
        nm_p++;
    }

    return 0;
}
