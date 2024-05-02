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


#define UNUSED(x)     ((void)x)
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))


typedef struct MessageObject {
    const char* message;
    YamlNode*   node;
    void*       data;
} MessageObject;
typedef struct SignalObject {
    const char*  signal;
    const char** signal_name;
    const char*  message;
    YamlNode*    node;
    void*        data;
} SignalObject;
typedef struct FunctionObject {
    const char* function;
    YamlNode*   node;
    void*       data;
} FunctionObject;


static uint32_t _get_uint32t(YamlNode* n, const char* p, bool err)
{
    uint32_t v = 0;
    int      rc = dse_yaml_get_uint(n, p, &v);
    if (rc != 0 && err) {
        log_error("Missing annotation: %s", p);
    } else {
        log_debug("  %s = %u", p, v);
    }
    return v;
}


static void* _message_object_generator(ModelInstanceSpec* mi, void* data)
{
    UNUSED(mi);
    YamlNode* n = dse_yaml_find_node((YamlNode*)data, "message");
    if (n && n->scalar) {
        MessageObject* o = calloc(1, sizeof(MessageObject));
        o->message = n->scalar;
        o->node = (YamlNode*)data;
        return o;
    }
    return NULL;
}


static void* _signal_object_generator(ModelInstanceSpec* mi, void* data)
{
    UNUSED(mi);
    YamlNode* n = dse_yaml_find_node((YamlNode*)data, "signal");
    if (n && n->scalar) {
        SignalObject* o = calloc(1, sizeof(SignalObject));
        o->signal = n->scalar;
        o->node = (YamlNode*)data;
        return o;
    }
    return NULL;
}


static void* _parse_signals(ModelInstanceSpec* mi, SchemaObject* object)
{
    uint32_t      index = 0;
    SignalObject* sig_obj;
    HashList      s_list;
    hashlist_init(&s_list, 100);

    do {
        sig_obj = schema_object_enumerator(
            mi, object, "signals", &index, _signal_object_generator);
        if (sig_obj == NULL) break;
        if (sig_obj->signal) {
            NetworkSignal* sig = calloc(1, sizeof(NetworkSignal));
            sig->signal_name = strdup(sig_obj->signal);
            /* struct_member_name */
            sig->name = dse_yaml_get_scalar(
                sig_obj->node, "annotations/struct_member_name");
            log_debug("Scan match on name: %s", sig->name);
            /* struct_member_offset */
            sig->buffer_offset = _get_uint32t(
                sig_obj->node, "annotations/struct_member_offset", true);
            /* struct_member_primitive_type */
            sig->member_type = dse_yaml_get_scalar(
                sig_obj->node, "annotations/struct_member_primitive_type");
            if (sig->member_type) {
                log_debug("Scan match on type: %s", sig->member_type);
            } else
                log_error(
                    "Missing struct_member_primitive_type for %s", sig->name);
            /* init_value */
            dse_yaml_get_double(
                sig_obj->node, "annotations/init_value", &sig->init_value);
            log_debug("Scan match on init value: %f", sig->init_value);
            /* Container related (internal / value). */
            sig->internal = (bool)_get_uint32t(
                sig_obj->node, "annotations/internal", false);
            dse_yaml_get_double(
                sig_obj->node, "annotations/value", &sig->value);
            sig->mux_signal = (bool)_get_uint32t(
                sig_obj->node, "annotations/mux_signal", false);

            /* Put the Network Message into a HashList (i.e. ordered) */
            hashlist_append(&s_list, sig);
        }
        free(sig_obj);
    } while (1);

    /* Convert the HashList to a NULL terminated list. */
    size_t         count = hashlist_length(&s_list);
    NetworkSignal* ns = calloc(count + 1, sizeof(NetworkSignal));
    for (uint32_t i = 0; i < count; i++) {
        memcpy(&ns[i], hashlist_at(&s_list, i), sizeof(NetworkSignal));
        free(hashlist_at(&s_list, i));
    }

    /* Cleanup */
    hashlist_destroy(&s_list);

    return ns;
}


