---
title: Network API Reference
linkTitle: Network
---
## Network Model


The Network Model runs a Communication Stack which represents the connection
between Physical Signals and Network Messages.




## Typedefs

### MarshalItem

```c
typedef struct MarshalItem {
    NetworkSignal* signal;
    NetworkMessage* message;
    size_t signal_vector_index;
}
```

### Network

```c
typedef struct Network {
    const char* name;
    int* doc;
    NetworkMessage* messages;
    const char* message_lib_path;
    const char* function_lib_path;
    void* message_lib_handle;
    void* function_lib_handle;
    MarshalItem* marshal_list;
    size_t signal_count;
    const char** signal_name;
    double* signal_vector;
    NetworkScheduleItem* schedule_list;
    uint32_t tick;
    uint32_t bus_id;
    uint32_t node_id;
    uint32_t interface_id;
}
```

### NetworkFunction

```c
typedef struct NetworkFunction {
    char* name;
    int* annotations;
    void* data;
    NetworkFunctionFunc function;
}
```

### NetworkMessage

```c
typedef struct NetworkMessage {
    const char* name;
    uint32_t frame_id;
    uint8_t frame_type;
    NetworkSignal* signals;
    const char* container;
    uint32_t mux_id;
    NetworkSignal* mux_signal;
    void* buffer;
    size_t buffer_len;
    uint8_t cycle_time_ms;
    void* payload;
    uint8_t payload_len;
    uint32_t buffer_checksum;
    bool needs_tx;
    PackFunc pack_func;
    UnpackFunc unpack_func;
    bool update_signals;
    NetworkFunction* encode_functions;
    NetworkFunction* decode_functions;
}
```

### NetworkScheduleItem

```c
typedef struct NetworkScheduleItem {
    NetworkMessage* message;
    uint32_t alarm;
}
```

### NetworkSignal

```c
typedef struct NetworkSignal {
    const char* name;
    char* signal_name;
    const char* member_type;
    unsigned int buffer_offset;
    double init_value;
    bool internal;
    double value;
    bool mux_signal;
    MarshalItem* mux_mi;
    EncodeFuncInt8 encode_func_int8;
    EncodeFuncInt16 encode_func_int16;
    EncodeFuncInt32 encode_func_int32;
    EncodeFuncInt64 encode_func_int64;
    EncodeFuncFloat encode_func_float;
    EncodeFuncDouble encode_func_double;
    DecodeFuncInt8 decode_func_int8;
    DecodeFuncInt16 decode_func_int16;
    DecodeFuncInt32 decode_func_int32;
    DecodeFuncInt64 decode_func_int64;
    DecodeFuncFloat decode_func_float;
    DecodeFuncDouble decode_func_double;
    RangeFuncInt8 range_func_int8;
    RangeFuncInt16 range_func_int16;
    RangeFuncInt32 range_func_int32;
    RangeFuncInt64 range_func_int64;
    RangeFuncFloat range_func_float;
    RangeFuncDouble range_func_double;
}
```

## Functions

