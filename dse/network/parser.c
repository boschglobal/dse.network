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


static void* _message_object_generator(
    ModelInstanceSpec* model_instance, void* data)
{
    UNUSED(model_instance);
    YamlNode* n = dse_yaml_find_node((YamlNode*)data, "message");
    if (n && n->scalar) {
        MessageObject* o = calloc(1, sizeof(MessageObject));
        o->message = n->scalar;
        o->node = (YamlNode*)data;
        return o;
    }
    return NULL;
}


static void* _signal_object_generator(
    ModelInstanceSpec* model_instance, void* data)
{
    UNUSED(model_instance);
    YamlNode* n = dse_yaml_find_node((YamlNode*)data, "signal");
    if (n && n->scalar) {
        SignalObject* o = calloc(1, sizeof(SignalObject));
        o->signal = n->scalar;
        o->node = (YamlNode*)data;
        return o;
    }
    return NULL;
}


static void* _parse_signals(
    ModelInstanceSpec* model_instance, SchemaObject* object)
{
    uint32_t      index = 0;
    SignalObject* sig_obj;
    HashList      s_list;
    hashlist_init(&s_list, 100);

    do {
        sig_obj = schema_object_enumerator(model_instance, object, "signals",
            &index, _signal_object_generator);
        if (sig_obj == NULL) break;
        if (sig_obj->signal) {
            unsigned int   value = 0;
            NetworkSignal* sig = calloc(1, sizeof(NetworkSignal));
            sig->signal_name = strdup(sig_obj->signal);
            sig->name = dse_yaml_get_scalar(
                sig_obj->node, "annotations/struct_member_name");
            log_debug("Scan match on name: %s", sig->name);
            int rc = dse_yaml_get_uint(
                sig_obj->node, "annotations/struct_member_offset", &value);
            if (rc == 0) {
                sig->buffer_offset = value;
                log_debug("Scan match on offset: %u", value);
            } else
                log_error("Missing struct_member_offset for %s", sig->name);
            sig->member_type = dse_yaml_get_scalar(
                sig_obj->node, "annotations/struct_member_primitive_type");
            if (sig->member_type) {
                log_debug("Scan match on type: %s", sig->member_type);
            } else
                log_error(
                    "Missing struct_member_primitive_type for %s", sig->name);
            dse_yaml_get_double(
                sig_obj->node, "annotations/init_value", &sig->init_value);
            log_debug("Scan match on init value: %f", sig->init_value);

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


static void* _function_object_generator(
    ModelInstanceSpec* model_instance, void* data)
{
    UNUSED(model_instance);
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
    ModelInstanceSpec* model_instance, SchemaObject* object, const char* path)
{
    uint32_t        index = 0;
    FunctionObject* func_obj;
    HashList        f_list;
    hashlist_init(&f_list, 100);

    do {
        func_obj = schema_object_enumerator(
            model_instance, object, path, &index, _function_object_generator);

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


static void* _parse_messages(
    ModelInstanceSpec* model_instance, SchemaObject* object)
{
    uint32_t       index = 0;
    MessageObject* msg_obj;
    HashList       m_list;
    HashMap        m_map;
    hashmap_init(&m_map);
    hashlist_init(&m_list, 100);

    do {
        msg_obj = schema_object_enumerator(model_instance, object,
            "spec/messages", &index, _message_object_generator);
        if (msg_obj == NULL) break;

        if (msg_obj->message) {
            NetworkMessage* msg = calloc(1, sizeof(NetworkMessage));

            /* Message name */
            msg->name = dse_yaml_get_scalar(msg_obj->node, "message");
            if (msg->name) {
                log_debug("Scan match on name: %s", msg->name);
            }
            /* Struct Size */
            dse_yaml_get_int(msg_obj->node, "annotations/struct_size",
                (int*)&msg->buffer_len);
            msg->buffer = calloc(msg->buffer_len, sizeof(char*));
            if (msg->buffer_len) {
                log_debug("Scan match on struct_size: %d", msg->buffer_len);
            } else
                log_error("Missing struct_size for %s", msg->name);
            /* Frame ID */
            const char* id_str = NULL;
            dse_yaml_get_string(msg_obj->node, "annotations/frame_id", &id_str);
            if (id_str) {
                msg->frame_id = (int)strtol(id_str, NULL, 0);
                log_debug("Scan match on frame_id: %u", msg->frame_id);
            } else
                log_error("Missing frame_id %s", msg->name);
            /* Length */
            const char* length_str = NULL;
            dse_yaml_get_string(
                msg_obj->node, "annotations/frame_length", &length_str);
            if (length_str) {
                msg->payload_len = (uint8_t)strtol(length_str, NULL, 0);
                msg->payload = (char*)calloc(msg->payload_len, sizeof(char*));
                log_debug("Scan match on frame_length: %u", msg->payload_len);
            } else
                log_error("Missing frame_length %s", msg->name);
            unsigned int frame_type_value;
            int          rc = dse_yaml_get_uint(
                         msg_obj->node, "annotations/frame_type", &frame_type_value);
            if (rc == 0) {
                msg->frame_type = (uint8_t)frame_type_value;
                log_debug("Scan match on frame_type: %u", msg->frame_type);
            } else
                log_error("Missing frame_type %s", msg->name);
            /* Cycle Time */
            const char* cycle_time_str = NULL;
            dse_yaml_get_string(
                msg_obj->node, "annotations/cycle_time_ms", &cycle_time_str);
            if (cycle_time_str) {
                msg->cycle_time_ms = (uint8_t)strtol(cycle_time_str, NULL, 0);
                log_debug(
                    "Scan match on cycle_time_ms: %u", msg->cycle_time_ms);
            }
            /* Parse Signals */
            msg->signals = _parse_signals(
                model_instance, &(SchemaObject){ .doc = msg_obj->node });

            /* Parse Functions */
            msg->encode_functions = _parse_functions(model_instance,
                &(SchemaObject){ .doc = msg_obj->node }, "functions/encode");
            msg->decode_functions = _parse_functions(model_instance,
                &(SchemaObject){ .doc = msg_obj->node }, "functions/decode");

            hashlist_append(&m_list, msg);
            hashmap_set(&m_map, id_str, msg);
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


static int _network_match_handler(
    ModelInstanceSpec* model_instance, SchemaObject* object)
{
    Network* network = object->data;
    network->doc = object->doc;
    unsigned int value = 0;
    /* Parse metadata. */
    dse_yaml_get_string(network->doc, "metadata/annotations/message_lib",
        &network->message_lib_path);
    dse_yaml_get_string(network->doc, "metadata/annotations/function_lib",
        &network->function_lib_path);
    log_debug("Network Message Lib Path: %s", network->message_lib_path);
    log_debug("Network Function Lib Path: %s", network->function_lib_path);
    if (network->message_lib_path == NULL) return 1;

    /* Node ID */
    dse_yaml_get_uint(network->doc, "metadata/annotations/node_id", &value);
    network->node_id = value;
    log_debug("Scan match on node id: %u", value);
    /* Interface ID */
    value = 0;
    dse_yaml_get_uint(
        network->doc, "metadata/annotations/interface_id", &value);
    network->interface_id = value;
    log_debug("Scan match on interface id: %u", value);
    /* Bus ID */
    value = 0;
    dse_yaml_get_uint(network->doc, "metadata/annotations/bus_id", &value);
    network->bus_id = value;
    log_debug("Scan match on bus id: %u", value);

    /* Enumerate over the messages of the Network doc. */
    network->messages = _parse_messages(model_instance, object);

    return 0;
}


int network_parse(Network* network, ModelInstanceSpec* model_instance)
{
    assert(network->name);

    /* Parse each type definition, and search for declarations. */
    SchemaObjectSelector selector = {
        .kind = "Network",
        .name = network->name,
        .data = network,
    };
    schema_object_search(model_instance, &selector, _network_match_handler);

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
    NetworkSignal* ns_p = signals;
    while (ns_p && ns_p->signal_name) {
        free(ns_p->signal_name);
        ns_p++;
    }
    if (signals) free(signals);
}


static void _free_functions(NetworkFunction* functions)
{
    NetworkFunction* nt_f = functions;
    while (nt_f && nt_f->name) {
        free(nt_f->name);
        if (nt_f->data) free(nt_f->data);
        nt_f++;
    }
    if (functions) free(functions);
}


int network_unload_parser(Network* network)
{
    if (network) {
        NetworkMessage* nm_p = network->messages;
        while (nm_p && nm_p->name) {
            _free_signals(nm_p->signals);
            _free_message(nm_p);
            _free_functions(nm_p->encode_functions);
            _free_functions(nm_p->decode_functions);
            nm_p++;
        }
        if (network->messages) free(network->messages);
    }

    return 0;
}