static void* _function_object_generator(ModelInstanceSpec* mi, void* data)
{
    UNUSED(mi);
    YamlNode* n = dse_yaml_find_node((YamlNode*)data, "function");
    if (n && n->scalar) {
        FunctionObject* o = calloc(1, sizeof(FunctionObject));
        o->function = n->scalar;
        o->node = (YamlNode*)data;
        return o;
    }
    return NULL;
}


static void* _parse_functions(
    ModelInstanceSpec* mi, SchemaObject* object, const char* path)
{
    uint32_t        index = 0;
    FunctionObject* func_obj;
    HashList        f_list;
    hashlist_init(&f_list, 100);

    do {
        func_obj = schema_object_enumerator(
            mi, object, path, &index, _function_object_generator);

        if (func_obj == NULL) break;
        if (func_obj->function) {
            NetworkFunction* func = calloc(1, sizeof(NetworkFunction));
            func->name = strdup(func_obj->function);
            func->annotations =
                dse_yaml_find_node(func_obj->node, "annotations");
            log_debug("Scan match on name: %s", func->name);

            /* Put the Network Function into a HashList (i.e. ordered) */
            hashlist_append(&f_list, func);
        }
        free(func_obj);
    } while (1);

    /* Convert the HashList to a NULL terminated list. */
    size_t           count = hashlist_length(&f_list);
    NetworkFunction* nt_f = calloc(count + 1, sizeof(NetworkFunction));
    for (uint32_t i = 0; i < count; i++) {
        memcpy(&nt_f[i], hashlist_at(&f_list, i), sizeof(NetworkFunction));
        free(hashlist_at(&f_list, i));
    }

    /* Cleanup */
    hashlist_destroy(&f_list);

    return nt_f;
}


static void* _parse_messages(ModelInstanceSpec* mi, SchemaObject* object)
{
    uint32_t       index = 0;
    MessageObject* msg_obj;
    HashList       m_list;
    HashMap        m_map;
    hashmap_init(&m_map);
    hashlist_init(&m_list, 100);

    do {
        msg_obj = schema_object_enumerator(
            mi, object, "spec/messages", &index, _message_object_generator);
        if (msg_obj == NULL) break;

        if (msg_obj->message) {
            NetworkMessage* msg = calloc(1, sizeof(NetworkMessage));

            /* Message name */
            msg->name = dse_yaml_get_scalar(msg_obj->node, "message");
            log_debug("Scan match on name: %s", msg->name);
            /* Struct Size */
            msg->buffer_len =
                _get_uint32t(msg_obj->node, "annotations/struct_size", true);
            if (msg->buffer_len) {
                msg->buffer = calloc(msg->buffer_len, sizeof(char*));
            }
            /* Frame ID */
            const char* frame_id_str =
                dse_yaml_get_scalar(msg_obj->node, "annotations/frame_id");
            if (frame_id_str) {
                msg->frame_id = strtoul(frame_id_str, NULL, 0);
                log_debug("Scan match on frame_id: %u", msg->frame_id);
            } else {
                log_error("Missing frame_id %s", msg->name);
            }
            /* Length */
            msg->payload_len =
                _get_uint32t(msg_obj->node, "annotations/frame_length", true);
            if (msg->payload_len) {
                msg->payload = (char*)calloc(msg->payload_len, sizeof(char*));
            }
            /* Type */
            msg->frame_type =
                _get_uint32t(msg_obj->node, "annotations/frame_type", true);
            /* Cycle Time */
            msg->cycle_time_ms =
                _get_uint32t(msg_obj->node, "annotations/cycle_time_ms", false);
            /* Container */
            dse_yaml_get_string(
                msg_obj->node, "annotations/container", &msg->container);
            log_debug("Scan match on container: %u", msg->container);
            /* Container Mux Id */
            msg->mux_id = _get_uint32t(
                msg_obj->node, "annotations/container_mux_id", false);

            /* Parse Signals */
            msg->signals =
                _parse_signals(mi, &(SchemaObject){ .doc = msg_obj->node });

            /* Parse Functions */
            msg->encode_functions = _parse_functions(mi,
                &(SchemaObject){ .doc = msg_obj->node }, "functions/encode");
            msg->decode_functions = _parse_functions(mi,
                &(SchemaObject){ .doc = msg_obj->node }, "functions/decode");

            hashlist_append(&m_list, msg);
            hashmap_set(&m_map, frame_id_str, msg);
        }

        free(msg_obj);
    } while (1);

    /* Convert the HashList to a NULL terminated list. */
    size_t          count = hashlist_length(&m_list);
    NetworkMessage* nm_p = calloc(count + 1, sizeof(NetworkMessage));
    for (uint32_t i = 0; i < count; i++) {
        memcpy(&nm_p[i], hashlist_at(&m_list, i), sizeof(NetworkMessage));
        free(hashlist_at(&m_list, i));
    }

    /* Cleanup */
    hashlist_destroy(&m_list);
    hashmap_destroy(&m_map);
    return nm_p;
}


