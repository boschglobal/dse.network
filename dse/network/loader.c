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


void* network_load_message_lib(Network* n, const char* dll_path)
{
    char* dlerror_str;

    dlerror();
    n->message_lib_handle = dlopen(dll_path, RTLD_NOW | RTLD_LOCAL);
    dlerror_str = dlerror();
    if (dlerror_str) log_fatal(dlerror_str);

    return n->message_lib_handle;
}

int network_load_message_funcs(Network* n)
{
    void* handle = n->message_lib_handle;
    char  func_name[1024];

    /* Loop over messages. */
    for (NetworkMessage* nm = n->messages; nm && nm->name; nm++) {
        // If container is specified, then load functions from the
        // container message (they are common with this message).

        // Pack
        snprintf(func_name, sizeof(func_name), "%s_%s_pack", n->name,
            nm->container ? nm->container : nm->name);
        nm->pack_func = (PackFunc)dlsym(handle, func_name);
        if (nm->pack_func == NULL)
            log_error("Network function not loaded (%s)", func_name);
        // Unpack
        snprintf(func_name, sizeof(func_name), "%s_%s_unpack", n->name,
            nm->container ? nm->container : nm->name);
        nm->unpack_func = (UnpackFunc)dlsym(handle, func_name);
        if (nm->unpack_func == NULL)
            log_error("Network function not loaded (%s)", func_name);
    }

    return 0;
}

typedef struct {
    const char* name;
    void*       func;
} NetFunc_t;

static void __load_network_funcs(
    Network* n, NetworkMessage* nm, NetworkSignal* ns)
{
    NetFunc_t net_func[] = {
        { .name = "encode" },
        { .name = "decode" },
        { .name = "is_in_range" },
    };
    void* handle = n->message_lib_handle;

    for (uint32_t i = 0; i < ARRAY_SIZE(net_func); i++) {
        char func_name[1024];
        // If container is specified, then load functions from the
        // container message (they are common with this message).
        snprintf(func_name, sizeof(func_name), "%s_%s_%s_%s", n->name,
            nm->container ? nm->container : nm->name, ns->name,
            net_func[i].name);
        net_func[i].func = dlsym(handle, func_name);
        if (net_func[i].func == NULL)
            log_error("Network function not loaded (%s)", func_name);
    }
    if ((net_func[0].func == NULL) || (net_func[1].func == NULL) ||
        (net_func[2].func == NULL)) {
        log_fatal(
            "Missing network functions or bad network signal configuration!");
    }

    if ((strcmp(ns->member_type, "int8_t") == 0) ||
        (strcmp(ns->member_type, "uint8_t") == 0)) {
        ns->encode_func_int8 = net_func[0].func;
        ns->decode_func_int8 = net_func[1].func;
        ns->range_func_int8 = net_func[2].func;
    }
    if ((strcmp(ns->member_type, "int16_t") == 0) ||
        (strcmp(ns->member_type, "uint16_t") == 0)) {
        ns->encode_func_int16 = net_func[0].func;
        ns->decode_func_int16 = net_func[1].func;
        ns->range_func_int16 = net_func[2].func;
    } else if ((strcmp(ns->member_type, "int32_t") == 0) ||
               (strcmp(ns->member_type, "uint32_t") == 0)) {
        ns->encode_func_int32 = net_func[0].func;
        ns->decode_func_int32 = net_func[1].func;
        ns->range_func_int32 = net_func[2].func;
    } else if ((strcmp(ns->member_type, "int64_t") == 0) ||
               (strcmp(ns->member_type, "uint64_t") == 0)) {
        ns->encode_func_int64 = net_func[0].func;
        ns->decode_func_int64 = net_func[1].func;
        ns->range_func_int64 = net_func[2].func;
    } else if (strcmp(ns->member_type, "float") == 0) {
        ns->encode_func_float = net_func[0].func;
        ns->decode_func_float = net_func[1].func;
        ns->range_func_float = net_func[2].func;
    } else if (strcmp(ns->member_type, "double") == 0) {
        ns->encode_func_double = net_func[0].func;
        ns->decode_func_double = net_func[1].func;
        ns->range_func_double = net_func[2].func;
    }
}


int network_load_signal_funcs(Network* n)
{
    for (NetworkMessage* nm = n->messages; nm && nm->name; nm++) {
        for (NetworkSignal* ns = nm->signals; ns && ns->name; ns++) {
            __load_network_funcs(n, nm, ns);
        }
    }
    return 0;
}


void* network_load_function_lib(Network* n, const char* dll_path)
{
    char* dlerror_str;
    dlerror();
    n->function_lib_handle = dlopen(dll_path, RTLD_NOW | RTLD_LOCAL);
    dlerror_str = dlerror();
    if (dlerror_str) log_fatal(dlerror_str);

    return n->function_lib_handle;
}


int network_load_function_funcs(Network* n)
{
    if (n->function_lib_handle == NULL) return 1;

    for (NetworkMessage* nm = n->messages; nm && nm->name; nm++) {
        /* Encode functions. */
        for (NetworkFunction* ef = nm->encode_functions; ef && ef->name; ef++) {
            ef->function = dlsym(n->function_lib_handle, ef->name);
            if (ef->function == NULL) {
                log_fatal("Could not load encode function %s for message %s",
                    ef->name, nm->name);
            }
        }
        /* Decode functions. */
        for (NetworkFunction* df = nm->decode_functions; df && df->name; df++) {
            df->function = dlsym(n->function_lib_handle, df->name);
            if (df->function == NULL) {
                log_fatal("Could not load decode function %s for message %s",
                    df->name, nm->name);
            }
        }
    }

    return 0;
}
