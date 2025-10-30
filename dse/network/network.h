// Copyright 2024 Robert Bosch GmbH
//
// SPDX-License-Identifier: Apache-2.0

#ifndef DSE_NETWORK_NETWORK_H_
#define DSE_NETWORK_NETWORK_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <dse/platform.h>
#include <dse/logger.h>
#include <dse/clib/util/yaml.h>
#include <dse/clib/collections/set.h>
#include <dse/modelc/model.h>
#include <dse/modelc/schema.h>


/**
Network Model
=============

The Network Model runs a Communication Stack which represents the connection
between Physical Signals and Network Messages.

*/


/* Forward declarations. */
typedef struct NetworkMessage  NetworkMessage;
typedef struct NetworkFunction NetworkFunction;
typedef struct MarshalItem     MarshalItem;

/*
Message Library
---------------
Definition of interface for Network messages which are loaded from
the Message Library (annotation `message_lib`).
*/
typedef int8_t (*EncodeFuncInt8)(double signal);
typedef int16_t (*EncodeFuncInt16)(double signal);
typedef int32_t (*EncodeFuncInt32)(double signal);
typedef int64_t (*EncodeFuncInt64)(double signal);
typedef float (*EncodeFuncFloat)(double signal);
typedef double (*EncodeFuncDouble)(double signal);

// typedef double (*DecodeFunc)(void value);
typedef double (*DecodeFuncInt8)(int8_t value);
typedef double (*DecodeFuncInt16)(int16_t value);
typedef double (*DecodeFuncInt32)(int32_t value);
typedef double (*DecodeFuncInt64)(int64_t value);
typedef double (*DecodeFuncFloat)(float signal);
typedef double (*DecodeFuncDouble)(double signal);

// typedef bool (*RangeFunc)(void value);
typedef bool (*RangeFuncInt8)(int8_t value);
typedef bool (*RangeFuncInt16)(int16_t value);
typedef bool (*RangeFuncInt32)(int32_t value);
typedef bool (*RangeFuncInt64)(int64_t value);
typedef bool (*RangeFuncFloat)(float value);
typedef bool (*RangeFuncDouble)(double value);

typedef int (*PackFunc)(uint8_t*, const void*, size_t);
typedef int (*UnpackFunc)(void*, const uint8_t*, size_t);


/*
Function Library
----------------
Definition of interface for Functions (applied to the encode/decode message
flow) which are loaded from the Function Library (annotation `function_lib`).
*/
void        network_message_recalculate(NetworkMessage* message);
const char* network_function_annotation(
    NetworkFunction* function, const char* name);

typedef int (*NetworkFunctionFunc)(
    NetworkFunction* function, uint8_t* payload, size_t payload_len);

typedef struct NetworkFunction {
    char*     name;
    YamlNode* annotations;
    void*     data;

    /* Function pointers (loaded from library). */
    NetworkFunctionFunc function;
} NetworkFunction;


typedef struct NetworkSignal {
    const char*      name;
    char*            signal_name;
    /* Annotations. */
    const char*      member_type;
    unsigned int     buffer_offset;
    double           init_value;  // Initial value (at T=0).
    bool             internal;
    double           value;  // Constant value (at T=[0..t])
    /* Container message: Mux signal. */
    bool             mux_signal;
    MarshalItem*     mux_mi;  // Only set if _this_ is the mux signal.
    /* Function pointers (loaded from library). */
    EncodeFuncInt8   encode_func_int8;
    EncodeFuncInt16  encode_func_int16;
    EncodeFuncInt32  encode_func_int32;
    EncodeFuncInt64  encode_func_int64;
    EncodeFuncFloat  encode_func_float;
    EncodeFuncDouble encode_func_double;
    DecodeFuncInt8   decode_func_int8;
    DecodeFuncInt16  decode_func_int16;
    DecodeFuncInt32  decode_func_int32;
    DecodeFuncInt64  decode_func_int64;
    DecodeFuncFloat  decode_func_float;
    DecodeFuncDouble decode_func_double;
    RangeFuncInt8    range_func_int8;
    RangeFuncInt16   range_func_int16;
    RangeFuncInt32   range_func_int32;
    RangeFuncInt64   range_func_int64;
    RangeFuncFloat   range_func_float;
    RangeFuncDouble  range_func_double;
} NetworkSignal;