static int _network_match_handler(ModelInstanceSpec* mi, SchemaObject* object)
{
    Network* n = object->data;
    n->doc = object->doc;
    unsigned int value = 0;
    /* Parse metadata. */
    dse_yaml_get_string(
        n->doc, "metadata/annotations/message_lib", &n->message_lib_path);
    dse_yaml_get_string(
        n->doc, "metadata/annotations/function_lib", &n->function_lib_path);
    log_debug("Network Message Lib Path: %s", n->message_lib_path);
    log_debug("Network Function Lib Path: %s", n->function_lib_path);
    if (n->message_lib_path == NULL) return 1;

    /* Node ID */
    dse_yaml_get_uint(n->doc, "metadata/annotations/node_id", &value);
    n->node_id = value;
    log_debug("Scan match on node id: %u", value);
    /* Interface ID */
    value = 0;
    dse_yaml_get_uint(n->doc, "metadata/annotations/interface_id", &value);
    n->interface_id = value;
    log_debug("Scan match on interface id: %u", value);
    /* Bus ID */
    value = 0;
    dse_yaml_get_uint(n->doc, "metadata/annotations/bus_id", &value);
    n->bus_id = value;
    log_debug("Scan match on bus id: %u", value);

    /* Enumerate over the messages of the Network doc. */
    n->messages = _parse_messages(mi, object);

    return 0;
}


int network_parse(Network* n, ModelInstanceSpec* mi)
{
    assert(n);
    assert(n->name);

    /* Parse each type definition, and search for declarations. */
    SchemaObjectSelector selector = {
        .kind = "Network",
        .name = n->name,
        .data = n,
    };
    schema_object_search(mi, &selector, _network_match_handler);

    return 0;
}


static void _free_message(NetworkMessage* message)
{
    if (message) {
        if (message->buffer) free(message->buffer);
        if (message->payload) free(message->payload);
    }
}


static void _free_signals(NetworkSignal* signals)
{
    for (NetworkSignal* ns = signals; ns && ns->signal_name; ns++) {
        free(ns->signal_name);
    }
    if (signals) free(signals);
}


static void _free_functions(NetworkFunction* functions)
{
    for (NetworkFunction* nf = functions; nf && nf->name; nf++) {
        free(nf->name);
        if (nf->data) free(nf->data);
    }
    if (functions) free(functions);
}


int network_unload_parser(Network* n)
{
    if (n == NULL) return 0;

    for (NetworkMessage* nm = n->messages; nm && nm->name; nm++) {
        _free_signals(nm->signals);
        _free_message(nm);
        _free_functions(nm->encode_functions);
        _free_functions(nm->decode_functions);
    }
    if (n->messages) free(n->messages);

    return 0;
}