typedef struct NetworkMessage {
    const char*    name;
    uint32_t       frame_id;
    uint8_t        frame_type;
    NetworkSignal* signals;  // NULL terminated list.
    /* Container Messages (typically name is <container>_<mux_id>). */
    const char*    container;  // Name of container message.
    uint32_t       mux_id;
    NetworkSignal* mux_signal;  // When set, this _is_ the container message.
    /* Buffer representing the message struct (intermediate object). */
    void*          buffer;
    size_t         buffer_len;
    uint8_t        cycle_time_ms;
    /* Payload (object of pack/unpack operations). */
    void*          payload;
    uint8_t        payload_len;
    /* Operational properties. */
    uint32_t       buffer_checksum;
    bool           needs_tx;
    PackFunc       pack_func;
    UnpackFunc     unpack_func;
    bool           update_signals;

    /* Message Functions. */
    NetworkFunction* encode_functions;  // NULL terminated list.
    NetworkFunction* decode_functions;  // NULL terminated list.
} NetworkMessage;


typedef struct MarshalItem {
    NetworkSignal*  signal;  // Set to NULL to end list.
    NetworkMessage* message;
    size_t          signal_vector_index;  // to signal vector on Network
} MarshalItem;


typedef struct NetworkScheduleItem {
    NetworkMessage* message;
    uint32_t        alarm;
} NetworkScheduleItem;


typedef struct Network {
    const char*          name;
    YamlNode*            doc;
    NetworkMessage*      messages;  // NULL terminated list.
    /* Shared libraries representing the Network implementation. */
    const char*          message_lib_path;
    const char*          function_lib_path;
    void*                message_lib_handle;
    void*                function_lib_handle;
    /* Marshalling items. */
    MarshalItem*         marshal_list;  // NULL terminated list.
    /* Signal interface. */
    size_t               signal_count;
    const char**         signal_name;
    double*              signal_vector;
    /* Schedule. */
    NetworkScheduleItem* schedule_list; /* NULL terminated list. */
    uint32_t             tick;          /* 1 ms clock. */

    /* Annotations. */
    uint32_t bus_id;
    uint32_t node_id;
    uint32_t interface_id;

    /* Sleep signal. */
    const char* netoff_signal;
    double*     netoff_value;
} Network;


/* network.c - Loads all parsing and loader functions from the Network shared
 * lib. */
DLL_PUBLIC int network_load(Network* n, ModelInstanceSpec* mi);
DLL_PUBLIC int network_unload(Network* n);

/* loader.c - Loads functions from the Network shared lib. */
DLL_PUBLIC void* network_load_message_lib(Network* n, const char* dll_path);
DLL_PUBLIC int   network_load_message_funcs(Network* n);
DLL_PUBLIC int   network_load_signal_funcs(Network* n);
DLL_PUBLIC void* network_load_function_lib(Network* n, const char* dll_path);
DLL_PUBLIC int   network_load_function_funcs(Network* n);

/* parser.c - Loads functions from the Network shared lib. */
DLL_PUBLIC int network_parse(Network* n, ModelInstanceSpec* mi);
DLL_PUBLIC int network_unload_parser(Network* n);

/* engine.c - Loads functions from the Network shared lib. */
DLL_PUBLIC int network_load_marshal_lists(Network* n);
DLL_PUBLIC int network_get_signal_names(
    MarshalItem* ml, const char*** signal_names, size_t* signal_count);
DLL_PUBLIC int network_marshal_signals_to_messages(Network* n, MarshalItem* mi);
DLL_PUBLIC int network_marshal_messages_to_signals(
    Network* n, MarshalItem* mi, bool signal);
DLL_PUBLIC void network_pack_messages(Network* n);
DLL_PUBLIC void network_unpack_messages(Network* n);
DLL_PUBLIC int  network_unload_marshal_lists(Network* n);

/* encoder.c - Loads functions from the Network shared lib. */
DLL_PRIVATE uint32_t simbus_generate_uid_hash(const uint8_t* key, size_t len);
DLL_PUBLIC void      network_encode_to_bus(Network* n, void* nc);
DLL_PUBLIC void      network_decode_from_bus(Network* n, void* nc);

/* function.c */
DLL_PUBLIC const char* network_function_annotation(
    NetworkFunction* function, const char* name);
DLL_PUBLIC int network_function_apply_encode(Network* n);
DLL_PUBLIC int network_function_apply_decode(Network* n);

/* schedule.c */
DLL_PUBLIC void network_schedule_reset(Network* n);
DLL_PUBLIC void network_schedule_tick(Network* n);

#endif  // DSE_NETWORK_NETWORK_H_
